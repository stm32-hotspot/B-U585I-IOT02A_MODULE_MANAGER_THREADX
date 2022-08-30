/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_filex.h
  * @author  MCD Application Team
  * @brief   FileX applicative header file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_FILEX_H__
#define __APP_FILEX_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NOR_FLASH_FS    // enable the NOR Flash file system support. If commented fallback to RAM FS
  
/* Includes ------------------------------------------------------------------*/
#include "fx_api.h"
#ifdef  NOR_FLASH_FS    
#include "fx_stm32_levelx_nor_driver.h"
//#include "lx_stm32_ospi_driver.h"  
#else  
#include "fx_stm32_sram_driver.h"
#endif
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/

/* USER CODE BEGIN EFP */
extern UINT App_FileX_Init();
extern UINT App_FileX_Create_Module_File(char *);
extern UINT App_FileX_Check_Module_File(char *);
extern UINT App_FileX_DeInit();

extern VOID _fx_ram_driver(FX_MEDIA *media_ptr);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define FX_SECTOR_NUMBER        (15)
#define FX_SECTOR_SIZE          (512)
#define FX_DISK_MEM_POOL_SIZE  (FX_SECTOR_SIZE * FX_SECTOR_NUMBER)
#define FX_APP_MEM_POOL_SIZE    (FX_DISK_MEM_POOL_SIZE + 1024)
#define FX_FAULT_TOLERANT_MAXIMUM_LOG_FILE_SIZE      (1024)   
#define MODULE_FILE_NAME       "Module.bin"   // the module file name

extern FX_MEDIA        media_disk;
extern UCHAR           ram_disk_sector_cache[FX_SECTOR_SIZE];
extern UCHAR           fault_tolerant_memory[FX_FAULT_TOLERANT_MAXIMUM_LOG_FILE_SIZE];

/* USER CODE END PD */

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
#ifdef __cplusplus
}
#endif
#endif /* __APP_FILEX_H__ */

