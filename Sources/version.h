/*
 * version.h
 *
 * This module has the version number info.
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#ifndef VERSION_H
#define VERSION_H

#define GUI_VERSION_MAJOR 4
#define GUI_VERSION_MINOR 0
#define GUI_VERSION_BUILD 1

/* Version history
*
* 3.1.0
*
-   Added option to select different compression schemes RLE, Enhanced RLE and Uncompressed
-   Firmware download speed improvement made; [FastFlashUpdate] feature added; this option basically
    maintains the cached firmware binary on the PC which will be always kept in sync with the last programmed
    content on the flash; for any future flash updates the new firmare binary file will be compared to the
    cached firmare binary and only the mismatch areas on the flash will be erased and programmed
-   GUI crashes when creating firmware with images and batch file at same time
-   TI DLP Solutions & Services hyperlink updated
*
* 3.0.0
*
-   Option added to set the illuminator capability to respond to ON/OFF signaling from DLPC900; this option
    with illumination modulated driver hardware allows displaying 8-bit patterns at higher pattern rate upto 1KHz.
-   Added option to set Spread Spectrum for DLPC900 to reduce EMI.
*
* 2.1.1
*
- Fix for the issue where downloading a Firmware image > 16MB erases the boot loader.
*
* 2.1.0
*
-   This version includes the release of the GUI source code as part of the installer
-   Seperated the GUI and the App functionalities
-   The App functionalities are slpit seperate C files which can be compiled seperately
-   The Mainwindow.cpp file didvided into different .cpp files corresponding to each
    Page which we see in the GUI. The header file is same across all the corresponding .cpp files.
*
* 2.0.0
*
-   Multiple Flash Erase / Program support – Supports three flash devices
    connected at chipselect CS1, CS2 & CS0 pins of DLPC900
-   Functionality added to read bootloader version information in the firmawre
    binary and detect with installed bootloader version on hardware and update
    user on new bootloader version for upgrade.
-   GUI flash download progress bar now made as part of MainWindow.
-   File browser history now maintained.
-   Backward compatibility – GUI v2.0 not allow using old firmware files
    version < 2.0 to program or build.
*
* 1.1.0
*
-	Added the capability for Save & Load pattern settings upon reopening the GUI
-	Added Tool tips to the Batch File and Patterns in Firmware Page
-	Changed "Dummy Connection" to "Virtual Connection"
-	Removed Port0 from Peripherals page - I2c port dropdown
-	Fixed box alignment in Pattern mode layout
-	Added the DMD Saver enable/disable  capability
-	Updated the minimum dark time calculation
-	Added the cancel/continue option for flash download checksum error
-	Corrected the urls in information page
*
* 1.0.0 - Initial release
*
*/

#endif //VERSION_H
