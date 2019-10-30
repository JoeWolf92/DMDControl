/*
 * This file provides all the handler functions for the UI controls on
 * Firmware page and teh required helper functions
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QTimer>
#include <QTime>
#include <stdexcept>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "API.h"
#include "usb.h"
#include "flashloader.h"
#include <windows.h>

using namespace std;
/******************helper functions***************/

/**
 * @brief MainWindow::ProcessChosenFirmwareImage
 * Gets the Batch files and configuration details from the selected Firmware Image
 * and updates the respective "Power Up Defaults" controls on the Firmware Page accordingly
 */
void MainWindow::processChosenFirmwareImage()
{

    QStringList fileNameList = (ui->firmwareName_textEdit->toPlainText()).split("\n", QString::SkipEmptyParts);

    if(m_firmware)
        delete m_firmware;
    if(m_firmwareSlave)
        delete m_firmwareSlave;

    m_firmwareSlave = NULL;

    m_firmware =  new Firmware(fileNameList[0]);
    if(m_firmware == NULL)
    {
        showError(GET_ERR_STR());
        return;
    }
    if(!m_firmware->isValid())
    {
        showError("Not a valid Firmware Image.");
        return;
    }

    if(m_dualAsic)
    {
        if(fileNameList.size() > 1)
        {
            m_firmwareSlave = new Firmware(fileNameList[1]);
        }
        else
        {
            m_firmwareSlave = new Firmware(fileNameList[0]);
        }

        if(m_firmwareSlave == NULL)
        {
            showError(GET_ERR_STR());
            return;
        }
        if(!m_firmwareSlave->isValid())
        {
            showError("Not a valid Firmware Slave Image.");
            return;
        }
    }
    ui->firmwareBatchFile_comboBox->clear();
    ui->defaultBatch_comboBox->clear();
    ui->defaultBatch_comboBox->addItem("None");
    ui->frmwBatchFile_tbl->setRowCount(0);
    ui->firmwareBatchFile_groupBox->setEnabled(false);
    ui->i2cSlaveAdd->clear();
    ui->seqCacheSize_spinBox->setValue(0);


    int batchFileCount = m_firmware->getNumBatchFiles();
    if(batchFileCount < 0)
        showError(GET_ERR_STR());
    for(int i = 0; i < batchFileCount ; i ++)
    {
        char batchFileName[16];
        BAT_BatchFile_t* batch_File = m_firmware->getBatchFile(i);

        if (batchFileCount)
            ui->firmwareBatchFile_groupBox->setEnabled(true);

        if(BAT_GetName(batch_File, batchFileName) < 0)
            showError(GET_ERR_STR());
        ui->firmwareBatchFile_comboBox->addItem(QString(batchFileName));
        ui->defaultBatch_comboBox->addItem(QString(batchFileName));

    }

    uint32 config_value;
    if(m_firmware->getConfigBits(FW_CFG_SPREAD_SPECTRUM,&config_value) < 0)
        showError(GET_ERR_STR());
    ui->spreadSpectrum_comboBox->setCurrentIndex(config_value);

    if(m_firmware->getConfigBits(FW_CFG_STARTUP_STATE,&config_value) < 0)
        showError(GET_ERR_STR());
    ui->startupState_comboBox->setCurrentIndex((int)config_value);

    if(m_firmware->getConfigBits(FW_CFG_HDMI_ENABLE,&config_value) < 0)
        showError(GET_ERR_STR());
    ui->hdmiReceiver_comboBox->setCurrentIndex(config_value);

    if(m_firmware->getConfigBits( FW_CFG_DEF_BATCH, &config_value) < 0)
        showError(GET_ERR_STR());
    int defaultBatchFile = config_value + 1;
    ui->defaultBatch_comboBox->setCurrentIndex(defaultBatchFile);

    if(m_firmware->getConfigBits( FW_CFG_SEQ_CACHE, &config_value) < 0)
        showError(GET_ERR_STR());
    ui->seqCacheSize_spinBox->setValue(config_value);

    if(m_firmware->getConfigBits(FW_CFG_I2C_ADDRESS,&config_value) < 0)
        showError(GET_ERR_STR());
    ui->i2cSlaveAdd->setText(QString("%1").arg(config_value));

    if(m_firmware->getConfigBits( FW_CFG_MASTER_DMD_A_SWAP,&config_value) < 0)
        showError(GET_ERR_STR());
    ui->bitSwapAMaster->setChecked(config_value);

    if(m_firmware->getConfigBits(FW_CFG_MASTER_DMD_B_SWAP,&config_value) < 0)
        showError(GET_ERR_STR());
    ui->bitSwapBMaster->setChecked(config_value);

    if(m_firmware->getConfigBits( FW_CFG_MASTER_DMD_AB_SWAP,&config_value) < 0)
        showError(GET_ERR_STR());
    ui->channelSwapABMaster->setChecked(config_value);

    if(m_firmware->getConfigBits( FW_CFG_SLAVE_DMD_A_SWAP,&config_value) < 0)
        showError(GET_ERR_STR());
    ui->bitSwapASlave->setChecked(config_value);

    if(m_firmware->getConfigBits(FW_CFG_SLAVE_DMD_B_SWAP,&config_value) < 0)
        showError(GET_ERR_STR());
    ui->bitSwapBSlave->setChecked(config_value);

    if(m_firmware->getConfigBits( FW_CFG_SLAVE_DMD_AB_SWAP,&config_value) < 0)
        showError(GET_ERR_STR());
    ui->channelSwapABSlave->setChecked(config_value);

    if(m_firmware->getConfigBits(FW_CFG_FLASH_READ_DELAY,&config_value) < 0)
        showError(GET_ERR_STR());
    ui->readDelay->setText(QString("%1").arg(config_value));

    if(m_firmware->getConfigBits(FW_CFG_FLASH_WRITE_DELAY,&config_value) < 0)
        showError(GET_ERR_STR());
    ui->writeDelay->setText(QString("%1").arg(config_value));

    if(m_firmware->getConfigBits(FW_CFG_FLASH_WRITE_WIDTH,&config_value) < 0)
        showError(GET_ERR_STR());
    ui->writePulseWidth->setText(QString("%1").arg(config_value));

    if(m_firmware->getConfigBits(FW_CFG_FLASH_CS_HOLD,&config_value) < 0)
        showError(GET_ERR_STR());
    ui->CS_DataHoldTime->setText(QString("%1").arg(config_value));

    uint08 tag[32];

    if(m_firmware->getConfigBytes(FW_CFG_FW_TAG,tag) < 0)
        showError(GET_ERR_STR());
    QString firmwareTag = "";
    firmwareTag.append(QString::fromUtf8((const char*)tag));
    ui->firmwareTag_lineEdit->setText(firmwareTag);

}

/**
 * @brief MainWindow::processCachedFirmwareImage
 * validates if the firmware image is valid
 */
