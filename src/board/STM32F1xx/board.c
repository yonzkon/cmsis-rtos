#include <stm32f1xx.h>
#include <assert.h>
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

void SystemClock_Config(void)
{
    // init flash
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= FLASH_ACR_LATENCY_0;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_0);

    // use HSE as clock source
    RCC->CR |= RCC_CR_HSEON;
    while ((RCC->CR & RCC_CR_HSERDY) != RCC_CR_HSERDY);

    // config HSE / 2 * 6 as PLL source
    RCC->CFGR |= RCC_CFGR_PLLSRC;
    RCC->CFGR |= RCC_CFGR_PLLXTPRE;
    RCC->CFGR &= ~RCC_CFGR_PLLMULL;
    RCC->CFGR |= RCC_CFGR_PLLMULL6;

    // enable PLL
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) != RCC_CR_PLLRDY);

    // config system clock prescaler
    RCC->CFGR &= ~RCC_CFGR_HPRE;
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
    RCC->CFGR &= ~RCC_CFGR_PPRE1;
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV1;
    RCC->CFGR &= ~RCC_CFGR_PPRE2;
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;

    // switch system clock source
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    // now we can disable HSI
    RCC->CR &= ~RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == RCC_CR_HSIRDY);

    // the system clock config to HSE / 2 * 6
    uint32_t HCLK = HSE_VALUE / 2 * 6;
    SysTick->LOAD = (uint32_t)((HCLK / 1000) - 1UL);
    SysTick->VAL = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

    // this function provided by ST will calc SystemCoreClock by itself,
    // and it's a good way to testify our config
    SystemCoreClockUpdate();
    assert(SystemCoreClock == HCLK);
}

void board_init(void)
{
    // enable AFIO & PWR clock
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN_Msk;
    RCC->APB1ENR |= RCC_APB1ENR_PWREN_Msk;

    // config NVIC
    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_SetPriority(SVCall_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),14, 0));
    NVIC_SetPriority(PendSV_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),15, 0));

    // config system clock
    SystemClock_Config();
}

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
    check_psp();
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
