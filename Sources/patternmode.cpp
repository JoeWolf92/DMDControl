/*
 * This file provides all the handler functions for the UI controls on
 * Pattern Mode page and required helper functions
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "API.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>
#include <QTime>
#include <QTimer>
#include <QProgressDialog>



/***************Helper Functions**************************/

/**
 * @brief MainWindow::UpdatePatternMemory
 * Creates Splash images from all the Pattern elements
 * If it is Firmware update, adds the splash images to the firmware
 * If on the fly, converts the slpash images to splash blocks and updates on teh fly
 * @param totalSplashImages - I - total number of Splash images to be updated in Firmware
 * @param firmware - I - boolean to determine if it is to update firmware or On the Fly mode
 * @return
 */
int MainWindow::updatePatternMemory(int totalSplashImages, BOOL firmware )
{

    //Stop timer
    usbPollTimer->stop();
    for(int image = 0; image < totalSplashImages; image++)
    {

        int spalshImageCount;

        if(firmware)
        {
            spalshImageCount = image;
        }
        else
        {
            spalshImageCount = totalSplashImages - 1 - image;
        }

        PtnImage merge_image(m_ptnWidth, m_ptnHeight,24);

        merge_image.fill(0);


        int i;
        int es = m_elements.size();
        for(i = 0; i < es; i++)
        {
            int ei = m_elements[i].splashImageIndex;
            if(ei != spalshImageCount)
                continue;
            int bitpos = m_elements[i].splashImageBitPos;
            int bitdepth = m_elements[i].bits;
            PtnImage image(m_elements[i].name);
            merge_image.merge(image,bitpos,bitdepth);
        }

        merge_image.swapColors(PTN_COLOR_RED, PTN_COLOR_BLUE, PTN_COLOR_GREEN);
        uint08* splash_block = NULL;

        PtnImage merge_image_master(m_ptnWidth, m_ptnHeight,24);
        PtnImage merge_image_slave(m_ptnWidth, m_ptnHeight,24);
        merge_image_master = merge_image;
        merge_image_slave = merge_image;
        if (m_dualAsic)
        {

            merge_image_master.crop(0, 0, m_ptnWidth/2, m_ptnHeight);
            merge_image_slave.crop(m_ptnWidth/2, 0, m_ptnWidth/2, m_ptnHeight);

            if(firmware == true)
            {
                if(m_firmware->addSplash(&merge_image_master,(SPL_Compression_t)ui->comboBox_imageCompression->currentIndex()) < 0)
                {
                    showError(GET_ERR_STR());
                    return -1;
                }
                if(m_firmwareSlave != NULL)
                {
                    if(m_firmwareSlave->addSplash(&merge_image_slave,(SPL_Compression_t)ui->comboBox_imageCompression->currentIndex()) < 0)
                    {
                        showError(GET_ERR_STR());
                        return -1;
                    }
                }
            }

            else
            {
                uint08* splash_block_master = NULL;
                uint08* splash_block_slave = NULL;

                int splashSizeMaster  = merge_image_master.toSplash(&splash_block_master,(SPL_Compression_t)ui->comboBox_imageCompression->currentIndex());
                int splashSizeSlave = merge_image_slave.toSplash(&splash_block_slave,(SPL_Compression_t)ui->comboBox_imageCompression->currentIndex());

                if(splashSizeMaster <= 0 || splashSizeSlave <= 0)
                    return -1;

                if(uploadPatternToEVM(true, spalshImageCount, splashSizeMaster, splash_block_master) == -1)
                    return -1;

                if(uploadPatternToEVM(false, spalshImageCount, splashSizeSlave, splash_block_slave) == -1)
                    return -1;
            }
        }
        else
        {
            if(firmware == true)
            {
                if( m_firmware->addSplash(&merge_image,(SPL_Compression_t)ui->comboBox_imageCompression->currentIndex()) < 0)
                {
                    showError(GET_ERR_STR());
                    return -1;
                }
            }
            else

            {
                int splashSize = merge_image.toSplash(&splash_block,(SPL_Compression_t)ui->comboBox_imageCompression->currentIndex());
                if(splashSize <= 0)
                    return -1;                
                if(uploadPatternToEVM(true, spalshImageCount, splashSize, splash_block) < 0)
                    return -1;
            }
        }
    }

    usbPollTimer->start();

    return 0;
}


/**
 * @brief MainWindow::UpdateLUTOnTheFly
 * Updates the Pattern images into the Splash block on the Firmware image in the EVM on the fly
 * @param master - I - boolean to indicate if it is madetr or slave
 * @param splashImageCount - I - the Index of the Splash Image to be updated
 * @param splash_size - I - size of the splash image that is being updated
 * @param splash_block - I - the updated splash block
 * @return
 */
int MainWindow::uploadPatternToEVM(bool master, int splashImageCount, int splash_size, uint08* splash_block)
{    
    int origSize = splash_size;

    LCR_InitPatternMemLoad(master, splashImageCount, splash_size);

    QProgressDialog imgDataDownload("Image data download", "Abort", 0, splash_size, this);
    imgDataDownload.setWindowTitle(QString("Pattern Data Download.."));
    imgDataDownload.setWindowModality(Qt::WindowModal);
    imgDataDownload.setLabelText(QString("Uploading to EVM"));
    imgDataDownload.setValue(0);
    int imgDataDwld = 0;
    imgDataDownload.setMaximum(origSize);
    imgDataDownload.show();
    QApplication::processEvents();
    while(splash_size > 0)
    {
        int dnldSize = LCR_pattenMemLoad(master, splash_block + (origSize - splash_size), splash_size);
        if (dnldSize < 0)
        {
            // free(imageBuffer);
            showCriticalError("Downloading failed");
            usbPollTimer->start();
            imgDataDownload.close();
            return -1;
        }

        splash_size -= dnldSize;

        if (splash_size < 0)
            splash_size = 0;

        imgDataDwld += dnldSize;
        imgDataDownload.setValue(imgDataDwld);
         QApplication::processEvents();
        if(imgDataDownload.wasCanceled())
        {
            imgDataDownload.setValue(splash_size);
            imgDataDownload.close();
            return -1;
        }
    }

    QApplication::processEvents();
    imgDataDownload.close();

    return 0;

}

