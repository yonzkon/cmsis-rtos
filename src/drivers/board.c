#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_rcc.h"
#include "stm32f1xx_ll_system.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_utils.h"
#include "stm32f1xx_ll_iwdg.h"
#include "stm32f1xx_ll_wwdg.h"
#include <stdint.h>
#include <sched.h>
#include <printk.h>

#ifndef NVIC_PRIORITYGROUP_0
#define NVIC_PRIORITYGROUP_0         ((uint32_t)0x00000007) /*!< 0 bit  for pre-emption priority,
                                                                 4 bits for subpriority */
#define NVIC_PRIORITYGROUP_1         ((uint32_t)0x00000006) /*!< 1 bit  for pre-emption priority,
                                                                 3 bits for subpriority */
#define NVIC_PRIORITYGROUP_2         ((uint32_t)0x00000005) /*!< 2 bits for pre-emption priority,
                                                                 2 bits for subpriority */
#define NVIC_PRIORITYGROUP_3         ((uint32_t)0x00000004) /*!< 3 bits for pre-emption priority,
                                                                 1 bit  for subpriority */
#define NVIC_PRIORITYGROUP_4         ((uint32_t)0x00000003) /*!< 4 bits for pre-emption priority,
                                                                 0 bit  for subpriority */
#endif

extern void save_current();
extern void switch_to(struct task_struct *task);

uint64_t sys_tick_ms;

/*
 * board init
 */

static void SystemClock_Config(void)
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
    while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_0)
    {
    }
    LL_RCC_HSI_SetCalibTrimming(16);
    LL_RCC_HSI_Enable();

    /* Wait till HSI is ready */
    while(LL_RCC_HSI_IsReady() != 1)
    {
    }
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI_DIV_2, LL_RCC_PLL_MUL_4);
    LL_RCC_PLL_Enable();

    /* Wait till PLL is ready */
    while(LL_RCC_PLL_IsReady() != 1)
    {
    }
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

    /* Wait till System clock is ready */
    while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
    {
    }
    LL_Init1msTick(16000000);
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
    LL_SetSystemCoreClock(16000000);
}

static void IWDG_Init(void)
{
    LL_IWDG_Enable(IWDG);
    LL_IWDG_EnableWriteAccess(IWDG);
    LL_IWDG_SetPrescaler(IWDG, LL_IWDG_PRESCALER_4);
    LL_IWDG_SetReloadCounter(IWDG, 4095);
    while (LL_IWDG_IsReady(IWDG) != 1)
    {
    }

    LL_IWDG_ReloadCounter(IWDG);
}

static void WWDG_Init(void)
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_WWDG);

    LL_WWDG_SetCounter(WWDG, 64);
    LL_WWDG_Enable(WWDG);
    LL_WWDG_SetPrescaler(WWDG, LL_WWDG_PRESCALER_1);
    LL_WWDG_SetWindow(WWDG, 64);
}

void board_init(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    /* System interrupt init*/
    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    /* SysTick_IRQn interrupt configuration */
    NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_SetPriority(SVCall_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),14, 0));
    NVIC_SetPriority(PendSV_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),15, 0));

    /*
     * DISABLE: JTAG-DP Disabled and SW-DP Disabled
     */
    //LL_GPIO_AF_DisableRemap_SWJ();

    /* Configure the system clock */
    SystemClock_Config();

    //IWDG_Init();
    //WWDG_Init();
}

/*
 * Processor Exceptions Handlers
 */

void NMI_Handler(void)
{
    panic("NMI_Handler");
}

void HardFault_Handler(void)
{
    panic("HardFault_Handler");
}

void MemManage_Handler(void)
{
    panic("MemManage_Handler");
}

void BusFault_Handler(void)
{
    panic("BusFault_Handler");
}

void UsageFault_Handler(void)
{
    panic("UsageFault_Handler");
}

void SVC_Handler(void)
{
    save_current();
    __asm__("bl at_syscall;");
    switch_to(current);
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
    save_current();
    schedule();
    switch_to(current);
}

void SysTick_Handler(void)
{
    sys_tick_ms++;
    if (sys_tick_ms % 0x80 == 0 && current != 0)
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}
