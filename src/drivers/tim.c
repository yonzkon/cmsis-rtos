#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_tim.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fs/fs.h>

uint64_t tim1_tick_ms;

static struct tim_struct {
    struct inode *inode;
} tim1;

static int tim_open(struct inode *inode)
{
    return 0;
}

static int tim_close(struct inode *inode)
{
    return 0;
}

static int tim_ioctl(struct inode *inode, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int tim_write(struct inode *inode, const void *buf, uint32_t len)
{
    return 0;
}

static int tim_read(struct inode *inode, void *buf, uint32_t len)
{
    return snprintf(buf, len, "%llu", tim1_tick_ms);
}

static inode_ops_t tim_ops =  {
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
    tim1.inode = calloc(1, sizeof(*tim1.inode));
    tim1.inode->type = INODE_TYPE_CHAR;
    tim1.inode->ops = tim_ops;
    INIT_LIST_HEAD(&tim1.inode->node);
    struct dentry *den1 = calloc(1, sizeof(*den1));
    snprintf(den1->name, sizeof(den1->name), "%s", "tim1");
    den1->type = DENTRY_TYPE_FILE;
    den1->parent = NULL;
    INIT_LIST_HEAD(&den1->childs);
    INIT_LIST_HEAD(&den1->child_node);
    den1->inode = tim1.inode;
    dentry_add("/dev", den1);
}

void tim_init(void)
{
    TIM1_init();
}
