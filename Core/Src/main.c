/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "fatfs.h"
#include "libjpeg.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "decode.h"
#include "stb_image_resize.h"
#include "../../Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_sdram.h"
#include "stm32f4xx_ll_fmc.h"
#include "stm32f4xx_hal_sdram.h"

//#include "ili9341.h"
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
CRC_HandleTypeDef hcrc;

DMA2D_HandleTypeDef hdma2d;

I2C_HandleTypeDef hi2c3;

LTDC_HandleTypeDef hltdc;

SPI_HandleTypeDef hspi5;

UART_HandleTypeDef huart1;

SDRAM_HandleTypeDef hsdram1;

osThreadId defaultTaskHandle;
/* USER CODE BEGIN PV */

enum USB_HOST_STATE {
	USB_HOST_STATE_DISCONNECTED = 0,
	USB_HOST_STATE_CONNECTED,
	USB_HOST_STATE_READY
};

enum APPLICATION_STATE {
	APPLICATION_STATE_NO_DRIVE,
	APPLICATION_STATE_CONNECT_DRIVE,
	APPLICATION_STATE_LOAD_FIRST_IMAGE,
	APPLICATION_STATE_DECODE_IMAGE,
	APPLICATION_STATE_BLEND_IMAGE_IN,
	APPLICATION_STATE_PUT_IMAGE_TO_BACKGROUND,
	APPLICATION_STATE_LOAD_NEXT_IMAGE
};

// Imports from fatfs.c
extern uint8_t retUSBH;    /* Return value for USBH */
extern char USBHPath[4];   /* USBH logical drive path */
extern FATFS USBHFatFS;    /* File system object for USBH logical drive */
extern FIL USBHFile;       /* File object for USBH */

enum USB_HOST_STATE usbHostState = USB_HOST_STATE_DISCONNECTED;
enum USB_HOST_STATE previousUSBHostState = USB_HOST_STATE_DISCONNECTED;

enum APPLICATION_STATE applicationState = APPLICATION_STATE_NO_DRIVE;
enum APPLICATION_STATE previousApplicationState = APPLICATION_STATE_NO_DRIVE;

//FATFS USBDISK_FatFs;  /* File system object for USB logical drive */
uint8_t USBDISK_FatFs_is_mounted = 0;

uint8_t rootDirOpened = 0;
DIR rootDir;
FILINFO fileInfo;

static uint32_t inAlpha = 0;

static uint32_t decode_buffer_1__width, decode_buffer_1_height = 0;

uint8_t _aucLine[2048];
static __IO uint8_t* targetRowPtr = 0;


// SDRAM
uint8_t LAYER0_BUFFER[LAYER_SIZE] __attribute__((section(".sdram")));
uint8_t LAYER1_BUFFER[LAYER_SIZE] __attribute__((section(".sdram")));
uint8_t DECODE_BUFFER_1[DECODE_BUFFER_SIZE] __attribute__((section(".sdram")));
uint8_t DECODE_BUFFER_2[DECODE_BUFFER_SIZE] __attribute__((section(".sdram")));

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_DMA2D_Init(void);
static void MX_FMC_Init(void);
static void MX_I2C3_Init(void);
static void MX_SPI5_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_LTDC_Init(void);
void StartDefaultTask(void const * argument);

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

  /* Initialization of ILI9341 component */
  ili9341_Init();
  ili9341_DisplayOff();


  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CRC_Init();
  MX_DMA2D_Init();
  MX_FMC_Init();
  MX_I2C3_Init();
  MX_SPI5_Init();
  MX_USART1_UART_Init();
  MX_FATFS_Init();
  MX_LIBJPEG_Init();
  MX_LTDC_Init();
  /* USER CODE BEGIN 2 */

  RetargetInit(&huart1);
  printf("I/O mapped to UART1\n\r");

  printf("Clearing the image buffers\n\r");
  for (uint32_t i = 0; i < DECODE_BUFFER_SIZE; i++) {
	  DECODE_BUFFER_1[i] = 0;
  }

  if (HAL_DMA2D_Start(&hdma2d, (uint32_t)DECODE_BUFFER_1, (uint32_t)LAYER0_BUFFER, LCD_WIDTH, LCD_HEIGHT) == HAL_OK)
  {
	  /* Polling For DMA transfer */
	  HAL_DMA2D_PollForTransfer(&hdma2d, 10);
  }
  if (HAL_DMA2D_Start(&hdma2d, (uint32_t)DECODE_BUFFER_1, (uint32_t)LAYER1_BUFFER, LCD_WIDTH, LCD_HEIGHT) == HAL_OK)
  {
	  /* Polling For DMA transfer */
	  HAL_DMA2D_PollForTransfer(&hdma2d, 10);
  }

  printf("Display ON\n\r");
  ili9341_DisplayOn();


  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 8192);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */

  printf("Starting the RTOS kernel\n\r");
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 6;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 96;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 4;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_8;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief DMA2D Initialization Function
  * @param None
  * @retval None
  */
