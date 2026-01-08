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
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <ctype.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define WIFI_BAUD     9600
#define AP_SSID       "NUCLEO_AP"
#define AP_PW         "12345678"
#define TCP_PORT      5000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;

/* USER CODE BEGIN PV */
UART_HandleTypeDef huart1;

static volatile uint8_t led_state = 0;  // LD2
static uint8_t  btn_prev  = 1;         // PC13
static char     line_buf[128];

static int last_client_id = 0; // 마지막으로 접속한 클라이언트 ID 저장용
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// ----- UART1 @9600, PC4=TX, PC5=RX -----
static void MX_USART1_UART_Init(void)
{
  __HAL_RCC_USART1_CLK_ENABLE();
  huart1.Instance = USART1;
  huart1.Init.BaudRate = WIFI_BAUD;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) { Error_Handler(); }
}

static void uart_send(const char *s){ HAL_UART_Transmit(&huart1,(uint8_t*)s,(uint16_t)strlen(s),1000); }
static void uart_send_ln(const char *s){ uart_send(s); uart_send("\r\n"); }

// '\n'까지 라인 받기
static int uart_read_line(char *out, uint16_t maxlen, uint32_t timeout_ms)
{
  uint32_t t0=HAL_GetTick(); uint16_t i=0; uint8_t b;
  while((HAL_GetTick()-t0)<timeout_ms && i<(maxlen-1)){
    if(HAL_UART_Receive(&huart1,&b,1,10)==HAL_OK){ out[i++]=(char)b; if(b=='\n') break; }
  }
  out[i]=0; return (i>0);
}

// 보드레이트 변경(런타임)
static void uart_set_baud(uint32_t baud)
{
  HAL_UART_DeInit(&huart1);
  huart1.Init.BaudRate = baud;
  HAL_UART_Init(&huart1);
}

// AT 기대 토큰 대기 (OK/no change/READY도 통과)
static int at_expect(const char *cmd, const char *expect, uint32_t timeout_ms)
{
	if(cmd&&*cmd) uart_send_ln(cmd);
	char buf[128]; uint32_t t0=HAL_GetTick();
	while((HAL_GetTick()-t0)<timeout_ms){
	  if(uart_read_line(buf,sizeof(buf),200)){
	    // 응답 로그 확인용 출력 추가해도 좋음
	    // uart_send("[R] "); uart_send_ln(buf);

	    if (expect && *expect && strstr(buf, expect)) return 1;
	    if (strstr(buf,"OK") || strstr(buf,"no change") || strstr(buf,"READY") || strstr(buf,"ready")) return 1;
	    if (strstr(buf,"ERROR")) return 0;

	    // 한 번 더 반복문 돌며 다른 라인 확인
	  }
	}
	return 0;
}

// ESP 보드레이트 자동 동기화: 9600 시도→실패면 115200에서 9600으로 강제
static int esp_sync_force_9600(void)
{
  // 1) 9600에서 AT 시도
  for(int i=0;i<3;i++){ uart_send_ln("AT"); if(at_expect(NULL,"OK",200)) return 1; HAL_Delay(100); }

  // 2) 115200로 전환해 AT 잡고 9600으로 내리기
  uart_set_baud(115200);
  for(int i=0;i<8;i++){
    uart_send_ln("AT");
    if(at_expect(NULL,"OK",250)){
      uart_send_ln("AT+UART_DEF=9600,8,1,0,0");
      at_expect(NULL,"OK",800);
      uart_send_ln("AT+RST");
      HAL_Delay(700);
      uart_set_baud(9600);
      for(int j=0;j<6;j++){ uart_send_ln("AT"); if(at_expect(NULL,"OK",250)) return 1; HAL_Delay(100); }
      break;
    }
    HAL_Delay(120);
  }
  return 0;
}

static void LED_Set(uint8_t on){
  led_state = on?1:0;
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, on?GPIO_PIN_SET:GPIO_PIN_RESET);
}

