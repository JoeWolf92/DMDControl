/*
 * This file provides the handler functions for the UI controls on
 * the Information page on Mainwindow and generic helper functions
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <QFileDialog>
#include <QFileInfo>
#include <QProgressDialog>
#include <QMessageBox>
#include <QTimer>
#include <QTime>
#include <QDateTime>
#include <QTextStream>
#include <QInputDialog>
#include <QDesktopServices>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "BMPParser.h"
#include "API.h"
#include "string.h"
#include "usb.h"
#include "version.h"
#include "firmware.h"
#include "splash.h"
#include "flashloader.h"
#include "PtnImage.h"
#include "batchfile.h"
#include "FirmwareW.h"
#include <string>
#include <windows.h>

#define MAX_PATTERNS        24
#define PTN_WIDTH_WQXGA     2560
#define PTN_HEIGHT_WQXGA    1600
#define PTN_WIDTH_1080p     1920
#define PTN_HEIGHT_1080p    1080
#define BYTES_PER_PIXEL     3


QString ErrCodeList[] =
{
    "No Error",
    "Batch file checksum error",
    "Device failure",
    "Invalid command number",
    "In compatible controller / DMD",
    "Command not allowed in current mode",
    "Invalid command parameter",
    "Item referred by the parameter is not present",
    "Out of resource (RAM / Flash)",
    "Invalid BMP compression type",
    "Pattern bit number out of range",
    "Pattern BMP not present in flash",
    "Pattern dark time is out of range",
    "Signal delay parameter is out of range",
    "Pattern exposure time is out of range",
    "Pattern number is out of range",
    "Invalid pattern definition",
};

/**
 * CallBack function which displays the batch file commands on the window
 *
 * @param   Cmd - I - the command to be updated in the BatchFile page
 * @param   Payload - I - the Payload data for the command
 * @param   PayloadSize - size of the Payload data
 */

static void updateBatchCallback(void* param, API_CommandInfo_t *CmdInfo)
{
    ((MainWindow*)param)->updateBatchCommands(CmdInfo);
}