bool MainWindow::processCachedFirmwareImage()
{

    Firmware *firmwareCached = NULL;
    m_cachedFimwareFile.clear();
    if(m_dualAsic)
        m_cachedFimwareFileSlave.clear();

    QFileInfo tempFileInfo;
    QStringList fileNames;
    QString fileList;
    fileList = ui->firmwareName_textEdit->toPlainText();
    if(fileList.isEmpty())
    {
        return false;
    }
    fileNames = fileList.split("\n", QString::SkipEmptyParts);

    if(m_dualAsic)
    {
        tempFileInfo.setFile(fileNames.at(0));
    }
    else
    {
        tempFileInfo.setFile(QFile(ui->firmwareName_textEdit->toPlainText()));
    }

    QDir dirCachedImage(QString(tempFileInfo.absolutePath()+"/FlashLoaderCache"));
    if(dirCachedImage.exists() == false)
        return false;

    QStringList nameFilter;
    QFileInfoList fileInfoList;
    uint32 config_value;
    uint08 tag[32];

    if(m_dualAsic == false)
    {
        nameFilter << "dlp6500_*";
    }
    else
    {
        nameFilter << "dlp9000_master_*";
    }


    fileInfoList = dirCachedImage.entryInfoList(nameFilter,QDir::Files);
    if(fileInfoList.size()<=0)
        return false;

    firmwareCached =  new Firmware(fileInfoList.at(0).absoluteFilePath());
    if((firmwareCached == NULL) || (!firmwareCached->isValid()))
    {
        return false;
    }


    if((firmwareCached->getNumBatchFiles()<0) ||\
            (firmwareCached->getConfigBits(FW_CFG_SPREAD_SPECTRUM,&config_value) < 0) ||\
            (firmwareCached->getConfigBits(FW_CFG_STARTUP_STATE,&config_value) < 0) ||\
            (firmwareCached->getConfigBits(FW_CFG_HDMI_ENABLE,&config_value) < 0) ||\
            (firmwareCached->getConfigBits( FW_CFG_DEF_BATCH, &config_value) < 0) ||\
            (firmwareCached->getConfigBits( FW_CFG_SEQ_CACHE, &config_value) < 0) ||\
            (firmwareCached->getConfigBits(FW_CFG_I2C_ADDRESS,&config_value) < 0) ||\
            (firmwareCached->getConfigBits( FW_CFG_MASTER_DMD_A_SWAP,&config_value) < 0) ||\
            (firmwareCached->getConfigBits(FW_CFG_MASTER_DMD_B_SWAP,&config_value) < 0) ||\
            (firmwareCached->getConfigBits( FW_CFG_MASTER_DMD_AB_SWAP,&config_value) < 0) ||\
            (firmwareCached->getConfigBits( FW_CFG_SLAVE_DMD_A_SWAP,&config_value) < 0) ||\
            (firmwareCached->getConfigBits(FW_CFG_SLAVE_DMD_B_SWAP,&config_value) < 0) ||\
            (firmwareCached->getConfigBits( FW_CFG_SLAVE_DMD_AB_SWAP,&config_value) < 0) ||\
            (firmwareCached->getConfigBits(FW_CFG_FLASH_READ_DELAY,&config_value) < 0) ||\
            (firmwareCached->getConfigBits(FW_CFG_FLASH_WRITE_DELAY,&config_value) < 0) ||\
            (firmwareCached->getConfigBits(FW_CFG_FLASH_WRITE_WIDTH,&config_value) < 0) ||\
            (firmwareCached->getConfigBits(FW_CFG_FLASH_CS_HOLD,&config_value) < 0) ||\
            (firmwareCached->getConfigBytes(FW_CFG_FW_TAG,tag) < 0))
    {
        if(firmwareCached)
            delete firmwareCached;

        return false;
    }
    else
    {
        m_cachedFimwareFile = QString(fileInfoList.at(0).absolutePath()+"/"+fileInfoList.at(0).fileName());
    }

    //parse for the slave image
    if(m_dualAsic == true)
    {
        nameFilter.clear();
        fileInfoList.clear();
        if(firmwareCached)
            delete firmwareCached;

        nameFilter << "dlp9000_slave_*";
        fileInfoList = dirCachedImage.entryInfoList(nameFilter,QDir::Files);
        if(fileInfoList.size()<=0)
            return false;

        firmwareCached =  new Firmware(QString(fileInfoList.at(0).absoluteFilePath()));
        if((firmwareCached == NULL) || (!firmwareCached->isValid()))
        {
            return false;
        }

        if((firmwareCached->getNumBatchFiles()<0) ||\
                (firmwareCached->getConfigBits(FW_CFG_SPREAD_SPECTRUM,&config_value) < 0) ||\
                (firmwareCached->getConfigBits(FW_CFG_STARTUP_STATE,&config_value) < 0) ||\
                (firmwareCached->getConfigBits(FW_CFG_HDMI_ENABLE,&config_value) < 0) ||\
                (firmwareCached->getConfigBits( FW_CFG_DEF_BATCH, &config_value) < 0) ||\
                (firmwareCached->getConfigBits( FW_CFG_SEQ_CACHE, &config_value) < 0) ||\
                (firmwareCached->getConfigBits(FW_CFG_I2C_ADDRESS,&config_value) < 0) ||\
                (firmwareCached->getConfigBits( FW_CFG_MASTER_DMD_A_SWAP,&config_value) < 0) ||\
                (firmwareCached->getConfigBits(FW_CFG_MASTER_DMD_B_SWAP,&config_value) < 0) ||\
                (firmwareCached->getConfigBits( FW_CFG_MASTER_DMD_AB_SWAP,&config_value) < 0) ||\
                (firmwareCached->getConfigBits( FW_CFG_SLAVE_DMD_A_SWAP,&config_value) < 0) ||\
                (firmwareCached->getConfigBits(FW_CFG_SLAVE_DMD_B_SWAP,&config_value) < 0) ||\
                (firmwareCached->getConfigBits( FW_CFG_SLAVE_DMD_AB_SWAP,&config_value) < 0) ||\
                (firmwareCached->getConfigBits(FW_CFG_FLASH_READ_DELAY,&config_value) < 0) ||\
                (firmwareCached->getConfigBits(FW_CFG_FLASH_WRITE_DELAY,&config_value) < 0) ||\
                (firmwareCached->getConfigBits(FW_CFG_FLASH_WRITE_WIDTH,&config_value) < 0) ||\
                (firmwareCached->getConfigBits(FW_CFG_FLASH_CS_HOLD,&config_value) < 0) ||\
                (firmwareCached->getConfigBytes(FW_CFG_FW_TAG,tag) < 0))
        {
            if(firmwareCached)
                delete firmwareCached;

            return false;
        }
        else
        {
            m_cachedFimwareFileSlave = QString(fileInfoList.at(0).absolutePath()+"/"+fileInfoList.at(0).fileName());
        }
    }

    if(firmwareCached)
        delete firmwareCached;

    return true;
}

