/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_netxduo.c
  * @author  MCD Application Team
  * @brief   NetXDuo applicative file
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
#include "app_netxduo.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "nx_ip.h"
#include "stm32u5xx_hal_rtc.h" 
#include <time.h>
#include "nxd_ftp_client.h"
#include "fx_api.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#ifdef HOME_WIFI
#define FTP_SERVER_ADDRESS  IP_ADDRESS(192,168,1,5)
#define FTP_SERVER_ADDRESS_STRING     "192,168,1,5"
#else
#define FTP_SERVER_ADDRESS  IP_ADDRESS(192,168,1,47)
#define FTP_SERVER_ADDRESS_STRING     "192,168,1,47"
#endif
#define FTP_ACCOUNT         "mapelli"
#define FTP_PASSW           "licio1234"

TX_THREAD NetXThread;
TX_THREAD AppSNTPThread;
TX_SEMAPHORE Semaphore;
TX_SEMAPHORE Semaphore_ip_addr_ready;
TX_EVENT_FLAGS_GROUP     SntpFlags;

NX_PACKET_POOL           NetXPool;
NX_IP                    IpInstance;
NX_DHCP                  DhcpClient;
NX_DNS                   DnsClient;
NX_SNTP_CLIENT           SntpClient;
NX_FTP_CLIENT            ftp_client;

ULONG                    IpAddress;
ULONG                    NetMask;
CHAR                     buffer[64];
CHAR                     *pointer;

struct tm timeInfos;
  
/* RTC handler declaration */
extern RTC_HandleTypeDef hrtc;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
static UINT kiss_of_death_handler(NX_SNTP_CLIENT *client_ptr, UINT KOD_code);
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
static void display_rtc_time(RTC_HandleTypeDef *hrtc);
static void rtc_time_update(NX_SNTP_CLIENT *client_ptr);
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
/* set the SNTP network interface to the primary interface. */
UINT  iface_index =0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static VOID NetX_Thread_Entry(ULONG thread_input);
static VOID App_SNTP_Thread_Entry(ULONG thread_input);

static VOID ip_address_change_notify_callback(NX_IP *ip_instance, VOID *ptr);
static VOID time_update_callback(NX_SNTP_TIME_MESSAGE *time_update_ptr, NX_SNTP_TIME *local_time);
/* USER CODE END PFP */
/**
  * @brief  Application NetXDuo Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT MX_NetXDuo_Init(VOID *memory_ptr)
{
  UINT ret = NX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

   /* USER CODE BEGIN App_NetXDuo_MEM_POOL */

  /* USER CODE END App_NetXDuo_MEM_POOL */

  /* USER CODE BEGIN MX_NetXDuo_Init */
  /* Initialize the NetX system.  */
  nx_system_initialize();  
  
