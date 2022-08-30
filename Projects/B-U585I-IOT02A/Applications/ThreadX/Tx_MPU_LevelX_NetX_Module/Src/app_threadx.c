/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_threadx.c
  * @author  MCD Application Team
  * @brief   ThreadX applicative file
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

/* Includes ------------------------------------------------------------------*/
#include "app_threadx.h"
#include "app_filex.h"
#include "app_netxduo.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
PROCESSING_NOT_STARTED    = 99,
WRITING_TO_READWRITE      = 88,
WRITING_TO_READONLY       = 77,
READING_FROM_READWRITE    = 66,
READING_FROM_READONLY     = 55,
PROCESSING_FINISHED       = 44,
MEMORY_ACCESS_VIOLATION   = 45
} ProgressState;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEFAULT_STACK_SIZE         (3*1024)
#define MODULE_DATA_SIZE           (32*1024)
#define OBJECT_MEM_SIZE            (16*1024)

#define READONLY_REGION            0x20010000
#define READWRITE_REGION           0x20010100
#define SHARED_MEM_SIZE            0xFF

#define NX_APP_MEM_POOL_SIZE       (80*1024)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
/* Define the ThreadX object control blocks */
TX_THREAD ModuleManager;
TXM_MODULE_INSTANCE ModuleOne;
TX_QUEUE ResidentRxQueue;
TX_QUEUE ResidentTxQueue;

/* Define the module data pool area. */
UCHAR  module_data_area[MODULE_DATA_SIZE];

/* Define the object pool area.  */
UCHAR  object_memory[OBJECT_MEM_SIZE];

/* Define the NetX pool buffer */
#if defined ( __ICCARM__ )
#pragma data_alignment=4
#endif
__ALIGN_BEGIN static UCHAR  nx_byte_pool_buffer[NX_APP_MEM_POOL_SIZE] __ALIGN_END;
static TX_BYTE_POOL nx_app_byte_pool;


/* Define the count of memory faults.  */
ULONG memory_faults = 0;

static UCHAR tx_byte_pool_buffer[TX_APP_MEM_POOL_SIZE];
static TX_BYTE_POOL ModuleManagerBytePool;

extern TX_SEMAPHORE Semaphore_ip_addr_ready;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
VOID pretty_msg(char *p_msg, ULONG r_msg);
VOID ModuleManager_Entry(ULONG thread_input);
VOID module_fault_handler(TX_THREAD *thread, TXM_MODULE_INSTANCE *module);
/* USER CODE END PFP */

/**
  * @brief  Define the initial system.
  * @param  first_unused_memory : Pointer to the first unused memory
  * @retval None
  */