// +IPD,ID,LEN:PAYLOAD → payload 한 줄 추출 (서버 측)
static int esp_read_payload_line(char *out, uint16_t maxlen, uint32_t timeout_ms)
{
  char buf[128];
  if(!uart_read_line(buf,sizeof(buf),timeout_ms)) return 0;

  // 이벤트: "0,CONNECT"/"0,CLOSED" 같은 건 무시
  if (strchr(buf,',') && !strstr(buf,"+IPD")) return 0;

  char *p = strstr(buf, "+IPD,"); if(!p) return 0;
  char *colon = strchr(p, ':');   if(!colon) return 0;

  // ID 추출해서 전역변수에 저장
  sscanf(p, "+IPD,%d,", &last_client_id);  // ★ 중요!

  // payload 복사
  strncpy(out, colon+1, maxlen-1); out[maxlen-1]=0;
  printf("📡 ID %d sent: %s\n", last_client_id, out);
  return 1;
}

// 최근 접속자로 보내기(멀티연결)
static void esp_send_to_last(const char *payload)
{
  int len = (int)strlen(payload);
  char cmd[32];

  // 저장된 id에 보내기
  snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d,%d", last_client_id, len);
  uart_send_ln(cmd);

  // '>' 프롬프트 대기
  char b[64]; uint32_t t0 = HAL_GetTick(); int ok = 0;
  while((HAL_GetTick() - t0) < 1200){
    if(uart_read_line(b, sizeof(b), 200)){
      if(strchr(b, '>')){ ok = 1; break; }
      if(strstr(b, "ERROR")) break;
    }
  }
  if(!ok) return;

  HAL_UART_Transmit(&huart1, (uint8_t*)payload, (uint16_t)len, 1000);
  at_expect(NULL, "SEND OK", 800);
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
  /* USER CODE BEGIN 2 */
  MX_USART1_UART_Init();   // UART1 @9600 시작
  LED_Set(0);

  // 0) 보드레이트 동기화 (가장 중요)
  if (!esp_sync_force_9600()){
    // 실패 표시(빠르게 10번 점멸)
    for(int i=0;i<10;i++){ LED_Set(i&1); HAL_Delay(120); }
  }

  // 1) AP 모드
  at_expect("AT", "OK", 2000);
  at_expect("AT+CWMODE=2", "OK", 2000);

  // 2) AP 개설 (펌웨어에 따라 _CUR로 다시 시도)
  { char cmd[96];
    snprintf(cmd,sizeof(cmd),"AT+CWSAP=\"%s\",\"%s\",5,3", AP_SSID, AP_PW);
    if(!at_expect(cmd,"OK",4000)){
      snprintf(cmd,sizeof(cmd),"AT+CWSAP_CUR=\"%s\",\"%s\",5,3", AP_SSID, AP_PW);
      at_expect(cmd,"OK",4000);
    }
  }

  // 3) TCP 서버 오픈
  at_expect("AT+CIPMUX=1", "OK", 2000);
  at_expect("AT+CIPSERVER=0", "OK", 2000);
  { char cmd[48]; snprintf(cmd,sizeof(cmd),"AT+CIPSERVER=1,%d", TCP_PORT); at_expect(cmd,"OK",3000); }
  at_expect("AT+CIPSTO=300", "OK", 2000);

  // 4) AP IP 확인 (로그용)
  uart_send_ln("AT+CIFSR");  // 여기서 +CIFSR:APIP,"192.168.4.1" 등이 옴

  // 성공 표시: 두 번 점멸
  for(int i=0;i<2;i++){ LED_Set(1); HAL_Delay(200); LED_Set(0); HAL_Delay(200); }
  /* USER CODE END 2 */

  /* Initialize led */
  BSP_LED_Init(LED_GREEN);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    /* USER CODE END WHILE */
	  // 서버에서 보낸 "LED ON"/"LED OFF" 처리
	   if (esp_read_payload_line(line_buf, sizeof(line_buf), 50)) {
	     if (strstr(line_buf, "LED ON"))  LED_Set(1);
	     if (strstr(line_buf, "LED OFF")) LED_Set(0);
	   }

	   // 버튼 에지 → LED 토글 + 최근 연결(id=0 가정)로 통지
	   uint8_t now = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
	   if (btn_prev != now){ HAL_Delay(30); now = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13); }
	   if (btn_prev==GPIO_PIN_SET && now==GPIO_PIN_RESET){
	     LED_Set(!led_state);
	     esp_send_to_last( led_state ? "LED ON\n" : "LED OFF\n" );
	   }
	   btn_prev = now;
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
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // LD2 (PA5)
  GPIO_InitStruct.Pin   = GPIO_PIN_5;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // 버튼 (PC13)
  GPIO_InitStruct.Pin  = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  // USART1 PC4(TX)/PC5(RX) AF7
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
