/*
 * This file provides all the handler functions for the UI controls on
 * Batch Files page
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/



#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "API.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QTime>
#include <QTimer>

/**
 * @brief MainWindow::on_executeBatchCommands_Button_clicked
 */
void MainWindow::on_executeBatchCommands_Button_clicked()
{
    m_displayCommand = false;
    int rowCount = ui->batchCommands_tableWidget->rowCount();
    if (!rowCount || !m_batchFile)
    {
        showStatus("No commands to execute\n");
        return;
    }
    // Stop USB poll timer
    usbPollTimer->stop();
    int size = BAT_GetCommandCount(m_batchFile);

    if(size < 0)
        showError(GET_ERR_STR());

    int count = 0;

    int cmdIndex = 0;

    while(count < size)
    {
        BAT_CommandInfo_t commandInfo;
        if(BAT_GetCommandInfo(m_batchFile,cmdIndex++, &commandInfo) < 0)
            showError(GET_ERR_STR());
        if(commandInfo.USBCmd == 0x1A16)
        {
            QTime t;
            int delay = ((commandInfo.Payload[3] << 24) |
                         (commandInfo.Payload[2] << 16) |
                         (commandInfo.Payload[1] <<  8) |
                         (commandInfo.Payload[0]      ));
            delay = (delay);

            t.start();
            while(t.elapsed() < delay);
        }
        else
        {
            if(LCR_ExecuteRawCommand(commandInfo.USBCmd, commandInfo.Payload, commandInfo.PayloadLen) < 0)
            {
                char buf[100];
                sprintf(buf, "Error: Batch execution failed due to command idx %d failure\n", (count+1));
                showStatus(buf);
                break;
            }
        }

        count++;
    }
   usbPollTimer->start();
   m_displayCommand = true;
}
/**
 * @brief MainWindow::on_insertDelay_Button_clicked
 */
void MainWindow::on_insertDelay_Button_clicked()
{
    QList<QTableWidgetSelectionRange> selectedRanges = ui->batchCommands_tableWidget->selectedRanges();

    if (!selectedRanges.size())
    {
        showStatus("Error: Choose the command after which delay has to be inserted\n");
        return;
    }

    bool ok = false;
    int delay =  QInputDialog::getInt(this, tr("Input Box"),
                                      tr("Delay in ms"), 0, 0, 2147483647, 1, &ok);

    if (!ok)
        return;

    QString delayStr;
    uint08 param[4];
    delayStr = " 0x";
    for(int i = 0; i < 4; i++)
    {
        QString temp;
        param[i] = (delay >> (8 * i)) & 0xFF;
        temp.setNum(param[i], 16);
        delayStr += temp;
        if (i!=3)
            delayStr += " 0x";
    }

    int row = selectedRanges[0].topRow() + 1;
    ui->batchCommands_tableWidget->insertRow(row);

    QTableWidgetItem *item;

    item = new QTableWidgetItem("DELAY");
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->batchCommands_tableWidget->setItem(row,0,item);

    item = new QTableWidgetItem(delayStr);
    ui->batchCommands_tableWidget->setItem(row,1,item);

    API_CommandInfo_t cmdInfo;

    cmdInfo.CmdName = "DELAY";
    cmdInfo.I2CCmd = 0x3C;
    cmdInfo.PayloadLen = 4;
    cmdInfo.Payload = param;
    cmdInfo.USBCmd = 0x1A16;
    cmdInfo.Read = 0;
    //or with 0x80 , since only write commands are added to the batch files
    if(BAT_AddCommand(m_batchFile,row,&cmdInfo) < 0)
        showError(GET_ERR_STR());
}
/**
 * @brief MainWindow::on_clearBatchTable_Button_clicked
 */
void MainWindow::on_clearBatchTable_Button_clicked()
{
    int rowCount = ui->batchCommands_tableWidget->rowCount();
    for(int i = 0; i < rowCount; i++)
        ui->batchCommands_tableWidget->removeRow(0);

    if(BAT_Clear(m_batchFile) < 0)
        showError(GET_ERR_STR());
}

/**
 * @brief MainWindow::on_deleteCommand_Button_clicked
 */
