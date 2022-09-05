#include "stm32f1xx_hal.h"
#include <printk.h>

static RTC_HandleTypeDef hrtc;

void HAL_RTC_MspInit(RTC_HandleTypeDef* hrtc)
{
    if (hrtc->Instance == RTC) {
        HAL_PWR_EnableBkUpAccess();
        __HAL_RCC_BKP_CLK_ENABLE();
        __HAL_RCC_RTC_ENABLE();
    }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* hrtc)
{
  if (hrtc->Instance == RTC)
      __HAL_RCC_RTC_DISABLE();
}

void rtc_init(void)
{
    hrtc.Instance = RTC;
    hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
    hrtc.Init.OutPut = RTC_OUTPUTSOURCE_ALARM;
    if (HAL_RTC_Init(&hrtc) != HAL_OK)
        panic("init rtc failed");
}
