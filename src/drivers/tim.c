#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_tim.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fs/fs.h>

uint64_t tim1_tick_ms;

static struct tim_device {
    struct inode *inode;
    struct dentry *dentry;
} tim1_dev;

static int tim_open(struct file *filp)
{
    return 0;
}

static int tim_close(struct file *filp)
{
    return 0;
}

static int tim_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int tim_write(struct file *filp, const void *buf, uint32_t len)
{
    return 0;
}

static int tim_read(struct file *filp, void *buf, uint32_t len)
{
    return snprintf(buf, len, "%llu", tim1_tick_ms);
}

static struct file_operations tim_fops =  {
    .open = tim_open,
    .close = tim_close,
    .ioctl = tim_ioctl,
    .write = tim_write,
    .read = tim_read,
};

void TIM1_UP_IRQHandler(void)
{
    TIM1->SR &= ~TIM_SR_UIF;
    tim1_tick_ms += 100;
}

static void TIM1_init(void)
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);

    // update 100ms once
    LL_TIM_InitTypeDef TIM_InitStruct = {0};
    TIM_InitStruct.Prescaler = 15999;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = 99;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    TIM_InitStruct.RepetitionCounter = 0;
    LL_TIM_Init(TIM1, &TIM_InitStruct);
    LL_TIM_EnableARRPreload(TIM1);
    LL_TIM_SetClockSource(TIM1, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetTriggerOutput(TIM1, LL_TIM_TRGO_RESET);
    LL_TIM_DisableMasterSlaveMode(TIM1);
    TIM1->CR1 |= TIM_CR1_CEN;
    TIM1->DIER |= TIM_DIER_UIE;

    NVIC_SetPriority(TIM1_UP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(TIM1_UP_IRQn);

    // fs
    struct inode *inode = calloc(1, sizeof(*inode));
    inode->type = INODE_TYPE_CHAR;
    inode->f_ops = &tim_fops;
    struct dentry *den = calloc(1, sizeof(*den));
    snprintf(den->name, sizeof(den->name), "%s", "tim1");
    den->type = DENTRY_TYPE_FILE;
    den->inode = inode;
    den->parent = NULL;
    INIT_LIST_HEAD(&den->childs);
    INIT_LIST_HEAD(&den->child_node);
    dentry_add("/dev", den);

    // tim1_device
    tim1_dev.inode = inode;
    tim1_dev.dentry = den;
}

void tim_init(void)
{
    TIM1_init();
}