/**
 * @brief MainWindow::GetMinDarktime
 * get the minimum accepted darktime based on the selected bit depth and DMD bolcks
 * @param bitDepth - I - bit Depth selected
 * @return
 */
int MainWindow::GetMinDarktime(int )
{
    return minPatExposure[0];
}

/**
 * @brief MainWindow::GetMinExposure
 * get the minimum accepted exposure based on the selected bit depth and DMD bolcks
 * @param bitDepth - I - Bitdepth of teh selected Pattern Image
 * @return
 */
int MainWindow::GetMinExposure(int bitDepth)
{
    return minPatExposure[bitDepth-1];
}

/**
 * @brief MainWindow::resizeEvent
 * redraws the Waveform window(i.e the Pattern Image window) on window resize
 */
void MainWindow::resizeEvent(QResizeEvent *)
{
    waveWindow->draw();
}
/**
 * @brief MainWindow::UpdatePtnCheckbox
 */

void MainWindow::updatePtnCheckbox(void)
{
    bool enable = m_videoPatternMode == false && m_elements.size() > 0;
    if(enable)
    {
        ui->addPat_checkBox->setEnabled(true);
    }
    else
    {
        ui->addPat_checkBox->setChecked(false);
        ui->addPat_checkBox->setEnabled(false);
    }
}

/***************Handler Functions**************************/

/**
 * @brief MainWindow::on_saveButton_patternSettings_clicked
 * capability to save the pattern settings in a file
 */
void MainWindow::on_saveButton_patternSettings_clicked()
{

    if(m_elements.size() <= 0)
    {
        showStatus("Error:No pattern sequence to save");
        return;
    }

    QString fileName;

    fileName = QFileDialog::getSaveFileName(this,QString("Enter name of the settings file"),
                                            m_ptnSettingPath,
                                            tr("settings files(*.txt)"));


    if(fileName.isEmpty())
        return;

    QFile settingsFile(fileName);




    if(!settingsFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        showStatus("Error:Unable to open file for saving\n");
        return;
    }

    QDir dir = QFileInfo(settingsFile).absoluteDir();
    m_ptnSettingPath = dir.absolutePath();
    settings.setValue("PtnSettingPath",m_ptnSettingPath);

    QTextStream out(&settingsFile);

    if(m_videoPatternMode)
    {
        out << "Video Mode\n";

        for(int i=0;i<m_elements.size();i++)
        {
            out << m_elements[i].splashImageBitPos << ",";
            out << m_elements[i].bits <<",";
            out << m_elements[i].exposure <<",";
            out << m_elements[i].darkPeriod <<",";
            out << m_elements[i].color <<",";
            out << m_elements[i].trigIn <<",";
            out << m_elements[i].trigOut2 <<"\n";
        }

    }
    else
    {
        QDir myDir = QFileInfo(settingsFile).absoluteDir();

        out << "Normal Mode\n";
        for(int i=0;i<m_elements.size();i++)
        {
            //store only the realtive path of the image
            QString relPath = myDir.relativeFilePath(m_elements[i].name);

            out << relPath << ",";
            out << m_elements[i].bits <<",";
            out << m_elements[i].exposure <<",";
            out << m_elements[i].darkPeriod <<",";
            out << m_elements[i].color <<",";
            out << m_elements[i].trigIn <<",";
            out << m_elements[i].trigOut2 <<"\n";

        }
    }

    settingsFile.close();
}

/**
 * @brief MainWindow::on_loadButton_patternSettings_clicked
 * capability to load a presaved settings from a file
 */