VOID tx_application_define(VOID *first_unused_memory)
{
  CHAR *pointer;
  
  if (tx_byte_pool_create(&ModuleManagerBytePool, "Module Manager Byte Pool", tx_byte_pool_buffer, TX_APP_MEM_POOL_SIZE) != TX_SUCCESS)
  {
    /* USER CODE BEGIN TX_Byte_Pool_Error */
    Error_Handler();
    /* USER CODE END TX_Byte_Pool_Error */
  }
  else
  {
    
    /* Allocate the stack for Module Manager Thread.  */
    if (tx_byte_allocate(&ModuleManagerBytePool, (VOID **) &pointer,
                         2*DEFAULT_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
    {
      Error_Handler();
    }
    
    /* Create Module Manager Thread.  */
    if (tx_thread_create(&ModuleManager, "Module Manager Thread", ModuleManager_Entry, 0,
                         pointer, 2*DEFAULT_STACK_SIZE,
                         MODULE_MANAGER_THREAD_PRIO, MODULE_MANAGER_THREAD_PREEMPTION_THRESHOLD,
                         TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
    {
      Error_Handler();
    }
    
    /* Allocate the stack for ResidentRxQueue.  */
    if (tx_byte_allocate(&ModuleManagerBytePool, (VOID **) &pointer,
                         16 * sizeof(ULONG), TX_NO_WAIT) != TX_SUCCESS)
    {
      Error_Handler();
    }
    
    /* Create the ResidentRxQueue */
    if (tx_queue_create(&ResidentRxQueue, "Resident Rx Queue",TX_1_ULONG,
                        pointer, 16 * sizeof(ULONG)) != TX_SUCCESS)
    {
      Error_Handler();
    }
  }
  
    /* Allocate the stack for ResidentTxQueue.  */
    if (tx_byte_allocate(&ModuleManagerBytePool, (VOID **) &pointer,
                         16 * sizeof(ULONG), TX_NO_WAIT) != TX_SUCCESS)
    {
      Error_Handler();
    }
    
    /* Create the ResidentTxQueue */
    if (tx_queue_create(&ResidentTxQueue, "Resident Tx Queue",TX_1_ULONG,
                        pointer, 16 * sizeof(ULONG)) != TX_SUCCESS)
    {
      Error_Handler();
    } 
}

/**
  * @brief  MX_ThreadX_Init
  * @param  None
  * @retval None
  */
void MX_ThreadX_Init(void)
{
  /* USER CODE BEGIN  Before_Kernel_Start */

  /* USER CODE END  Before_Kernel_Start */

  tx_kernel_enter();

  /* USER CODE BEGIN  Kernel_Start_Error */

  /* USER CODE END  Kernel_Start_Error */
}

/* USER CODE BEGIN  1 */
/**
  * @brief  Module Manager main thread.
  * @param  thread_input: thread id
  * @retval none
  */
VOID ModuleManager_Entry(ULONG thread_input)
{
  UINT   status;
  CHAR   p_msg[64];
  ULONG  r_msg = PROCESSING_NOT_STARTED;
  ULONG  module_properties;
  VOID *memory_ptr;

  if (tx_byte_pool_create(&nx_app_byte_pool, "Nx App memory pool", nx_byte_pool_buffer, NX_APP_MEM_POOL_SIZE) != TX_SUCCESS)
  {
    /* USER CODE BEGIN NX_Byte_Pool_Error */

    /* USER CODE END NX_Byte_Pool_Error */
  }
  else
  {
    /* USER CODE BEGIN TX_Byte_Pool_Success */

    /* USER CODE END TX_Byte_Pool_Success */

    memory_ptr = (VOID *)&nx_app_byte_pool;
    status = MX_NetXDuo_Init(memory_ptr);
    if (status != NX_SUCCESS)
    {
      /* USER CODE BEGIN  MX_NetXDuo_Init_Error */
      Error_Handler();
      /* USER CODE END  MX_NetXDuo_Init_Error */
    }
    /* USER CODE BEGIN  MX_NetXDuo_Init_Success */
  /* wait until an IP address is obtained */
    if(tx_semaphore_get(&Semaphore_ip_addr_ready, TX_WAIT_FOREVER) != TX_SUCCESS)
    {
      Error_Handler();
    }
    /* USER CODE END MX_NetXDuo_Init_Success */

  }  
  
#ifdef MODULE_LOAD_FROM_FILE   
  if (App_FileX_Init() != FX_SUCCESS) 
  {
    return;
  }    
/* use this function only if module file is to be created from a previously flashed module 
instead of downloading it from FTP srv */  
//  App_FileX_Create_Module_File(MODULE_FILE_NAME);    
//#endif
  
    status = Ftp_File_Get(MODULE_FILE_NAME, &media_disk);
    if (status != TX_SUCCESS) {
      printf ("Module %s dowload from FTP failed\n", MODULE_FILE_NAME);
      printf ("Checking if module file is already present on NOR file system ...\n");
        
      if (App_FileX_Check_Module_File(MODULE_FILE_NAME)) {
        printf ("Module file not present on file system: demo end\n");
        App_FileX_DeInit();
        return;
      } else {
        printf ("Module file already present on file system: demo it\n");
      }
    }  
#endif  
  /* Initialize the module manager. */
  status = txm_module_manager_initialize((VOID *) module_data_area, MODULE_DATA_SIZE);

  if(status != TX_SUCCESS)
  {
    Error_Handler();
  }

  /* Create a pool for module objects. */
  status = txm_module_manager_object_pool_create(object_memory, OBJECT_MEM_SIZE);

  if(status != TX_SUCCESS)
  {
    Error_Handler();
  }

  /* Register a fault handler. */
  status = txm_module_manager_memory_fault_notify(module_fault_handler);

  if(status != TX_SUCCESS)
  {
    Error_Handler();
  } 

#ifdef MODULE_LOAD_FROM_FILE 
  
 /* Load the module from the specified FILE and execute it in SRAM */
  status = txm_module_manager_file_load(&ModuleOne, "Module One", &media_disk, MODULE_FILE_NAME);  // OK
  
#else
#ifdef MODULE_EXECUTE_IN_PLACE
  /* execute the module from the specified FLASH address, it needs to enable module flashing from IDE -> Option -> Debugger -> DownloadExtra Image */
  /* in this case the Module execution address and Module load/compile address are the same: "in place execution" */
  status = txm_module_manager_in_place_load(&ModuleOne, "Module One", (VOID *) MODULE_FLASH_ADDRESS);  // OK
#else  
 /* Load the module from the specified FLASH address and execute it in SRAM */ 
  status = txm_module_manager_memory_load(&ModuleOne, "Module One", (VOID *) MODULE_FLASH_ADDRESS);  // OK
#endif  
#endif
    
  if(status != TX_SUCCESS)
  {
    Error_Handler();
  }

  /* Enable shared memory region for module with read-only access permission. */
  status = txm_module_manager_external_memory_enable(&ModuleOne, (void*)READONLY_REGION, SHARED_MEM_SIZE, TXM_MODULE_ATTRIBUTE_READ_ONLY);

  if(status != TX_SUCCESS)
  {
    Error_Handler();
  }

  /* Enable shared memory region for module with read and write access permission. */
  status = txm_module_manager_external_memory_enable(&ModuleOne, (void*)READWRITE_REGION, SHARED_MEM_SIZE, TXM_MODULE_ATTRIBUTE_READ_WRITE);

  if(status != TX_SUCCESS)
  {
    Error_Handler();
  }

  /* Get module properties. */
  status = txm_module_manager_properties_get(&ModuleOne, &module_properties);

  if(status != TX_SUCCESS)
  {
    Error_Handler();
  }

  /* Print loaded module info */
#ifdef MODULE_LOAD_FROM_FILE 
  printf("Module <%s> is loaded from file %s executed from address: 0x%08X \n", ModuleOne.txm_module_instance_name, MODULE_FILE_NAME, ModuleOne.txm_module_instance_code_start);
#else  
  printf("Module <%s> is loaded from address 0x%08X executed from address: 0x%08X \n", ModuleOne.txm_module_instance_name, MODULE_FLASH_ADDRESS, ModuleOne.txm_module_instance_code_start);
#endif
  printf("Module code section size: %i bytes, data section size: %i\n", (int)ModuleOne.txm_module_instance_code_size, (int)ModuleOne.txm_module_instance_data_size);
  printf("Module Attributes:\n");
  printf("  - Compiled for %s compiler\n", ((module_properties >> 25) == 1)? "STM32CubeIDE (GNU)" : ((module_properties >> 24) == 1)? "ARM KEIL" : "IAR EW");
  printf("  - Shared/external memory access is %s\n", ((module_properties & 0x04) == 0)? "Disabled" : "Enabled");
  printf("  - MPU protection is %s\n", ((module_properties & 0x02) == 0)? "Disabled" : "Enabled");
  printf("  - %s mode execution is enabled for the module\n\n", ((module_properties & 0x01) == 0)? "Privileged" : "User");

  /* Start the modules. */
  status = txm_module_manager_start(&ModuleOne);

  if(status != TX_SUCCESS)
  {
    Error_Handler();
  }

  printf("Module Manager execution is started\n");

  /* Send to Module the start msg */
  ULONG tx_msg = 0x55;
  tx_queue_send(&ResidentTxQueue, &tx_msg, TX_NO_WAIT);
  
  /* Get Module's progress messages */
  while(1)
  {
    if(tx_queue_receive(&ResidentRxQueue, &r_msg, TX_WAIT_FOREVER) == TX_SUCCESS)    
    {
      /* Convert the message to a user friendly string */
      pretty_msg(p_msg, r_msg);

      printf("Module is executing: %s\n", p_msg);

      /* Check if the last executed Module operation resulted in memory violation or
          module processing is finished */      
      if (r_msg == MEMORY_ACCESS_VIOLATION || r_msg == PROCESSING_FINISHED)
      {        
        break;
      }
    }
  }

  /* Close the File System */
  App_FileX_DeInit();
  
  /* Stop the modules. */
  status = txm_module_manager_stop(&ModuleOne);

  if(status != TX_SUCCESS)
  {
    Error_Handler();
  }

  /* Unload the modules. */
  status = txm_module_manager_unload(&ModuleOne);

  if(status != TX_SUCCESS)
  {
    Error_Handler();
  }

  /* Toggle green LED to indicated success of operations */
  while(1) {
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    tx_thread_sleep(50);
  }
}

VOID module_fault_handler(TX_THREAD *thread, TXM_MODULE_INSTANCE *module)
{
  ULONG s_msg;  
  /* send msg to MM thread to chain the fault event */ 
  s_msg = MEMORY_ACCESS_VIOLATION;
  tx_queue_send(&ResidentRxQueue, &s_msg, TX_NO_WAIT);
}

VOID pretty_msg(char *p_msg, ULONG r_msg)
{
  memset(p_msg, 0, 64);

  switch(r_msg)
  {
  case WRITING_TO_READWRITE:
    memcpy(p_msg, "Writing to ReadWrite Region", 27);
    break;
  case WRITING_TO_READONLY:
    memcpy(p_msg, "Writing to ReadOnly Region", 26);
    break;
  case READING_FROM_READWRITE:
    memcpy(p_msg, "Reading from ReadWrite Region", 29);
    break;
  case READING_FROM_READONLY:
    memcpy(p_msg, "Reading from ReadOnly Region", 28);
    break;
  case PROCESSING_FINISHED:
    memcpy(p_msg, "All operations were done", 24);
    break;
  case MEMORY_ACCESS_VIOLATION:
    memcpy(p_msg, "Memory access violation", 23);
    break;    
  default:
    memcpy(p_msg, "Invalid option", 14);
    break;
  }
}

/* USER CODE END  1 */

