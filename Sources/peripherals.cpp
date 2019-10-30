/*
 * This file provides all the handler functions for the UI controls on
 * Peripherals page
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/


#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "API.h"

/*********************Peripherals Page**********************/
/**
 * @brief MainWindow::on_i2cget_clicked
 */
void MainWindow::on_i2cget_clicked()
{

}
/**
 * @brief MainWindow::on_i2cset_clicked
 */
void MainWindow::on_i2cset_clicked()
{
    unsigned int port   = ui->i2cport_setting->currentIndex();
    unsigned int addm   = ui->i2caddressmode->currentIndex();
    unsigned int clk    = ui->i2cclock->text().toULong();

    if (LCR_I2CConfigure(port+1,addm,clk) < 0) //KV updated after removing port0
        showError("Unable to set I2C port settings");
}
/**
 * @brief MainWindow::on_gpioGet_button_clicked
 */
void MainWindow::on_gpioGet_button_clicked()
{
    unsigned int pinNum;
    int dirOutput, outTypeOpenDrain, pinState;

    pinNum = ui->gpioPinSelect_comboBox->currentIndex();

    if(LCR_GetGPIOConfig(pinNum,&dirOutput,&outTypeOpenDrain,&pinState)<0)
        showError("Error getting GPIO config!");
    else
    {
        if(dirOutput)
            ui->gpioDirection_comboBox->setCurrentIndex(1);
        else
            ui->gpioDirection_comboBox->setCurrentIndex(0);

        if(outTypeOpenDrain)
            ui->gpioOutputType_comboBox->setCurrentIndex(1);
        else
            ui->gpioOutputType_comboBox->setCurrentIndex(0);

        if(pinState)
            ui->gpioOutputState_comboBox->setCurrentIndex(1);
        else
            ui->gpioOutputState_comboBox->setCurrentIndex(0);
    }

}
/**
 * @brief MainWindow::on_gpioSet_button_clicked
 */
void MainWindow::on_gpioSet_button_clicked()
{
    unsigned int pinNum;
    bool dirOutput, outTypeOpenDrain, pinState;

    pinNum = ui->gpioPinSelect_comboBox->currentIndex();

    if(ui->gpioDirection_comboBox->currentIndex()==0)
        dirOutput=false;
    else
        dirOutput=true;

    if(ui->gpioOutputType_comboBox->currentIndex()==0)
        outTypeOpenDrain=false;
    else
        outTypeOpenDrain=true;

    if(ui->gpioOutputState_comboBox->currentIndex()==0)
        pinState=false;
    else
        pinState=true;

    if(LCR_SetGPIOConfig(pinNum,dirOutput,outTypeOpenDrain,pinState)<0)
        showError("Error setting GPIO config!");
}
/**
 * @brief MainWindow::on_i2cRead_Button_clicked
 */
void MainWindow::on_i2cRead_Button_clicked()
{
    QString data;
    char hexbuf[3];
    QString str;
    QStringList datalist;
    unsigned char rdata[100];
    unsigned char wdata[512];
    bool ok;
    unsigned int port, devadd, nwbytes, nrbytes;


    port = ui->i2cport_readwrite->currentIndex();
    devadd = ui->i2cdevaddress->text().toULong(&ok, 16);
    //    nwbytes = ui->i2cnumwbytes->text().toULong();
    nrbytes = ui->i2cnumrbytes->text().toULong();


    data = ui->i2cwdata->text();
    datalist = data.split(' ');

    nwbytes = datalist.length();

    if (nrbytes <= 0)
    {
        showStatus("Error: Specify the number of bytes to be read\n");
        return;
    }

    for(int i = 0; i<100; i++)
        rdata[i] = 0;

    if( (nrbytes == 0) && (nwbytes == 0) )
        return;

    if( (nrbytes > 63) || (nwbytes > 512) )
        return;

    if( nwbytes > 0 )
    {
        for(uint i = 0; i < nwbytes; i++)
            wdata[i] = datalist[i].toULong(&ok, 16);
    }

    if(LCR_ReadI2CPassThrough(port+1, devadd, wdata, nwbytes, nrbytes, rdata)<0)
    {
        showError("Unable to Read");
        return;
    }

    datalist.clear();
    for(uint i = 0; i < nrbytes; i++)
    {
        sprintf(hexbuf, "%02X", rdata[i]);
        str = QString(hexbuf);
        datalist.append( str );
    }

    ui->i2crdata->setText( datalist.join(" ") );
}
/**
 * @brief MainWindow::on_i2cWrite_Button_clicked
 */