/**
 * @brief MainWindow::AddBatchFile
 * adds the Batch File commands displayed in the Batch Files frame "Batch Commads Sequence" to
 * the firmware with the name mentioned in the batchFileName_lineEdit textbox
 * If the Batch commands have on the fly command in it, sets the isAddPattenrs to true
 * @param isAddPatterns - O - a bool is set If Pattenrs hav eto be added to teh firmware
 * @return
 */
int MainWindow::addBatchFile(bool *isAddPatterns, BAT_BatchFile_t* batchFile, BAT_BatchFile_t* batchFileSlave)
{
    QString batchFileName = ui->batchFileName_lineEdit->text();
    if (batchFileName.isEmpty())
    {
        showStatus("Error: Enter a name for the batch file");
        return -1;
    }
    int numSplashImages = m_firmware->getNumSplashes();

    if(BAT_SetName(batchFile, batchFileName.toUtf8().constData()) < 0)
        showError(GET_ERR_STR());

    if(batchFileSlave != NULL)
    {
        if(BAT_SetName(batchFileSlave, batchFileName.toUtf8().constData()) < 0)
            showError(GET_ERR_STR());
    }

    if(BAT_IsOTFPtnPresent(batchFile))
    {
        BAT_OTF2SplashPtn(batchFile,numSplashImages);
        if(batchFileSlave != NULL)
        {
            BAT_OTF2SplashPtn(batchFileSlave,numSplashImages);
        }

        *isAddPatterns = true;
    }
    else
    {
        *isAddPatterns = false;
    }

    if(m_firmware->addBatchFile(batchFile) < 0)
        showError(GET_ERR_STR());

    if(m_dualAsic && m_firmwareSlave)
        m_firmwareSlave->addBatchFile(batchFileSlave);

    return 0;
}


/**
 * @brief MainWindow::AddPatterns
 * Adds the patterns from the Pattern Mode page, to the firmware Or updates
 * On the Fly
 * @return
 */
int MainWindow::addPatterns()
{
    int totalSplashImages;

    if(m_elements.size() <= 0)
    {
        showStatus("No patterns to add!");
        return -1;
    }

    if(calculateSplashImageDetails(&totalSplashImages, TRUE))
    {
        showStatus("Error while adding patterns to firmware");
        return -1;
    }

    if(updatePatternMemory(totalSplashImages, true))
    {
        showStatus("Error while adding patterns to firmware");
        return -1;
    }
    return 0;
}
/**
 * @brief MainWindow::UpdateFWConfigData
 * Gets the firmware configuration details from the repsective UI fields on Firmware Page
 * and updates the selected Firmware Image accordingly
 * @param fw - I - A pointer to the current Firmware Image that is selected
 */
void MainWindow::updateFWConfigData(Firmware *fw)
{
    QString firmwareTag = ui->firmwareTag_lineEdit->text();
    QByteArray ba = firmwareTag.toLocal8Bit();
    uint08 tag[32];

    //just fill contents with 0xFF since the content will be copied in both
    //master and slave images it will be easier to compare
    memset(tag,0xFF,sizeof(tag));
    strcpy((char *)tag,ba.data());

    if(fw == NULL)
        return;

    if(fw->setConfigBytes(FW_CFG_FW_TAG,tag) < 0)
        showError(GET_ERR_STR());

    if (ui->startupState_checkBox->isChecked())
    {
        if(fw->setConfigBits(FW_CFG_STARTUP_STATE,ui->startupState_comboBox->currentIndex()) < 0)
            showError(GET_ERR_STR());
    }

    if (ui->hdmiReceiver_checkBox->isChecked())
    {
        if(fw->setConfigBits(FW_CFG_HDMI_ENABLE, ui->hdmiReceiver_comboBox->currentIndex()) < 0)
            showError(GET_ERR_STR());
    }

    if (ui->defaultBatch_checkBox->isChecked())
    {
        if(fw->setConfigBits(FW_CFG_DEF_BATCH, ui->defaultBatch_comboBox->currentIndex() - 1) < 0)
            showError(GET_ERR_STR());
    }

    if(ui->seqCacheSize_checkBox->isChecked())
    {
        if(fw->setConfigBits(FW_CFG_SEQ_CACHE, ui->seqCacheSize_spinBox->value()) < 0)
            showError(GET_ERR_STR());
    }

    if (ui->setAsDefaultBatch->isChecked())
    {
        if(ui->clearAll_checkBox->isChecked())
        {
            if(fw->setConfigBits(FW_CFG_DEF_BATCH, 0) < 0)
                showError(GET_ERR_STR());
        }
        else
        {
            if(fw->setConfigBits(FW_CFG_DEF_BATCH, ui->defaultBatch_comboBox->count() - 1) < 0)
                showError(GET_ERR_STR());

        }
    }

    if (ui->i2cSlaveAdd_checkBox->isChecked())
    {
        if(fw->setConfigBits(FW_CFG_I2C_ADDRESS, ui->i2cSlaveAdd->text().toUInt()) < 0)
            showError(GET_ERR_STR());
    }

    if (ui->setSpreadSpectrum_checkBox->isChecked())
    {
        if(fw->setConfigBits(FW_CFG_SPREAD_SPECTRUM, ui->spreadSpectrum_comboBox->currentIndex()) < 0)
            showError(GET_ERR_STR());
    }

    if (ui->DMDBusSwap_checkBox->isChecked())
    {
        if(ui->bitSwapAMaster->isChecked())
        {
            if(fw->setConfigBits(FW_CFG_MASTER_DMD_A_SWAP, 1) < 0)
                showError(GET_ERR_STR());
        }
        else
        {
            if(fw->setConfigBits(FW_CFG_MASTER_DMD_A_SWAP, 0) < 0)
                showError(GET_ERR_STR());
        }

        if(ui->bitSwapBMaster->isChecked())
        {
            if(fw->setConfigBits(FW_CFG_MASTER_DMD_B_SWAP,1) < 0)
                showError(GET_ERR_STR());
        }
        else
        {
            if(fw->setConfigBits(FW_CFG_MASTER_DMD_B_SWAP,0) < 0)
                showError(GET_ERR_STR());
        }

        if(ui->channelSwapABMaster->isChecked())
        {
            if(fw->setConfigBits(FW_CFG_MASTER_DMD_AB_SWAP,1) < 0)
                showError(GET_ERR_STR());
        }
        else
        {
            if(fw->setConfigBits(FW_CFG_MASTER_DMD_AB_SWAP,0) < 0)
                showError(GET_ERR_STR());
        }

        if(ui->bitSwapASlave->isChecked())
        {
            if(fw->setConfigBits(FW_CFG_SLAVE_DMD_A_SWAP, 1) < 0)
                showError(GET_ERR_STR());
        }
        else
        {
            if(fw->setConfigBits(FW_CFG_SLAVE_DMD_A_SWAP, 0) < 0)
                showError(GET_ERR_STR());
        }

        if(ui->bitSwapBSlave->isChecked())
        {
            if(fw->setConfigBits(FW_CFG_SLAVE_DMD_B_SWAP,1) < 0)
                showError(GET_ERR_STR());
        }
        else
        {
            if(fw->setConfigBits(FW_CFG_SLAVE_DMD_B_SWAP,0) < 0)
                showError(GET_ERR_STR());
        }

        if(ui->channelSwapABSlave->isChecked())
        {
            if(fw->setConfigBits(FW_CFG_SLAVE_DMD_AB_SWAP,1) < 0)
                showError(GET_ERR_STR());
        }
        else
        {
            if(fw->setConfigBits(FW_CFG_SLAVE_DMD_AB_SWAP,0) < 0)
                showError(GET_ERR_STR());
        }

    }

    if(ui->updateFlashWaitCycle_checkBox->isChecked())
    {

        if(fw->setConfigBits(FW_CFG_FLASH_READ_DELAY,ui->readDelay->text().toUInt()) < 0)
            showError(GET_ERR_STR());
        if(fw->setConfigBits(FW_CFG_FLASH_WRITE_DELAY,ui->writeDelay->text().toUInt()) < 0)
            showError(GET_ERR_STR());
        if(fw->setConfigBits(FW_CFG_FLASH_WRITE_WIDTH,ui->writePulseWidth->text().toUInt()) < 0)
            showError(GET_ERR_STR());
        if(fw->setConfigBits(FW_CFG_FLASH_CS_HOLD,ui->CS_DataHoldTime->text().toUInt()) < 0)
            showError(GET_ERR_STR());

    }
}