//The MainWindow class has handler functions for all the GUI controls

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),

    settings("Texas Instruments", "LightCrafter 6500 9000 GUI")
{
    ui->setupUi(this);
    this->setCentralWidget(ui->scrollArea);

    waveWindow = new WaveFormWindow();
    ui->waveFormArea->setWidget(waveWindow);

    QObject::connect(waveWindow, SIGNAL(selectionChange(int, QList<PatternElement>)),
                     this, SLOT(on_patternSelect(int, QList<PatternElement>)));

    USB_Init();

    ui->batchFileName_lineEdit->setText("from Batch Files tab");

    m_firmwarePath = settings.value("FirmwarePath", "").toString();
    m_ptnImagePath = settings.value("PtnImagePath", "").toString();
    m_ptnSettingPath = settings.value("PtnSettingPath", "").toString();
    m_batchFilePath = settings.value("BatchFilePath", "").toString();
    m_lutReorderFilePath = settings.value("ReorderFilePath", "").toString();

    m_patternImageChange = false;
    m_ptnWidth = PTN_WIDTH_1080p;
    m_ptnHeight = PTN_HEIGHT_1080p;
    waveWindow->setPatternSize(m_ptnWidth, m_ptnHeight);

    m_dualAsic = false;

    m_dmdDetected = false;

    updateBlocks(true);

    m_videoPatternMode = false;

    m_cachedFimwareFile.clear();
    m_cachedFimwareFileSlave.clear();
    m_firmware = NULL;
    m_firmwareSlave = NULL;
    m_batchFile = NULL;

    ui->startPos_label->hide();
    ui->startPos_ComboBox->hide();

    API_SetDataCallback(updateBatchCallback, this);// Cmd, PayLoad, PayLoadSize));//Cmd, PayLoad,PayLoadSize));

    m_firstConnect = false;

    m_batchFile = BAT_Alloc();

    m_displayCommand = true;

    emit on_pushButton_info_clicked();
    usbPollTimer = new QTimer(this);
    usbPollTimer->setInterval(2000);
    connect(usbPollTimer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    usbPollTimer->start();
    ui->colorFrame->hide();
    ui->EVM_Picture_stackedWidget->setCurrentIndex(0);
    ui->onlineResources_stackedWidget->setCurrentIndex(0);
    ui->setAsDefaultBatch->setEnabled(false);

    char versionStr[255];
    // Display GUI Version #
//    sprintf(versionStr, "DLP LightCrafter 6500 & 9000 Test Version - %d.%d.%d [%s %s]", \
//            GUI_VERSION_MAJOR, GUI_VERSION_MINOR, GUI_VERSION_BUILD,__DATE__,__TIME__);
    sprintf(versionStr, "DLP LightCrafter 6500 & 9000 - %d.%d.%d", \
                GUI_VERSION_MAJOR, GUI_VERSION_MINOR, GUI_VERSION_BUILD);
    setWindowTitle(versionStr);

    updatePtnCheckbox();

    m_firmware = NULL;
    m_firmwareSlave = NULL;

    updateMinExposure();
    emit on_connectButton_clicked();

    //Hide these controls until API is fixed for not enough sequnece cache memory and EVM locks up
    //The cache size in the Composer projects are set to 0 which is max cache memory of 1MB.
    // ui->KB_label->hide();
    // ui->seqCacheSize_checkBox->hide();
    // ui->seqCacheSize_spinBox->hide();
    ui->editLUT_Button->hide();
    ui->splashIndex_lineEdit->setEnabled(FALSE);
    ui->splashBitPos_lineEdit->setEnabled(FALSE);
}

MainWindow::~MainWindow()
{
    //Close USB connection
    USB_Close();
    USB_Exit();

    //Store the settings to reuse next time
    settings.value("FirmwarePath", m_firmwarePath);
    settings.value("PtnImagePath", m_ptnImagePath);
    settings.value("PtnSettingPath", m_ptnSettingPath);
    settings.value("BatchFilePath", m_batchFilePath);
    settings.value("ReorderFilePath", m_lutReorderFilePath);

    delete ui;

    if(m_firmware)
        delete m_firmware;
    if(m_firmwareSlave)
        delete m_firmwareSlave;
    if(m_batchFile)
        BAT_Free(m_batchFile);
}
/**
 * helper function to show the appropriate status messages
 * @param str - I - the status message string to be displayed
 */
void MainWindow::showStatus(const char *str)
{
    QString title(" Status Message");
    QString text(str);
    QMessageBox msgBox(QMessageBox::Warning, title, text, QMessageBox::NoButton, this);
    msgBox.exec();
}

/**
 * helper function to show the appropriate critical Error messages
 * @param str - I - the error message string to be displayed
 */

void MainWindow::showCriticalError(QString str)
{
    QString title("Critical Error!!!");
    QMessageBox msgBox(QMessageBox::Critical, title, str, QMessageBox::NoButton, this);
    msgBox.exec();
}
/**
 * @brief MainWindow::showError
 * helper function to show the appropriate API Error message
 * @param errMsg - I - error messgae to be shown
 */
void MainWindow::showError(QString errMsg)
{
    QMessageBox errMsgBox;
    char errStr[128];

    if(LCR_ReadErrorString(errStr)<0)
        errMsgBox.setText("Error: " + errMsg);
    else
        errMsgBox.setText(errMsg + "\nError: " + QString::fromLocal8Bit(errStr));

    errMsgBox.exec();
}
/**
 * @brief MainWindow::timerTimeout
 */

void MainWindow::timerTimeout(void)
{
    emit on_connectButton_clicked();
}
/**
 * @brief MainWindow::calculateSplashImageDetails
 * for each of the pattern image on the pattern settings page, calculates the
 * total number of splash images of bit depth 24 based on the bit depth of each image
 * Also calculates the bitposition of each pattern element in the splash Image
 * and the index of the Splash image for each Pattern element
 * @param totalSplashImages - O - Total number of splash images to be created from
 *                                the available Pattern images
 * @return - 0 - success
 *          -1 - failure
 */
int MainWindow::calculateSplashImageDetails(int *totalSplashImages, bool firmware)
{
    int maxbits = 400;
    int imgCount = 0;
    int bits = 0;
    int totalBits = 0;
    for(int elemCount = 0; elemCount < m_elements.size(); elemCount++)
    {
        if (m_elements[elemCount].bits > 8)
        {
            char dispStr[255];
            sprintf(dispStr, "Error:Bit depth not selected for pattern=%d\n", elemCount);
            showStatus(dispStr);
            return -1;
        }

        totalBits = totalBits + m_elements[elemCount].bits;

        if(firmware == TRUE)
            maxbits = 3984;
        if(totalBits > maxbits)
        {
            char dispStr[255];
            if(firmware == FALSE)
                sprintf(dispStr, "Error:Total Bit Depth cannot exceed 400");
            else
                sprintf(dispStr, "Error:Total Bit Depth cannot exceed 3984");
            showError(dispStr);
            return -1;
        }

        /* Check if the same pattern is used already */
        int i;
        for(i = 0; i < elemCount; i++)
        {
            /* Only if file name and bit depth matches */
            if(m_elements[i].bits == m_elements[elemCount].bits &&
                    m_elements[i].name == m_elements[elemCount].name)
            {
                break;
            }
        }

        /* Match found. use the same splash image */
        if(i < elemCount)
        {
            m_elements[elemCount].splashImageIndex = m_elements[i].splashImageIndex;
            m_elements[elemCount].splashImageBitPos = m_elements[i].splashImageBitPos;
            continue;
        }

        /* If it is the last image or cant fit in the current image */
        if(elemCount == m_elements.size() ||
                (bits + m_elements[elemCount].bits) > 24)
        {
            /* Goto next image */
            imgCount++;
            bits = 0;
        }

        m_elements[elemCount].splashImageIndex = imgCount;
        m_elements[elemCount].splashImageBitPos = bits;
        bits += m_elements[elemCount].bits;
    }

    *totalSplashImages = imgCount + 1;

    return 0;
}


/**
 * @brief MainWindow::Hide_Frames
 * function to hide all the Frames
 * individual Frames are shown in the Handlers of respective Pushbuttons in the functions below
 */
void MainWindow::hideFrames()
{
    ui->pushButton_info->setChecked(false);
    ui->pushButton_systemControls->setChecked(false);
    ui->pushButton_videoMode->setChecked(false);
    ui->pushButton_patternMode->setChecked(false);
    ui->pushButton_batchFiles->setChecked(false);
    ui->pushButton_firmware->setChecked(false);
    ui->pushButton_peripherals->setChecked(false);
}
/**
 * @brief MainWindow::UpdateBatchCommands
 * Updates the m_batchFile with the batch commands received form API CallBack function
 * and updates the "Batch Commads Sequence" widget, if the "enable command log" is checked
 * @param Cmd - The I2C command name
 * @param Payload - The command Params
 * @param PayloadSize - The command Params size
 */
void MainWindow::updateBatchCommands(API_CommandInfo_t *cmdInfo)
{
    if(ui->enableCmdLog->isChecked() == false)
        return;

    if(m_displayCommand == false)
        return; //do not display if it is execute commands press

    if(BAT_AddCommand(m_batchFile,-1,cmdInfo) < 0)
        showError(GET_ERR_STR());

    int row = ui->batchCommands_tableWidget->rowCount();
    ui->batchCommands_tableWidget->insertRow(row);

    QTableWidgetItem *item;

    item = new QTableWidgetItem(QString(cmdInfo->CmdName));
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->batchCommands_tableWidget->setItem(row,0,item);

    QString data = "";

    int count = 0;
    while(count < cmdInfo->PayloadLen)
    {
        QString param;

        data.append("0x");
        param.setNum(cmdInfo->Payload[count++], 16);
        data.append(param);
        data.append(" ");

    }
    item = new QTableWidgetItem(data);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->batchCommands_tableWidget->setItem(row,1,item);

}

/**
 * @brief MainWindow::getVersion
 * gets the Application Software, API Software, Software Config and the Sequence Config versions of DLPC350 firmware
 * displays them in the Version label on the Information page
 */
void MainWindow::getVersion()
{
    char versionStr[255];
    unsigned int API_ver, App_ver, SWConfig_ver, SeqConfig_ver;

    if (LCR_GetVersion(&App_ver, &API_ver, &SWConfig_ver, &SeqConfig_ver) == 0)
    {
        sprintf(versionStr, "%d.%d.%d", (App_ver >> 24), ((App_ver << 8) >> 24), ((App_ver << 16) >> 16));
        ui->apiVersionLabel->setText(versionStr);

    }
}

/**
 * @brief MainWindow::getFrmwVersion
 * gets the firmware version and tag information from API
 * sets the Firmware tag label on information Page
 * populates either the "DLP6500" or "DLP9000" radio buttons based on the
 * firmwaretType received from the API
 */
void MainWindow::getFrmwVersion()
{
    char firmwareTag[32];
    unsigned int firmwareType;

    if(LCR_GetFrmwVersion(&firmwareType,firmwareTag)==0)
    {
        ui->firmwareTag_label->setText(QString::fromUtf8(firmwareTag));

        if(!m_dmdDetected)
        {
            if(firmwareType == 2) //WQXGA firmware
            {
                ui->pDMD_radioButton->setChecked(false);
                ui->wqxgaDMD_radioButton->setChecked(true);
                on_wqxgaDMD_radioButton_clicked();
            }
            else if(firmwareType == 1) //1080p firmware
            {
                ui->pDMD_radioButton->setChecked(true);
                ui->wqxgaDMD_radioButton->setChecked(false);
                on_pDMD_radioButton_clicked();
            }
            else if(firmwareType == 0) //Unknown
                showStatus("Unknown Firmware type detected!");

            m_dmdDetected = true;
        }
    }
}
/**
 * @brief MainWindow::getStatus
 * reads various status indicators from the controller and updates the Status checkboxes accordingly
 */
void MainWindow::getStatus()
{
    unsigned char HWStatus,SysStatus,MainStatus;
    BootLoaderStaus BLStatus;

    if(LCR_GetStatus(&HWStatus,&SysStatus,&MainStatus)==0)
    {
        if(SysStatus & BIT0)
            ui->internalMemTest_checkBox->setChecked(true);
        else
            ui->internalMemTest_checkBox->setChecked(false);

        if(HWStatus & BIT0)
            ui->internalInit_checkBox->setChecked(true);
        else
            ui->internalInit_checkBox->setChecked(false);

        if(HWStatus & BIT1)
            ui->incompatibleASICorDMD_checkBox->setChecked(true);
        else
            ui->incompatibleASICorDMD_checkBox->setChecked(false);

        if(HWStatus & BIT4)
            ui->slaveReady_checkBox->setChecked(true);
        else
            ui->slaveReady_checkBox->setChecked(false);

        if(HWStatus & BIT2)
            ui->DMDResetWaveformControllerErr_checkBox->setChecked(true);
        else
            ui->DMDResetWaveformControllerErr_checkBox->setChecked(false);

        if(HWStatus & BIT3)
            ui->forcedSwapErr_checkBox->setChecked(true);
        else
            ui->forcedSwapErr_checkBox->setChecked(false);

        if(HWStatus & BIT6)
            ui->seqAbortStatus_checkBox->setChecked(true);
        else
            ui->seqAbortStatus_checkBox->setChecked(false);

        if(HWStatus & BIT7)
            ui->seqErr_checkBox->setChecked(true);
        else
            ui->seqErr_checkBox->setChecked(false);

        if(MainStatus & BIT0)
            ui->DMDParkStatus_checkBox->setChecked(true);
        else
            ui->DMDParkStatus_checkBox->setChecked(false);

        if(MainStatus & BIT1)
            ui->sequencerRunState_checkBox->setChecked(true);
        else
            ui->sequencerRunState_checkBox->setChecked(false);

        if(MainStatus & BIT2)
            ui->videoState_checkBox->setChecked(false);
        else
            ui->videoState_checkBox->setChecked(true);

        if(MainStatus & BIT3)
            ui->extSourceLocked_checkBox->setChecked(true);
        else
            ui->extSourceLocked_checkBox->setChecked(false);
    }
    else if(LCR_GetBLStatus(&BLStatus) == 0)
    {
        //This means the device is in boot mode
    }
    else
    {
        USB_Close();
    }

}

/**
 * handler function for connectButton on Information page and clicked() event
 * Checks for the USB connection and
 * appropriately sets the "Connected" with green and "Disconnected" with Red
 * This button is not user clickable, but this function is called from MainWindow constructor
 * and is also called on timerTimeout
 */

void MainWindow::on_connectButton_clicked()
{

    static int SLModePrev = -1;
    int SLmode=0;
    int standBy = 0;

    if(USB_IsConnected() == false)
    {
        USB_Open();
    }

    if(USB_IsConnected())
    {
        getStatus();
        QIcon icon(":/new/prefix1/Icons/Led_G.png");
        ui->connectButton->setIcon(icon);
        ui->connectButton->setText("Connected");
        ui->operatingModes_groupBox->setEnabled(true);
        ui->status_groupBox->setEnabled(true);


        if (LCR_GetPowerMode(&standBy) == 0)
        {
            if(standBy)
            {
                ui->standBy_radioButton->setChecked(true);
                ui->powerOn_radioButton->setChecked(false);
            }
            else
            {
                ui->standBy_radioButton->setChecked(false);
                ui->powerOn_radioButton->setChecked(true);
            }
        }

        if(LCR_GetMode(&SLmode) == 0)
        {
            if(SLmode == 0)
            {
                updateEditLutButton(0);
                ui->patternMode_radioButton->setChecked(false);
                ui->videoPatternMode_radioButton->setChecked(false);
                ui->videoMode_radioButton->setChecked(true);
                ui->patternMemory_radioButton->setChecked(false);
                if(SLModePrev != SLmode)
                    on_videoMode_radioButton_clicked();
            }
            else if (SLmode == 1)
            {
                updateEditLutButton(1);
                ui->patternMode_radioButton->setChecked(true);
                ui->videoPatternMode_radioButton->setChecked(false);
                ui->videoMode_radioButton->setChecked(false);
                ui->patternMemory_radioButton->setChecked(false);
                if(SLModePrev != SLmode)
                    on_patternMode_radioButton_clicked();
            }
            else if (SLmode == 2)
            {
                updateEditLutButton(2);
                ui->patternMode_radioButton->setChecked(false);
                ui->videoPatternMode_radioButton->setChecked(true);
                ui->videoMode_radioButton->setChecked(false);
                ui->patternMemory_radioButton->setChecked(false);
                if(SLModePrev != SLmode)
                    on_videoPatternMode_radioButton_clicked();
            }
            else if (SLmode == 3)
            {
                updateEditLutButton(3);
                ui->patternMode_radioButton->setChecked(false);
                ui->videoPatternMode_radioButton->setChecked(false);
                ui->videoMode_radioButton->setChecked(false);
                ui->patternMemory_radioButton->setChecked(true);
                if(SLModePrev != SLmode)
                    on_patternMemory_radioButton_clicked();
            }
            SLModePrev = SLmode;
        }
        if (!m_firstConnect)
        {
            ui->onBoardBatchFile_comboBox->clear();
            int i = 0;
            getVersion();
            getFrmwVersion();
            while (true)
            {
                char batchFileName[16];
                unsigned char length;
                int ret;

                ret = LCR_getBatchFileName(i++, batchFileName, &length);
                if (ret < 0)
                    break;

                ui->onBoardBatchFile_comboBox->addItem(QString::fromUtf8(batchFileName, length));
            }
            m_firstConnect = true;
            m_patternImageChange = true;
        }
        updateMinExposure();
    }
    else
    {
        QIcon icon(":/new/prefix1/Icons/Led_R.png");
        ui->connectButton->setIcon(icon);
        ui->connectButton->setText("Disconnected");
        ui->operatingModes_groupBox->setEnabled(false);
        ui->status_groupBox->setEnabled(false);
        m_firstConnect = false;
        ui->onBoardBatchFile_comboBox->clear();

        ui->internalMemTest_checkBox->setChecked(false);
        ui->internalInit_checkBox->setChecked(false);
        ui->DMDResetWaveformControllerErr_checkBox->setChecked(false);
        ui->forcedSwapErr_checkBox->setChecked(false);
        ui->seqAbortStatus_checkBox->setChecked(false);
        ui->seqErr_checkBox->setChecked(false);
        ui->incompatibleASICorDMD_checkBox->setChecked(false);
        ui->slaveReady_checkBox->setChecked(false);
        ui->DMDParkStatus_checkBox->setChecked(false);
        ui->sequencerRunState_checkBox->setChecked(false);
        ui->videoState_checkBox->setChecked(false);
        ui->extSourceLocked_checkBox->setChecked(false);
        ui->firmwareTag_label->setText("xxxx");
        ui->apiVersionLabel->setText("xx.xx.xx");

        m_dmdDetected = false;
    }
}
/**
 * @brief MainWindow::on_resetDMD_Button_clicked
 *  handler function for resetDMD_Button on Information Page and clicked() event
 *  Send API command to reset the board
 */

void MainWindow::on_resetDMD_Button_clicked()
{
    if (LCR_SetPowerMode(0x2) < 0)
        showError("Unable to reset the board");
}
/**
 * @brief MainWindow::on_powerOn_radioButton_clicked
 */

void MainWindow::on_powerOn_radioButton_clicked()
{
    int standBy = 1;
    if (LCR_GetPowerMode(&standBy) < 0)
    {
        showError("Unable to get board status");
        return;
    }
    if (standBy)
    {
        if (LCR_SetPowerMode(0) < 0)
            showError("Unable to power on the board");
        usbPollTimer->start();
    }
    Sleep(500);
}
/**
 * @brief MainWindow::on_standBy_radioButton_clicked
 */
void MainWindow::on_standBy_radioButton_clicked()
{

    int standBy = 0;
    if (LCR_GetPowerMode(&standBy) < 0)
    {
        showError("Unable to get board status");
        return;
    }

    if (!standBy)
    {
        if (LCR_SetPowerMode(1) < 0)
            showError("Unable to go to stand by");
        ui->sequencerRunState_checkBox->setChecked(FALSE);
        ui->videoState_checkBox->setChecked(FALSE);
        usbPollTimer->stop();
    }
    Sleep(500);
}

/**
 * @brief MainWindow::updateEditLutButton
 * Hide/Show edit LUT button based on mode
 */
void MainWindow::updateEditLutButton(int mode)
{
    if(mode == 0)
    {
        ui->editLUT_Button->setEnabled(FALSE);
        ui->editLUT_Button->hide();
    }
    else if (mode == 2)
    {
        ui->editLUT_Button->show();
        ui->editLUT_Button->setEnabled(FALSE);
    }
    else if (mode == 1)
    {
        ui->editLUT_Button->show();
        ui->autoupdate_checkBox->setEnabled(TRUE);
        if(ui->apiVersionLabel->text().split(".")[0].toInt() > 4)
        {
            if(isPatternLoaded)
               ui->editLUT_Button->setEnabled(TRUE);
            else
               ui->editLUT_Button->setEnabled(FALSE);
        }
    }
    else
    {
        ui->editLUT_Button->show();
        if(ui->apiVersionLabel->text().split(".")[0].toInt() > 4)
        {
            if(isPatternLoaded)
               ui->editLUT_Button->setEnabled(TRUE);
            else
               ui->editLUT_Button->setEnabled(FALSE);
        }
        ui->autoupdate_checkBox->setChecked(TRUE);
        on_autoupdate_checkBox_clicked();
        ui->autoupdate_checkBox->setEnabled(FALSE);
    }
}

/**
 * @brief MainWindow::on_videoMode_radioButton_clicked
 * Send the API command to go into Video mode
 * Displays the Video Mode page of the MainWindow
 */
void MainWindow::on_videoMode_radioButton_clicked()
{
    if (LCR_SetMode(0x0) < 0)
    {
        showError("Unable to switch to video mode");
        return;
    }
    updateEditLutButton(0);
    emit on_pushButton_videoMode_clicked();
}
/**
 * @brief MainWindow::on_patternMode_radioButton_clicked
 *  Send the API command to go into Pattern mode
 *  Displays the Pattern Mode page of the MainWindow
 */
void MainWindow::on_patternMode_radioButton_clicked()
{
    if (LCR_SetMode(0x1) < 0)
    {
        showError("Unable to switch to pattern mode");
        return;
    }
    ui->editLUT_Button->show();
    emit on_pushButton_patternMode_clicked();

    if (!m_videoPatternMode)
        return;

    m_videoPatternMode = false;
    m_elements.clear();
    waveWindow->updatePatternList(m_elements);
    waveWindow->draw();
    waveWindow->updateVideoPatternMode(false);
    ui->bitDepth_ComboBox->setCurrentText("8");
    ui->startPos_label->hide();
    ui->startPos_ComboBox->hide();
    ui->triggerIn_checkBox->setText("Trigger Input");
    updatePtnCheckbox();
    updateEditLutButton(1);

}
/**
 * @brief MainWindow::on_videoPatternMode_radioButton_clicked
 *  Send the API command to go into Video Pattern mode
 *  Displays the Pattern Mode page of the MainWindow
 */
void MainWindow::on_videoPatternMode_radioButton_clicked()
{
    int SLmode = 0;
    if (LCR_SetMode(0x2) < 0)
    {
        showError("Unable to switch to video pattern mode");
        return;
    }

    Sleep(500);

    if(LCR_GetMode(&SLmode) == 0)
    {
        if (SLmode != 0x2)
        {
            showError("Unable to switch to video pattern mode.");
            return;
        }
    }

    emit on_pushButton_patternMode_clicked();


    if (m_videoPatternMode)
        return;

    m_videoPatternMode = true;
    m_elements.clear();
    waveWindow->updatePatternList(m_elements);
    waveWindow->draw();
    waveWindow->updateVideoPatternMode(true);
    ui->bitDepth_ComboBox->setCurrentText("1");
    ui->startPos_label->show();
    ui->startPos_ComboBox->show();
    ui->triggerIn_checkBox->setText("Frame Change");
    ui->bitDepth_ComboBox->removeItem(8);
    updatePtnCheckbox();
    updateEditLutButton(2);
}
/**
 * @brief MainWindow::on_patternMemory_radioButton_clicked
 *  Send the API command to go into Memory mode
 *  Displays the Pattern Mode page of the MainWindow
 */
void MainWindow::on_patternMemory_radioButton_clicked()
{
    if (LCR_SetMode(0x3) < 0)
    {
        showError("Unable to switch to pattern from Memory mode");
        return;
    }

    m_patternImageChange = true;

    emit on_pushButton_patternMode_clicked();

    if (!m_videoPatternMode)
        return;

    m_videoPatternMode = false;
    m_elements.clear();
    waveWindow->updatePatternList(m_elements);
    waveWindow->draw();
    waveWindow->updateVideoPatternMode(false);
    ui->bitDepth_ComboBox->setCurrentText("8");
    ui->startPos_label->hide();
    ui->startPos_ComboBox->hide();
    ui->triggerIn_checkBox->setText("Trigger Input");
    updateEditLutButton(3);
}

/**
 * @brief MainWindow::on_pDMD_radioButton_clicked
 */

void MainWindow::on_pDMD_radioButton_clicked()
{
    m_ptnWidth = PTN_WIDTH_1080p;
    m_ptnHeight = PTN_HEIGHT_1080p;
    ui->EVM_Picture_stackedWidget->setCurrentIndex(0);
    ui->onlineResources_stackedWidget->setCurrentIndex(0);
    waveWindow->setPatternSize(m_ptnWidth, m_ptnHeight);
    m_dualAsic = false;
    updateBlocks(true);
}
/**
 * @brief MainWindow::on_wqxgaDMD_radioButton_clicked
 */

void MainWindow::on_wqxgaDMD_radioButton_clicked()
{
    m_ptnWidth = PTN_WIDTH_WQXGA;
    m_ptnHeight = PTN_HEIGHT_WQXGA;
    ui->EVM_Picture_stackedWidget->setCurrentIndex(1);
    ui->onlineResources_stackedWidget->setCurrentIndex(1);
    waveWindow->setPatternSize(m_ptnWidth, m_ptnHeight);
    m_dualAsic = true;
    updateBlocks(true);
}

/**
 * @brief MainWindow::on_pushButton_info_clicked
 */
void MainWindow::on_pushButton_info_clicked()
{
    hideFrames();

    ui->stackedWidget->setCurrentIndex(0);
    ui->pushButton_info->setChecked(true);
}
/**
 * @brief MainWindow::on_pushButton_systemControls_clicked
 */
void MainWindow::on_pushButton_systemControls_clicked()
{
    hideFrames();

    ui->stackedWidget->setCurrentIndex(1);
    ui->pushButton_systemControls->setChecked(true);
}
/**
 * @brief MainWindow::on_pushButton_videoMode_clicked
 */
void MainWindow::on_pushButton_videoMode_clicked()
{
    hideFrames();

    ui->stackedWidget->setCurrentIndex(2);
    ui->pushButton_videoMode->setChecked(true);
}
/**
 * @brief MainWindow::on_pushButton_patternMode_clicked
 */
void MainWindow::on_pushButton_patternMode_clicked()
{
    static bool first = false;
    hideFrames();

    if (!first)
    {
        emit on_pushButton_patternControls_clicked();
        first = true;
    }

    ;
    ui->stackedWidget->setCurrentIndex(3);
    ui->pushButton_patternMode->setChecked(true);
}
/**
 * @brief MainWindow::on_pushButton_batchFiles_clicked
 */
void MainWindow::on_pushButton_batchFiles_clicked()
{
    hideFrames();

    ui->stackedWidget->setCurrentIndex(4);
    ui->pushButton_batchFiles->setChecked(true);
}
/**
 * @brief MainWindow::on_pushButton_firmware_clicked
 */
void MainWindow::on_pushButton_firmware_clicked()
{
    hideFrames();
    ui->stackedWidget->setCurrentIndex(6);
    ui->pushButton_firmware->setChecked(true);
}
/**
 * @brief MainWindow::on_pushButton_peripherals_clicked
 */
void MainWindow::on_pushButton_peripherals_clicked()
{
    hideFrames();
    ui->stackedWidget->setCurrentIndex(5);

    ui->pushButton_peripherals->setChecked(true);
}

/**
 * @brief MainWindow::on_pushButton_patternControls_clicked
 */
void MainWindow::on_pushButton_patternControls_clicked()
{

    ui->patternMode_stackedWidget->setCurrentIndex(0);
    ui->pushButton_patternControls->setChecked(true);
    ui->pushButton_globalSettings->setChecked(false);
}
/**
 * @brief MainWindow::on_pushButton_globalSettings_clicked
 */
void MainWindow::on_pushButton_globalSettings_clicked()
{

    ui->patternMode_stackedWidget->setCurrentIndex(1);
    ui->pushButton_patternControls->setChecked(false);
    ui->pushButton_globalSettings->setChecked(true);
}

/**
 * @brief MainWindow::on_portConfig_pixelMode_comboBox_currentIndexChanged
 * @param index
 */
void MainWindow::on_portConfig_pixelMode_comboBox_currentIndexChanged(int index)
{
    if(index==0)
    {
        ui->portConfig_dataPort_comboBox->clear();
        ui->portConfig_dataPort_comboBox->addItem("Port 1");
        ui->portConfig_dataPort_comboBox->addItem("Port 2");
    }
    else
    {
        ui->portConfig_dataPort_comboBox->clear();
        ui->portConfig_dataPort_comboBox->addItem("Port 1-2");
        ui->portConfig_dataPort_comboBox->addItem("Port 2-1");
    }
}
/**
 * @brief MainWindow::on_portConfigSet_button_clicked
 */
void MainWindow::on_portConfigSet_button_clicked()
{
    unsigned int pixelMode, dataPort, pixelClock, dataEnable, syncSelect;
    static unsigned int oldPixelMode = -1;

    pixelMode = ui->portConfig_pixelMode_comboBox->currentIndex();

    dataPort = ui->portConfig_dataPort_comboBox->currentIndex();

    if(pixelMode)
        dataPort += 2;
    pixelClock = ui->portConfig_pixelClock_comboBox->currentIndex();

    dataEnable = ui->portConfig_dataEnable_comboBox->currentIndex();

    syncSelect = ui->portConfig_syncSelect_comboBox->currentIndex();

    if(LCR_SetPortConfig(dataPort,pixelClock,dataEnable,syncSelect)<0)
        showError("Unable to set port configuration!");

    if(oldPixelMode != pixelMode)
    {
        unsigned int powerMode;
        if(LCR_GetIT6535PowerMode(&powerMode) >=0)
        {
            if (powerMode == 2)
            {
                LCR_SetIT6535PowerMode(0);
                LCR_SetIT6535PowerMode(2);
            }
        }
    }
    oldPixelMode = pixelMode;
}
/**
 * @brief MainWindow::on_portConfigGet_button_clicked
 */
void MainWindow::on_portConfigGet_button_clicked()
{
    unsigned int pixelMode, dataPort, pixelClock, dataEnable, syncSelect;

    if(LCR_GetPortConfig(&dataPort,&pixelClock,&dataEnable,&syncSelect)<0)
        showError("Unable to get Port Configuration!");
    else
    {
        if(dataPort>1)
            pixelMode=1;
        else
            pixelMode=0;
        ui->portConfig_pixelMode_comboBox->setCurrentIndex(pixelMode);
        ui->portConfig_dataPort_comboBox->setCurrentIndex(dataPort%2);
        ui->portConfig_pixelClock_comboBox->setCurrentIndex(pixelClock);
        ui->portConfig_dataEnable_comboBox->setCurrentIndex(dataEnable);
        ui->portConfig_syncSelect_comboBox->setCurrentIndex(syncSelect);
    }

}
/**
 * @brief MainWindow::on_startupState_checkBox_clicked
 * @param checked
 */
void MainWindow::on_startupState_checkBox_clicked(bool checked)
{
    ui->startupState_comboBox->setEnabled(checked);
}
/**
 * @brief MainWindow::on_hdmiReceiver_checkBox_clicked
 * @param checked
 */
void MainWindow::on_hdmiReceiver_checkBox_clicked(bool checked)
{
    ui->hdmiReceiver_comboBox->setEnabled(checked);
}
/**
 * @brief MainWindow::on_defaultBatch_checkBox_clicked
 * @param checked
 */
void MainWindow::on_defaultBatch_checkBox_clicked(bool checked)
{
    ui->defaultBatch_comboBox->setEnabled(checked);
}
/**
 * @brief MainWindow::on_IT6535GetMode_button_clicked
 */
void MainWindow::on_IT6535GetMode_button_clicked()
{
    unsigned int powerMode;
    if(LCR_GetIT6535PowerMode(&powerMode)<0)
    {
        showError("Unable to get IT6535 Power Mode!");
    }
    else{
        if(powerMode==0)
            ui->it6535PowerDown_radioButton->setChecked(true);
        else if (powerMode == 1)
            ui->it6535HDMI_radioButton->setChecked(true);
        else if(powerMode == 2)
            ui->it6535DisplayPort_radioButton->setChecked(true);

    }
}

/**
 * @brief MainWindow::on_IT6535SetMode_button_clicked
 */
void MainWindow::on_IT6535SetMode_button_clicked()
{
    unsigned int powerMode;

    if(ui->it6535PowerDown_radioButton->isChecked())
        powerMode = 0;
    else if(ui->it6535HDMI_radioButton->isChecked())
        powerMode = 1;
    else if(ui->it6535DisplayPort_radioButton->isChecked())
        powerMode = 2;
    else
    {
        showStatus("Select one power mode!");
        return;
    }

    if(LCR_SetIT6535PowerMode(powerMode)<0)
    {
        showError("Unable to set IT6535 Power Mode!");
    }
}
/**
 * @brief MainWindow::on_updateFlashWaitCycle_checkBox_clicked
 * @param checked
 */
void MainWindow::on_updateFlashWaitCycle_checkBox_clicked(bool checked)
{
    QIntValidator *qv = new QIntValidator(0,31,this);
    ui->readDelay->setValidator(qv);
    ui->writeDelay->setValidator(qv);
    ui->writePulseWidth->setValidator(qv);
    ui->CS_DataHoldTime->setValidator(qv);
    ui->flashWaitCycleControls_frame->setEnabled(checked);
    ui->readDelay->setEnabled(checked);
    ui->writeDelay->setEnabled(checked);
    ui->writePulseWidth->setEnabled(checked);
    ui->CS_DataHoldTime->setEnabled(checked);
}
/**
 * @brief MainWindow::on_i2cSlaveAdd_checkBox_clicked
 * @param checked
 */
void MainWindow::on_i2cSlaveAdd_checkBox_clicked(bool checked)
{
    QIntValidator *qv = new QIntValidator(1,254,this);
    ui->i2cSlaveAdd->setEnabled(checked);
    ui->i2cSlaveAdd->setValidator(qv);
}
/**
 * @brief MainWindow::on_DMDBusSwap_checkBox_clicked
 * @param checked
 */
void MainWindow::on_DMDBusSwap_checkBox_clicked(bool checked)
{
    ui->DMDBusSwapControls_frame->setEnabled(checked);
}

/**
 * @brief MainWindow::on_addBatchFile_checkBox_clicked
 * @param checked
 */
void MainWindow::on_addBatchFile_checkBox_clicked(bool checked)
{
    ui->batchFileName_frame->setEnabled(checked);
    ui->setAsDefaultBatch->setEnabled(checked);
    if(checked == false)
        ui->setAsDefaultBatch->setChecked(false);
}

/**
 * @brief MainWindow::on_ti_e2e_pushButton_clicked
 */
void MainWindow::on_ti_e2e_pushButton_clicked()
{
    QDesktopServices::openUrl(QUrl("http://e2e.ti.com/support/dlp__mems_micro-electro-mechanical_systems/default.aspx", QUrl::TolerantMode));
}

/**
 * @brief MainWindow::on_ti_e2e_pushButton_2_clicked
 */
void MainWindow::on_ti_e2e_pushButton_2_clicked()
{
    QDesktopServices::openUrl(QUrl("http://e2e.ti.com/support/dlp__mems_micro-electro-mechanical_systems/default.aspx", QUrl::TolerantMode));
}
/**
 * @brief MainWindow::on_dlp_pushButton_clicked
 */

void MainWindow::on_dlp_pushButton_clicked()
{
    QDesktopServices::openUrl(QUrl("http://www.ti.com/dlp", QUrl::TolerantMode));
}
/**
 * @brief MainWindow::on_enableCmdLog_clicked
 * @param checked
 */

void MainWindow::on_enableCmdLog_clicked(bool checked)
{
    ui->batchCommands_tableWidget->setEnabled(checked);
}
/**
 * @brief MainWindow::on_dummyConnection_clicked
 * @param checked
 */
void MainWindow::on_dummyConnection_clicked(bool checked)
{
    USB_SetFakeConnection(checked);
}
/**
 * @brief MainWindow::UpdateBlocks
 * @param full
 */
void MainWindow::updateBlocks(bool full)
{
    int count = m_dualAsic ? 16 : 15;
    int prevStart = ui->startDmdBlock->currentText().toInt();

    if(full)
    {
        prevStart = 0;
        ui->endDmdBlock->clear();
    }

    ui->startDmdBlock->clear();
    for(int i = 0; i < count; i++)
    {
        ui->startDmdBlock->addItem(QString::number(i));
    }

    if(prevStart < count && prevStart >= 0)
        ui->startDmdBlock->setCurrentIndex(prevStart);
}
/**
 * @brief MainWindow::on_startDmdBlock_currentIndexChanged
 * @param index
 */
void MainWindow::on_startDmdBlock_currentIndexChanged(int index)
{
    int count = m_dualAsic ? 16 : 15;

    int prevEnd = -1;

    if(ui->endDmdBlock->count() > 0)
    {
        prevEnd = ui->endDmdBlock->currentText().toInt();
    }

    ui->endDmdBlock->clear();
    for(int i = index; i < count; i++)
    {
        ui->endDmdBlock->addItem(QString::number(i));
    }

    if(prevEnd >= index)
    {
        ui->endDmdBlock->setCurrentIndex(prevEnd - index);
    }
    else
    {
        ui->endDmdBlock->setCurrentIndex(count - index - 1);
    }
}

void MainWindow::on_ledPulseWidthGet_clicked()
{
    int pulseWidth;
    if(LCR_GetMinLEDPulseWidth(&pulseWidth) < 0)
    {
        showError("Unable to get LED Pulse Width");
        return;
    }

    ui->ledPulseWidth->setValue(pulseWidth);
    updateMinExposure();
}

void MainWindow::on_ledPulseWidthSet_clicked()
{
   int pulseWidth = ui->ledPulseWidth->value();
   if(LCR_SetMinLEDPulseWidth(pulseWidth) < 0)
   {
       showError("Unable to set LED Pulse Width");
       return;
   }
   updateMinExposure();
}



void MainWindow::on_seqCacheSize_checkBox_clicked(bool checked)
{
    ui->seqCacheSize_spinBox->setEnabled(checked);
}

/**
 * @brief MainWindow::on_setSpreadSpectrum_checkBox_clicked
 * @param checked
 */
void MainWindow::on_setSpreadSpectrum_checkBox_clicked(bool checked)
{
    ui->spreadSpectrum_comboBox->setEnabled(checked);
}

void MainWindow::on_addBatchFile_checkBox_clicked()
{
    ui->batchFileName_lineEdit->clear();
}

void MainWindow::on_editLUT_Button_clicked()
{
    if(m_elements.size() <= 0)
    {
        showStatus("Error:No pattern sequence to edit");
        return;
    }
    hideFrames();
    ui->stackedWidget->setCurrentIndex(7);
    lut_entries_live.clear();
    reorder_idx_live.clear();

    ui->lutEditTableWidget->horizontalHeader()->show();
    ui->lutEditTableWidget->horizontalHeader()->setStretchLastSection(TRUE);
    ui->lutEditTableWidget->setColumnCount(2);
    ui->lutEditTableWidget->setRowCount(512);


    for(int i = 0; i < 512; i++)
    {
        QTableWidgetItem* pattern_idx = new QTableWidgetItem();
        QTableWidgetItem* lut_entry = new QTableWidgetItem();
        pattern_idx->setFlags(pattern_idx->flags() ^ Qt::ItemIsEditable);
        pattern_idx->setText(QString::number(i));
        if(lut_entries.size() > i)
        {
            lut_entry->setText(lut_entries[i]);
        }
        ui->lutEditTableWidget->setItem(i, 0, pattern_idx);
        ui->lutEditTableWidget->setItem(i, 1, lut_entry);
    }

}

//typedef struct __attribute__((packed)) {
//    uint8 patCount[2];
//    uint8 repeatCount[4];
//    uint8 patIdx[1][2];
//}reorder_payload_t;

/**
 * @brief MainWindow::lutEditUpdateButton
 */
void MainWindow::on_lutEditUpdateButton_clicked()
{
    lut_entries_live.clear();
    reorder_idx_live.clear();
    for(int i = 0; i < 512; i++)
    {
        QTableWidgetItem* item = ui->lutEditTableWidget->item(i,1);
        lut_entries_live.append(item->text());
    }

    int entries_count = 0;
    for(int i = 0; i < lut_entries_live.size(); i++)
    {
        QString qstr = lut_entries_live[i].trimmed();
        if (!qstr.isEmpty())
        {
            unsigned short patIdx;
            try
            {
                patIdx = (unsigned short)qstr.toInt();
            }
            catch(int e)
            {
                QString x = "Error: Parse error in reading pattern index at position:" + QString::number(i);
                showStatus((char*)(x.toLatin1().data()));
                return;
            }

            reorder_idx_live.append(patIdx);
            entries_count++;
        }
    }
    if(entries_count == 0)
    {
        showStatus("Error: Atleast one entry is required to reorder.");
        return;
    }

    int payloadSize = reorder_idx_live.size()*2 + 6;
    uint8* patMem = (uint8*)calloc(payloadSize, sizeof(uint8));
    patMem[0] = (uint8)reorder_idx_live.size();
    patMem[1] = (uint8)(reorder_idx_live.size() >> 8);
    if(!ui->checkBoxRepeatEditLUT->isChecked())
    {
        patMem[2] = (uint8)reorder_idx_live.size();
        patMem[3] = (uint8)(reorder_idx_live.size() >> 8);
        patMem[4] = (uint8)(reorder_idx_live.size() >> 16);
        patMem[5] = (uint8)(reorder_idx_live.size() >> 24);
    }

    for(int i = 0; i < reorder_idx_live.size(); i++)
    {
         patMem[6 + 2*i] = (uint8)reorder_idx_live[i];
         patMem[7 + 2*i] = (uint8)(reorder_idx_live[i] >> 8);
    }
    if(LCR_SendPatReorderUpdate(patMem, payloadSize) < 0)
    {
        free(patMem);
        showStatus("Error: Sending pattern LUT reorder update failed!");
        return;
    }
    free(patMem);
    lut_entries = lut_entries_live;
    reorder_idx = reorder_idx_live;
}


void MainWindow::on_startPatSequenceEdit_Button_clicked()
{
    if (LCR_PatternDisplay(0x2) < 0)
        showError("Unable to stat pattern display");
}

void MainWindow::on_stopPatSequenceEdit_Button_clicked()
{
    emit on_stopPatSequence_Button_clicked();
}

void MainWindow::on_lutEditBackButton_clicked()
{
    if(ui->patternMode_radioButton->isChecked())
    {
        emit on_patternMode_radioButton_clicked();
    }
    else
    {
        emit on_patternMemory_radioButton_clicked();
    }
}

void MainWindow::on_lutEditButton_clicked()
{

}

/**
 * @brief MainWindow::on_loadButton_LutEdit_clicked
 */
void MainWindow::on_loadButton_LutEdit_clicked()
{
    QString fileName;

    fileName = QFileDialog::getOpenFileName(this,
                                            QString("Choose Reorder File"),
                                            m_lutReorderFilePath,
                                            tr("Reorder File(*.txt)"));

    if(fileName.isEmpty())
        return;

    QFile settingsFile(fileName);
    if(!settingsFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        showStatus("Unable to open the reorder info file\n");
        return;
    }

    QDir dir = QFileInfo(settingsFile).absoluteDir();
    m_lutReorderFilePath = dir.absolutePath();
    settings.setValue("ReorderFilePath",m_lutReorderFilePath);

    QTextStream in(&settingsFile);

    QString line;
    QList<QString> reorder_list;

    line = in.readLine();

    if(line != "Reorder")
    {
        showStatus("Please select a valid Reorder settings file");
        settingsFile.close();
        return;
    }
    while(!in.atEnd())
    {
        line = in.readLine().trimmed();

        if(line == "\n" || line == "") //error checking for empty lines
            continue;
        reorder_list.append(line);
    }

    settingsFile.close();

    for(int i = 0; i < 512; i++)
    {
        QTableWidgetItem* item = ui->lutEditTableWidget->item(i,1);
        item->setText("");
        if(reorder_list.length() > i)
        {
            item->setText(reorder_list[i]);
        }
    }
}

void MainWindow::on_saveButton_lutEdit_clicked()
{
    QString fileName;

    fileName = QFileDialog::getSaveFileName(this,QString("Enter name of the settings file"),
                                            m_lutReorderFilePath,
                                            tr("settings files(*.txt)"));


    if(fileName.isEmpty())
        return;

    QFile settingsFile(fileName);

    if(!settingsFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        showStatus("Error: Unable to open file for saving\n");
        return;
    }

    QDir dir = QFileInfo(settingsFile).absoluteDir();
    m_lutReorderFilePath = dir.absolutePath();
    settings.setValue("ReorderFilePath",m_lutReorderFilePath);

    QTextStream out(&settingsFile);

    out << "Reorder\n";
    for(int i=0;i<512;i++)
    {
        QTableWidgetItem* item = ui->lutEditTableWidget->item(i,1);
        if(!item->text().isEmpty())
            out << item->text() << "\n";
    }

    settingsFile.close();
}


void MainWindow::on_lutEditClearButton_clicked()
{
    lut_entries_live.clear();
    for(int i = 0; i < 512; i++)
    {
        QTableWidgetItem* item = ui->lutEditTableWidget->item(i,1);
        item->setText("");
    }
}

void MainWindow::on_autoupdate_checkBox_clicked()
{
    if(ui->autoupdate_checkBox->isChecked())
    {
        ui->label_SplashIndex->setEnabled(FALSE);
        ui->label_BitPosition->setEnabled(FALSE);
        ui->splashIndex_lineEdit->setText("0");
        ui->splashBitPos_lineEdit->setText("0");
        ui->splashIndex_lineEdit->setEnabled(FALSE);
        ui->splashBitPos_lineEdit->setEnabled(FALSE);
    }
    else
    {
        ui->label_SplashIndex->setEnabled(TRUE);
        ui->label_BitPosition->setEnabled(TRUE);
        ui->splashIndex_lineEdit->setEnabled(TRUE);
        ui->splashBitPos_lineEdit->setEnabled(TRUE);
    }
}

void MainWindow::on_splashIndex_lineEdit_editingFinished()
{
    bool changed = FALSE;
    for (int i = 0; i < m_elements.size(); i++)
    {
        if (!m_elements[i].selected)
            continue;
        int idx = ui->splashIndex_lineEdit->text().toInt();
        if (idx >= 512)
        {
            char errMsg[255];
            sprintf(errMsg, "Splash index value should be less than 512\n");
            showStatus(errMsg);
            return;
        }
        m_elements[i].splashImageIndex = idx;
        changed = TRUE;
    }
    if (changed)
    {
        waveWindow->updatePatternList(m_elements);
        //	waveWindow->draw();
    }
}

void MainWindow::on_splashBitPos_lineEdit_editingFinished()
{
    bool changed = FALSE;
    for (int i = 0; i < m_elements.size(); i++)
    {
        if (!m_elements[i].selected)
            continue;
        int pos = ui->splashBitPos_lineEdit->text().toInt();
        if (pos >= 24)
        {
            char errMsg[255];
            sprintf(errMsg, "Bitposition value should be less than 24\n");
            showStatus(errMsg);
            return;
        }
        m_elements[i].splashImageBitPos = pos;
        changed = TRUE;
    }
    if (changed)
    {
        waveWindow->updatePatternList(m_elements);
    }
}

