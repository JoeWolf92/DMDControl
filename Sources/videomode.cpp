/*
 * This file provides all the handler functions for the UI controls on
 * Video Mode page
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "API.h"
/***********************Video Mode Page******************/

/**
 * @brief MainWindow::on_SourceGetBut_clicked
 */
void MainWindow::on_SourceGetBut_clicked()
{
    unsigned int source, portWidth, pattern, splashindex;
    unsigned int redIntensity,greenIntensity, blueIntensity;
    //char *str;

    if(LCR_GetInputSource(&source, &portWidth) >= 0)
    {
        ui->InputSourceList->setCurrentIndex(source);
        if(source == 0)
            ui->InputSourceOptionList->setCurrentIndex(portWidth);
        else if(source == 1)
        {
            if(LCR_GetTPGSelect(&pattern)<0)
                showError("Unable to get TPG Select");
            else
                ui->InputSourceOptionList->setCurrentIndex(pattern);
        }
        else if(source == 3)
        {
            if(LCR_GetCurtainColor(&redIntensity,&greenIntensity,&blueIntensity)<0)
                showError("Cannot Get Curtain Color!");
            else
            {
                ui->red_lineEdit->setText(QString::number(redIntensity));
                ui->green_lineEdit->setText(QString::number(greenIntensity));
                ui->blue_lineEdit->setText(QString::number(blueIntensity));
            }
        }
        else
        {
            if(LCR_GetSplashIndex(&splashindex)<0)
                showError("Unable to get Splash index");
            else
                ui->InputSourceOptionList->setCurrentIndex(splashindex);
        }
    }
    else
        showError("Could not Get Input Source info!");
}
/**
 * @brief MainWindow::on_SourceSetBut_clicked
 */
void MainWindow::on_SourceSetBut_clicked()
{
    unsigned int index = ui->InputSourceList->currentIndex();
    int ret;
    unsigned int greenIntensity,blueIntensity,redIntensity;

    if ( index == 3 )
    {
        if(!ui->green_lineEdit->text().compare("")||!ui->red_lineEdit->text().compare("")|| !ui->blue_lineEdit->text().compare(""))
        {
            showStatus("Color Intensities cannot be empty");
            return;
        }

        greenIntensity = ui->green_lineEdit->text().toUInt();
        blueIntensity  = ui->blue_lineEdit->text().toUInt();
        redIntensity   = ui->red_lineEdit->text().toUInt();


        if(greenIntensity>1023|| redIntensity>1023 || blueIntensity>1023)
        {
            showStatus("Valid range for color intensities : 0-1023");
            return;
        }
        ret = LCR_SetInputSource(ui->InputSourceList->currentIndex(), 0);
        if (ret < 0)
        {
            showError("Unable to set input source");
            return;
        }
        ret= LCR_SetCurtainColor(redIntensity,greenIntensity,blueIntensity);
        if (ret < 0)
        {
            showError("Unable to Set Curtain Color");
            return;
        }
    }

    if( (index == 0) || (index == 4) || (index == 5) || (index == 6) )
    {
        ret = LCR_SetInputSource(ui->InputSourceList->currentIndex(), ui->InputSourceOptionList->currentIndex());
        if (ret < 0)
            showError("Unable to set input source");
    }
    else if (index == 1)
    {
        //LCR_SetInputSource(3, 0);
        ret = LCR_SetInputSource(ui->InputSourceList->currentIndex(), 0);
        if (ret < 0)
            showError("Unable to set input source");

        ret = LCR_SetTPGSelect(ui->InputSourceOptionList->currentIndex());
        if (ret < 0)
            showError("Unable to set input source");

    }
    else if ( index == 2 )
    {
        unsigned int source, portWidth;

        LCR_GetInputSource(&source, &portWidth);
        if (source != 2)
        {
            ret = LCR_SetInputSource(ui->InputSourceList->currentIndex(), 0);
            if (ret < 0)
                showError("Unable to set input source");
        }

        ret = LCR_LoadSplash(ui->InputSourceOptionList->currentIndex());
        if (ret < 0)
            showError("Unable to load the splash image");
    }

}
/**
 * @brief MainWindow::on_GetSwapBut_clicked
 */
void MainWindow::on_GetSwapBut_clicked()
{
    unsigned int swap;

    if(LCR_GetDataChannelSwap(ui->SwapPortSelect->currentIndex(), &swap)<0)
        showError("Unable to get Channel Swap Info");
    else
        ui->SwapSelectList->setCurrentIndex(swap);
}
/**
 * @brief MainWindow::on_SetSwapBut_clicked
 */
void MainWindow::on_SetSwapBut_clicked()
{
    if (LCR_SetDataChannelSwap(ui->SwapPortSelect->currentIndex(), ui->SwapSelectList->currentIndex()) < 0)
        showError("Unable to set Input Port Swap");
}
/**
 * @brief MainWindow::on_PixelFormatGetBut_clicked
 */
void MainWindow::on_PixelFormatGetBut_clicked()
{
    unsigned int pixelFormat;

    //Get input source first so that the options for formatlist is correctly popoulated.
    emit on_SourceGetBut_clicked();

    if(LCR_GetPixelFormat(&pixelFormat) == 0)
    {
        //If input source selected is splash
        if(ui->InputSourceList->currentIndex() == 2)
        {
            //YUV422 is in index 1
            if(pixelFormat == 2)
                pixelFormat = 1;
        }
        ui->PixelFormatList->setCurrentIndex(pixelFormat);
    }
    else
        showError("Unable to get Pixel Format");
}
/**
 * @brief MainWindow::on_PixelFormatSetBut_clicked
 */
void MainWindow::on_PixelFormatSetBut_clicked()
{
    unsigned int PixelFormat = ui->PixelFormatList->currentIndex();

    //If input source selected is splash
    if(ui->InputSourceList->currentIndex() == 2)
    {
        //If second item in the pixel format box is selected, the value to be passed is 2 for YUV444.
        if(ui->PixelFormatList->currentIndex() == 1)
            PixelFormat = 2;
    }
    if (LCR_SetPixelFormat(PixelFormat) < 0)
        showError("Unable to set pixel format");
}