static void MX_DMA2D_Init(void)
{

  /* USER CODE BEGIN DMA2D_Init 0 */

  /* USER CODE END DMA2D_Init 0 */

  /* USER CODE BEGIN DMA2D_Init 1 */

  /* USER CODE END DMA2D_Init 1 */
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M_PFC;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB888;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 255;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DMA2D_Init 2 */

  /* USER CODE END DMA2D_Init 2 */

}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.ClockSpeed = 100000;
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief LTDC Initialization Function
  * @param None
  * @retval None
  */
static void MX_LTDC_Init(void)
{

  /* USER CODE BEGIN LTDC_Init 0 */

  /* USER CODE END LTDC_Init 0 */

  LTDC_LayerCfgTypeDef pLayerCfg = {0};
  LTDC_LayerCfgTypeDef pLayerCfg1 = {0};

  /* USER CODE BEGIN LTDC_Init 1 */

  /* USER CODE END LTDC_Init 1 */
  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Init.HorizontalSync = 9;
  hltdc.Init.VerticalSync = 1;
  hltdc.Init.AccumulatedHBP = 29;
  hltdc.Init.AccumulatedVBP = 3;
  hltdc.Init.AccumulatedActiveW = 269;
  hltdc.Init.AccumulatedActiveH = 323;
  hltdc.Init.TotalWidth = 279;
  hltdc.Init.TotalHeigh = 327;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = LCD_WIDTH;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = LCD_HEIGHT;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  pLayerCfg.Alpha = 255;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg.FBStartAdress = 0;
  pLayerCfg.ImageWidth = LCD_WIDTH;
  pLayerCfg.ImageHeight = LCD_HEIGHT;
  pLayerCfg.Backcolor.Blue = 128;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg1.WindowX0 = 0;
  pLayerCfg1.WindowX1 = LCD_WIDTH;
  pLayerCfg1.WindowY0 = 0;
  pLayerCfg1.WindowY1 = LCD_HEIGHT;
  pLayerCfg1.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  pLayerCfg1.Alpha = 0;
  pLayerCfg1.Alpha0 = 0;
  pLayerCfg1.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg1.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg1.FBStartAdress = 0;
  pLayerCfg1.ImageWidth = LCD_WIDTH;
  pLayerCfg1.ImageHeight = LCD_HEIGHT;
  pLayerCfg1.Backcolor.Blue = 0;
  pLayerCfg1.Backcolor.Green = 0;
  pLayerCfg1.Backcolor.Red = 128;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg1, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LTDC_Init 2 */

  // Reconfiguring the addresses
  pLayerCfg.FBStartAdress = (uint32_t)LAYER0_BUFFER;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg1.FBStartAdress = (uint32_t)LAYER1_BUFFER;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg1, 1) != HAL_OK)
  {
    Error_Handler();
  }



  /* USER CODE END LTDC_Init 2 */

}

/**
  * @brief SPI5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI5_Init(void)
{

  /* USER CODE BEGIN SPI5_Init 0 */

  /* USER CODE END SPI5_Init 0 */

  /* USER CODE BEGIN SPI5_Init 1 */

  /* USER CODE END SPI5_Init 1 */
  /* SPI5 parameter configuration*/
  hspi5.Instance = SPI5;
  hspi5.Init.Mode = SPI_MODE_MASTER;
  hspi5.Init.Direction = SPI_DIRECTION_2LINES;
  hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi5.Init.NSS = SPI_NSS_SOFT;
  hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi5.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI5_Init 2 */

  /* USER CODE END SPI5_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/* FMC initialization function */