/************************Handler functions******************/

/**
 * @brief MainWindow::on_chooseFirmware_Button_clicked
 */
void MainWindow::on_chooseFirmware_Button_clicked()
{
    QStringList fileNames;
    QFileInfo firmwareFileInfo;
    int indexMasterImage = 0;
    int indexSlaveImage = 0;

    //clearing the add  patterns, batch file controls
    ui->addBatchFile_checkBox->setChecked(false);
    ui->batchFileName_lineEdit->setText("from Batch Files tab");
    ui->clearAll_checkBox->setChecked(false);
    ui->addPat_checkBox->setChecked(false);
    ui->setAsDefaultBatch->setChecked(false);

    fileNames = QFileDialog::getOpenFileNames(this,
                                              QString("Select Firmware binary to update"),
                                              m_firmwarePath,
                                              "*.img");
    if (fileNames.size() <= 0)
        return;

    if((!m_dualAsic) && (fileNames.size() != 1))
    {
        showStatus("Choose only one firmware image for 1080p DMDs\n");
        return;
    }

    if(m_dualAsic)
    {
        if(fileNames.size() > 2)
        {
            showStatus("Maximum 2 firmware images can only be selected\n");
            return;
        }

        if (fileNames.size() == 2)
        {
            bool foundMaster = false, foundSlave = false;
            for(int i = 0; i < fileNames.size(); i++)
            {
                if(fileNames.at(i).endsWith("-master.img") || fileNames.at(i).endsWith("-master.bin"))
                {
                    foundMaster = true;
                    indexMasterImage = i;

                }
                if(fileNames.at(i).endsWith("-slave.img") || fileNames.at(i).endsWith("-slave.bin"))
                {
                    foundSlave = true;
                    indexSlaveImage = i;

                }
            }
            if (!foundMaster)
            {
                showStatus("Choose the correct master image. The image name should end with _master.img or _master.bin\n");
                return;
            }
            if (!foundSlave)
            {
                showStatus("Choose the correct slave image. The image name should end with _slave.img or _slave.bin\n");
                return;
            }
        }
    }

    firmwareFileInfo.setFile(fileNames.at(0));

    QDir dir = QFileInfo(firmwareFileInfo).absoluteDir();
    m_firmwarePath = dir.absolutePath();
    settings.setValue("FirmwarePath",m_firmwarePath);

    if (fileNames.size() == 2)
    {
        //This make the processChosenFirmwareImage file names order in the
        //processChosenFirmwareImage() is Master and Slave
        ui->firmwareName_textEdit->setText(fileNames.at(indexMasterImage));
        ui->firmwareName_textEdit->append(fileNames.at(indexSlaveImage));
    }
    else
    {
        ui->firmwareName_textEdit->setText(fileNames.at(0));
    }


    processChosenFirmwareImage();

}

/**
 * Flash programming callback funtion to update the progress
 * @param param Main window ui
 * @param oper Current operation
 * @param address Current Address
 * @param percent Percentage completed
 * @return 0
 */
static int flashCallback(void *param, FL_Operation_t oper, uint32 address,
                         uint08 percent)
{
    Ui::MainWindow *ui = (Ui::MainWindow *)param;
    char const *operText[] = { "Verifying", "Erasing", "Programming" };
    char dispText[256];
    sprintf(dispText, "%s : 0x%08X", operText[oper], address);
    ui->firmwareDownloadStat_textEdit->setText(dispText);
    ui->firmwareDownloadStat_progressBar->setValue(percent);
    QApplication::processEvents();
    return 0;
}

/**
 * @brief MainWindow::on_downloadFirmware_Button_clicked
 */

