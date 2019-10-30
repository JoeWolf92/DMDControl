/*
 * This file provides all the handler functions for the UI controls on
 * System settings page
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/


#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "API.h"


/**
 * @brief MainWindow::on_getPatternDisplayInvertData_button_clicked
 */
void MainWindow::on_getPatternDisplayInvertData_button_clicked()
{
    int invert;

    if(LCR_GetInvertData(&invert)<0)
        showError("Unable to get Pattern Display Invert Data!");
    else
        ui->patternDisplayInvertData_checkBox->setChecked(invert);
}
/**
 * @brief MainWindow::on_setPatternDisplayInvertData_button_clicked
 */
void MainWindow::on_setPatternDisplayInvertData_button_clicked()
{
    bool invert = ui->patternDisplayInvertData_checkBox->isChecked();

    if(LCR_SetInvertData(invert)<0)
        showError("Unable to set Pattern Display Invert Data!");
}

/**
 * @brief MainWindow::on_setDMDPark_button_clicked
 */
void MainWindow::on_setDMDPark_button_clicked()
{
    bool parkDMD = ui->DMDPark_checkBox->isChecked();

    if(LCR_SoftwareDMDPark((parkDMD?1:0))<0)
        showError("Unable to Park DMD!");
}


/**
 * @brief MainWindow::on_getDMDPark_button_clicked
 */
void MainWindow::on_getDMDPark_button_clicked()
{
    int isDMDParked;

    if(LCR_GetSoftwareDMDPark(&isDMDParked)<0)
        showError("Unable to read DMD park status!");
    else
        ui->DMDPark_checkBox->setChecked(isDMDParked);
}



/**
 * @brief MainWindow::on_GetLEDpushButton_clicked
 */
void MainWindow::on_GetLEDpushButton_clicked()
{
    int  SeqCtrl, Red, Green, Blue, inverted;
    unsigned char RedCurrent, GreenCurrent, BlueCurrent;
    unsigned int Frequency;
    char currentStr[8];

    if (LCR_GetLedEnables(&SeqCtrl, &Red, &Green, &Blue) == 0)
    {
        ui->radioButton_ColorDisplayAuto->setChecked(SeqCtrl);
        ui->radioButton_ColorDisplayManual->setChecked(!SeqCtrl);

        ui->RedCheckBox->setChecked(Red);
        ui->GreenCheckBox->setChecked(Green);
        ui->BlueCheckBox->setChecked(Blue);

    }
    else
        showError("Unable to get LED status!");


    if (LCR_GetLedCurrents(&RedCurrent, &GreenCurrent, &BlueCurrent) == 0)
    {
        sprintf(currentStr,"%d", RedCurrent);
        ui->RedLEDCurrent->setText(currentStr);
        sprintf(currentStr,"%d", GreenCurrent);
        ui->GreenLEDCurrent->setText(currentStr);
        sprintf(currentStr,"%d", BlueCurrent);
        ui->BlueLEDCurrent->setText(currentStr);
    }
    else
        showError("Unable to get LED currents!");

    if(LCR_GetLEDPWMInvert(&inverted) == 0)
    {
        ui->LedPwmInvert_checkBox->setChecked(inverted);
    }
    else
        showError("Unable to get LED PWM polarity!");

    if(LCR_GetLedFrequency(&Frequency) == 0)
    {
        sprintf(currentStr,"%d", Frequency);
        ui->LEDFrequency->setText(currentStr);
    }
    else
        showError("Unable to get LED PWM frequency!");
}
/**
 * @brief MainWindow::on_SetLEDpushButton_clicked
 */
void MainWindow::on_SetLEDpushButton_clicked()
{
    bool SeqCtrl, Red, Green, Blue;
    unsigned char RedCurrent, GreenCurrent, BlueCurrent;
    unsigned int Frequency;

    Frequency = ui->LEDFrequency->text().toInt();

    if(LCR_SetLedFrequency(Frequency)< 0)
        showError("Unable to set LED PWM Frequency");

    // Should the LEDs be controlled by the sequence?
    SeqCtrl = ui->radioButton_ColorDisplayAuto->isChecked();
    Red = ui->RedCheckBox->isChecked();
    Green = ui->GreenCheckBox->isChecked();
    Blue = ui->BlueCheckBox->isChecked();

    if(LCR_SetLedEnables(SeqCtrl, Red, Green, Blue)<0)
        showError("Unable to set LED enable!");

    if(LCR_SetLEDPWMInvert(ui->LedPwmInvert_checkBox->isChecked())<0)
        showError("Unable to set LED PWM polarity");

    RedCurrent      = ui->RedLEDCurrent->text().toInt();
    GreenCurrent    = ui->GreenLEDCurrent->text().toInt();
    BlueCurrent     = ui->BlueLEDCurrent->text().toInt();

    if(LCR_SetLedCurrents(RedCurrent, GreenCurrent, BlueCurrent)<0)
        showError("Unable to set LED currents");
}


/**
 * @brief MainWindow::on_getFlipPushButton_clicked
 */
void MainWindow::on_getFlipPushButton_clicked()
{
    ui->LongAxisFlipCheckBox->setChecked(LCR_GetLongAxisImageFlip());
    ui->ShortAxisFlipCheckBox->setChecked(LCR_GetShortAxisImageFlip());
}
/**
 * @brief MainWindow::on_setFlipPushButton_clicked
 */
void MainWindow::on_setFlipPushButton_clicked()
{
    if(LCR_SetLongAxisImageFlip(ui->LongAxisFlipCheckBox->isChecked()) < 0)
        showError("Unable to set north south flip");
    if(LCR_SetShortAxisImageFlip(ui->ShortAxisFlipCheckBox->isChecked()) < 0)
        showError("Unable to set east west flip");
}