void MainWindow::on_loadButton_patternSettings_clicked()
{
    QString fileName;

    char dispStr[255];

    fileName = QFileDialog::getOpenFileName(this,
                                            QString("Choose Settings File"),
                                            m_ptnSettingPath,
                                            tr("Settings File(*.txt)"));

    if(fileName.isEmpty())
        return;

    QFile settingsFile(fileName);
    if(!settingsFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        showStatus("Unable to open the settings file\n");
        return;
    }


    //first clear the existing patterns on window

    if(m_elements.size() > 0)
        m_elements.clear();

    QDir dir = QFileInfo(settingsFile).absoluteDir();
    m_ptnSettingPath = dir.absolutePath();
    settings.setValue("PtnSettingPath",m_ptnSettingPath);

    QTextStream in(&settingsFile);

    QString line;

    if(m_videoPatternMode)
    {

        line = in.readLine();

        if(line != "Video Mode")
        {
            showStatus("Please select a valid Video Pattern settings file");
            return;
        }
        while(!in.atEnd())
        {
            PatternElement pattern;

            line = in.readLine();

            if(line == "\n" || line == "") //error checking for empty lines
                continue;

            QStringList list = line.split(",");

            //calculating the mask from startbit and bitdepth

            pattern.bits = (list.size() >= 2)?list[1].toInt():1;
            pattern.splashImageBitPos = (list.size() >= 1)?list[0].toInt():0x1;
            pattern.exposure  = (list.size() >= 3)?list[2].toInt():GetMinExposure(1);
            pattern.darkPeriod  = (list.size() >= 4)?list[3].toInt():0;
            pattern.color  = (list.size() >= 5)?PatternElement::Color(list[4].toInt()):PatternElement::RED ;
            pattern.trigIn = (list.size() >= 6)?list[5].toInt():false;
            pattern.trigOut2 = (list.size() >= 7)?list[6].toInt():true;

            m_elements.push_back(pattern);

        }
    }
    else
    {
        QString first_line = in.readLine();
        if(first_line.contains("v2") && (ui->patternMode_radioButton->isChecked()))
        {
            ui->autoupdate_checkBox->setChecked(FALSE);
        }
        else
        {
            ui->autoupdate_checkBox->setChecked(TRUE);
        }
        on_autoupdate_checkBox_clicked();

        while(!in.atEnd())
        {
            PatternElement pattern;

            line = in.readLine();

            if(line == "\n" || line == "") //error checking for empty lines
                continue;

            QStringList list = line.split(",");

            if(list.size() == 0)
                continue;

            pattern.name = list[0];

            QString pFileStr = m_ptnSettingPath + "/" + list[0];

            QFile patternFile(pFileStr);

            //check if the pattern image file exists
            if(!patternFile.exists())
            {

                sprintf(dispStr, "the pattern image file %s does not exist", pFileStr.toStdString().c_str());
                showStatus(dispStr);
                return;
            }


            //backward capability for adding from List
            //where only the image file is given

            pattern.name = pFileStr;
            pattern.bits = (list.size() >= 2)?list[1].toInt():1;
            pattern.exposure  = (list.size() >= 3)?list[2].toInt():GetMinExposure(1);
            pattern.darkPeriod  = (list.size() >= 4)?list[3].toInt():0;
            pattern.color  = (list.size() >= 5)?PatternElement::Color(list[4].toInt()):PatternElement::RED ;
            pattern.trigIn = (list.size() >= 6)?list[5].toInt():false;
            pattern.trigOut2 = (list.size() >= 7)?list[6].toInt():true;
            if(!ui->autoupdate_checkBox->isChecked())
            {
                pattern.splashImageIndex = (list.size() >= 9)?list[7].toInt():0;
                pattern.splashImageBitPos = (list.size() >= 9)?list[8].toInt():0;
            }

            m_elements.push_back(pattern);
            m_patternImageChange = true;

        }
    }
    if (m_elements.size() > 0)
    {
        waveWindow->updatePatternList(m_elements);
        waveWindow->draw();
        ui->zoomSlider->setEnabled(true);
        ui->selectAllButton->setEnabled(true);
        ui->ptnSetting_groupBox->setEnabled(true);
        ui->removePatternsButton->setEnabled(true);
        on_patternSelect(m_elements.size()-1,m_elements);
        isPatternLoaded = TRUE;
        ui->editLUT_Button->setEnabled(TRUE);
    }
    settingsFile.close();

    updatePtnCheckbox();
}

/**
 * @brief MainWindow::on_addPatternsButton_clicked
 */
void MainWindow::on_addPatternsButton_clicked()
{
    int i;
    int numPatAdded = 0;

    if (m_videoPatternMode)
    {
        PatternElement pattern;
        if(m_elements.size()==0)
        {
            pattern.bits = 1;
            pattern.color = PatternElement::RED;
            pattern.exposure = GetMinExposure(1);
            pattern.darkPeriod = 0;
            pattern.trigIn = false;
            pattern.trigOut2 = true;
            pattern.splashImageBitPos = 0;
            pattern.splashImageIndex = 0;
        }
        else
        {
            for(i=0;i<m_elements.size();i++)
                m_elements[i].selected=false;
            pattern.bits = m_elements[m_elements.size()-1].bits;
            pattern.color = m_elements[m_elements.size()-1].color;
            pattern.exposure = m_elements[m_elements.size()-1].exposure;
            pattern.darkPeriod =m_elements[m_elements.size()-1].darkPeriod;
            pattern.trigIn = m_elements[m_elements.size()-1].trigIn;
            pattern.trigOut2 = m_elements[m_elements.size()-1].trigOut2;
            pattern.splashImageBitPos = m_elements[m_elements.size()-1].splashImageBitPos +
                    m_elements[m_elements.size()-1].bits;
            pattern.splashImageIndex = 0;
        }

        pattern.selected = true;
        m_elements.append(pattern);
        m_elements[0].trigIn = true;    // First pattern always on a Frame Change
        numPatAdded = 1;
    }
    else
    {
        QStringList fileNames;

        fileNames = QFileDialog::getOpenFileNames(this,
                                                  QString("Select Image for Pattern"),
                                                  m_ptnImagePath,
                                                  "*.bmp *.png");
        if(fileNames.isEmpty())
            return;

        fileNames.sort();

        QDir dir = QFileInfo(QFile(fileNames.at(0))).absoluteDir();
        m_ptnImagePath = dir.absolutePath();
        settings.setValue("PtnImagePath",m_ptnImagePath);

        for(i=0;i<m_elements.size();i++)
            m_elements[i].selected=false;

        for(i = 0; i < fileNames.size(); i++)
        {
#if 0
            QByteArray ba = fileNames.at(i).toLocal8Bit();
            int width, height;

            BMP_GetImageSize(ba.data(), &width, &height);
            if ((width != m_ptnWidth) || (height != m_ptnHeight))
            {

                if (m_dualAsic)
                    sprintf(errMsg, "Error: Cannot add image %s. Image resolution permitted is 2560 * 1600", ba.data());
                else
                    sprintf(errMsg, "Error: Cannot add image %s. Image resolution permitted is 1920 * 1080", ba.data());

                showCriticalError(errMsg);
                continue;
            }
#endif
            PatternElement pattern;

            if(m_elements.size()==0)
            {
                pattern.bits = 1;
                pattern.color = PatternElement::RED;
                pattern.exposure = GetMinExposure(1);
                pattern.darkPeriod = 0;
                pattern.trigIn = false;
                pattern.trigOut2 = true;
                if(!ui->autoupdate_checkBox->isChecked())
                {
                    pattern.splashImageIndex = 0;
                    pattern.splashImageBitPos = 0;
                }
            }
            else
            {
                pattern.bits = m_elements[m_elements.size()-1].bits;
                pattern.color = m_elements[m_elements.size()-1].color;
                pattern.exposure = m_elements[m_elements.size()-1].exposure;
                pattern.darkPeriod = m_elements[m_elements.size()-1].darkPeriod;
                pattern.trigIn = m_elements[m_elements.size()-1].trigIn;
                pattern.trigOut2 = m_elements[m_elements.size()-1].trigOut2;
                if(!ui->autoupdate_checkBox->isChecked())
                {
                    pattern.splashImageIndex = m_elements[m_elements.size()-1].splashImageIndex;
                    pattern.splashImageBitPos = m_elements[m_elements.size()-1].splashImageBitPos;
                }
            }


            pattern.name = fileNames.at(i);
            pattern.selected = true;

            m_elements.append(pattern);
            numPatAdded++;
            m_patternImageChange = true;
        }
    }

    if (m_elements.size() > 0)
    {
        waveWindow->updatePatternList(m_elements);
        waveWindow->draw();
        ui->zoomSlider->setEnabled(true);
        ui->selectAllButton->setEnabled(true);
        ui->ptnSetting_groupBox->setEnabled(true);
        ui->removePatternsButton->setEnabled(true);
        on_patternSelect(m_elements.size()-1,m_elements);
        isPatternLoaded = TRUE;
        ui->editLUT_Button->setEnabled(TRUE);
    }

    updatePtnCheckbox();
    return;
}
/**
 * @brief MainWindow::on_removePatternsButton_clicked
 */
