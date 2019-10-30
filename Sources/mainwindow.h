/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QFile>
#include <QTableWidget>
#include "waveformwindow.h"
#include "common.h"
#include "FirmwareW.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    void updateBatchCommands(API_CommandInfo_t *cmdInfo);
    ~MainWindow();

private slots:
    void showStatus(const char *str);
    
    void on_addPatternsButton_clicked();

    void on_patternSelect(int index, QList<PatternElement> patElem);

    void on_removePatternsButton_clicked();

    void on_zoomSlider_valueChanged(int value);

    void on_selectAllButton_clicked();

    void on_invertTrigOut2_checkBox_toggled(bool checked);

    void on_invertTrigOut1_checkBox_toggled(bool checked);

    void on_SourceGetBut_clicked();

    void on_SourceSetBut_clicked();

    void on_GetSwapBut_clicked();

    void on_SetSwapBut_clicked();

    void on_InputSourceList_currentIndexChanged(int index);

    void on_PixelFormatGetBut_clicked();

    void on_PixelFormatSetBut_clicked();

    void on_displayGetBut_clicked();

    void on_displaySetBut_clicked();

    void on_TPGColorGetBut_clicked();

    void on_TPGColorSetBut_clicked();

    void on_connectButton_clicked();

    void timerTimeout(void);

    void on_i2cget_clicked();

    void on_i2cset_clicked();

    void on_powerOn_radioButton_clicked();

    void on_standBy_radioButton_clicked();

    void on_startPatSequence_Button_clicked();

    int calculateSplashImageDetails(int *totalSplashImages, bool firmware);

    void on_stopPatSequence_Button_clicked();

    void on_videoMode_radioButton_clicked();

    void on_patternMode_radioButton_clicked();

    void on_trigOut1GetButton_clicked();

    void on_trigOut1SetButton_clicked();

    void on_trigOut2GetButton_clicked();

    void on_trigOut2SetButton_clicked();

    void on_ledGetDelayButton_clicked();

    void on_ledSetDelayButton_clicked();

    void on_updateLUT_Button_clicked();

    void on_pausePatSequence_Button_clicked();

    void on_exposure_lineEdit_editingFinished();

    void on_getFlipPushButton_clicked();

    void on_setFlipPushButton_clicked();

    void on_videoPatternMode_radioButton_clicked();

    void on_startPos_ComboBox_activated(int index);

    void on_GetLEDpushButton_clicked();

    void on_SetLEDpushButton_clicked();

    void on_pushButton_info_clicked();

    void on_pushButton_systemControls_clicked();

    void on_pushButton_videoMode_clicked();

    void on_pushButton_patternMode_clicked();

    void on_pushButton_batchFiles_clicked();

    void on_pushButton_firmware_clicked();

    void on_pushButton_patternControls_clicked();

    void on_pushButton_DMDSaverModeOn_clicked();

    void on_pushButton_DMDSaverModeOff_clicked();

    void on_pushButton_globalSettings_clicked();

    void on_chooseFirmware_Button_clicked();

    void on_updateFirmware_Button_clicked();

    void on_executeBatchFile_Button_clicked();

    void on_deleteCommand_Button_clicked();

    void on_clearBatchTable_Button_clicked();

    void on_insertDelay_Button_clicked();

    void on_deleteBatch_Button_clicked();

    void on_pDMD_radioButton_clicked();

    void on_wqxgaDMD_radioButton_clicked();

    void on_downloadFirmware_Button_clicked();

    void getVersion();

    void getStatus();

    void on_getClkConfig_pushButton_clicked();

    void on_setClkConfig_pushButton_clicked();

    void on_pushButton_peripherals_clicked();

    void on_gpioGet_button_clicked();

    void on_gpioSet_button_clicked();

    void on_pwmSetConfig_button_clicked();

    void on_pwmGetConfig_button_clicked();

    void on_darkPeriod_lineEdit_editingFinished();

    void on_triggerIn_checkBox_clicked();

    void on_triggerOut2_checkBox_clicked();

    void on_resetDMD_Button_clicked();

    void on_patternMemory_radioButton_clicked();

    void on_portConfig_pixelMode_comboBox_currentIndexChanged(int index);

    void on_portConfigSet_button_clicked();

    void on_portConfigGet_button_clicked();

    void on_color_ComboBox_activated(int index);

    void on_bitDepth_ComboBox_activated(int index);

    void on_saveBatchFile_Button_clicked();

    void on_loadBatchFile_Button_clicked();

    void on_executeBatchCommands_Button_clicked();

    void on_i2cWrite_Button_clicked();

    void on_i2cRead_Button_clicked();

    void on_startupState_checkBox_clicked(bool checked);

    void on_hdmiReceiver_checkBox_clicked(bool checked);

    void on_defaultBatch_checkBox_clicked(bool checked);

    void on_IT6535GetMode_button_clicked();

    void on_IT6535SetMode_button_clicked();

    void on_updateFlashWaitCycle_checkBox_clicked(bool checked);

    void on_i2cSlaveAdd_checkBox_clicked(bool checked);

    void on_DMDBusSwap_checkBox_clicked(bool checked);

    void on_getPatternDisplayInvertData_button_clicked();

    void on_setPatternDisplayInvertData_button_clicked();

    void on_trigIn1SetButton_clicked();

    void on_trigIn1GetButton_clicked();

    void on_trigIn2GetButton_clicked();

    void on_trigIn2SetButton_clicked();

    void showError(QString errMsg);

    void showCriticalError(QString errMsg);

    void on_addBatchFile_checkBox_clicked(bool checked);

    void on_ti_e2e_pushButton_clicked();

    void on_ti_e2e_pushButton_2_clicked();

    void on_dlp_pushButton_clicked();

    void getFrmwVersion();

    void resizeEvent(QResizeEvent *);

    void on_enableCmdLog_clicked(bool checked);

    void on_dummyConnection_clicked(bool checked);

    void on_startDmdBlock_currentIndexChanged(int index);

    void on_dmdBlockSet_clicked();

    void on_dmdBlockGet_clicked();

    void on_saveButton_patternSettings_clicked();

    void on_loadButton_patternSettings_clicked();


    void on_ledPulseWidthGet_clicked();

    void on_ledPulseWidthSet_clicked();

    void on_saveBatchFile_Button_2_clicked();

    void on_firmwareBatchFile_comboBox_currentIndexChanged(int index);

    void on_seqCacheSize_checkBox_clicked(bool checked);

    void on_setSpreadSpectrum_checkBox_clicked(bool checked);

    void on_addBatchFile_checkBox_clicked();
    void on_setDMDPark_button_clicked();
    void on_getDMDPark_button_clicked();

    void on_editLUT_Button_clicked();

    void on_lutEditButton_clicked();

    void on_lutEditUpdateButton_clicked();

    void on_startPatSequenceEdit_Button_clicked();

    void on_stopPatSequenceEdit_Button_clicked();

    void on_lutEditBackButton_clicked();

    void on_loadButton_LutEdit_clicked();

    void on_lutEditClearButton_clicked();

    void on_autoupdate_checkBox_clicked();

    void on_splashIndex_lineEdit_editingFinished();

    void on_splashBitPos_lineEdit_editingFinished();

    void on_saveButton_lutEdit_clicked();