void MainWindow::on_i2cWrite_Button_clicked()
{
    QString data;
    QStringList datalist;
    unsigned char wdata[512];
    bool ok;

    data = ui->i2cwdata->text();
    datalist = data.split(' ');


    unsigned int port, devadd, nwbytes;
    port = ui->i2cport_readwrite->currentIndex();
    devadd = ui->i2cdevaddress->text().toULong(&ok, 16);
    nwbytes = datalist.length();//ui->i2cnumwbytes->text().toULong();

    if(nwbytes == 0)
    {
        showStatus("Error: No data to send\n");
        return;
    }

    if(nwbytes > 512)
    {
        showStatus("Error: Too much data to send\n");
    }

    for(uint i = 0; i < nwbytes; i++)
    {
        wdata[i] = datalist[i].toULong(&ok, 16);
    }

    if (LCR_WriteI2CPassThrough(port+1,devadd, wdata, nwbytes) < 0)
        showError("Unable to send I2C pass through commands");
}
/**
 * @brief MainWindow::on_pwmGetConfig_button_clicked
 */
void MainWindow::on_pwmGetConfig_button_clicked()
{
    unsigned int channel, pulsePeriod,  dutyCycle;
    int enable;


    channel=ui->pwmChannelSelect_comboBox->currentIndex();

    if(LCR_GetPWMConfig(channel,&pulsePeriod,&dutyCycle)<0)
        showError("Error getting PWM Configuration!");
    else
    {
        pulsePeriod = ((pulsePeriod + 1)*53.5)/1000;
        pulsePeriod = qRound(float(pulsePeriod)/10)*10;
        ui->pwmClkPeriod_lineEdit->setText(QString::number(pulsePeriod));

        ui->pwmDutyCycle_lineEdit->setText(QString::number(dutyCycle));

        if(LCR_GetPWMEnable(channel,&enable)<0)
            showError("Error getting PWM enabled status!");
        else
            ui->pwmEnable_checkBox->setChecked(enable);
    }

}
/**
 * @brief MainWindow::on_pwmSetConfig_button_clicked
 */
void MainWindow::on_pwmSetConfig_button_clicked()
{
    unsigned int channel, pulsePeriod,  dutyCycle;
    bool enable;

    channel=ui->pwmChannelSelect_comboBox->currentIndex();

    pulsePeriod = ui->pwmClkPeriod_lineEdit->text().toInt();
    if(pulsePeriod>106666)
    {
        showStatus("Invalid Clock Period!");
        return;
    }
    else
        pulsePeriod = ((pulsePeriod*1000)/53.5)-1;

    dutyCycle=ui->pwmDutyCycle_lineEdit->text().toInt();

    if(dutyCycle<1 || dutyCycle>99)
    {
        showStatus("Duty Cycle must be between 1 - 99");
        return;
    }

    enable = ui->pwmEnable_checkBox->isChecked();

    if(LCR_SetPWMConfig(channel,pulsePeriod,dutyCycle)<0)
        showError("Error setting PWM Configuration!");
    else
    {
        if(LCR_SetPWMEnable(channel,enable)<0)
            showError("Error setting PWM enable/disable!");
    }

}
/**
 * @brief MainWindow::on_getClkConfig_pushButton_clicked
 */
void MainWindow::on_getClkConfig_pushButton_clicked()
{

    int enable;
    unsigned int clkDivider;

    if(LCR_GetGeneralPurposeClockOutFreq(0,&enable,&clkDivider)==0)
    {
        ui->clkEnable_checkBox->setChecked(enable);
        ui->clkDivisor->setText(QString::number(clkDivider));
    }
    else
    {
        showError("Error getting the clock configuration!");
    }
}
/**
 * @brief MainWindow::on_setClkConfig_pushButton_clicked
 */
void MainWindow::on_setClkConfig_pushButton_clicked()
{

    bool enable;
    unsigned int clkDivider;

    clkDivider = ui->clkDivisor->text().toULong();

    if(clkDivider<2 || clkDivider>127)
    {
        showStatus("Allowed values for Clock Divider : 2 - 127");
        return;
    }

    enable = ui->clkEnable_checkBox->isChecked();

    if(LCR_SetGeneralPurposeClockOutFreq(0, enable, clkDivider)<0)
        showError("Error setting the clock configuration!");

}

