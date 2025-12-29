/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "main.h"
#include "string.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "struct_shared_memory.h"
#include "remote_signal_processing.h"
#include "steer_adc_processing.h"
#include "vehicle_control.h"
#include "CAN_DB_Interface.h"
#include "myahrs_i2c.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

ETH_TxPacketConfig TxConfig;
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

ADC_HandleTypeDef hadc1;

CAN_HandleTypeDef hcan1;

ETH_HandleTypeDef heth;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart3;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

osThreadId TaskGetRemoteHandle;
osThreadId TaskPrintResultHandle;
osThreadId TaskGetSteerADCHandle;
osThreadId TaskVehicleContHandle;
osThreadId TaskCANTransmitHandle;
osThreadId TaskGetIMUHandle;
/* USER CODE BEGIN PV */
osMutexId vehicleDataMutexHandle;
EventGroupHandle_t eventGroupHandle; // FreeRTOS Event Group

// Shared Memory
SharedMemory_t vehicle_data_shm_;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ETH_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM1_Init(void);
static void MX_ADC1_Init(void);
static void MX_CAN1_Init(void);
static void MX_I2C1_Init(void);
void EntryGetRemote(void const * argument);
void EntryPrintResult(void const * argument);
void EntryGetSteerADC(void const * argument);
void EntryVehicleControl(void const * argument);
void EntryCANTransmit(void const * argument);
void EntryGetIMU(void const * argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ETH_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_TIM2_Init();
  MX_TIM1_Init();
  MX_ADC1_Init();
  MX_CAN1_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */


  
  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  osMutexDef(vehicleDataMutex);
  vehicleDataMutexHandle = osMutexCreate(osMutex(vehicleDataMutex));
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */

  // from remote_signal_processing.h
  osSemaphoreDef(remoteSigSem);
  remote_sig_sem_handle_ = osSemaphoreCreate(osSemaphore(remoteSigSem), 1);

  // from steer_adc_processing.h
  osSemaphoreDef(steerAdcSem);
  steer_adc_sem_handle_ = osSemaphoreCreate(osSemaphore(steerAdcSem), 1);

  // from myahrs_i2c.h
  osSemaphoreDef(imuDataSem);
  imu_data_sem_handle_ = osSemaphoreCreate(osSemaphore(imuDataSem), 1);

  // Create Event Group
  eventGroupHandle = xEventGroupCreate();

  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of TaskGetRemote */
  osThreadDef(TaskGetRemote, EntryGetRemote, osPriorityNormal, 0, 128);
  TaskGetRemoteHandle = osThreadCreate(osThread(TaskGetRemote), NULL);

  /* definition and creation of TaskPrintResult */
  osThreadDef(TaskPrintResult, EntryPrintResult, osPriorityNormal, 0, 512);
  TaskPrintResultHandle = osThreadCreate(osThread(TaskPrintResult), NULL);

  /* definition and creation of TaskGetSteerADC */
  osThreadDef(TaskGetSteerADC, EntryGetSteerADC, osPriorityNormal, 0, 128);
  TaskGetSteerADCHandle = osThreadCreate(osThread(TaskGetSteerADC), NULL);

  /* definition and creation of TaskVehicleCont */
  osThreadDef(TaskVehicleCont, EntryVehicleControl, osPriorityNormal, 0, 128);
  TaskVehicleContHandle = osThreadCreate(osThread(TaskVehicleCont), NULL);

  /* definition and creation of TaskCANTransmit */
  osThreadDef(TaskCANTransmit, EntryCANTransmit, osPriorityNormal, 0, 512);
  TaskCANTransmitHandle = osThreadCreate(osThread(TaskCANTransmit), NULL);

  /* definition and creation of TaskGetIMU */
  osThreadDef(TaskGetIMU, EntryGetIMU, osPriorityNormal, 0, 128);
  TaskGetIMUHandle = osThreadCreate(osThread(TaskGetIMU), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 6;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_11TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief ETH Initialization Function
  * @param None
  * @retval None
  */
static void MX_ETH_Init(void)
{

  /* USER CODE BEGIN ETH_Init 0 */

  /* USER CODE END ETH_Init 0 */

   static uint8_t MACAddr[6];

  /* USER CODE BEGIN ETH_Init 1 */

  /* USER CODE END ETH_Init 1 */
  heth.Instance = ETH;
  MACAddr[0] = 0x00;
  MACAddr[1] = 0x80;
  MACAddr[2] = 0xE1;
  MACAddr[3] = 0x00;
  MACAddr[4] = 0x00;
  MACAddr[5] = 0x00;
  heth.Init.MACAddr = &MACAddr[0];
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.RxBuffLen = 1524;

  /* USER CODE BEGIN MACADDRESS */

  /* USER CODE END MACADDRESS */

  if (HAL_ETH_Init(&heth) != HAL_OK)
  {
    Error_Handler();
  }

  memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfig));
  TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;
  /* USER CODE BEGIN ETH_Init 2 */

  /* USER CODE END ETH_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 84-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1000-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 84-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_BOTHEDGE;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 4;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, RIGHT_IN2_Pin|LEFT_IN1_Pin|LEFT_IN2_Pin|STEER_IN2_Pin
                          |RIGHT_IN1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(STEER_IN1_GPIO_Port, STEER_IN1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RIGHT_IN2_Pin LEFT_IN1_Pin LEFT_IN2_Pin STEER_IN2_Pin
                           RIGHT_IN1_Pin */
  GPIO_InitStruct.Pin = RIGHT_IN2_Pin|LEFT_IN1_Pin|LEFT_IN2_Pin|STEER_IN2_Pin
                          |RIGHT_IN1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : STEER_IN1_Pin */
  GPIO_InitStruct.Pin = STEER_IN1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(STEER_IN1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_EntryGetRemote */
/**
  * @brief  Function implementing the TaskGetRemote thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_EntryGetRemote */
void EntryGetRemote(void const * argument)
{
  /* USER CODE BEGIN 5 */
  HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
  HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);
  HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_3);
  HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_4);

  RemoteSignals_t remote_signals;

  /* Infinite loop */
  for(;;) {
    remote_signals = GetRemoteSignals(); // This function waits for semaphore

    osMutexWait(vehicleDataMutexHandle, osWaitForever);
    vehicle_data_shm_.remote = remote_signals;
    osMutexRelease(vehicleDataMutexHandle);

    if (eventGroupHandle != NULL) {
        xEventGroupSetBits(eventGroupHandle, EVT_REMOTE_UPDATED_FOR_LOG);
        xEventGroupSetBits(eventGroupHandle, EVT_REMOTE_UPDATED_FOR_CAN);
    }
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_EntryPrintResult */
/**
* @brief Function implementing the TaskPrintResult thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_EntryPrintResult */
void EntryPrintResult(void const * argument)
{
  /* USER CODE BEGIN EntryPrintResult */
  SharedMemory_t print_data;
  char str[100];
  EventBits_t event_bits;
  

  /* Infinite loop */
  for(;;) {
    event_bits = xEventGroupWaitBits(
      eventGroupHandle,
      EVT_ALL_UPDATED_FOR_LOG, // uxBitsToWaitFor
      pdTRUE,          // xClearOnExit:    Clear bits on exit (Auto-Reset)
      pdFALSE,         // xWaitForAllBits: Wait for ANY bit (OR logic)
      osWaitForever    // xBlockTime:      Block until event happens
    );

    // copy shared memory to local variable
    osMutexWait(vehicleDataMutexHandle, osWaitForever);
    print_data = vehicle_data_shm_;
    osMutexRelease(vehicleDataMutexHandle);

    // print remote signals
    int str_len = 0;
    if (event_bits & EVT_REMOTE_UPDATED_FOR_LOG) {
        str_len = snprintf(str, sizeof(str), "RC: %lu %lu %lu %lu\r\n",
                          print_data.remote.steering_pulse_width_us,
                          print_data.remote.throttle_pulse_width_us,
                          print_data.remote.mode_pulse_width_us,
                          print_data.remote.toggle_pulse_width_us);
        HAL_UART_Transmit(&huart3, (uint8_t*)str, str_len, 100);
    }

    if (event_bits & EVT_STEER_ADC_UPDATED_FOR_LOG) {
        str_len = snprintf(str, sizeof(str), "ADC: %d\r\n", print_data.steer_adc);
        HAL_UART_Transmit(&huart3, (uint8_t*)str, str_len, 100);
    }
    
    if (event_bits & EVT_VEHICLE_COMMAND_UPDATED_FOR_LOG) {
        str_len = snprintf(str, sizeof(str), "CMD: %f %f %f[deg] %lu\r\n",
                          print_data.vehicle_command.throttle,
                          print_data.vehicle_command.brake,
                          print_data.vehicle_command.steer_tire_degree,
                          print_data.vehicle_command.steer_adc);
        HAL_UART_Transmit(&huart3, (uint8_t*)str, str_len, 100);
    }

    if (event_bits & EVT_IMU_DATA_UPDATED_FOR_LOG) {
        str_len = snprintf(str, sizeof(str), "IMU: %f[deg] %f[deg] %f[deg]\r\n",
                          print_data.imu_data.roll_deg,
                          print_data.imu_data.pitch_deg,
                          print_data.imu_data.yaw_deg);
        HAL_UART_Transmit(&huart3, (uint8_t*)str, str_len, 100);
    }

    osDelay(100);
  }
  /* USER CODE END EntryPrintResult */
}

/* USER CODE BEGIN Header_EntryGetSteerADC */
/**
* @brief Function implementing the TaskGetSteerADC thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_EntryGetSteerADC */
void EntryGetSteerADC(void const * argument)
{
  /* USER CODE BEGIN EntryGetSteerADC */
  // HAL_ADC_Start_IT(&hadc1);

  uint32_t steer_adc_value;

  /* Infinite loop */
  for(;;) {
    steer_adc_value = GetSteerADCValue();   // This function waits for semaphore

    osMutexWait(vehicleDataMutexHandle, osWaitForever);
    vehicle_data_shm_.steer_adc = steer_adc_value;
    osMutexRelease(vehicleDataMutexHandle);


    if (eventGroupHandle != NULL) {
        xEventGroupSetBits(eventGroupHandle, EVT_STEER_ADC_UPDATED_FOR_LOG);
        xEventGroupSetBits(eventGroupHandle, EVT_STEER_ADC_UPDATED_FOR_CAN);
    }
    osDelay(10); // Prevent CPU hogging
  }
  /* USER CODE END EntryGetSteerADC */
}

/* USER CODE BEGIN Header_EntryVehicleControl */
/**
* @brief Function implementing the TaskVehicleCont thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_EntryVehicleControl */
void EntryVehicleControl(void const * argument)
{
  /* USER CODE BEGIN EntryVehicleControl */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);

  uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim1);      // arr is fixed value

  SharedMemory_t vehicle_data;

  /* Infinite loop */
  for(;;) {
    osMutexWait(vehicleDataMutexHandle, osWaitForever);
    vehicle_data = vehicle_data_shm_;
    osMutexRelease(vehicleDataMutexHandle);

    VehicleCommand_t command = PulseToVehicleCommand(vehicle_data.remote);
    
    if (command.mode == MANUAL_MODE) {
      if (command.throttle >= 0.5) {
        MoveForward(command.throttle, arr);
      } else if (command.brake >= 0.5) {
        MoveBackward(command.brake, arr);
      } else {
        StopRearWheels();
      }

      MoveSteer(command.steer_adc, vehicle_data.steer_adc, arr);
    }
    
    else if (command.mode == AUTO_MODE) {
        StopRearWheels();       // need to fix (or another task)
        StopSteer();
    }
    
    else if (command.mode == BRAKE_MODE || command.mode == ERROR_MODE) {
        StopRearWheels();
        StopSteer();
    }


    osMutexWait(vehicleDataMutexHandle, osWaitForever);
    vehicle_data_shm_.vehicle_command = command;
    osMutexRelease(vehicleDataMutexHandle);


    if (eventGroupHandle != NULL) {
        xEventGroupSetBits(eventGroupHandle, EVT_VEHICLE_COMMAND_UPDATED_FOR_LOG);
        xEventGroupSetBits(eventGroupHandle, EVT_VEHICLE_COMMAND_UPDATED_FOR_CAN);
    }
    osDelay(10);   // 100 Hz
  }
  /* USER CODE END EntryVehicleControl */
}