#if (USE_STATIC_ALLOCATION  == 1)
  printf("Nx_SNTP_Client application started..\n");
  
  CHAR *pointer;
  
  /* Allocate the memory for packet_pool.  */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,  NX_PACKET_POOL_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }
  
  /* Create the Packet pool to be used for packet allocation */
  ret = nx_packet_pool_create(&NetXPool, "Main Packet Pool", PAYLOAD_SIZE, pointer, NX_PACKET_POOL_SIZE);
  
  if (ret != NX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
  
  /* Allocate the memory for Ip_Instance */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, 2 * DEFAULT_MEMORY_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }
  
  /* Create the main NX_IP instance */
  ret = nx_ip_create(&IpInstance, "Main Ip instance", NULL_ADDRESS, NULL_ADDRESS, &NetXPool, nx_driver_emw3080_entry,
                     pointer, 2 * DEFAULT_MEMORY_SIZE, DEFAULT_NETX_PRIORITY);
  
  if (ret != NX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
  
  /* create the DHCP client */
  ret = nx_dhcp_create(&DhcpClient, &IpInstance, "DHCP Client");
  
  if (ret != NX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
  
  /* Allocate the memory for ARP */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, ARP_MEMORY_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }
  
  /* Enable the ARP protocol and provide the ARP cache size for the IP instance */
  ret = nx_arp_enable(&IpInstance, (VOID *)pointer, ARP_MEMORY_SIZE);
  
  if (ret != NX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
  
  /* Enable the ICMP */
  ret = nx_icmp_enable(&IpInstance);
  
  if (ret != NX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
  
  /* Enable the UDP protocol required for  DHCP communication */
  ret = nx_udp_enable(&IpInstance);
  
  if (ret != NX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
 
    /* Enable the TCP protocol */
  ret = nx_tcp_enable(&IpInstance);
  
  if (ret != NX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
  
  /* Allocate the memory for main thread   */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NETX_THREAD_MEMORY, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }
  
  /* Create the NetX thread */
/*  ret = tx_thread_create(&AppMainThread, "App Main thread", App_Main_Thread_Entry, 0, pointer, MAIN_THREAD_MEMORY,
                         DEFAULT_MAIN_PRIORITY, DEFAULT_MAIN_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START); */
  ret = tx_thread_create(&NetXThread, "NetX thread", NetX_Thread_Entry, 0, pointer, NETX_THREAD_MEMORY,
                         DEFAULT_NETX_PRIORITY, DEFAULT_NETX_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START); 
  
  if (ret != TX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
  
  /* Allocate the memory for SNTP client thread   */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, SNTP_CLIENT_THREAD_MEMORY, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }
  
  /* create the SNTP client thread */
  ret = tx_thread_create(&AppSNTPThread, "App SNTP Thread", App_SNTP_Thread_Entry, 0, pointer, SNTP_CLIENT_THREAD_MEMORY,
                         DEFAULT_PRIORITY, DEFAULT_PRIORITY, TX_NO_TIME_SLICE, TX_DONT_START);
  
  if (ret != TX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
    
  /* Create the event flags. */
  ret = tx_event_flags_create(&SntpFlags, "SNTP event flags");

  /* Check for errors */
  if (ret != NX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
  
  /* set DHCP notification callback  */
  tx_semaphore_create(&Semaphore, "DHCP Semaphore", 0);
  tx_semaphore_create(&Semaphore_ip_addr_ready, "IP addr ready Semaphore", 0);
#endif
  /* USER CODE END MX_NetXDuo_Init */

  return ret;
}

/* USER CODE BEGIN 1 */
/**
* @brief  ip address change callback.
* @param ip_instance: NX_IP instance
* @param ptr: user data
* @retval none
*/
static VOID ip_address_change_notify_callback(NX_IP *ip_instance, VOID *ptr)
{
  /* release the semaphore as soon as an IP address is available */
  tx_semaphore_put(&Semaphore);
}

/**
* @brief  Main thread entry.
* @param thread_input: ULONG user argument used by the thread entry
* @retval none
*/
static VOID NetX_Thread_Entry(ULONG thread_input)
{
  UINT ret;
  
  ret = nx_ip_address_change_notify(&IpInstance, ip_address_change_notify_callback, NULL);
  if (ret != NX_SUCCESS)
  {
    Error_Handler();
  }
  
  ret = nx_dhcp_start(&DhcpClient);
  
  if (ret != NX_SUCCESS)
  {
    Error_Handler();
  }
  printf("Waiting for IP address ...\n");
  /* wait until an IP address is ready */
  if(tx_semaphore_get(&Semaphore, TX_WAIT_FOREVER) != TX_SUCCESS)
  {
    Error_Handler();
  }
  
  ret = nx_ip_address_get(&IpInstance, &IpAddress, &NetMask);
    
  if (ret != TX_SUCCESS)
  {
    Error_Handler();
  }
  
  PRINT_IP_ADDRESS(IpAddress);
  
  /* the network is correctly initialized, start the TCP server thread */
  tx_thread_resume(&AppSNTPThread);
  
  /* this thread is not needed any more, we relinquish it */
  tx_thread_relinquish();
  
  return;
}

/**
* @brief  DNS Create Function.
* @param dns_ptr
* @retval ret
*/
UINT dns_create(NX_DNS *dns_ptr)
{
  UINT ret = NX_SUCCESS;

  /* Create a DNS instance for the Client */
  ret = nx_dns_create(dns_ptr, &IpInstance, (UCHAR *)"DNS Client");
  
  if (ret)
  {
    Error_Handler();
  }
  
  /* Initialize DNS instance with the DNS server Address */
  ret = nx_dns_server_add(dns_ptr, USER_DNS_ADDRESS);
  if (ret)
  {
    Error_Handler();
  }
  
  return ret;
}

/**
* @brief  SNTP thread entry.
* @param thread_input: ULONG user argument used by the thread entry
* @retval none
*/
/* Define the client thread.  */
static void App_SNTP_Thread_Entry(ULONG info)
{
  UINT ret;
  ULONG  seconds, fraction;
  ULONG  events = 0;
  UINT   server_status;
  NXD_ADDRESS sntp_server_ip;
  NX_PARAMETER_NOT_USED(info);
  
  sntp_server_ip.nxd_ip_version = 4;
  
  /* Create a DNS client */
  ret = dns_create(&DnsClient);
  
  if (ret != NX_SUCCESS)
  {
    Error_Handler();
  }
  
  /* Look up SNTP Server address. */
  ret = nx_dns_host_by_name_get(&DnsClient, (UCHAR *)SNTP_SERVER_NAME, 
                                &sntp_server_ip.nxd_ip_address.v4, DEFAULT_TIMEOUT);
  
    /* Check for error. */
  if (ret != NX_SUCCESS)
  {
    Error_Handler();
  }
  
   /* Create the SNTP Client */
  ret =  nx_sntp_client_create(&SntpClient, &IpInstance, iface_index, &NetXPool, NULL, kiss_of_death_handler, NULL);
  
  /* Check for error. */
  if (ret != NX_SUCCESS)
  {
    Error_Handler();
  }
  
  /* Setup time update callback function. */
   nx_sntp_client_set_time_update_notify(&SntpClient, time_update_callback);
  
  /* Use the IPv4 service to set up the Client and set the IPv4 SNTP server. */
  ret = nx_sntp_client_initialize_unicast(&SntpClient, sntp_server_ip.nxd_ip_address.v4);
  
  if (ret != NX_SUCCESS)
  {
    Error_Handler();
  }
  
  /* Run whichever service the client is configured for. */
  ret = nx_sntp_client_run_unicast(&SntpClient);
  
  if (ret != NX_SUCCESS)
  {
    Error_Handler();
  }
  else 
  {
    PRINT_CNX_SUCC();
  }
  
  /* Wait for a server update event. */
  tx_event_flags_get(&SntpFlags, SNTP_UPDATE_EVENT, TX_OR_CLEAR, &events, PERIODIC_CHECK_INTERVAL);
  
  if (events == SNTP_UPDATE_EVENT)
  {
    /* Check for valid SNTP server status. */
    ret = nx_sntp_client_receiving_updates(&SntpClient, &server_status);
    
    if ((ret != NX_SUCCESS) || (server_status == NX_FALSE))
    {
      /* We do not have a valid update. */
      Error_Handler();
    }
    /* We have a valid update.  Get the SNTP Client time.  */
    ret = nx_sntp_client_get_local_time_extended(&SntpClient, &seconds, &fraction, NX_NULL, 0);
    
    ret = nx_sntp_client_utility_display_date_time(&SntpClient,buffer,64);
    
    if (ret != NX_SUCCESS)
    {
      printf("Internal error with getting local time 0x%x\n", ret);
      Error_Handler();
    }
    else
    {
      printf("\nSNTP update :\n");
      printf("%s\n\n",buffer);
    }
  }
  else
  {
    Error_Handler();
  }
  
  /* Set Current time from SNTP TO RTC */
  rtc_time_update(&SntpClient);
  
  /* We can stop the SNTP service if for example we think the SNTP server has stopped sending updates */
  ret = nx_sntp_client_stop(&SntpClient);
  if (ret != NX_SUCCESS)
  {
    Error_Handler();
  }
  
  /* When done with the SNTP Client, we delete it */
  ret = nx_sntp_client_delete(&SntpClient);
  if (ret != NX_SUCCESS)
  {
    Error_Handler();
  }
   
  /* notify app_threadx of IP addr aquisition and SNTP sinchronization: Module play can start */
  tx_semaphore_put(&Semaphore_ip_addr_ready);
  
  /* Toggling LED after a success Time update */
  while(1)
  {
    /* Display RTC time each second  */
    display_rtc_time(&hrtc);
    /* Delay for 10s */
    tx_thread_sleep(1000);
  }
}

/* This application defined handler for handling a Kiss of Death packet is not
required by the SNTP Client. A KOD handler should determine
if the Client task should continue vs. abort sending/receiving time data
from its current time server, and if aborting if it should remove
the server from its active server list.

Note that the KOD list of codes is subject to change. The list
below is current at the time of this software release. */

static UINT kiss_of_death_handler(NX_SNTP_CLIENT *client_ptr, UINT KOD_code)
{
  UINT    remove_server_from_list = NX_FALSE;
  UINT    status = NX_SUCCESS;
  
  NX_PARAMETER_NOT_USED(client_ptr);
  
  /* Handle kiss of death by code group. */
  switch (KOD_code)
  {
    
  case NX_SNTP_KOD_RATE:
  case NX_SNTP_KOD_NOT_INIT:
  case NX_SNTP_KOD_STEP:
    
    /* Find another server while this one is temporarily out of service.  */
    status =  NX_SNTP_KOD_SERVER_NOT_AVAILABLE;
    
    break;
    
  case NX_SNTP_KOD_AUTH_FAIL:
  case NX_SNTP_KOD_NO_KEY:
  case NX_SNTP_KOD_CRYP_FAIL:
    
    /* These indicate the server will not service client with time updates
    without successful authentication. */
    
    remove_server_from_list =  NX_TRUE;
    
    break;
    
    
  default:
    
    /* All other codes. Remove server before resuming time updates. */
    
    remove_server_from_list =  NX_TRUE;
    break;
  }
  
  /* Removing the server from the active server list? */
  if (remove_server_from_list)
  {
    
    /* Let the caller know it has to bail on this server before resuming service. */
    status = NX_SNTP_KOD_REMOVE_SERVER;
  }
  
  return status;
}

/* This application defined handler for notifying SNTP time update event.  */
static VOID time_update_callback(NX_SNTP_TIME_MESSAGE *time_update_ptr, NX_SNTP_TIME *local_time)
{
  NX_PARAMETER_NOT_USED(time_update_ptr);
  NX_PARAMETER_NOT_USED(local_time);
  
  tx_event_flags_set(&SntpFlags, SNTP_UPDATE_EVENT, TX_OR);
}

/* This application updates Time from SNTP to STM32 RTC */
static void rtc_time_update(NX_SNTP_CLIENT *client_ptr)
{
  RTC_DateTypeDef sdatestructure ={0};
  RTC_TimeTypeDef stimestructure ={0};
  struct tm ts;
  CHAR  temp[32] = {0};
  
  /* convert SNTP time (seconds since 01-01-1900 to 01-01-1970)
  
  EPOCH_TIME_DIFF is equivalent to 70 years in sec
  calculated with www.epochconverter.com/date-difference
  This constant is used to delete difference between :
  Epoch converter (referenced to 1970) and SNTP (referenced to 1900) */
  time_t timestamp = client_ptr->nx_sntp_current_server_time_message.receive_time.seconds - EPOCH_TIME_DIFF;
  
  /* convert time in yy/mm/dd hh:mm:sec */
  ts = *localtime(&timestamp);
  
  /* convert date composants to hex format */
  sprintf(temp, "%d", (ts.tm_year - 100));
  sdatestructure.Year = strtol(temp, NULL, 16);
  sprintf(temp, "%d", ts.tm_mon + 1);
  sdatestructure.Month = strtol(temp, NULL, 16);
  sprintf(temp, "%d", ts.tm_mday);
  sdatestructure.Date = strtol(temp, NULL, 16);
  /* dummy weekday */
  sdatestructure.WeekDay =0x00;
  
  if (HAL_RTC_SetDate(&hrtc, &sdatestructure, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* convert time composants to hex format */
  sprintf(temp,"%d", ts.tm_hour);
  stimestructure.Hours = strtol(temp, NULL, 16);
  sprintf(temp,"%d", ts.tm_min);
  stimestructure.Minutes = strtol(temp, NULL, 16);
  sprintf(temp, "%d", ts.tm_sec);
  stimestructure.Seconds = strtol(temp, NULL, 16);
  
  if (HAL_RTC_SetTime(&hrtc, &stimestructure, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  
}

/* this application displays time from RTC */
static void display_rtc_time(RTC_HandleTypeDef *hrtc)
{
  RTC_TimeTypeDef RTC_Time = {0};
  RTC_DateTypeDef RTC_Date = {0};
  
  HAL_RTC_GetTime(hrtc, &RTC_Time,RTC_FORMAT_BCD);
  HAL_RTC_GetDate(hrtc, &RTC_Date,RTC_FORMAT_BCD);
  
  printf("%02x-%02x-20%02x / %02x:%02x:%02x\n",\
        RTC_Date.Date, RTC_Date.Month, RTC_Date.Year,RTC_Time.Hours,RTC_Time.Minutes,RTC_Time.Seconds);
}


ULONG Ftp_File_Get(CHAR *fname, FX_MEDIA * local_disk) {
    UINT        ret = NX_SUCCESS; 
    ULONG       error_counter = 0; 
    ULONG       file_size;  
    FX_FILE     my_file;
    NX_PACKET   * my_packet =NULL;    


    /* Create an FTP client using IP NetXPool pkt */
    ret =  nx_ftp_client_create(&ftp_client, "FTP Client", &IpInstance, 2000, &NetXPool);
    /* Check status.  */
    if (ret != NX_SUCCESS) 
    {
        error_counter++;
        return ret;
    }

    printf("\nCreated the FTP Client\n");  
    printf("\nConnecting the FTP Server: %s ...\n", FTP_SERVER_ADDRESS_STRING);  
  
    /* Now connect with the NetX FTP (IPv4) server. */
    ret =  nx_ftp_client_connect(&ftp_client, FTP_SERVER_ADDRESS, FTP_ACCOUNT, FTP_PASSW, 5*NX_IP_PERIODIC_RATE);

    /* Check status.  */
    if (ret != NX_SUCCESS) 
    {
        error_counter++;
        return ret;
    }

    printf("Connected to the FTP Server\n");

#ifdef PASSIVE_MODE
    /* Enable passive mode. */
    ret = nx_ftp_client_passive_mode_set(&ftp_client, NX_TRUE);
    if (ret != NX_SUCCESS) 
    {
        error_counter++;
        return ret;
    }

    printf("Enabled FTP Client Passive Mode\n");
#endif /* PASSIVE_MODE  */
    
    /* Now open the same file for reading.  */
    ret =  nx_ftp_client_file_open(&ftp_client, fname, NX_FTP_OPEN_FOR_READ, 20*NX_IP_PERIODIC_RATE);

    if (ret != NX_SUCCESS)
    {
        error_counter++;        
        return ret;
    }
    printf("Opened the FTP client %s file\n", fname);  
 
    /* create file on local FS */
    /* Create a file called fname in the root directory.  */
    ret =  fx_file_create(local_disk, fname);
    
    /* Check the create status.  */
    if (ret != FX_SUCCESS)
    {       
      if (ret != FX_ALREADY_CREATED)
      {
        return ret;
      }
    }    

    /* Open the local module file.  */
    ret =  fx_file_open(local_disk, &my_file, fname, FX_OPEN_FOR_WRITE);
    
    /* Check the file open status.  */
    if (ret != FX_SUCCESS)
    {
      return ret;
    }
    
    /* Seek to the beginning of the test file.  */
    ret =  fx_file_seek(&my_file, 0);
    
    /* Check the file seek status.  */
    if (ret != FX_SUCCESS)
    {
      return ret;
    }
    
    /* Loop to read the data.  */
    file_size = 0;
 //tx_thread_suspend(tx_thread_identify());   
    do
    {
       /* Read the file.  */
       ret =  nx_ftp_client_file_read(&ftp_client, &my_packet, 5*NX_IP_PERIODIC_RATE);

       /* Check status.  */
       if (ret == NX_SUCCESS)
       {
          /* dont need to pass a pkt as it allocate internally from the IP pkt pool */
          ret =  fx_file_write(&my_file, (VOID *) my_packet -> nx_packet_prepend_ptr,  my_packet -> nx_packet_length);    
    
          /* Check the file write status.  */
          if (ret != FX_SUCCESS)
          {
              nx_packet_release(my_packet);
              
              /* Close the local file.  */
              ret =  fx_file_close(&my_file);             
              return ret;
          }
         
          file_size += my_packet -> nx_packet_length;          
          nx_packet_release(my_packet);
       }
    } while(ret == NX_SUCCESS);

    /* Seek to the beginning of the local file.  */
    ret =  fx_file_seek(&my_file, 0);
    
    /* Check the file seek status.  */
    if (ret != FX_SUCCESS)
    {
      return ret;
    }    
    
    /* Close the local file.  */
    ret =  fx_file_close(&my_file);
    
    /* Check the file close status.  */
    if (ret != FX_SUCCESS)
    {
      return ret;
    }    
    
    printf ("Downloaded from FTP file: %s fsize: %d\n", fname, file_size);    
    
    /* Close the file on FTP serv  */
    ret =  nx_ftp_client_file_close(&ftp_client, 5*NX_IP_PERIODIC_RATE);

   /* Check status.  */
    if (ret != NX_SUCCESS)
    {
        error_counter++;
        return ret;
    }
    else
        printf("Closed the FTP client %s file\n", fname);
        
    return ret;
}

/* USER CODE END 1 */