static void MX_FMC_Init(void)
{

  /* USER CODE BEGIN FMC_Init 0 */

  /* USER CODE END FMC_Init 0 */

  FMC_SDRAM_TimingTypeDef SdramTiming = {0};

  /* USER CODE BEGIN FMC_Init 1 */

  /* USER CODE END FMC_Init 1 */

  /** Perform the SDRAM1 memory initialization sequence
  */
  hsdram1.Instance = FMC_SDRAM_DEVICE;
  /* hsdram1.Init */
  hsdram1.Init.SDBank = FMC_SDRAM_BANK2;
  hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
  hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;
  hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;
  hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
  hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_DISABLE;
  hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_1;
  /* SdramTiming */
  SdramTiming.LoadToActiveDelay = 2;
  SdramTiming.ExitSelfRefreshDelay = 7;
  SdramTiming.SelfRefreshTime = 4;
  SdramTiming.RowCycleDelay = 7;
  SdramTiming.WriteRecoveryTime = 3;
  SdramTiming.RPDelay = 2;
  SdramTiming.RCDDelay = 2;

  if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK)
  {
    Error_Handler( );
  }

  /* USER CODE BEGIN FMC_Init 2 */

  /* Program the SDRAM external device */
  //BSP_SDRAM_Initialization_Sequence(&hsdram1, &command);

  /* USER CODE END FMC_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, NCS_MEMS_SPI_Pin|CSX_Pin|OTG_FS_PSO_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ACP_RST_GPIO_Port, ACP_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, RDX_Pin|WRX_DCX_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, LD3_Pin|LD4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : NCS_MEMS_SPI_Pin CSX_Pin OTG_FS_PSO_Pin */
  GPIO_InitStruct.Pin = NCS_MEMS_SPI_Pin|CSX_Pin|OTG_FS_PSO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : B1_Pin MEMS_INT1_Pin MEMS_INT2_Pin TP_INT1_Pin */
  GPIO_InitStruct.Pin = B1_Pin|MEMS_INT1_Pin|MEMS_INT2_Pin|TP_INT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ACP_RST_Pin */
  GPIO_InitStruct.Pin = ACP_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ACP_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OC_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BOOT1_Pin */
  GPIO_InitStruct.Pin = BOOT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TE_Pin */
  GPIO_InitStruct.Pin = TE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(TE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RDX_Pin WRX_DCX_Pin */
  GPIO_InitStruct.Pin = RDX_Pin|WRX_DCX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : LD3_Pin LD4_Pin */
  GPIO_InitStruct.Pin = LD3_Pin|LD4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

static void UpdateLTDCLayer2(int layer1_alpha) {
	LTDC_LayerCfgTypeDef pLayerCfg1 = {0};
	pLayerCfg1.WindowX0 = 0;
	pLayerCfg1.WindowX1 = LCD_WIDTH;
	pLayerCfg1.WindowY0 = 0;
	pLayerCfg1.WindowY1 = LCD_HEIGHT;
	pLayerCfg1.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
	pLayerCfg1.Alpha = layer1_alpha;
	pLayerCfg1.Alpha0 = 0;
	pLayerCfg1.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
	pLayerCfg1.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
	pLayerCfg1.FBStartAdress = (uint32_t)LAYER1_BUFFER;
	pLayerCfg1.ImageWidth = LCD_WIDTH;
	pLayerCfg1.ImageHeight = LCD_HEIGHT;
	pLayerCfg1.Backcolor.Blue = 0;
	pLayerCfg1.Backcolor.Green = 0;
	pLayerCfg1.Backcolor.Red = 0;
	if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg1, 1) != HAL_OK)
	{
		Error_Handler();
	}
}

static void CopyDecodedJpegToLayer(uint8_t* address) {
	if (HAL_DMA2D_Start(&hdma2d, (uint32_t)DECODE_BUFFER_2, (uint32_t)address, LCD_WIDTH, LCD_HEIGHT) == HAL_OK)
	{
		/* Polling For DMA transfer */
		HAL_DMA2D_PollForTransfer(&hdma2d, 10);
	}
}

