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

// BUG: Normal processing time for trace() is 3.75us. However sometimes this jumps to ~40us. Needs investigation
// BUG:
// BUG:
// TODO: Optimize trace function
// TODO:
// TODO:



/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "spiflash.h"
#include "trace_spiflash.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
UART_HandleTypeDef hlpuart1;

SPI_HandleTypeDef hspi3;
DMA_HandleTypeDef hdma_spi3_rx;
DMA_HandleTypeDef hdma_spi3_tx;

/* USER CODE BEGIN PV */


// Demo traceer
int trace_trigger = 0;
// 1 block = 256 bytes
#define TRACEBUF_SZ_B (1024)
//#define TRACE_FILE_LEN_B (32768 * 8)
#define TRACE_FILE_LEN_B (1024)
uint8_t trace_buf[TRACEBUF_SZ_B+4];

uint32_t thing_a=0;
uint32_t thing_b=0;
trace_object_t traceobj = {
		.stat = 0,
		.buffer_start = trace_buf+4,
		.buffer_len_b = TRACEBUF_SZ_B,
		.trace_entry_len_b = 8,
		.trace_file_len_b = TRACE_FILE_LEN_B,
		.flash_len_b = 1024*1024*16,
		.num_tracevals = 2,
		.tracevals = {
		{&thing_a, 4},
		{&thing_b, 4}
		}
};





// parser
#define MAX_COMMMAND_SIZE (2048)
#define FLASH_BUF_SZ (4096)
#define MAX_PARAMS (512)
typedef enum {
	CMD_GET_ID = 0,
	CMD_GET_ID_BIN = 1,
    CMD_FLASH_READ = 2,
    CMD_FLASH_READ_BINARY = 3,
    CMD_FLASH_WRITE = 4,
    CMD_FLASH_ERASE_SECTOR = 5,
    CMD_FLASH_ERASE_CHIP = 6,
	CMD_TRIGGER_TRACE = 7,
    CMD_INVALID = -1
} command_index_t;
uint32_t params[MAX_PARAMS];



int parse_command(char *input, uint32_t *out_params, size_t max_params);

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_SPI3_Init(void);
/* USER CODE BEGIN PFP */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
int check_addr_len(uint32_t addr, uint32_t len);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */



void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13 );
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12 );
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_13)
    {
        // This gets called when PA0 has an edge event
        // Your interrupt logic goes here
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);  // e.g., toggle an LED
    }


}


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


  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);				// run commutator
  HAL_Delay(10);



  MX_DMA_Init();
  MX_LPUART1_UART_Init();
  MX_SPI3_Init();
  /* USER CODE BEGIN 2 */

  // enable GPIO interrupts
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  // enable
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */


  HAL_Delay(1000);

//  printf("\n\n\nstarting\n\n\n");


  printf("reading flash jedec ID\n");
  uint32_t jedec_id = flash_read_jedec_id(&hspi3);
  printf("Flash JEDEC ID: %02X %02X %02X\n", (jedec_id>>16)&0x000000FF, (jedec_id>>8)&0x000000FF, jedec_id&0x000000FF);


//  spi_test(&hspi3);
  char command_str[MAX_COMMMAND_SIZE];
  uint8_t flash_buffer[FLASH_BUF_SZ];


  printf("SystemCoreClock = %lu\n", SystemCoreClock);


  while (1)
  {

	  if (trace_trigger == 0)
	  {
		  fgets(command_str, sizeof(command_str), stdin);
		  int cmd = parse_command(command_str, params, MAX_PARAMS);
		  uint32_t i, id;
		  switch (cmd)
		  {
		  case CMD_GET_ID:
			  id = flash_read_jedec_id(&hspi3);
			  printf("Flash JEDEC ID: %02X %02X %02X\n", (id>>16)&0x000000FF, (id>>8)&0x000000FF, id&0x000000FF);
			  break;
		  case CMD_GET_ID_BIN:
			  id = flash_read_jedec_id(&hspi3);
			  //printf("%c%c%c\n", (id>>16)&0x000000FF, (id>>8)&0x000000FF, id&0x000000FF);

			  uint8_t id_bytes[3] = {
				  (id >> 16) & 0xFF,
				  (id >> 8)  & 0xFF,
				  id & 0xFF
			  };
			  HAL_UART_Transmit(&hlpuart1, id_bytes, 3, HAL_MAX_DELAY);

			  break;

		  case CMD_FLASH_READ:
			  printf("reading flash, address=%.8X, len=%.8X\n", params[0], params[1]);
			  if (check_addr_len (params[0], params[1]) )
			  {
				  flash_read_dma(params[0], flash_buffer , params[1], &hspi3);

				  for (i=0; i<params[1]-1; i++)
					  printf("%.2X,", flash_buffer[i]);
				  printf("%.2X\n", flash_buffer[i]);
			  }
			  else
			  {
				  printf("parameter error\n");
			  }
			  break;
		  case CMD_FLASH_READ_BINARY:
	//		  printf("reading flash, address=%.8X, len=%.8X\n", params[0], params[1]);
			  if (check_addr_len (params[0], params[1]) )
			  {
				  flash_read_dma(params[0], flash_buffer , params[1], &hspi3);

				  //for (i=0; i<params[1]; i++)
				//	  printf("%c", flash_buffer[i]);
				  HAL_UART_Transmit(&hlpuart1, flash_buffer, params[1], HAL_MAX_DELAY);

			  }
			  else
			  {
				  printf("parameter error\n");
			  }
			  break;
		  case CMD_FLASH_WRITE:
			  if (check_addr_len (params[0], params[1]) )
			  {
				  printf("Writing flash, address=%.8X, len=%.8X\n", params[0], params[1]);
				  uint8_t u8buf[256];
				  for (uint32_t i=0; i<SPIFLASH_PAGE_SIZE; ++i )  u8buf[i] = params[i+2];
				  //flash_page_program_dma_async(params[0], u8buf, params[1], &hspi3);
				  flash_page_program_dma(params[0], u8buf, params[1], &hspi3);
			  }
			  else
			  {
				  printf("parameter error\n");
			  }
			  break;
		  case CMD_FLASH_ERASE_SECTOR:
			  if (check_addr_len (params[0], 4096) )
			  {
				  printf("Erasing sector# %.8X\n", params[0]);
				  flash_erase_sector(params[0], &hspi3);
			  }
			  else
			  {
				  printf("parameter error\n");
			  }
			  break;

		  case CMD_FLASH_ERASE_CHIP:
			  printf("Erasing chip\n");
			  flash_erase_chip(&hspi3);
			  break;
		  case CMD_TRIGGER_TRACE:

				  // uncomment to create trace testfile
				  uint32_t trace_addr = trace_init(&traceobj, "trace_test#1-034", &hspi3);

				  trace_trigger = 1;
				  break;
		  default:
			  printf("Unknown command\n");
			  break;
		  }
	  }
	  else // trace_trigger
	  {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);				// run commutator
		thing_a = DWT->CYCCNT;
		uint32_t tt = DWT->CYCCNT;
		trace(&traceobj, &hspi3);
		thing_b = DWT->CYCCNT - tt;
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);				// run commutator
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);				// run commutator

		HAL_Delay(1);

	  }


	  //HAL_Delay(100);
	  //printf("\n\n\n");

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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 921600;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB13 PB14 PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART1 and Loop until the end of transmission */
  HAL_UART_Transmit(&hlpuart1, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}