void MainWindow::on_removePatternsButton_clicked()
{
    QList<PatternElement>::Iterator pattern = m_elements.begin();
    while(pattern != m_elements.end())
    {
        if(pattern->selected)
        {
            pattern = m_elements.erase(pattern);
            m_patternImageChange = true;
        }
        else
            pattern++;
    }

    waveWindow->updatePatternList(m_elements);
    waveWindow->select(WaveFormWindow::SELECT_NONE, -1);
    waveWindow->draw();
    if (!m_elements.size())
    {
        ui->zoomSlider->setEnabled(false);
        ui->selectAllButton->setEnabled(false);
        isPatternLoaded = FALSE;
        ui->editLUT_Button->setEnabled(FALSE);
    }

    updatePtnCheckbox();
}
/**
 * @brief MainWindow::on_updateLUT_Button_clicked
 */
void MainWindow::on_updateLUT_Button_clicked()
{
    int totalSplashImages = 0;
    int ret;
    QTime waitEndTime;
    char errStr[255];

    if(m_elements.size() <= 0)
    {
        showStatus("Error:No pattern sequence to send");
        return;
    }

    LCR_ClearPatLut();

    if (!m_videoPatternMode)
    {
        if(ui->autoupdate_checkBox->isChecked())
        {
            if (calculateSplashImageDetails(&totalSplashImages, FALSE))
                return;
        }
    }

    for(int i = 0; i < m_elements.size(); i++)
    {
        if(LCR_AddToPatLut(i, m_elements[i].exposure, true, m_elements[i].bits, m_elements[i].color, m_elements[i].trigIn, m_elements[i].darkPeriod, m_elements[i].trigOut2, m_elements[i].splashImageIndex, m_elements[i].splashImageBitPos)<0)
        {
            sprintf(errStr,"Unable to add pattern number %d to the LUT",i);
            showError(QString::fromLocal8Bit(errStr));
            break;
        }
    }

    if (LCR_SendPatLut() < 0)
    {
        showError("Sending pattern LUT failed!");
        return;
    }

    if (ui->repeat_radioButton->isChecked())
        ret = LCR_SetPatternConfig(m_elements.size(), 0);
    else
        ret = LCR_SetPatternConfig(m_elements.size(), m_elements.size());
    if (ret < 0)
    {
        showError("Sending pattern LUT size failed!");
        return;
    }

    if (ui->patternMemory_radioButton->isChecked() && m_patternImageChange)
    {
        if(updatePatternMemory(totalSplashImages, false) == 0)
        {
            m_patternImageChange = false;
        }
    }
}
/**
 * @brief MainWindow::on_startPatSequence_Button_clicked
 */
void MainWindow::on_startPatSequence_Button_clicked()
{
    unsigned int repeat;

    if(ui->repeat_radioButton->isChecked())
        repeat = 0;
    else
        repeat = m_elements.size();

    if(LCR_SetPatternConfig(m_elements.size(), repeat)<0)
    {
        showError("Error in setting LUT Configuration!");
        return;
    }
    if (LCR_PatternDisplay(0x2) < 0)
        showError("Unable to stat pattern display");
}
/**
 * @brief MainWindow::on_pausePatSequence_Button_clicked
 */
void MainWindow::on_pausePatSequence_Button_clicked()
{
    if (LCR_PatternDisplay(0x1) < 0)
        showError("Unable to pause pattern display");
}
/**
 * @brief MainWindow::on_stopPatSequence_Button_clicked
 */
void MainWindow::on_stopPatSequence_Button_clicked()
{
    if (LCR_PatternDisplay(0x0) < 0)
        showError("Unable to stop pattern display");
}
/**
 * @brief MainWindow::on_selectAllButton_clicked
 */
void MainWindow::on_selectAllButton_clicked()
{
    waveWindow->updatePatternList(m_elements);
    waveWindow->select(WaveFormWindow::SELECT_ALL, -1);
    waveWindow->draw();
}

/**
 * @brief MainWindow::on_dmdBlockSet_clicked
 */

void MainWindow::on_dmdBlockSet_clicked()
{
    int start = ui->startDmdBlock->currentText().toInt();
    int end =  ui->endDmdBlock->currentText().toInt();

    if(LCR_SetDMDBlocks(start, end - start + 1) < 0)
    {
        showError("Error setting active DMD blocks");
    }
    updateMinExposure();
}
/**
 * @brief MainWindow::on_dmdBlockGet_clicked
 */
void MainWindow::on_dmdBlockGet_clicked()
{
    int start;
    int count;

    if(LCR_GetDMDBlocks(&start, &count) < 0)
    {
        showError("Error getting active DMD blocks");
        return;
    }
    ui->startDmdBlock->setCurrentIndex(start);
    ui->endDmdBlock->setCurrentIndex(count - 1);
    updateMinExposure();
}
/**
 * @brief MainWindow::on_pushButton_DMDSaverModeOn_clicked
 */
