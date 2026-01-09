/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Final Line-buffered UART Router with all features and fixes
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_BUFFER_SIZE 128
#define KEY_ENTER       '\r'
#define KEY_BACKSPACE   '\b'
#define KEY_DELETE      127
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
// --- 수신 데이터 처리를 위한 변수들 ---
// PC(USART2)로부터의 입력을 처리하기 위한 변수
uint8_t rx_data_pc;
uint8_t pc_rx_buffer[MAX_BUFFER_SIZE];
uint16_t pc_rx_index = 0;

// 다른 보드(USART3)로부터의 입력을 처리하기 위한 변수
uint8_t rx_data_board;
uint8_t board_rx_buffer[MAX_BUFFER_SIZE];
uint16_t board_rx_index = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
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
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */

  // PC 터미널(USART2)로 프롬프트 출력
  printf(">> ");
  // ★★★ 핵심 수정: 맨 처음 프롬프트가 바로 보이도록 버퍼를 비웁니다.
  fflush(stdout);

  // 각 UART 채널에 대해 수신 인터럽트 시작
  HAL_UART_Receive_IT(&huart2, &rx_data_pc, 1);
  HAL_UART_Receive_IT(&huart3, &rx_data_board, 1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* 모든 동작은 인터럽트로 처리 */
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);
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
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK){Error_Handler();}
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK){Error_Handler();}
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK){Error_Handler();}
}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK){Error_Handler();}
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
}

/* USER CODE BEGIN 4 */

/**
  * @brief  printf 함수 재정의
  */
PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

/**
  * @brief  UART 수신 완료 통합 콜백 함수
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    // --- 경로 1: PC 터미널(USART2)로부터 명령어를 '입력'받는 경우 ---
    if (huart->Instance == USART2)
    {
        // 1-1. 엔터 키 처리
        if (rx_data_pc == KEY_ENTER)
        {
            if (pc_rx_index > 0)
            {
                // 다른 보드(USART3)로 완성된 명령어(버퍼 내용) 전송
                HAL_UART_Transmit(&huart3, pc_rx_buffer, pc_rx_index, HAL_MAX_DELAY);
                // 명령어 끝을 알리기 위해 newline 전송
                HAL_UART_Transmit(&huart3, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
            }
            // 로컬 터미널 줄바꿈 및 버퍼 초기화
            printf("\r\n");
            memset(pc_rx_buffer, 0, MAX_BUFFER_SIZE);
            pc_rx_index = 0;
            // 프롬프트 재출력
            printf(">> ");
            fflush(stdout); // 프롬프트도 바로 보이게 하기 위해 추가
        }
        // 1-2. 백스페이스 키 처리
        else if (rx_data_pc == KEY_BACKSPACE || rx_data_pc == KEY_DELETE)
        {
            if (pc_rx_index > 0)
            {
                pc_rx_index--; // 버퍼에서 마지막 글자 제거
                printf("\b \b"); // 터미널에서 시각적으로 제거
                fflush(stdout); // 지우는 효과도 바로 보이게 하기 위해 추가
            }
        }
        // 1-3. 일반 문자 처리
        else
        {
            if (pc_rx_index < MAX_BUFFER_SIZE - 1)
            {
                printf("%c", rx_data_pc); // 로컬 에코
                fflush(stdout); // 한 글자씩 바로 터미널에 보이도록 버퍼 강제 비움
                pc_rx_buffer[pc_rx_index++] = rx_data_pc; // 버퍼에 저장
            }
        }
        // 다시 PC(USART2)로부터 1바이트 수신 대기
        HAL_UART_Receive_IT(&huart2, &rx_data_pc, 1);
    }

    // --- 경로 2: 다른 보드(USART3)로부터 명령어를 '수신'하는 경우 ---
    else if (huart->Instance == USART3)
    {
        // 2-1. 명령어의 끝(Enter)을 수신한 경우
        if (rx_data_board == '\r' || rx_data_board == '\n')
        {
            if (board_rx_index > 0)
            {
                // 수신된 명령어를 PC 터미널(USART2)에 출력
                printf("\r\n[Received: %s]\r\n", board_rx_buffer);

                // 버퍼 초기화 및 프롬프트 재출력
                memset(board_rx_buffer, 0, MAX_BUFFER_SIZE);
                board_rx_index = 0;
                printf(">> ");
                fflush(stdout);
            }
        }
        // 2-2. 명령어 내용을 한 글자씩 수신하여 버퍼에 저장하는 경우
        else
        {
            if (board_rx_index < MAX_BUFFER_SIZE - 1)
            {
                board_rx_buffer[board_rx_index++] = rx_data_board;
            }
        }
        // 다시 다른 보드(USART3)로부터 1바이트 수신 대기
        HAL_UART_Receive_IT(&huart3, &rx_data_board, 1);
    }
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
  * where the assert_param error has occurred.
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