int _read(int file, char *ptr, int len)
{
    for (int i = 0; i < len; i++) {
        HAL_UART_Receive(&hlpuart1, (uint8_t *)&ptr[i], 1, HAL_MAX_DELAY);

        // Optional: echo back the character
        //HAL_UART_Transmit(&hlpuart1, (uint8_t *)&ptr[i], 1, HAL_MAX_DELAY);

        if (ptr[i] == '\r') {
            ptr[i] = '\n';
            return i + 1;
        }
    }
    return len;
}

int check_addr_len(uint32_t addr, uint32_t len)
{
	if (addr > (1<<24) )
		return 0;
	if (len == 0)
		return 0;
	if (len > (1<<24) )
		return 0;
	return 1;
}
// Max number of parameters the parser will allow

/**
 * Parses a command string into command index and parameters.
 *
 * @param input The input string (e.g., "flash_read, 0x1000, 256")
 * @param out_params Array to fill with parsed uint32_t params
 * @param max_params Size of the out_params array
 * @return Command index (0–3), or CMD_INVALID (-1) if error
 */
char parse_buffer[MAX_COMMMAND_SIZE];
int parse_command(char *input, uint32_t *out_params, size_t max_params) {
    if (!input || !out_params || max_params == 0)
        return CMD_INVALID;

    // Make a modifiable copy
    strncpy(parse_buffer, input, sizeof(parse_buffer));
    parse_buffer[sizeof(parse_buffer) - 1] = '\0';

    // Tokenize
    char *tokens[MAX_PARAMS + 1];  // +1 for command name
    size_t count = 0;
    char *tok = strtok(parse_buffer, ",");
    while (tok && count < MAX_PARAMS + 1) {
        while (*tok == ' ') tok++; // Trim leading spaces
        tokens[count++] = tok;
        tok = strtok(NULL, ",");
    }

    if (count == 0)
        return CMD_INVALID;

    // Identify command
    int cmd_index = CMD_INVALID;
    if (strcmp(tokens[0], "id") == 0)
        cmd_index = CMD_GET_ID;
    else if (strcmp(tokens[0], "idb") == 0)
        cmd_index = CMD_GET_ID_BIN;
    else if (strcmp(tokens[0], "rd") == 0)
        cmd_index = CMD_FLASH_READ;
    else if (strcmp(tokens[0], "rdb") == 0)
        cmd_index = CMD_FLASH_READ_BINARY;
    else if (strcmp(tokens[0], "wr") == 0)
        cmd_index = CMD_FLASH_WRITE;
    else if (strcmp(tokens[0], "er") == 0)
        cmd_index = CMD_FLASH_ERASE_SECTOR;
    else if (strcmp(tokens[0], "erase_chip") == 0)
        cmd_index = CMD_FLASH_ERASE_CHIP;
    else if (strcmp(tokens[0], "tg") == 0)
        cmd_index = CMD_TRIGGER_TRACE;
    else
        return CMD_INVALID;

    // Parameter count check
    size_t expected_min_params = 0;
    switch (cmd_index) {
    	case CMD_GET_ID: expected_min_params = 0; break;
    	case CMD_GET_ID_BIN: expected_min_params = 0; break;
        case CMD_FLASH_READ: expected_min_params = 2; break;
        case CMD_FLASH_WRITE: expected_min_params = 3; break;  // addr, len, 1st byte
        case CMD_FLASH_ERASE_SECTOR: expected_min_params = 1; break;
        case CMD_FLASH_ERASE_CHIP: expected_min_params = 0; break;
    }

    size_t param_count = count - 1;
    if (param_count < expected_min_params || param_count > max_params)
        return CMD_INVALID;

    // Parse parameters as unsigned 32-bit integers
    for (size_t i = 0; i < param_count; ++i) {
        out_params[i] = (uint32_t)strtoul(tokens[i + 1], NULL, 0);
    }

    return cmd_index;
}





/* USER CODE END 4 */

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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
