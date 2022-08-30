/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_filex.c
  * @author  MCD Application Team
  * @brief   FileX applicative file
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
#include "app_filex.h"
#include "app_threadx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEFAULT_MEDIA_BUF_LENGTH         512 
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* Buffer for FileX FX_MEDIA sector cache. This must be large enough for at least one
   sector, which are typically 512 bytes in size.  */
static UCHAR fx_byte_pool_buffer[FX_APP_MEM_POOL_SIZE];
static TX_BYTE_POOL  fx_app_byte_pool;

#ifdef FX_ENABLE_FAULT_TOLERANT
static UCHAR         fault_tolerant_memory[FX_FAULT_TOLERANT_MAXIMUM_LOG_FILE_SIZE];
#endif /* FX_ENABLE_FAULT_TOLERANT */

/* Define FileX global data structures.  */
/* Buffer for FileX FX_MEDIA sector cache. This must be large enough for at least one
   sector, which are typically 512 bytes in size.  */

FX_MEDIA               media_disk;
static ULONG           media_disk_sector_cache[FX_SECTOR_SIZE / sizeof(ULONG)];

static CHAR * pointer = FX_NULL;
static FX_FILE          my_file;
#define MAX_MODULE_SIZE   2048
static ULONG tmp_module [MAX_MODULE_SIZE / sizeof(ULONG)];

#ifdef NOR_FLASH_FS
#define MEDIA_DISK_NAME         "NOR_DISK"   

#else
/* Define RAM device driver entry.  */
#define MEDIA_DISK_NAME         "RAM_DISK"
VOID _fx_ram_driver(FX_MEDIA *media_ptr);
#endif

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/**
  * @brief  Application FileX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT App_FileX_Init()
{

  UINT  ret=0;
  
/** Initialize the FileX application  **/        
  ret = tx_byte_pool_create(&fx_app_byte_pool, "Fx App memory pool", fx_byte_pool_buffer, FX_APP_MEM_POOL_SIZE);  
  if (ret != TX_SUCCESS)
  {
    return ret;
  }    

  ret = tx_byte_allocate(&fx_app_byte_pool, (VOID **)&pointer, FX_DISK_MEM_POOL_SIZE, TX_NO_WAIT);
  if (ret != TX_SUCCESS) {
    return ret;
  }
  
    /* Initialize FileX.  */
  fx_system_initialize();  

#ifdef NOR_FLASH_FS
  
 /* Check if NOR disk was already formatted  */
  ret =  fx_media_open(&media_disk, MEDIA_DISK_NAME, fx_stm32_levelx_nor_driver, (VOID*)LX_NOR_OSPI_DRIVER_ID, media_disk_sector_cache, sizeof(media_disk_sector_cache));
  if (ret != FX_SUCCESS) 
  {
    /* Print the absolute size of the NOR chip*/
    printf("Formatted Total NOR Flash Chip size is: %lu bytes.\n",(ULONG)LX_STM32_OSPI_FLASH_SIZE);
    /* Format the NOR flash as FAT */
    ret =  fx_media_format(&media_disk,
                            fx_stm32_levelx_nor_driver,   // Driver entry
                            (VOID*)LX_NOR_OSPI_DRIVER_ID,    // Device info pointer
                            (UCHAR *) media_disk_sector_cache,       // Media buffer pointer
                            DEFAULT_MEDIA_BUF_LENGTH,     // Media buffer size
                            MEDIA_DISK_NAME,             // Volume Name
                            1,                            // Number of FATs
                            32,                           // Directory Entries
                            0,                            // Hidden sectors
                            LX_STM32_OSPI_FLASH_SIZE / 512,    // Total sectors
                            512,                          // Sector size
                            8,                            // Sectors per cluster
                            1,                            // Heads
                            1);                           // Sectors per track

    /* Check if the format ret */
    if (ret != FX_SUCCESS)
    {
      Error_Handler();
    }
  }
  return ret;
  
#else  /* !NOR_FLASH_FS    */ 
  /* volatile RAM disk need to be formatted at each reboot */