static uint8_t jpeg_decode_callback(uint8_t* stride, uint32_t stride_length, uint32_t output_width, uint32_t output_height)
{

	decode_buffer_1__width = output_width;
	decode_buffer_1_height = output_height;
//#define ROTATE_ON_DECODE

#ifdef ROTATE_ON_DECODE

	if (targetRowPtr < DECODE_BUFFER_1 + 3 * output_width) {

		uint32_t h = stride_length / 3;// < output_height ? stride_length / 3 : output_height;
		// Rotating image by 90 degreees
		for (uint32_t i = 0; i < h; i++) {
			targetRowPtr[3 * i * output_width + 0] = stride[3 * i + 0];
			targetRowPtr[3 * i * output_width + 1] = stride[3 * i + 1];
			targetRowPtr[3 * i * output_width + 2] = stride[3 * i + 2];
		}
#else
		memcpy(targetRowPtr, stride, stride_length);
#endif


#ifdef SWAP_RB
	for (__IO uint8_t* ppix = targetRowPtr; ppix < targetRowPtr + DataLength; ppix += 3) {
		uint8_t r = ppix[0];
		uint8_t b = ppix[2];
		ppix[0] = b;
		ppix[2] = r;
	}
#endif

#ifdef ROTATE_ON_DECODE
	}
	targetRowPtr += 3;
#else
	targetRowPtr += stride_length;
#endif
	return 0;
}