/* USER CODE BEGIN Header_EntryCANTransmit */
/**
* @brief Function implementing the TaskCANTransmit thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_EntryCANTransmit */
void EntryCANTransmit(void const * argument)
{
  /* USER CODE BEGIN EntryCANTransmit */
  uint32_t txmailbox;
  CAN_TxHeaderTypeDef txheader;
  txheader.IDE = CAN_ID_STD;
  txheader.RTR = CAN_RTR_DATA;
  txheader.DLC = 8;
  txheader.TransmitGlobalTime = DISABLE;

  HAL_CAN_Start(&hcan1);
  HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

  EventBits_t event_bits;
  SharedMemory_t vehicle_data;
  VehicleCANFrame_t vehicle_can_dataframe;

  /* Infinite loop */
  for(;;) {
    event_bits = xEventGroupWaitBits(
      eventGroupHandle,
      EVT_ALL_UPDATED_FOR_CAN, // uxBitsToWaitFor
      pdTRUE,          // xClearOnExit:    Clear bits on exit (Auto-Reset)
      pdFALSE,         // xWaitForAllBits: Wait for ANY bit (OR logic)
      osWaitForever    // xBlockTime:      Block until event happens
    );

    osMutexWait(vehicleDataMutexHandle, osWaitForever);
    vehicle_data = vehicle_data_shm_;
    osMutexRelease(vehicleDataMutexHandle);

    
    if (event_bits & EVT_REMOTE_UPDATED_FOR_CAN) {
      txheader.StdId = CANID_REMOTE_SIGNALS;
      SetRemoteSignalsCANFrame(&vehicle_can_dataframe, vehicle_data);
      if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) > 0) {
        HAL_CAN_AddTxMessage(&hcan1, &txheader, vehicle_can_dataframe.data, &txmailbox);
      } else {
        // fatal:transmit error!!
        // HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
      }
    }


    if (event_bits & EVT_STEER_ADC_UPDATED_FOR_CAN) {
      txheader.StdId = CANID_STEER_ADC;
      SetSteerADCCANFrame(&vehicle_can_dataframe, vehicle_data);
      if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) > 0) {
        HAL_CAN_AddTxMessage(&hcan1, &txheader, vehicle_can_dataframe.data, &txmailbox);
      } else {
        // fatal:transmit error!!
        // HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
      }
    }


    if (event_bits & EVT_VEHICLE_COMMAND_UPDATED_FOR_CAN) {                           
      txheader.StdId = CANID_VEHICLE_COMMAND1;
      SetVehicleCommand1CANFrame(&vehicle_can_dataframe, vehicle_data);
      if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) > 0) {
        HAL_CAN_AddTxMessage(&hcan1, &txheader, vehicle_can_dataframe.data, &txmailbox);
      } else {
        // fatal:transmit error!!
        // HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
      }

      txheader.StdId = CANID_VEHICLE_COMMAND2;
      SetVehicleCommand2CANFrame(&vehicle_can_dataframe, vehicle_data);
      if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) > 0) {
        HAL_CAN_AddTxMessage(&hcan1, &txheader, vehicle_can_dataframe.data, &txmailbox);
      } else {
        // fatal:transmit error!!
        // HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
      }
    }

    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    osDelay(10);
  }
  /* USER CODE END EntryCANTransmit */
}