#ifdef FX_ENABLE_EXFAT
    ret = fx_media_exFAT_format(&media_disk,
                          _fx_ram_driver,         // Driver entry
                          pointer,        // RAM disk memory pointer
                          (UCHAR *) media_disk_sector_cache,           // Media buffer pointer
                          sizeof(media_disk_sector_cache),   // Media buffer size
                          MEDIA_DISK_NAME,          // Volume Name
                          1,                      // Number of FATs
                          0,                      // Hidden sectors
                          FX_SECTOR_NUMBER,       // Total sectors
                          FX_SECTOR_SIZE,         // Sector size
                          8,                      // exFAT Sectors per cluster
                          12345,                  // Volume ID
                          1);                     // Boundary unit
#else
    ret = fx_media_format(&media_disk,
                    _fx_ram_driver,               // Driver entry
                    pointer,              // RAM disk memory pointer
                    (UCHAR *) media_disk_sector_cache,                 // Media buffer pointer
                    sizeof(media_disk_sector_cache),         // Media buffer size
                    MEDIA_DISK_NAME,                // Volume Name
                    1,                            // Number of FATs
                    32,                           // Directory Entries
                    0,                            // Hidden sectors
                    FX_SECTOR_NUMBER,             // Total sectors
                    FX_SECTOR_SIZE,               // Sector size
                    8,                            // Sectors per cluster
                    1,                            // Heads
                    1);                           // Sectors per track      
#endif /* FX_ENABLE_EXFAT */
    if (ret != FX_SUCCESS) {
      return ret;
    }        
      /* Open the fs disk.  */
    ret =  fx_media_open(&media_disk, MEDIA_DISK_NAME, _fx_ram_driver, pointer, media_disk_sector_cache, sizeof(media_disk_sector_cache));
    if (ret != FX_SUCCESS) {
      return ret;
    }  
    return ret;
#endif /* NOR_FLASH_FS    */         
}