void MainWindow::on_pushButton_DMDSaverModeOn_clicked()
{
    int SLmode = 0;

    if(LCR_GetMode(&SLmode) == 0)
    {
        if(!SLmode) //this is not supported in video mode
        {
            showStatus("Error:Option not supported in Video Mode");
            return;
        }

    }


    short mode = 1;

    if(LCR_SetDMDSaverMode(mode) < 0)
    {
        showError("DMD Saver Mode Enable failed!");
        return;
    }
    else
    {
        ui->pushButton_DMDSaverModeOn->setEnabled(false);
        ui->pushButton_DMDSaverModeOff->setEnabled(true);
    }
}
/**
 * @brief MainWindow::on_pushButton_DMDSaverModeOff_clicked
 */
void MainWindow::on_pushButton_DMDSaverModeOff_clicked()
{

    int SLmode = 0;

    if(LCR_GetMode(&SLmode) == 0)
    {
        if(!SLmode) //this is not supported in video mode
        {
            showStatus("Error:Option not supported in Video Mode");
            return;
        }

    }

    short mode = 0;


    if(LCR_SetDMDSaverMode(mode) < 0)
    {
        showError("DMD Saver Mode Disable failed!");
        return;
    }
    else

    {
        ui->pushButton_DMDSaverModeOff->setEnabled(false);
        ui->pushButton_DMDSaverModeOn->setEnabled(true);
    }
}
/**
 * @brief MainWindow::on_trigOut1GetButton_clicked
 */
void MainWindow::on_trigOut1GetButton_clicked()
{
    int invert;
    short risingDelay, fallingDelay;

    int ret = LCR_GetTrigOutConfig(1, &invert, &risingDelay, &fallingDelay);
    if (ret)
    {
        showError("Unable to get Trigger Out1 details");
        return;
    }
    ui->invertTrigOut1_checkBox->setChecked(invert);
    ui->trigOut1_risingDelay_spinBox->setValue(risingDelay);
    ui->trigOut1_fallingDelay_spinBox->setValue(fallingDelay);
}
/**
 * @brief MainWindow::on_trigOut1SetButton_clicked
 */
void MainWindow::on_trigOut1SetButton_clicked()
{
    int ret = LCR_SetTrigOutConfig(1, ui->invertTrigOut1_checkBox->isChecked(), ui->trigOut1_risingDelay_spinBox->value(), ui->trigOut1_fallingDelay_spinBox->value());

    if (ret <= 0)
        showError("Unable to set Trigger Out1 details");
}
/**
 * @brief MainWindow::on_trigOut2GetButton_clicked
 */
void MainWindow::on_trigOut2GetButton_clicked()
{
    int invert;
    short risingDelay, fallingDelay;

    int ret = LCR_GetTrigOutConfig(2, &invert, &risingDelay, &fallingDelay);
    if (ret)
    {
        showError("Unable to get Trigger Out2 details");
        return;
    }
    ui->invertTrigOut2_checkBox->setChecked(invert);
    ui->trigOut2_risingDelay_spinBox->setValue(risingDelay);
    ui->trigOut2_fallingDelay_spinBox->setValue(fallingDelay);

}
/**
 * @brief MainWindow::on_trigOut2SetButton_clicked
 */
void MainWindow::on_trigOut2SetButton_clicked()
{
    int ret = LCR_SetTrigOutConfig(2, ui->invertTrigOut2_checkBox->isChecked(), ui->trigOut2_risingDelay_spinBox->value(), ui->trigOut2_fallingDelay_spinBox->value());

    if (ret <= 0)
        showError("Unable to set Trigger Out2 details");
}
/**
 * @brief MainWindow::on_trigIn1SetButton_clicked
 */
void MainWindow::on_trigIn1SetButton_clicked()
{
    if(LCR_SetTrigIn1Config(ui->invertTrigIn1_checkBox->isChecked(),ui->trigIn1_Delay_spinBox->value())<0)
    {
        showError("Unable to set Trig In 1 Config!");
        return;
    }

    waveWindow->updateTriggerInType(ui->invertTrigIn1_checkBox->isChecked());
    waveWindow->draw();
}
/**
 * @brief MainWindow::on_trigIn1GetButton_clicked
 */
void MainWindow::on_trigIn1GetButton_clicked()
{
    int invert;
    unsigned int trigDelay;
    if(LCR_GetTrigIn1Config(&invert,&trigDelay)<0)
        showError("Unable to get Trig In 1 Config!");
    else
    {
        ui->invertTrigIn1_checkBox->setChecked(invert);
        ui->trigIn1_Delay_spinBox->setValue(trigDelay);
    }
}
/**
 * @brief MainWindow::on_trigIn2GetButton_clicked
 */
void MainWindow::on_trigIn2GetButton_clicked()
{
    int invert;

    if(LCR_GetTrigIn2Config(&invert)<0)
        showError("Unable to get Trig In 2 Config!");
    else
        ui->invertTrigIn2_checkBox->setChecked(invert);

}
/**
 * @brief MainWindow::on_trigIn2SetButton_clicked
 */
void MainWindow::on_trigIn2SetButton_clicked()
{
    if(LCR_SetTrigIn2Config(ui->invertTrigIn2_checkBox->isChecked())<0)
        showError("Unable to Set Trig In 2 Config!");
}
/**
 * @brief MainWindow::on_ledGetDelayButton_clicked
 */
