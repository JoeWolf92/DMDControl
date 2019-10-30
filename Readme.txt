--------------------------------------------------------------------------
DLPLCR6500_9000_GUI:  LightCrafter 6500 & 9000 Graphical User Interface
Release:              4.0.1
Date:                 01/10/19
--------------------------------------------------------------------------

--------------------------------------------------------------------------
GENERAL DESCRIPTION
--------------------------------------------------------------------------

This GUI provides the ability to control and configure the DLP® LightCrafter™
6500 & 9000 EVM through USB.  

--------------------------------------------------------------------------
FEATURES SUPPORTED
--------------------------------------------------------------------------
1.  Automatic detection of LightCrafter 6500/9000 EVM
2.  Operating mode selection: Video, Video-Pattern, Pre-Stored Pattern Sequence, Pattern On the Fly. 
3.  Periodic reporting of hardware status
4.  Power standby mode
5.  Park command
6.  System Settings
       - Image orientation
       - Optical invert
       - Automatic/Manual LED selection and LED current and LED PWM frequency settings 
7.  Video Mode
       - Video input source selection and configuration 
       - Internal test pattern color controls
       - Display Dimension controls
       - Port Configuration controls
       - IT6535 receiver controls
8.  Pattern Mode
       - Pattern sequence setup and control
       - Image creation
       - On the fly pattern mode
       - Trigger edge delay and polarity control
       - LED edge delay Control
       - DMD Block Load
       - DMD Idle Mode (50/50 Duty Cycle)
9.  Look-Up Table (LUT) for Pre-stored Pattern and Pattern-on-the-fly modes
       - Change pattern display order in memory without reloading patterns
       - Update pattern exposure times without reloading patterns
10. Batch Files
       - Create a batch command sequence and execute
       - Load/Save batch files from/to computer
11. Peripherals
       - GPIO configuration
       - Clock configuration
       - PWM configuration
       - I2C settings
12. Firmware
       - Add batch file to firmware
       - App Defaults settings 
       - Firmware tagging supported
       - Upload firmware to EVM

--------------------------------------------------------------------------
REVISION HISTORY
--------------------------------------------------------------------------
4.0.1

* Added comments to FlashDeviceParameters.txt file regarding usage. User needs to
  update the text file for their specific flash part otherwise GUI will throw an error in firmware 
  programming.
* Bug Fix:  Firmware update failure for image size exceeding 128Mb.
* Bug Fix:  Batch file page crash on loading batch file that contains MBOX_ADDRESS
  command having more than 25 entries
* Updated flash update programming sequence to minimize risk of requiring JTAG to recover if
  programming is interrupted.  If the operation fails in the middle of programming the 
  controller (single or master/slave), the controller will remain in bootloader mode after power cycling.
  This allows recovery using USB to reprogram rather than JTAG.  
   
--------------------------------------------------------------------------
--------------------------------------------------------------------------

4.0.0

* Added support for these features introduced in DLPR900PROM v4.2 and v5.0:
  - Park command
  - Set LED PWM frequency
  - Pattern look-up table (LUT) capabilities
* Added browse option for uploading batch file
* Added selectable image compression options (Auto, None, RLE, Enhanced RLE)
* Updated Win 10 support and fixed rendering for 4K displays
* Improved firmware download speed
* Bug fix: Frequent crashes when updating or uploading firmware images
* Bug fix: Reduced length of image filenames previously producing "Not a Valid Firmware Image" error
* Bug fix: Creating firmware with images and batch files at same time results in crash
   
--------------------------------------------------------------------------

3.0.0

* Option added to set the illuminator capability to respond to LED ON/OFF signaling from DLPC900; 
  this option with illumination modulated driver hardware allows displaying 8-bit patterns at 
  higher pattern rate up to 1KHz.
* Added option to set Spread Spectrum for DLPC900 to reduce EMI.
* Bug fix: DLP9000 dual ASIC firmware not built properly when a pre-stored pattern sequence image 
  created
* Bug fix: Bootloader version queried information incorrectly processed

--------------------------------------------------------------------------

2.1.0

* This version includes the release of the GUI source code as part of the installer
* Separated the GUI and the App functionalities
* The App functionalities are split seperate C files which can be compiled separately
* The Mainwindow.cpp file divided into different .cpp files corresponding to each
* Page which we see in the GUI. The header file is same across all the corresponding .cpp files.

--------------------------------------------------------------------------

2.0.0
* Additional external video detection status flags.
* Multiple Flash Erase / Program support – Supports all three flash devices.
* Functionality added to read bootloader version information in the firmware
  binary and compare with installed bootloader version on hardware and inform
  user of new bootloader version for upgrade.
* GUI flash download progress bar now made as part of MainWindow.
* File browser history now maintained.
* Backward compatibility – GUI v2.0 not allow using old firmware files
  version < 2.0 to program or build.

--------------------------------------------------------------------------

1.1.0
* DMD Active area selection in pattern mode.
* DMD Idle Mode to exercise the DMD mirror array with 50/50 Duty Cycle.
* Load/Save Pattern Design from/to computer.
* Improvements to pattern uploads.

--------------------------------------------------------------------------

1.0.0
* Initial release

--------------------------------------------------------------------------

Please refer to the LightCrafter 6500 & 9000 EVM Users Guide for complete details.
This is available on http://www.ti.com/lit/dlpu028.

--------------------------------------------------------------------------