void MainWindow::on_downloadFirmware_Button_clicked()
{

    //Clear the status update
    m_cachedFimwareFile.clear();
    m_cachedFimwareFileSlave.clear();
    ui->firmwareDownloadStat_textEdit->clear();
    ui->firmwareDownloadStat_progressBar->setValue(0);

    QString fileList = ui->firmwareName_textEdit->toPlainText();
    if (fileList.isEmpty())
    {
        showStatus("Error: Choose the firmware image to be updated\n");
        return;
    }

    QStringList fileNames = fileList.split("\n", QString::SkipEmptyParts);
    int indexMasterImage = 0;
    int indexSlaveImage = 0;
    char const *errMsg = NULL;

    if(fileNames.size() <= 0)
        return;

    if((!m_dualAsic) && (fileNames.size() != 1))
    {
        showStatus("Choose only one firmware image for 1080p DMDs\n");
        return;
    }

    if(m_dualAsic)
    {
        if(fileNames.size() > 2)
        {
            showStatus("Maximum 2 firmware images can only be selected\n");
            return;
        }
        if(fileNames.size() == 2)
        {
            bool foundMaster = false, foundSlave = false;
            for(int i = 0; i < fileNames.size(); i++)
            {
                if(fileNames.at(i).endsWith("-master.img") || fileNames.at(i).endsWith("-master.bin"))
                {
                    foundMaster = true;
                    indexMasterImage = i;

                }
                if(fileNames.at(i).endsWith("-slave.img") || fileNames.at(i).endsWith("-slave.bin"))
                {
                    foundSlave = true;
                    indexSlaveImage = i;

                }
            }
            if (!foundMaster)
            {
                showStatus("Choose the correct master image. The image name should end with _master.img or _master.bin\n");
                return;
            }
            if (!foundSlave)
            {
                showStatus("Choose the correct slave image. The image name should end with _slave.img or _slave.bin\n");
                return;
            }
        }
    }

    QFileInfo firmwareFileInfo;
    QTime waitEndTime;
    BootLoaderStaus blInfoFrmHW;
    int blMajor = 0;
    int blMinor = 0;
    int blPatch = 0;
    bool blFWVerUpdateNeeded = true;
    BOOL isFlashContentChanged = TRUE;

    firmwareFileInfo.setFile(fileNames.at(0));

    QDir dir = QFileInfo(firmwareFileInfo).absoluteDir();
    m_firmwarePath = dir.absolutePath();
    settings.setValue("FirmwarePath",m_firmwarePath);

    uint32 blSize = m_firmware->getBLSize();
    uint32 progOffset = blSize;


    if((int)blSize < 0)
        showError(GET_ERR_STR());


    //Start timer
    QTime execMeaTimer;
    execMeaTimer.start();

    ui->firmwareupdate_label->setText("Entering programming mode...");

    if(LCR_EnterProgrammingMode() < 0)
    {
        errMsg = "Unable to Enter Programming Mode";
        goto exit;
    }

    //If going from normal mode to bootloader mode, wait until connection closes; then wait again for reconnection
    USB_Close();
    waitEndTime = QTime::currentTime().addSecs(5);

    while(true)
    {
        //Update the GUI
        QApplication::processEvents();
        //timeout in case the target is already in bootloader mode.
        if(QTime::currentTime() > waitEndTime)
        {
            break;
        }
    }

    while(ui->connectButton->isEnabled() == false)
    {
        QApplication::processEvents(); //Update the GUI
    }

    waitEndTime = QTime::currentTime().addSecs(3);

    while(true)
    {
        //Update the GUI
        QApplication::processEvents();
        //timeout in case the target is already in bootloader mode.
        if(QTime::currentTime() > waitEndTime)
        {
            break;
        }
    }

    ui->firmwareupdate_label->setText("Getting flash info...");
    QApplication::processEvents();

    //Disable the timer
    usbPollTimer->stop();

    FL_SetCallback(flashCallback, (void *)ui);

    if(LCR_GetBLStatus(&blInfoFrmHW) < 0)
    {
        errMsg = "Unable to get bootloader information";
        goto exit;
    }

    if(m_firmware->getBLVersion(&blMajor,&blMinor,&blPatch)== 0)
    {
        unsigned long blVerOnFile;
        unsigned long blVerOnHW;

        //Build big number of version
        blVerOnHW = MAKE_WORD32(0,blInfoFrmHW.blVerMajor,
                                blInfoFrmHW.blVerMinor,
                                blInfoFrmHW.blVerPatch);

        //Build big number of version
        blVerOnFile = MAKE_WORD32(0, blMajor, blMinor, blPatch);

        //If BL on HW is latest then skip
        if(blVerOnHW >= blVerOnFile)
        {
            blFWVerUpdateNeeded = false;
        }

        //Old firmware 1.1 detected, has v1.0.0 as BL Version
        //must use old method of erasing an programming the bootloader
        if(blVerOnHW <= 0x010000)
        {
            //Ensure the SetDataLength and SetAddress for firmware update
            //fallback to v1.0.0 bootloader supported commands
            FL_UseLegacyCommands(1);
        }
    }

    if(blFWVerUpdateNeeded)
    {
        QString title("BOOT LOADER UPDATE PENDING...");

        QString msg = QString("Bootloader needs update.\n"
                              "Exisiting on HW - v%1.%2.%3\nNew - %4.%5.%6.\n"
                              "Select YES to update bootloader")
                .arg(blInfoFrmHW.blVerMajor).arg(blInfoFrmHW.blVerMinor)
                .arg(blInfoFrmHW.blVerPatch).arg(blMajor)
                .arg(blMinor).arg(blPatch);

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(0, title, msg,
                                      QMessageBox::Yes|QMessageBox::No);

        if (reply != QMessageBox::Yes)
        {
            blFWVerUpdateNeeded = false;
        }
    }

    if(blFWVerUpdateNeeded)
        progOffset = 0;


    if(FL_UpdateFlashInfo("Flash/FlashDeviceParameters.txt") < 0)
    {
        errMsg = GET_ERR_STR();
        goto exit;
    }


//    if(ui->fastFlashUpdate_checkBox->isChecked())         //This feature allows to GUI to only update the sectors of the flash that have been altered
                                                            //since the last download. It creates a cache file and compares it on the user's machine
                                                            //**In testing a checkbox was impletmented to make this feature optional** release version makes this standard
                                                            // Advanced users choose to implement this as an optional feature or not
    if(true)
    {
       QFileInfo tempFileInfo;
       QDir dirCachedImage;

        if(m_dualAsic)
        {
            tempFileInfo.setFile(fileNames.at(indexMasterImage));
        }
        else
        {
            tempFileInfo.setFile(QFile(ui->firmwareName_textEdit->toPlainText()));
        }

        dirCachedImage.setPath(QString(tempFileInfo.absolutePath()+"/FlashLoaderCache"));
        if(dirCachedImage.exists() == true)
        {
            if(processCachedFirmwareImage() == false)
            {
                m_cachedFimwareFile.clear();
                if(m_dualAsic)
                    m_cachedFimwareFileSlave.clear();
            }
            else
            {
                ui->firmwareupdate_label->setText("Cache firmware detected...");
                QApplication::processEvents();
            }
        }
    }

    ui->firmwareDownloadStat_progressBar->setValue(0);

    if ((m_dualAsic) && (fileNames.size() == 2))
    {

#if 1
        //Enable master to process command
        LCR_EnableMasterSlave(TRUE,FALSE);

        //If cached file present then compare them
        if(m_cachedFimwareFile.isEmpty() == false)
        {
            ui->firmwareupdate_label->setText("DLPC900 Master comparing cached firmware with flash...");
            QApplication::processEvents();
            if(FL_CheckFlash(m_cachedFimwareFile.toLatin1().data(),progOffset,0) != SUCCESS)
            {
                QFile temp(m_cachedFimwareFile);
                temp.remove();
                m_cachedFimwareFile.clear();
                ui->firmwareupdate_label->setText("DLPC900 Master Cached firmware NOT matching");
                QApplication::processEvents();
            }
            else
            {
                ui->firmwareupdate_label->setText("DLPC900 Master Cached firmware matching");
                QApplication::processEvents();
            }
        }

        //Enable slave to process command
        LCR_EnableMasterSlave(FALSE,TRUE);

        //If cached file present then compare them
        if(m_cachedFimwareFileSlave.isEmpty() == false)
        {
            ui->firmwareupdate_label->setText("DLPC900 Slave Comparing cached firmware with flash...");
            QApplication::processEvents();
            if(FL_CheckFlash(m_cachedFimwareFileSlave.toLatin1().data(),progOffset,0) != SUCCESS)
            {
                QFile temp(m_cachedFimwareFileSlave);
                temp.remove();
                m_cachedFimwareFileSlave.clear();
                ui->firmwareupdate_label->setText("DLPC900 Slave Cached firmware NOT matching...");
                QApplication::processEvents();
            }
        }

        //Enable master to process command
        LCR_EnableMasterSlave(TRUE,FALSE);

        FL_ProgramSignature(m_firmware->getBLSize(), FALSE);

        //Enable slave to process command
        LCR_EnableMasterSlave(FALSE,TRUE);

        FL_ProgramSignature(m_firmware->getBLSize(), FALSE);
#endif

#if 1
        //Enable master to process command
        LCR_EnableMasterSlave(TRUE,FALSE);

        if(m_cachedFimwareFile.isEmpty() == false)
        {
            ui->firmwareupdate_label->setText("Programming DLPC900 Master...");
        }
        else
        {
            ui->firmwareupdate_label->setText("Programming DLPC900 Master...");
        }


        ui->firmwareDownloadStat_progressBar->setValue(0);
        QApplication::processEvents();

        if (FL_ProgramFlash(fileNames.at(indexMasterImage).toLatin1().data(), progOffset, 0,\
                            ((m_cachedFimwareFile.isEmpty() == false)?TRUE:FALSE),\
                            m_cachedFimwareFile.toLatin1().data(),&isFlashContentChanged, FALSE))
        {
            ui->firmwareDownloadStat_textEdit->clear();
            ui->firmwareDownloadStat_progressBar->setValue(0);
            errMsg = GET_ERR_STR();
            goto exit;
        }
#endif

        /* Wait. Else sometimes flash cannot be accessed */
        waitEndTime = QTime::currentTime().addSecs(3);
        while(true)
        {
            QApplication::processEvents(); //Update the GUI
            if(QTime::currentTime() > waitEndTime)
                break;
        }
#if 1

        //Enable slave to process command
        LCR_EnableMasterSlave(FALSE,TRUE);

        if(m_cachedFimwareFileSlave.isEmpty() == false)
        {
            ui->firmwareupdate_label->setText("Programming DLPC900 Slave...");
        }
        else
        {
            ui->firmwareupdate_label->setText("Programming DLPC900 Slave...");
        }


        ui->firmwareDownloadStat_progressBar->setValue(0);
        QApplication::processEvents();

        BOOL tempBOOL;
        if (FL_ProgramFlash(fileNames.at(indexSlaveImage).toLatin1().data(), progOffset, 0,\
                            ((m_cachedFimwareFileSlave.isEmpty() == false)?TRUE:FALSE),\
                            m_cachedFimwareFileSlave.toLatin1().data(),&tempBOOL, FALSE))
        {
            ui->firmwareDownloadStat_textEdit->clear();
            ui->firmwareDownloadStat_progressBar->setValue(0);
            errMsg = GET_ERR_STR();
            goto exit;
        }

#endif
        if(!isFlashContentChanged && (tempBOOL == TRUE))
           isFlashContentChanged=TRUE;
#if 1
        //Enable slave to process command
        LCR_EnableMasterSlave(FALSE,TRUE);

        FL_ProgramSignature(m_firmware->getBLSize(), TRUE);

        //Enable master to process command
        LCR_EnableMasterSlave(TRUE,FALSE);

        FL_ProgramSignature(m_firmware->getBLSize(), TRUE);
#endif

        //Enable both master and slave to process command
        LCR_EnableMasterSlave(TRUE,TRUE);
    }
    else
    {
        if(m_cachedFimwareFile.isEmpty() == false)
        {
            ui->firmwareupdate_label->setText("Comparing cached firmware with flash...");
            QApplication::processEvents();
            if(FL_CheckFlash(m_cachedFimwareFile.toLatin1().data(),progOffset,0) != SUCCESS)
            {
                QFile temp(m_cachedFimwareFile);
                temp.remove();
                m_cachedFimwareFile.clear();
                ui->firmwareupdate_label->setText("Cached firmware NOT matching...");
                QApplication::processEvents();
            }
            else
            {
                ui->firmwareupdate_label->setText("Cached firmware matching...");
                QApplication::processEvents();
            }
        }

        if(m_cachedFimwareFile.isEmpty() == false)
        {
            ui->firmwareupdate_label->setText("Programming DLPC900 ...");
        }
        else
        {
            ui->firmwareupdate_label->setText("Programming DLPC900 ...");
        }

        ui->firmwareDownloadStat_progressBar->setValue(0);
        QApplication::processEvents();

        if (FL_ProgramFlash(fileNames.at(0).toLatin1().data(), progOffset, 0,\
                            ((m_cachedFimwareFile.isEmpty() == false)?TRUE:FALSE),\
                            m_cachedFimwareFile.toLatin1().data(),&isFlashContentChanged, TRUE))
        {
            ui->firmwareDownloadStat_textEdit->clear();
            ui->firmwareDownloadStat_progressBar->setValue(0);
            errMsg = GET_ERR_STR();
            goto exit;
        }
    }

//    if(ui->fastFlashUpdate_checkBox->isChecked())         //This feature allows to GUI to only update the sectors of the flash that have been altered
                                                            //since the last download. It creates a cache file and compares it on the user's machine
                                                            //**In testing a checkbox was impletmented to make this feature optional** release version makes this standard
                                                            // Advanced users can implement this as an optional feature
    if(true)
    {
        QFileInfo tempFileInfo;
        QDir dirCachedImage;

        if(m_dualAsic)
        {
            tempFileInfo.setFile(fileNames.at(indexMasterImage));
        }
        else
        {
            tempFileInfo.setFile(QFile(ui->firmwareName_textEdit->toPlainText()));
        }

        dirCachedImage.setPath(QString(tempFileInfo.absolutePath()+"/FlashLoaderCache"));
        if(dirCachedImage.exists() == false)
        {
           dirCachedImage.setPath(QString(tempFileInfo.absolutePath()));
           dirCachedImage.mkdir("FlashLoaderCache");
           dirCachedImage.setPath(QString(tempFileInfo.absolutePath()+"/FlashLoaderCache"));
        }

        //Remove old contents in the director
        dirCachedImage.setFilter(QDir::Files);

        QDate cDate;
        QTime cTime;
        QString date = cDate.currentDate().toString("MMddyyyy");
        QString time = cTime.currentTime().toString("HHmmss");

        bool isSaveCacheSuccess = true;
        if(m_dualAsic)
        {

            ui->firmwareupdate_label->setText("Creating DLPC900 Master cached firmware...");
            ui->firmwareDownloadStat_progressBar->setValue(0);
            QApplication::processEvents();

            //Enable master to process command
            LCR_EnableMasterSlave(TRUE,FALSE);

            foreach(QString dirFile, dirCachedImage.entryList())
            {
                if(isFlashContentChanged && dirFile.contains("dlp9000_"))
                    dirCachedImage.remove(dirFile);
            }

            if(isFlashContentChanged)
            {
                if(FL_CheckFlash(fileNames.at(indexMasterImage).toLatin1().data(), progOffset, 0) != SUCCESS)
                {
                    isSaveCacheSuccess = false;
                    errMsg = GET_ERR_STR();
                }

                QString temp = m_firmwarePath+"/FlashLoaderCache/dlp9000_master_"+date+time;
                if(isSaveCacheSuccess)
                {
                    QFile srcFile;
                    srcFile.setFileName(fileNames.at(indexMasterImage));
                    if(srcFile.copy(temp) == false)
                    {
                        isSaveCacheSuccess = false;
                        errMsg = "Failed create cached copy of master binary";
                    }
                }
            }

            if(isFlashContentChanged && isSaveCacheSuccess)
            {
                ui->firmwareupdate_label->setText("Creating DLPC900 Slave cached firmware...");
                ui->firmwareDownloadStat_progressBar->setValue(0);
                QApplication::processEvents();

                //Enable slave to process command
                LCR_EnableMasterSlave(FALSE,TRUE);

                if(isFlashContentChanged)
                {
                    if(FL_CheckFlash(fileNames.at(indexSlaveImage).toLatin1().data(), progOffset, 0) != SUCCESS)
                    {
                        isSaveCacheSuccess = false;
                        errMsg = GET_ERR_STR();
                    }

                    QString temp = m_firmwarePath+"/FlashLoaderCache/dlp9000_slave_"+date+time;
                    if(isSaveCacheSuccess)
                    {
                        QFile srcFile;
                        srcFile.setFileName(fileNames.at(indexSlaveImage));
                        if(srcFile.copy(temp) == false)
                        {
                            isSaveCacheSuccess = false;
                            errMsg = "Failed create cached copy of slave binary";
                        }
                    }
                }
            }

            //Enable both master and slave to process command
            LCR_EnableMasterSlave(TRUE,TRUE);
        }
        else
        {
            ui->firmwareupdate_label->setText("Creating DLPC900 cached firmwrae...");
            ui->firmwareDownloadStat_progressBar->setValue(0);
            QApplication::processEvents();

            foreach(QString dirFile, dirCachedImage.entryList())
            {
                if(isFlashContentChanged && dirFile.contains("dlp6500_"))
                    dirCachedImage.remove(dirFile);
            }

            //Run checksum to ensure the both firmware image and partially
            //programmed flash checksum matching

            if(isFlashContentChanged)
            {
                if(FL_CheckFlash(fileNames.at(0).toLatin1().data(), progOffset, 0) != SUCCESS)
                {
                    isSaveCacheSuccess = false;
                    errMsg = GET_ERR_STR();
                }

                QString temp = m_firmwarePath+"/FlashLoaderCache/dlp6500_"+date+time;
                if(isSaveCacheSuccess)
                {
                    QFile srcFile;
                    srcFile.setFileName(fileNames.at(0));
                    if(srcFile.copy(temp) == false)
                    {
                        isSaveCacheSuccess = false;
                        errMsg = "Failed create cached copy DLPC900 firmware binary";
                    }
                }
            }

        }

        if(!isSaveCacheSuccess)
        {
            showError("Error occured during cached file storage");
        }
    }

  exit:
    if(errMsg)
    {
        showError(errMsg);
        ui->firmwareupdate_label->setText("Programming Failed.");
        ui->firmwareDownloadStat_textEdit->clear();
    }
    else
    {
        int timeLap = execMeaTimer.elapsed();

        //Convert milliseconds to HH:MM:SS
        timeLap /= 1000; //ms to seconds
        int hh = (timeLap/3600); //number of hours
        timeLap -= (hh*3600); //remaining seconds
        int mm = (timeLap/60); // number of minutes
        timeLap -= (mm*60); //remaining seconds
        int ss = timeLap;
        ui->firmwareupdate_label->setText("Programming Completed successfully.");
        ui->firmwareDownloadStat_textEdit->setText("Download Completed in "+
                                               QString("%1").arg(hh,2,10,QChar('0')) +
                                               ":" +
                                               QString("%1").arg(mm,2,10,QChar('0')) +
                                               ":" +
                                               QString("%1").arg(ss,2,10,QChar('0')) +
                                               "(time in HH:MM:SS)");

    }

    ui->firmwareDownloadStat_progressBar->setValue(0);
    QApplication::processEvents();

    LCR_ExitProgrammingMode(); //Exit programming mode; Start application.
    Sleep(2000);
    on_connectButton_clicked();
    m_firstConnect = false;
    usbPollTimer->start();
}

