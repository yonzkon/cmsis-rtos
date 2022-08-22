#include "main.h"
#include <assert.h>
#include <string.h>
#include <log.h>
#include <atbuf.h>
#include <srrp.h>
#include <crc16.h>
#include <svcx.h>
#include <apix-service.h>

UART_HandleTypeDef huart1;
atbuf_t *rxbuf;

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

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
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

int _write(int fd, char *buf, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, HAL_MAX_DELAY);
    return len;
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // PC13
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // PA0
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    // PA1
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
        Error_Handler();
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET) {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        } else {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        }
    }
}

static void apinode_init()
{
    struct srrp_packet *pac = srrp_write_request(
        0x3333, APICORE_SERVICE_ADD, "{header:'/0012/echo'}");
    HAL_UART_Transmit(&huart1, (uint8_t *)pac->raw, pac->len, HAL_MAX_DELAY);
    srrp_free(pac);
}

static int on_echo(struct srrp_packet *req, struct srrp_packet **resp)
{
    uint16_t crc = crc16(req->header, req->header_len);
    crc = crc16_crc(crc, req->data, req->data_len);
    *resp = srrp_write_response(req->reqid, crc, req->header, "{msg:'world'}");
    return 0;
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    log_set_level(LOG_LV_DEBUG);

    rxbuf = atbuf_new(0);
    struct svchub *hub = svchub_new();
    svchub_add_service(hub, "/0012/echo", on_echo);
    apinode_init();
    LOG_INFO("system initial finished, start main loop ...");

    while (1) {
        int nread = 0;
        int spare = atbuf_spare(rxbuf);
        HAL_UART_Receive(&huart1, (uint8_t *)atbuf_write_pos(rxbuf), spare, 500);
        nread = spare - huart1.RxXferCount;
        if (nread == 0) continue;
        atbuf_write_advance(rxbuf, nread);

        struct srrp_packet *req = srrp_read_one_packet(atbuf_read_pos(rxbuf));
        if (req == NULL) {
            HAL_UART_Transmit(&huart1, (uint8_t *)atbuf_read_pos(rxbuf), nread, HAL_MAX_DELAY);
            atbuf_read_advance(rxbuf, nread);
            continue;
        }
        atbuf_read_advance(rxbuf, req->len);

        struct srrp_packet *resp = NULL;
        if (svchub_deal(hub, req, &resp) == 0) {
            assert(resp);
            HAL_UART_Transmit(&huart1, (uint8_t *)resp->raw, resp->len, HAL_MAX_DELAY);
            if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET) {
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
            } else {
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
            }
            srrp_free(resp);
        }
        srrp_free(req);
    }

    svchub_del_service(hub, "/0012/echo");
    svchub_destroy(hub);
    atbuf_delete(rxbuf);
    LOG_INFO("exit main loop ...");
}