void MainWindow::on_ledGetDelayButton_clicked()
{
    int ret;
    short risingDelay, fallingDelay;

    ret = LCR_GetRedLEDStrobeDelay(&risingDelay, &fallingDelay);
    if (ret)
    {
        showError("Unable to get Red Led Delay");
    }
    else
    {
        ui->ledRedRising_spinBox->setValue(risingDelay);
        ui->ledRedFalling_spinBox->setValue(fallingDelay);
    }

    ret = LCR_GetGreenLEDStrobeDelay(&risingDelay, &fallingDelay);
    if (ret)
    {
        showError("Unable to get Green Led Delay");
    }
    else
    {
        ui->ledGreenRising_spinBox->setValue(risingDelay);
        ui->ledGreenFalling_spinBox->setValue(fallingDelay);
    }

    ret = LCR_GetBlueLEDStrobeDelay(&risingDelay, &fallingDelay);
    if (ret)
    {
        showError("Unable to get Blue Led Delay");
    }
    else
    {
        ui->ledBlueRising_spinBox->setValue(risingDelay);
        ui->ledBlueFalling_spinBox->setValue(fallingDelay);
    }
}
/**
 * @brief MainWindow::on_ledSetDelayButton_clicked
 */
void MainWindow::on_ledSetDelayButton_clicked()
{
    int ret;

    ret = LCR_SetRedLEDStrobeDelay(ui->ledRedRising_spinBox->value(), ui->ledRedFalling_spinBox->value());
    if (ret <= 0)
        showError("Unable to set Red Led delay");

    ret = LCR_SetGreenLEDStrobeDelay(ui->ledGreenRising_spinBox->value(), ui->ledGreenFalling_spinBox->value());
    if (ret <= 0)
        showError("Unable to set Green Led delay");

    ret = LCR_SetBlueLEDStrobeDelay(ui->ledBlueRising_spinBox->value(), ui->ledBlueFalling_spinBox->value());
    if (ret <= 0)
        showError("Unable to set Blue Led delay");
}

/**
 * @brief MainWindow::on_patternSelect
 * updates the GUI contraols on selecting a particular pattern
 * @param index - I - index of teh selected PatternElement
 * @param patElem - I - the selected pattern element
 */
void MainWindow::on_patternSelect(int index, QList<PatternElement> patElem)
{

    m_elements = patElem;


    if (index < 0)
    {
        ui->ptnSetting_groupBox->setTitle("Pattern -");
        ui->ptnSetting_groupBox->setEnabled(false);
        ui->removePatternsButton->setEnabled(false);
        return;
    }

    ui->ptnSetting_groupBox->setEnabled(true);
    ui->ptnSetting_groupBox->setTitle(QString("Pattern %1").arg(index));
    ui->color_ComboBox->setCurrentIndex(m_elements[index].color - 1);
    ui->triggerIn_checkBox->setChecked(m_elements[index].trigIn);
    ui->triggerOut2_checkBox->setChecked(m_elements[index].trigOut2);
    ui->bitDepth_ComboBox->setCurrentIndex(m_elements[index].bits - 1);
    ui->exposure_lineEdit->setText(QString::number(m_elements[index].exposure));
    ui->darkPeriod_lineEdit->setText(QString::number(m_elements[index].darkPeriod));

    if(!ui->autoupdate_checkBox->isChecked())
    {
        ui->splashIndex_lineEdit->setText(QString::number(m_elements[index].splashImageIndex));
        ui->splashBitPos_lineEdit->setText(QString::number(m_elements[index].splashImageBitPos));
    }

    ui->removePatternsButton->setEnabled(true);

    if(m_videoPatternMode)
    {
        ui->startPos_ComboBox->setCurrentIndex(m_elements[index].splashImageBitPos);
    }

    return;
}
/**
 * @brief MainWindow::on_triggerIn_checkBox_clicked
 */
void MainWindow::on_triggerIn_checkBox_clicked()
{
    bool changed = false;

    for (int i = 0; i < m_elements.size(); i++)
    {
        if (m_elements[i].selected)
        {
            m_elements[i].trigIn = ui->triggerIn_checkBox->isChecked();
            changed = true;
        }
    }

    if (changed)
    {
        waveWindow->updatePatternList(m_elements);
        waveWindow->draw();
    }

}
/**
 * @brief MainWindow::on_triggerOut2_checkBox_clicked
 */
void MainWindow::on_triggerOut2_checkBox_clicked()
{
    bool changed = false;

    for (int i = 0; i < m_elements.size(); i++)
    {
        if (m_elements[i].selected)
        {
            m_elements[i].trigOut2 = ui->triggerOut2_checkBox->isChecked();
            changed = true;
        }
    }

    if (changed)
    {
        waveWindow->updatePatternList(m_elements);
        waveWindow->draw();
    }
}
/**
 * @brief MainWindow::on_color_ComboBox_activated
 * @param index
 */
void MainWindow::on_color_ComboBox_activated(int index)
{
    bool changed = false;

    for (int i = 0; i < m_elements.size(); i++)
    {
        if (m_elements[i].selected)
        {
            m_elements[i].color = PatternElement::Color(index + 1);
            changed = true;
        }
    }

    if (changed)
    {
        waveWindow->updatePatternList(m_elements);
        waveWindow->draw();
    }
}

/**
 * @brief MainWindow::on_bitDepth_ComboBox_activated
 * @param index
 */

void MainWindow::on_bitDepth_ComboBox_activated(int index)
{

    bool changed = false;
    int bitDepth = index + 1;
    int curExp = ui->exposure_lineEdit->text().toInt();

    for (int i = 0; i < m_elements.size(); i++)
    {
        if (!m_elements[i].selected)
            continue;

        if (bitDepth == m_elements[i].bits)
            continue;

        if (m_videoPatternMode)
        {
            if(bitDepth + m_elements[i].splashImageBitPos > 24)
            {
                bitDepth = 24 - m_elements[i].splashImageBitPos;
                ui->bitDepth_ComboBox->setCurrentIndex(bitDepth - 1);
            }
        }
        m_elements[i].bits = bitDepth;
        changed = true;
        int minExp = GetMinExposure(m_elements[i].bits);

        if(curExp < minExp)
        {
            ui->exposure_lineEdit->setText(QString::number(minExp));
            m_elements[i].exposure = minExp;
        }

        m_patternImageChange = true;
    }

    if (changed)
    {
        waveWindow->updatePatternList(m_elements);
        waveWindow->draw();
    }

    return;
}
/**
 * @brief MainWindow::on_exposure_lineEdit_editingFinished
 */