void MainWindow::on_saveBatchFile_Button_2_clicked()
{
    int index = ui->firmwareBatchFile_comboBox->currentIndex();

    if(index < 0 || index >= ui->firmwareBatchFile_comboBox->count())
        return;

    BAT_BatchFile_t* batchFile = m_firmware->getBatchFile(index);

    if(batchFile == NULL)
        return;

    QString batchFileName = QFileDialog::getSaveFileName(this,
                                              QString("Batch file name to save"),
                                              m_batchFilePath,
                                              "*.txt");

    if(BAT_SaveToFile(batchFile, batchFileName.toLatin1().data()) < 0)
    {
        showError(GET_ERR_STR());
        return;
    }
}

/**
 * @brief
 */
void MainWindow::on_firmwareBatchFile_comboBox_currentIndexChanged(int index)
{
    if(index < 0 || index >= ui->firmwareBatchFile_comboBox->count())
    {
        ui->frmwBatchFile_tbl->setRowCount(0);
        return;
    }

    BAT_BatchFile_t* batchFile = m_firmware->getBatchFile(index);

    if(batchFile == NULL)
    {
        ui->frmwBatchFile_tbl->setRowCount(0);
        return;
    }

    displayBatchFile(batchFile, ui->frmwBatchFile_tbl);
}