UINT App_FileX_Create_Module_File(char * module_file_name)
{
   UINT ret = FX_SUCCESS;
   TXM_MODULE_PREAMBLE * p_flash_module_preamble = (TXM_MODULE_PREAMBLE *)MODULE_FLASH_ADDRESS;

#ifdef FX_ENABLE_FAULT_TOLERANT
    ret = fx_fault_tolerant_enable(&media_disk, fault_tolerant_memory, sizeof(fault_tolerant_memory));
    
    if (ret != FX_SUCCESS)
    {
      return ret;
    }
#endif /* FX_ENABLE_FAULT_TOLERANT */  
  
    /* check if the module file already exists in NOR FS  */
    ret =  fx_file_open(&media_disk, &my_file, module_file_name, FX_OPEN_FOR_WRITE);
    /* if file exist but is corrupted delete it */
    if (ret == FX_FILE_CORRUPT)
    {
      fx_file_delete(&media_disk, module_file_name);
    }
    /* if file does not exists create it */
    if (ret == FX_NOT_FOUND || ret == FX_FILE_CORRUPT)
    {
      /* Create the module file to host the flashed module */
      ret =  fx_file_create(&media_disk, module_file_name);   
      
      if (ret != FX_SUCCESS)
      {       
        return ret;
      }     
      /* open and seek to 0 the newly created file */
      fx_file_open(&media_disk, &my_file, module_file_name, FX_OPEN_FOR_WRITE);
      /* Seek to the beginning of the module file.  */
      ret =  fx_file_seek(&my_file, 0);
  
      if (ret != FX_SUCCESS)
      {
        fx_file_close(&my_file);
        return ret;
      }  
  
      /* copy whole module from flash to the module file.  */      
      ret =  fx_file_write(&my_file, (VOID *) p_flash_module_preamble,  p_flash_module_preamble->txm_module_preamble_code_size); 
   
      fx_file_close(&my_file);        
    /* if fopen return other kind of errors abort and return  */
    } else if (ret != FX_SUCCESS) 
      {
        return ret;        
      } else if (ret == FX_SUCCESS)
      /* Module file already existing in FS   */      
      /* check if existing file is different than the Module in flash, in case of differences override the file*/
      {
        
         /* Seek to the beginning of the module file.  */                
        ret =  fx_file_seek(&my_file, 0);
        if (ret != FX_SUCCESS)
        {
          fx_file_close(&my_file);        
          return ret;
        }
        
        /* Read and check the preamble of the module file vs module in flash  */
        TXM_MODULE_PREAMBLE     preamble;
        ULONG actual_size=0;        
        ret =  fx_file_read(&my_file, (VOID *) &preamble, sizeof(TXM_MODULE_PREAMBLE), &actual_size);
        if (ret != FX_SUCCESS)
        {
          fx_file_close(&my_file);        
          return ret;
        }
       
        /* check if modules preamble are equal */
        if (!memcmp ((VOID *) &preamble, (VOID *) p_flash_module_preamble, sizeof(TXM_MODULE_PREAMBLE)))
        {
          /* check if flash module code fits in memory */
          if (p_flash_module_preamble -> txm_module_preamble_code_size > MAX_MODULE_SIZE )
          {
            /* no: abort */
            printf ("ERROR: Module size exceed memory, File: %s, Line: %d\n", __FILE__, __LINE__);
            fx_file_close(&my_file);
            return -1;
          } 
          /* yes: check if the whole module in flash was changed */
          fx_file_seek(&my_file, 0);          
          ret =  fx_file_read(&my_file, (VOID *)tmp_module, preamble.txm_module_preamble_code_size, &actual_size);
          if (ret != FX_SUCCESS)
          {
            fx_file_close(&my_file);        
            return ret;
          }
          
          if (!memcmp ((VOID *)tmp_module, ((UCHAR *)p_flash_module_preamble), p_flash_module_preamble -> txm_module_preamble_code_size))
          {
            /* module code in flash is equal to code in file */
            fx_file_close (&my_file);
            return (FX_SUCCESS);
          }
        } 
        
        /* module code or preamble in flash differs to code in file: update file */
        fx_file_seek(&my_file, 0);
        /* copy whole module from flash to the module file.  */      
        ret =  fx_file_write(&my_file, (VOID *) p_flash_module_preamble,  p_flash_module_preamble->txm_module_preamble_code_size);           
        if (ret != FX_SUCCESS)
        {
          fx_file_close(&my_file);        
          return ret;
        }
        
        fx_file_close (&my_file);
      }
  return ret;
}

UINT App_FileX_DeInit()
{
  fx_media_close(&media_disk);  
  tx_byte_release(pointer);
  tx_byte_pool_delete(&fx_app_byte_pool);
  return FX_SUCCESS;
}
                          
UINT App_FileX_Check_Module_File(char * module_file_name)
{
  UINT ret = FX_SUCCESS;
  /* check if the module file already exists in NOR FS  */
  ret =  fx_file_open(&media_disk, &my_file, module_file_name, FX_OPEN_FOR_WRITE);
  /* if file exist but is corrupted delete it */
  if (ret == FX_FILE_CORRUPT)
  {
    fx_file_delete(&media_disk, module_file_name);
    return ret;
  }
  /* if file does not exists return */
  if (ret == FX_NOT_FOUND || ret == FX_FILE_CORRUPT)
  {
     fx_file_close(&my_file);
     return ret;
  }      
  
  /* Read and check the preamble of the module file vs module in flash  */
  TXM_MODULE_PREAMBLE     preamble;
  ULONG actual_size=0;        
  fx_file_seek(&my_file, 0);
  ret =  fx_file_read(&my_file, (VOID *) &preamble, sizeof(TXM_MODULE_PREAMBLE), &actual_size);
  fx_file_close(&my_file);     
  if (ret != FX_SUCCESS)
  {
    return ret;
  }  

  if (preamble.txm_module_preamble_id == TXM_MODULE_ID) 
  {     
    return FX_SUCCESS;
  } else {
    return FX_FILE_CORRUPT;
  }
}



                          