void MainWindow::on_exposure_lineEdit_editingFinished()
{
    bool changed = false;

    for (int i = 0; i < m_elements.size(); i++)
    {
        if (!m_elements[i].selected)
            continue;
        int exposure = ui->exposure_lineEdit->text().toInt();
        int minExp = GetMinExposure(m_elements[i].bits);
        if (exposure < minExp)
        {
            char errMsg[255];
            sprintf(errMsg, "Exposure value should be greater than %d us\n", minExp);
            showStatus(errMsg);
            return;
        }
        m_elements[i].exposure = exposure;
        changed = true;
    }
    if (changed)
    {
        waveWindow->updatePatternList(m_elements);
        //	waveWindow->draw();
    }
}
/**
 * @brief MainWindow::on_darkPeriod_lineEdit_editingFinished
 */
void MainWindow::on_darkPeriod_lineEdit_editingFinished()
{
    bool changed = false;

    for (int i = 0; i < m_elements.size(); i++)
    {
        if (!m_elements[i].selected)
            continue;
        int darkPeriod = ui->darkPeriod_lineEdit->text().toInt();

        int minDarktime = GetMinDarktime(m_elements[i].bits);
        if ((darkPeriod) && (darkPeriod < minDarktime))
        {
            char errMsg[255];
            sprintf(errMsg, "Error: Dark time should be greater than %d us\n", minDarktime);
            showStatus(errMsg);
            return;
        }
        m_elements[i].darkPeriod = darkPeriod;
        changed = true;
    }

    if (changed)
    {
        waveWindow->updatePatternList(m_elements);
        waveWindow->draw();
    }
}
/**
 * @brief MainWindow::on_startPos_ComboBox_activated
 * @param index
 */
void MainWindow::on_startPos_ComboBox_activated(int index)
{
    bool changed = false;

    for (int i = 0; i < m_elements.size(); i++)
    {
        if (!m_elements[i].selected)
            continue;

        if(index == m_elements[i].splashImageBitPos)
            continue;

        if (m_videoPatternMode)
        {
            if(index + m_elements[i].bits > 24)
            {
                m_elements[i].bits = 24 - index;
                ui->bitDepth_ComboBox->setCurrentIndex(m_elements[i].bits - 1);
            }
            m_elements[i].splashImageBitPos = index;
        }
        changed = true;
    }

    if (changed)
    {
        waveWindow->updatePatternList(m_elements);
        waveWindow->draw();
    }
}
/**
 * @brief MainWindow::on_zoomSlider_valueChanged
 * @param value
 */

void MainWindow::on_zoomSlider_valueChanged(int value)
{
    waveWindow->setZoom(value);
    waveWindow->updatePatternList(m_elements);
    waveWindow->draw();
}

/**
 * @brief MainWindow::on_invertTrigOut2_checkBox_toggled
 * @param checked
 */
void MainWindow::on_invertTrigOut2_checkBox_toggled(bool checked)
{
    waveWindow->updatePatternList(m_elements);
    waveWindow->updateInvertTrigOut2(checked);
    waveWindow->draw();
    return;
}
/**
 * @brief MainWindow::on_invertTrigOut1_checkBox_toggled
 * @param checked
 */
void MainWindow::on_invertTrigOut1_checkBox_toggled(bool checked)
{
    waveWindow->updatePatternList(m_elements);
    waveWindow->updateInvertTrigOut1(checked);
    waveWindow->draw();
    return;
}
/**
 * @brief MainWindow::on_InputSourceList_currentIndexChanged
 * @param index
 */
void MainWindow::on_InputSourceList_currentIndexChanged(int index)
{
    unsigned int i;
    QStringList PixelFormats;
    QStringList PortWidth  = QStringList() << "30 BITS" << "24 BITS" << "20 BITS" << "16 BITS" << "10 BITS" << "8 BITS";
    QStringList TestPatterns = QStringList() << "Solid Field" << "Horizontal Ramp" << "Vertical Ramp" << "Horizontal Lines" << \
                                                "Diagonal Lines" << "Vertical Lines" << "Grid" << "Checkerboard" << "RGB Ramp" <<\
                                                "Color Bar";
    QStringList SplashIndices;

    for(i=0;i<256;i++)
        SplashIndices.append(QString::number(i));

    ui->TPGColorSetBut->setEnabled(false);
    ui->TPGColorGetBut->setEnabled(false);
    ui->TPGRed_BG_spinBox->setEnabled(false);
    ui->TPGRed_FG_spinBox->setEnabled(false);
    ui->TPGBlue_BG_spinBox->setEnabled(false);
    ui->TPGBlue_FG_spinBox->setEnabled(false);
    ui->TPGGreen_BG_spinBox->setEnabled(false);
    ui->TPGGreen_FG_spinbox->setEnabled(false);

    ui->SwapSelectList->setEnabled(false);
    ui->GetSwapBut->setEnabled(false);
    ui->SetSwapBut->setEnabled(false);

    ui->InputSourceOptionList->clear();
    ui->InputSourceOptionList->show();
    ui->colorFrame->hide();
    switch(index)
    {
    case 0:
        PixelFormats = QStringList() << "RGB 444" << "YUV 444" << "YUV 422";
        ui->InputSourceOptionList->addItems(PortWidth);
        ui->SwapSelectList->setEnabled(true);
        ui->GetSwapBut->setEnabled(true);
        ui->SetSwapBut->setEnabled(true);
        break;
    case 1:
        PixelFormats = QStringList() << "RGB 444" ;
        ui->InputSourceOptionList->addItems(TestPatterns);
        ui->TPGColorSetBut->setEnabled(true);
        ui->TPGColorGetBut->setEnabled(true);
        ui->TPGRed_BG_spinBox->setEnabled(true);
        ui->TPGRed_FG_spinBox->setEnabled(true);
        ui->TPGBlue_BG_spinBox->setEnabled(true);
        ui->TPGBlue_FG_spinBox->setEnabled(true);
        ui->TPGGreen_BG_spinBox->setEnabled(true);
        ui->TPGGreen_FG_spinbox->setEnabled(true);
        break;
    case 2:
        PixelFormats = QStringList() << "RGB 444" << "YUV 422";
        ui->InputSourceOptionList->addItems(SplashIndices);
        break;
    case 3:
        PixelFormats = QStringList() << "RGB 444" ;
        ui->InputSourceOptionList->addItems(PortWidth);
        ui->InputSourceOptionList->hide();
        ui->colorFrame->show();
        ui->SwapSelectList->setEnabled(true);
        ui->GetSwapBut->setEnabled(true);
        ui->SetSwapBut->setEnabled(true);
        break;
    }
    ui->PixelFormatList->clear();
    ui->PixelFormatList->addItems(PixelFormats);
}
/**
 * @brief MainWindow::on_displayGetBut_clicked
 */