/**
 * @brief MainWindow::on_deleteBatch_Button_clicked
 */
void MainWindow::on_deleteBatch_Button_clicked()
{
    int id = ui->firmwareBatchFile_comboBox->currentIndex();
    if(m_firmware->removeBatchFile(id) < 0)
        showError(GET_ERR_STR());
    ui->firmwareBatchFile_comboBox->removeItem(id);
    ui->defaultBatch_comboBox->removeItem(id);
}
/**
 * @brief MainWindow::on_updateFirmware_Button_clicked
 */
void MainWindow::on_updateFirmware_Button_clicked()
{
    BAT_BatchFile_t *pSaveBatchFile = NULL;
    BAT_BatchFile_t *pSaveBatchFileSlave = NULL;
    try
    {

        bool isAddPatterns = ui->addPat_checkBox->isChecked();

        if (ui->firmwareName_textEdit->toPlainText().isEmpty())
        {
            showStatus("Error: Choose the firmware image to be updated\n");
            throw std::runtime_error("Error");
        }


        QStringList fileNameList = (ui->firmwareName_textEdit->toPlainText()).split("\n", QString::SkipEmptyParts);

        if(m_firmware == NULL)
        {
            m_firmware = new Firmware(fileNameList[0]);
        }
        else
        {
            delete m_firmware;
            m_firmware = new Firmware(fileNameList[0]);
        }

        if((fileNameList.size() > 1) && m_dualAsic)
        {   if(m_firmwareSlave == NULL)
            {
                m_firmwareSlave = new Firmware(fileNameList[1]);
            }
            else
            {
                delete m_firmwareSlave;
                m_firmwareSlave = new Firmware(fileNameList[1]);
            }
        }

        //if only one image is slected, save the same as slave
        if(m_dualAsic && (fileNameList.size() <= 1))
        {
            if(m_firmwareSlave == NULL)
            {
               m_firmwareSlave  = new Firmware(fileNameList[0]);
            }
            else
            {
               delete m_firmwareSlave;
               m_firmwareSlave  = new Firmware(fileNameList[0]);
            }
        }

        //update once this function added in firmware

        //Prevent using old Firmware v1.1 and v1.0 on this GUI tool
        int blMajor, blMinor,blPatch;
        if(m_firmware->getBLVersion(&blMajor,&blMinor,&blPatch) < 0)
        {
            //The firmware binary is old one and you must use old GUI
            //tool if you would like revert back
            showCriticalError("Please use GUI tool v1.1 or earlier to build images with firmware v1.1 or earlier..");
            throw std::runtime_error("Error");
        }

        //For Dual ASIC and the input has master and slave image selected check
        // the second file bootloader version is correct
        if(m_dualAsic && m_firmwareSlave)
        {
            if(m_firmwareSlave->getBLVersion(&blMajor,&blMinor,&blPatch) <  0)
            {
                //The firmware binary is old one and you must use old GUI
                //tool if you would like revert back
                showCriticalError("Please use GUI tool v1.1 or earlier to build images with firmware v1.1 or earlier..");
                throw std::runtime_error("Error");
            }
            //Check for dual asic system both master and slave has same number of batchfiles
            //and splash images else they are not in sync so signal error
            if(m_firmware->getNumBatchFiles()!=m_firmwareSlave->getNumBatchFiles())
            {
                showCriticalError("Master and Slave firmware image files not in sync - number of batchfiles mismatch");
                throw std::runtime_error("Error");
            }

            if(m_firmware->getNumSplashes()!=m_firmwareSlave->getNumSplashes())
            {
                showCriticalError("Master and Slave firmware image files not in sync - number of 24-bit images mismatch");
                throw std::runtime_error("Error");
            }

        }

        bool rmSpalshBatch = false;
        if (ui->clearAll_checkBox->isChecked())
        {
            int batchfileCount = m_firmware->getNumBatchFiles();

            if(batchfileCount < 0)
                showError(GET_ERR_STR());

            for(int i = batchfileCount - 1; i >= 0 ; i--)
            {
                rmSpalshBatch = true;
                if(m_firmware->removeBatchFile(i) < 0)
                    showError(GET_ERR_STR());
                if(m_dualAsic && m_firmwareSlave)
                {
                    if(m_firmwareSlave->removeBatchFile(i) < 0)
                        showError(GET_ERR_STR());
                }
            }

            int splashCount = m_firmware->getNumSplashes();
            if(splashCount < 0)
                showError(GET_ERR_STR());

            for(int i = splashCount - 1; i >= 0 ; i-- )
            {
                rmSpalshBatch = true;
                if(m_firmware->removeSplash(i) < 0)
                    showError(GET_ERR_STR());

                if(m_dualAsic && m_firmwareSlave)
                {
                    if(m_firmwareSlave->removeSplash(i) < 0)
                        showError(GET_ERR_STR());
                }
            }
        }


        if (ui->addBatchFile_checkBox->isChecked())
        {
            pSaveBatchFile = BAT_Copy(m_batchFile);
            if(m_dualAsic && m_firmwareSlave)
            {
                pSaveBatchFileSlave =  BAT_Copy(m_batchFile);
            }

            if (addBatchFile(&isAddPatterns, pSaveBatchFile, pSaveBatchFileSlave))
                throw std::runtime_error("Error");
        }

        if (ui->addPat_checkBox->isChecked())
        {
            if(isAddPatterns)
             {
                if (addPatterns())
                {
                    ui->addPat_checkBox->setChecked(true);
                    throw std::runtime_error("Error");
                }
            }
        }
        if (ui->firmwareName_textEdit->toPlainText().isEmpty())
        {
            showStatus("Error: Choose the firmware image to be updated\n");
            throw std::runtime_error("Error");
        }

        updateFWConfigData(m_firmware);


        //Save the it is .tmp file first then save it as original file
        if(rmSpalshBatch)
        {
            if(m_firmware->getNumSplashes()==0 && m_firmware->getNumBatchFiles()==0)
            {
                if(m_firmware->saveToFile(".tmp.bin") < 0)
                {
                    showError(GET_ERR_STR());
                    throw std::runtime_error("Error");
                }
                delete m_firmware;
                m_firmware = new Firmware(".tmp.bin");
            }
        }


        QString outFileName;

        //To avoid overwriting files with same name
        //add date and time information to file
        QDate cDate;
        QTime cTime;

        QString date = cDate.currentDate().toString("MMddyyyy");
        QString time = cTime.currentTime().toString("HHmmss");

        if(m_dualAsic)
            outFileName = "dlp9000_" + date + time + "_firmwareimg-master.img";
        else
            outFileName = "dlp6500_" + date + time + "_firmwareimg.img";

        QString outFile(m_firmwarePath + "/" + outFileName);

        if(m_firmware->saveToFile(outFile) < 0)
        {
            showError(GET_ERR_STR());
            throw std::runtime_error("Error");
        }

        if(rmSpalshBatch)
        {
            QFile tmpFile(".tmp.bin");
            tmpFile.remove();
            if(m_firmware == NULL)
            {
                showError("Clear All Existing Pattern Images and Batch Files, Error saving the file");
                throw std::runtime_error("Error");
            }
        }

        QString statusMaster = "Firmware binary saved as " + outFile + "\n";

        QString outFileslave = "";

        if(m_dualAsic && m_firmwareSlave)
        {
            updateFWConfigData(m_firmwareSlave);

            if(rmSpalshBatch)
            {
                if(m_firmwareSlave->getNumSplashes()==0 && m_firmwareSlave->getNumBatchFiles()==0)
                {
                    if(m_firmwareSlave->saveToFile(".tmp.bin") < 0)
                    {
                        showError(GET_ERR_STR());
                        throw std::runtime_error("Error");
                    }
                    delete m_firmwareSlave;
                    m_firmwareSlave = new Firmware(".tmp.bin");
                }
            }

            outFileName = "dlp9000_" + date + time + "_firmwareimg-slave.img";

            outFileslave = m_firmwarePath + "/" + outFileName;
            if(m_firmwareSlave->saveToFile(outFileslave) < 0)
            {
                showError(GET_ERR_STR());
                throw std::runtime_error("Error");
            }

            if(rmSpalshBatch)
            {
                QFile tmpFile(".tmp.bin");
                tmpFile.remove();
                if(m_firmware == NULL)
                {
                    showError("Clear All Existing Pattern Images and Batch Files, Error saving the file");
                    throw std::runtime_error("Error");
                }
            }

            QString status  = statusMaster + "Slave binary saved as " + outFileslave;
            showStatus((const char*)status.toLocal8Bit().data());

        }
        else
        {
            showStatus((const char*)statusMaster.toLocal8Bit().data());
        }
    }
    catch(std::exception &e)
    {
        showError("Exception in FW update");
    }

}