void MainWindow::on_deleteCommand_Button_clicked()
{
    QList<QTableWidgetSelectionRange> selectedRanges =ui->batchCommands_tableWidget->selectedRanges();

    while (selectedRanges.size())
    {
        int topRow = selectedRanges[0].topRow();
        int bottomRow = selectedRanges[0].bottomRow();
        for (int j = 0; j <= (bottomRow - topRow); j++)
        {
            ui->batchCommands_tableWidget->removeRow(topRow);
            BAT_RemoveCommand(m_batchFile, topRow + j);
        }

        selectedRanges = ui->batchCommands_tableWidget->selectedRanges();
    }
}
/**
 * @brief MainWindow::on_saveBatchFile_Button_clicked
 */
void MainWindow::on_saveBatchFile_Button_clicked()
{
    QString fileName;

    fileName = QFileDialog::getSaveFileName(this,
                                            QString("Enter name of the batch file"),
                                            m_batchFilePath,
                                            tr("batch files(*.txt)"));
    if(fileName.isEmpty())
        return;

    if( BAT_SaveToFile(m_batchFile,fileName.toUtf8().constData()) < 0)
        showError(GET_ERR_STR());


}
/**
 * @brief MainWindow::on_loadBatchFile_Button_clicked
 */
void MainWindow::on_loadBatchFile_Button_clicked()
{
    QString fileName;

    fileName = QFileDialog::getOpenFileName(this,
                                            QString("Choose Batch File"),
                                            m_batchFilePath,
                                            tr("Batch File(*.txt)"));

    if(fileName.isEmpty())
        return;

    QFile batch(fileName);
    if(!batch.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        showStatus("Unable to open the batch file\n");
        return;
    }

    QDir dir = QFileInfo(batch).absoluteDir();
    m_batchFilePath = dir.absolutePath();
    settings.setValue("BatchFilePath",m_batchFilePath);

    emit on_clearBatchTable_Button_clicked();

    if(BAT_Clear(m_batchFile) < 0)
        showError(GET_ERR_STR());
    if(BAT_LoadFromFile(m_batchFile,fileName.toUtf8().constData()) < 0)
        showError(GET_ERR_STR());

    displayBatchFile(m_batchFile, ui->batchCommands_tableWidget);
}


/**
 * @brief Display the batchfile content in a table widget
 * @param batchFile Batchfile to be displayed
 * @param table table widget
 *
 * @return 0 on success, -1 on failure
 */
int MainWindow::displayBatchFile(BAT_BatchFile_t* batchFile, QTableWidget *table)
{
    int cmdLine = 0;
    int numCmds = BAT_GetCommandCount(batchFile);

    table->setRowCount(0);

    while(cmdLine < numCmds)
    {
        BAT_CommandInfo_t commandInfo;
        QTableWidgetItem *item;

        if(cmdLine == 0)
        {
            if(BAT_GetCommandInfo(batchFile, 0, &commandInfo) < 0)
            {
                showError(GET_ERR_STR());
                return -1;
            }
        }
        else
        {
            if(BAT_NextCommandInfo(&commandInfo) < 0)
            {
                showError(GET_ERR_STR());
                return -1;
            }
        }

        table->insertRow(cmdLine);

        item = new QTableWidgetItem(commandInfo.CmdName);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        table->setItem(cmdLine,0,item);

        char paramLine[512 * 5 + 1];
        int charIndex = 0;

        for(int i = 0; i < commandInfo.PayloadLen &&
                            charIndex + 6 < (int)sizeof(paramLine); i++)
        {
            int len = sprintf(paramLine + charIndex, "0x%02X ",
                                commandInfo.Payload[i]);
            if(len <= 0)
                return -1;
            charIndex += len;
        }

        item = new QTableWidgetItem(paramLine);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        table->setItem(cmdLine, 1, item);
        cmdLine++;
    }

    table->resizeColumnToContents(0);

    return 0;
}

/**
 * @brief MainWindow::on_executeBatchFile_Button_clicked
 */
void MainWindow::on_executeBatchFile_Button_clicked()
{
    int batchFileId = ui->onBoardBatchFile_comboBox->currentIndex();

    if (batchFileId < 0)
        return;

    if (LCR_executeBatchFile(batchFileId & 0xFF) < 0)
        showError("Unable to execute the batch file");
}
