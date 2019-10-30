/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/
#ifndef FIRMWAREW_H
#define FIRMWAREW_H

#include "PtnImage.h"
#include "Firmware.h"

/**
 * Class for loading, storing and manipulating the firmware file. This is a
 * c++ wrapper for the firmware c library module.
 */


class Firmware
{
    FW_Firmware_t *firmware;

public:
	/**
	 * Initializes the firmware data structure using the given file content
	 *
	 * @param fileName Flash image file name
	 */
	Firmware(QString fileName)
	{
        firmware = FW_LoadFromFile(fileName.toLatin1().data());
	}

	/**
	 * Deallocates all the memory associated with firmware data structure 
	 */
    ~Firmware()
    {
        FW_Free(firmware);
    }
    /**
     * @brief checks if the Firmware image is a valid Image
     * @return
     */
    bool isValid()
    {
        return(firmware != NULL);
    }

	/**
	 * Gets the given splash image from the firmware
	 *
	 * @param index Splash image index
	 *
	 * @return Pointer to the splash image. NULL in case of error
	 */
    PtnImage *getSplash(int index)
    {
        Image_t *img = FW_GetSplashImage(firmware, index);
        if(img == NULL)
            return NULL;
        return new PtnImage(img);
    }

	/**
	 * Removes the gien splash image from the firmware
	 *
	 * @param index Splash image index to be removed
	 *
	 * @return 0 on success, -ve on failure
	 */
    int removeSplash(int index)
    {
        return FW_RemoveSplash(firmware, index, TRUE);
    }

	/**
	 * Adds the given image as a splash image to the firmware
	 *
	 * @param splashImage Image to be added
	 *
	 * @return 0 on success, -ve on failure
	 */
    int addSplash(PtnImage *splashImage, SPL_Compression_t compression)
    {
        return FW_AddSplash(firmware, splashImage->toImage(), compression);
    }

	/**
	 * Gets the given batch file from the firmware
	 * @param index Index of the batch file to be read
	 *
	 * @return 0 on success, -ve on failure
	 */
    BAT_BatchFile_t *getBatchFile(int index)
    {
        return FW_GetBatchFile(firmware, index);
    }

	/**
	 * Adds the given batch file to the firmware data structure
	 *
	 * @param batchFile Batch file data structure
	 *
	 * @return 0 on success, -ve on failure
	 */
    int addBatchFile(BAT_BatchFile_t *batchFile)
    {
        return FW_AddBatchFile(firmware, batchFile);
    }

	/**
	 * Removes the given batch file from the firmware data structure
	 *
	 * @param index Index of the batch file to be removed
	 *
	 * @return 0 on success, -ve on failure
	 */
    int removeBatchFile(int index)
    {
        return FW_RemoveBatchFile(firmware, index, TRUE);
    }

	/**
	 * Save the current firmware to a disk file
	 * @param fileName File name to be saved to
	 * @return 0 on success, -ve on failure
	 */
    int saveToFile(QString fileName)
    {
        return FW_SaveToFile(firmware, fileName.toLatin1().data());
    }

	/**
	 * Gets the number of splash images present in the system
	 * @return Number of splash images
	 */
    int getNumSplashes(void)
    {
        return FW_GetNumSplash(firmware);
    }

	/**
	 * Gets the number of batch files present in the system
	 * @return Number of batch files
	 */
    int getNumBatchFiles(void)
    {
        return FW_GetNumBatchFiles(firmware);
    }

	/**
	 * Gets the given configuration field value. The value should be
	 * 1 to 32 bit long.
	 *
	 * @param field Configuration field to be read
	 * @param [out] value Value of the configuration field
	 *
	 * @return 0 on success, -ve on failure
	 */
	int getConfigBits(FW_ConfigField_t field, uint32 *value)
	{
        return FW_GetConfigBits(firmware, field, value);
	}

	/**
	 * Gets the given configuration field value. The value should be
	 * a byte array.
	 *
	 * @param field Configuration field to be read
	 * @param [out] value Value of the configuration field
	 *
	 * @return 0 on success, -ve on failure
	 */
	int getConfigBytes(FW_ConfigField_t field, uint08 *value)
	{
		return FW_GetConfigBytes(firmware, field, value);
	}

	/**
	 * Sets the given configuration field value. The value should be
	 * 1 to 32 bit long.
	 *
	 * @param field Configuration field to be updated
	 * @param value Value to be set to the field
	 *
	 * @return 0 on success, -ve on failure
	 */
	int setConfigBits(FW_ConfigField_t field, uint32 value)
	{
       return FW_SetConfigBits(firmware, field, value);
	}

	/**
	 * Sets the given configuration field value. The value should be
	 * a byte array
	 *
	 * @param field Configuration field to be updated
	 * @param value Value to be set to the field
	 *
	 * @return 0 on success, -ve on failure
	 */
	int setConfigBytes(FW_ConfigField_t field, uint08 *value)
	{
		return FW_SetConfigBytes(firmware, field, value);
	}

	/**
	 * Read the bootloader version stored in the firmware
	 *
	 * @param Major Major version number
	 * @param Minor Minor version number
	 * @param Patch Patch number
	 *
	 * @return SUCCESS/ERR_NULL_PTR/ERR_NOT_FOUND
	 *
	 */
	int getBLVersion(int *major, int *minor, int *patch)
	{
		return FW_GetBLVersion(firmware, major, minor, patch);
	}

	/**
	 * Gets the Bootloader size from the firmware image
	 *
	 * @return Bootloader size / flash table offset in bytes
	 */
	int getBLSize(void)
	{
		return FW_GetFlashTableOffset(firmware);
	}

	/**
	 * Gets the flash block information of the given index
	 * @param index Index of the flash block
	 * @param [out] info Flash block information
	 *
	 * @return SUCCESS/FAIL/ERR_NOT_FOUND
	 */
	int getBlockInfo(int index, FI_BlockInfo_t *info)
	{
		return FW_GetBlockInfo(firmware, index, info);
	}
};


#endif // FIRMWAREW_H
