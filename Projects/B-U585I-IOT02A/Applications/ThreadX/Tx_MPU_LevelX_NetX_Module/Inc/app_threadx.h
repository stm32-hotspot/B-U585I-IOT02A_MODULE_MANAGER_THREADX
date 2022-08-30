/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_threadx.h
  * @author  MCD Application Team
  * @brief   ThreadX applicative header file
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
#ifndef __APP_THREADX_H
#define __APP_THREADX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "txm_module.h"
#include "main.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#if defined(__GNUC__) || defined(__CC_ARM)

#else    /* __ICCARM__ */
extern ULONG module_entry_address; 
#define MODULE_FLASH_ADDRESS        &module_entry_address // from linker script
#endif

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
UINT App_ThreadX_Init(VOID *memory_ptr);
void MX_ThreadX_Init(void);
void MX_ThreadX_Process(void);
ULONG Ftp_File_Get(CHAR *fname, FX_MEDIA * loc_ram_disk);
/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TX_APP_MEM_POOL_SIZE                               10*1024
#define MODULE_MANAGER_THREAD_PRIO                         4
#define MODULE_MANAGER_THREAD_PREEMPTION_THRESHOLD         MODULE_MANAGER_THREAD_PRIO

#define MODULE_LOAD_FROM_FILE                           // download the ThreadX Module from FTP and execute it from file on FS */
#ifndef MODULE_LOAD_FROM_FILE
#define MODULE_EXECUTE_IN_PLACE                        // Execute the Module "in place" from Flash, if undefined load from Flash and execute from SRAM
#endif

/* USER CODE END PD */

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

#ifdef __cplusplus
}
#endif
#endif /* __APP_THREADX_H__ */