private:
    Ui::MainWindow *ui;
    WaveFormWindow *waveWindow;
    QTimer *usbPollTimer;
    QSettings settings;


    QList<PatternElement> m_elements;
    QString m_firmwarePath;
    QString m_ptnImagePath;
    QString m_ptnSettingPath;
    QString m_batchFilePath;
    QString m_lutReorderFilePath;
    QString m_cachedFimwareFile;
    QString m_cachedFimwareFileSlave;

    Firmware* m_firmware;
    Firmware* m_firmwareSlave;
    BAT_BatchFile_t* m_batchFile;

    bool m_dualAsic;
    uint32 m_ptnWidth, m_ptnHeight;
    bool m_dmdDetected;


    bool m_patternImageChange;
    bool m_firstConnect;
    bool m_videoPatternMode;

    int m_displayCommand;

    int minPatExposure[8];
    bool isPatternLoaded = FALSE;
    QList<QString> lut_entries;
    QList<QString> lut_entries_live;
    QList<unsigned short> reorder_idx;
    QList<unsigned short> reorder_idx_live;

    int addBatchFile(bool *, BAT_BatchFile_t*, BAT_BatchFile_t*);
    int addPatterns();
    void processChosenFirmwareImage();
    bool processCachedFirmwareImage();
    void hideFrames();
    int updatePatternMemory(int totalSplashImages, BOOL firmware);
    void changePtnBitDepth(int ptnNum, int bitDepth);
    void updatePtnCheckbox(void);
    void updateBlocks(bool full);
    int GetMinExposure(int bitDetph);
    int GetMinDarktime(int);
    void updateFWConfigData(Firmware *fw);
    int uploadPatternToEVM(bool master, int splashImageCount, int splash_size, uint08* splash_block);
    void updateMinExposure(void);
    int displayBatchFile(BAT_BatchFile_t* batchFile, QTableWidget *table);
    void updateEditLutButton(int mode);
};

#endif // MAINWINDOW_H
