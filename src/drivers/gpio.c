#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_exti.h"

void EXTI0_IRQHandler(void)
{
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_0);
    LL_GPIO_TogglePin(GPIOC, LL_GPIO_PIN_13);
    LL_GPIO_TogglePin(GPIOA, LL_GPIO_PIN_1);
}

void gpio_init(void)
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOC);

    LL_GPIO_AF_SetEXTISource(LL_GPIO_AF_EXTI_PORTA, LL_GPIO_AF_EXTI_LINE0);

    LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
    EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_0;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_0, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_0, LL_GPIO_MODE_INPUT);

    NVIC_SetPriority(EXTI0_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(EXTI0_IRQn);
}