void MainWindow::on_displayGetBut_clicked()
{
    rectangle croppedArea, displayArea;

    if(LCR_GetDisplay(&croppedArea, &displayArea) == 0)
    {
        ui->firstPixInput_CroppedArea->setText(QString::number(croppedArea.firstPixel));
        ui->firstLineInput_CroppedArea->setText(QString::number(croppedArea.firstLine));
        ui->widthInput_CroppedArea->setText(QString::number(croppedArea.pixelsPerLine));
        ui->heightInput_CroppedArea->setText(QString::number(croppedArea.linesPerFrame));

        ui->firstPixInput_DisplayArea->setText(QString::number(displayArea.firstPixel));
        ui->firstLineInput_DisplayArea->setText(QString::number(displayArea.firstLine));
        ui->widthInput_DisplayArea->setText(QString::number(displayArea.pixelsPerLine));
        ui->heightInput_DisplayArea->setText(QString::number(displayArea.linesPerFrame));
    }
    else
    {
        showError("Unable to get display dimensions!");
    }
}
/**
 * @brief MainWindow::on_displaySetBut_clicked
 */
void MainWindow::on_displaySetBut_clicked()
{
    rectangle croppedArea, displayArea;

    croppedArea.firstPixel = ui->firstPixInput_CroppedArea->text().toInt();
    croppedArea.firstLine = ui->firstLineInput_CroppedArea->text().toInt();
    croppedArea.pixelsPerLine = ui->widthInput_CroppedArea->text().toInt();
    croppedArea.linesPerFrame =ui->heightInput_CroppedArea->text().toInt();
    displayArea.firstPixel =ui->firstPixInput_DisplayArea->text().toInt();
    displayArea.firstLine = ui->firstLineInput_DisplayArea->text().toInt();
    displayArea.pixelsPerLine = ui->widthInput_DisplayArea->text().toInt();
    displayArea.linesPerFrame = ui->heightInput_DisplayArea->text().toInt();

    if(LCR_SetDisplay(croppedArea, displayArea) < 0)
        showError("Unable to set display dimensions!");
}
/**
 * @brief MainWindow::on_TPGColorGetBut_clicked
 */
void MainWindow::on_TPGColorGetBut_clicked()
{
    unsigned short redFG, greenFG, blueFG;
    unsigned short redBG, greenBG, blueBG;

    if(LCR_GetTPGColor(&redFG, &greenFG, &blueFG, &redBG, &greenBG, &blueBG) == 0)
    {
        ui->TPGRed_FG_spinBox->setValue(redFG);
        ui->TPGGreen_FG_spinbox->setValue(greenFG);
        ui->TPGBlue_FG_spinBox->setValue(blueFG);
        ui->TPGRed_BG_spinBox->setValue(redBG);
        ui->TPGGreen_BG_spinBox->setValue(greenBG);
        ui->TPGBlue_BG_spinBox->setValue(blueBG);
    }
    else
        showError("Unable to get TPG Color Settings");
}
/**
 * @brief MainWindow::on_TPGColorSetBut_clicked
 */
void MainWindow::on_TPGColorSetBut_clicked()
{
    int ret = LCR_SetTPGColor(ui->TPGRed_FG_spinBox->value(), ui->TPGGreen_FG_spinbox->value(), ui->TPGBlue_FG_spinBox->value(), ui->TPGRed_BG_spinBox->value(), ui->TPGGreen_BG_spinBox->value(), ui->TPGBlue_BG_spinBox->value());

    if(ret < 0)
        showError("Unable to set Internal test pattern colors");
}

/**
 * @brief Updates the minimum exposure period for each bitdepth
 */
void MainWindow::updateMinExposure(void)
{
    /* Get the min exposure through command */
    if(LCR_GetMinPatExposure(minPatExposure) < 0)
    {
        /* If the command fails, use the default values */

        /* Exposure for bit depth */
        static const int bitDepthExposure[2][8] =
        {
            {105, 304, 394, 823, 1215, 1487, 1998, 4046},
            {105, 304, 380, 733, 1215, 1487, 1998, 4046}
        };

        /* Exposure for DMD block (1 bit) */
        static const int oneBitExposure[2][16] =
        {
            { 24, 45, 45, 45, 48, 54, 60, 66, 72, 78, 84, 90, 96, 101, 105,   0 },
            { 24, 42, 42, 42, 45, 51, 56, 61, 67, 72, 77, 83, 88,  93,  99, 105 }
        };

        int activeBlocks = ui->endDmdBlock->currentText().toInt() -
                            ui->startDmdBlock->currentText().toInt();

        for(int i = 0; i < 8; i++)
        {
            if(i == 0)
                minPatExposure[i] = oneBitExposure[m_dualAsic][activeBlocks];
            else
                minPatExposure[i] = bitDepthExposure[m_dualAsic][i];
        }
    }
}