/* USER CODE BEGIN Header_EntryGetIMU */
/**
* @brief Function implementing the TaskGetIMU thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_EntryGetIMU */
void EntryGetIMU(void const * argument)
{
  /* USER CODE BEGIN EntryGetIMU */
  uint8_t who_am_i = 0;
  if (HAL_I2C_Mem_Read(&hi2c1, MYAHRS_I2C_ADDR, REG_WHO_AM_I,
                        I2C_MEMADD_SIZE_8BIT, &who_am_i, 1, 100) != HAL_OK) {
    // int len = snprintf(uart_buf, sizeof(uart_buf), "myAHRS+ Found! ID: 0x%02X\r\n", who_am_i);
    // HAL_UART_Transmit(&huart3, (uint8_t*)uart_buf, len, 100);
  } else {
    // char* msg = "myAHRS+ NOT Found!\r\n";
    // HAL_UART_Transmit(&huart3, (uint8_t*)msg, strlen(msg), 100);
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
  }

  // initial read
  InitIMUData();
  IMUData_t imu_data;

  /* Infinite loop */
  for(;;) {
    imu_data = GetIMUData();  // This function waits for semaphore
    
    osMutexWait(vehicleDataMutexHandle, osWaitForever);
    vehicle_data_shm_.imu_data = imu_data;
    osMutexRelease(vehicleDataMutexHandle);

    if (eventGroupHandle != NULL) {
      xEventGroupSetBits(eventGroupHandle, EVT_IMU_DATA_UPDATED_FOR_LOG);
      xEventGroupSetBits(eventGroupHandle, EVT_IMU_DATA_UPDATED_FOR_CAN);
    }
  }
  /* USER CODE END EntryGetIMU */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