void USBH_UserProcessHandler  (USBH_HandleTypeDef *phost, uint8_t id)
{
  /* USER CODE BEGIN CALL_BACK_1 */
  switch(id)
  {
  case HOST_USER_SELECT_CONFIGURATION:
  break;

  case HOST_USER_DISCONNECTION:
  usbHostState = USB_HOST_STATE_DISCONNECTED;
  break;

  case HOST_USER_CLASS_ACTIVE:
  usbHostState = USB_HOST_STATE_READY;
  break;

  case HOST_USER_CONNECTION:
  usbHostState = USB_HOST_STATE_CONNECTED;
  break;

  default:
  break;
  }
  /* USER CODE END CALL_BACK_1 */
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for USB_HOST */
  MX_USB_HOST_Init();
  /* USER CODE BEGIN 5 */
  	printf("Entering StartDefaultTask() loop\n\r");
  	for(;;)
  	{
  		// USB state machine

  		if (usbHostState != previousUSBHostState) {
			previousUSBHostState = usbHostState;

			if (usbHostState == USB_HOST_STATE_READY)
			{
				printf("On USB_HOST_STATE_READY\n\r");
				applicationState = APPLICATION_STATE_CONNECT_DRIVE;

			} else if (usbHostState == USB_HOST_STATE_DISCONNECTED) {
				printf("On USB_HOST_STATE_DISCONNECTED\n\r");
				applicationState = APPLICATION_STATE_NO_DRIVE;
			}
  		}

  		// Application state-machine

  		if (applicationState == previousApplicationState &&
  			applicationState != APPLICATION_STATE_BLEND_IMAGE_IN) {		// Blending state can repeat itself
 			// Wait till the state changes
 			osDelay(1);
  		} else {
  			// For clean logging
  			uint8_t alreadyBlending = (previousApplicationState == APPLICATION_STATE_BLEND_IMAGE_IN) && (applicationState == APPLICATION_STATE_BLEND_IMAGE_IN);

  			previousApplicationState = applicationState;

  			if (applicationState == APPLICATION_STATE_CONNECT_DRIVE) {
				printf("[*] APPLICATION_STATE_CONNECT_DRIVE\n\r");
  				if (!USBDISK_FatFs_is_mounted) {
					printf("Mounting the drive FS\n\r");
					if (f_mount(&USBHFatFS, (const TCHAR*)USBHPath, 0) == FR_OK)
					{
						printf("The FS is mounted successfully\n\r");
						USBDISK_FatFs_is_mounted = 1;
						applicationState = APPLICATION_STATE_LOAD_FIRST_IMAGE;
					} else {
						/* FatFs Initialization Error */
						printf("Error. Can't mount the FS\n\r");
					}
				} else {
					printf("Finding a file\n\r");
				}
  			} else if (applicationState == APPLICATION_STATE_LOAD_FIRST_IMAGE) {
				printf("[*] APPLICATION_STATE_LOAD_FIRST_IMAGE\n\r");
  				if (f_opendir(&rootDir, "0:") == FR_OK) {
					if (f_findfirst(&rootDir, &fileInfo, "", "*.jpg") == FR_OK) {
						rootDirOpened = 1;
					} else {
						printf("Can't find an image file\r\n");
						applicationState = APPLICATION_STATE_NO_DRIVE;
					}
				} else {
					printf("Can't open root folder\r\n");
					applicationState = APPLICATION_STATE_NO_DRIVE;
				}

				printf("Trying to read the file %s\r\n", fileInfo.fname);

  				/* Open the JPG image with read access */
				if (f_open(&USBHFile, fileInfo.fname, FA_READ) == FR_OK)
				{
					applicationState = APPLICATION_STATE_DECODE_IMAGE;
				} else {
					printf("Can't read the file\r\n");
					applicationState = APPLICATION_STATE_NO_DRIVE;
				}

  			} else if (applicationState == APPLICATION_STATE_DECODE_IMAGE) {
				printf("[*] APPLICATION_STATE_DECODE_IMAGE\n\r");
  				printf("Decoding the file %s\r\n", fileInfo.fname);

				/* Decode the jpg image file */
				targetRowPtr = (__IO uint8_t*)DECODE_BUFFER_1;

				jpeg_decode(&USBHFile, IMAGE_WIDTH, _aucLine, jpeg_decode_callback);

				/* Close the JPG image */
				f_close(&USBHFile);
				printf("Decoding finished. File %s closed\r\n", fileInfo.fname);

				// Copying from the decoded image buffer

				printf("Resizing the image to %dx%d\r\n", IMAGE_WIDTH, IMAGE_HEIGHT);
				stbir_resize_uint8(
						DECODE_BUFFER_1, decode_buffer_1__width, decode_buffer_1_height, 0,
						DECODE_BUFFER_2, IMAGE_WIDTH, IMAGE_HEIGHT, 0,
						3
				);

				printf("Copying the image into LAYER1\r\n");
				CopyDecodedJpegToLayer(LAYER1_BUFFER);

				inAlpha = 0;
				applicationState = APPLICATION_STATE_BLEND_IMAGE_IN;
  			} else if (applicationState == APPLICATION_STATE_BLEND_IMAGE_IN) {
				if (!alreadyBlending) printf("[*] APPLICATION_STATE_BLEND_IMAGE_IN\n\r");
  				if (inAlpha < 255) {
					inAlpha ++;
					osDelay(20);
  					//printf("inAlpha: %ld\r\n", inAlpha);
  				} else {
					applicationState = APPLICATION_STATE_PUT_IMAGE_TO_BACKGROUND;
  				}

  				UpdateLTDCLayer2(inAlpha);

  			} else if (applicationState == APPLICATION_STATE_PUT_IMAGE_TO_BACKGROUND) {
				printf("[*] APPLICATION_STATE_PUT_IMAGE_TO_BACKGROUND\n\r");

  				// Painting the current image on the background layer
  				CopyDecodedJpegToLayer(LAYER0_BUFFER);

  				// Hiding the foreground layer
  				UpdateLTDCLayer2(0);

  				applicationState = APPLICATION_STATE_LOAD_NEXT_IMAGE;

  			} else if (applicationState == APPLICATION_STATE_LOAD_NEXT_IMAGE) {
				printf("[*] APPLICATION_STATE_LOAD_NEXT_IMAGE\n\r");

				FRESULT findNextRes = f_findnext(&rootDir, &fileInfo);
				if (findNextRes == FR_OK) {
					if (strlen(fileInfo.fname) == 0) {
						printf("No more files found in the directory. Starting from the beginning\r\n");
						f_closedir(&rootDir);
						applicationState = APPLICATION_STATE_LOAD_FIRST_IMAGE;
					} else {
						printf("Opening the file \"%s\"\r\n", fileInfo.fname);

						/* Open the JPG image with read access */
						if (f_open(&USBHFile, fileInfo.fname, FA_READ) == FR_OK)
						{
							applicationState = APPLICATION_STATE_DECODE_IMAGE;
						}
						else {
							printf("Failed to find a file: %d\r\n", findNextRes);
							applicationState = APPLICATION_STATE_NO_DRIVE;
						}
					}
				} else {
					printf("Failed to find a file: %d\r\n", findNextRes);
					applicationState = APPLICATION_STATE_NO_DRIVE;
				}
  			} else if (applicationState == APPLICATION_STATE_NO_DRIVE) {
				printf("[*] APPLICATION_STATE_NO_DRIVE\n\r");

				// If the drive was mounted, unmounting it
  				if (USBDISK_FatFs_is_mounted) {
					printf("Unmounting the drive FS\n\r");
				    if (f_mount(0, "", 0) == FR_OK)
				    {
				    	printf("The FS is unmounted successfully\n\r");
						USBDISK_FatFs_is_mounted = 0;
				    } else
				    {
				    	/* FatFs Initialization Error */
						printf("Error. Can't unmount the FS\n\r");

						Error_Handler();
				    }
				}

  				// Waiting for a drive to connect
  				osDelay(10);
  			}

  		}

  	}
  /* USER CODE END 5 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  printf("Error_Handler()\n\r");
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
