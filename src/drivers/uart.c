#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_usart.h"
#include "uart.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <printk.h>
#include <ringbuf.h>
#include <fs/fs.h>

static struct ringbuf *usart1_rxbuf;
static struct ringbuf *usart2_rxbuf;

void USART1_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_RXNE(USART1))
        ringbuf_write_byte(usart1_rxbuf, USART1->DR & USART_DR_DR);
}

void USART2_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_RXNE(USART2))
        ringbuf_write_byte(usart2_rxbuf, USART2->DR & USART_DR_DR);
}

struct uart_struct {
    struct inode *inode;
};

static struct uart_struct uart1;
static struct uart_struct uart2;

static int uart_open(struct inode *inode)
{
    return 0;
}

static int uart_close(struct inode *inode)
{
    return 0;
}

static int uart_ioctl(struct inode *inode, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int uart_write(struct inode *inode, const void *buf, uint32_t len)
{
    if (inode == uart1.inode) {
        return UART1_write(buf, len);
    } else if (inode == uart2.inode) {
        return UART2_write(buf, len);
    }

    return -1;
}

static int uart_read(struct inode *inode, void *buf, uint32_t len)
{
    if (inode == uart1.inode) {
        return UART1_read(buf, len);
    } else if (inode == uart2.inode) {
        return UART2_read(buf, len);
        NVIC_DisableIRQ(USART2_IRQn);
    }

    return -1;
}

static inode_ops_t uart_ops =  {
    .open = uart_open,
    .close = uart_close,
    .ioctl = uart_ioctl,
    .write = uart_write,
    .read = uart_read,
};

static void USART1_init(void)
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);

    /*
     * USART1 GPIO Configuration
     * PA9     ------> USART1_TX
     * PA10    ------> USART1_RX
     */

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_FLOATING;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(USART1_IRQn);

    LL_USART_InitTypeDef USART_InitStruct = {0};
    USART_InitStruct.BaudRate = 115200;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART1, &USART_InitStruct);
    LL_USART_ConfigAsyncMode(USART1);
    LL_USART_Enable(USART1);

    // buffer init
    usart1_rxbuf = ringbuf_new(1024);
    LL_USART_EnableIT_RXNE(USART1);

    // fs init
    uart1.inode = calloc(1, sizeof(*uart1.inode));
    uart1.inode->type = INODE_TYPE_CHAR;
    uart1.inode->ops = uart_ops;
    INIT_LIST_HEAD(&uart1.inode->node);
    struct dentry *den1 = calloc(1, sizeof(*den1));
    snprintf(den1->name, sizeof(den1->name), "%s", "ttyS1");
    den1->type = DENTRY_TYPE_FILE;
    den1->parent = NULL;
    INIT_LIST_HEAD(&den1->childs);
    INIT_LIST_HEAD(&den1->child_node);
    den1->inode = uart1.inode;
    dentry_add("/dev", den1);
}

static void USART2_init(void)
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);

    /*
     * USART2 GPIO Configuration
     * PA2     ------> USART2_TX
     * PA3     ------> USART2_RX
     */

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_3;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_FLOATING;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    NVIC_SetPriority(USART2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(USART2_IRQn);

    LL_USART_InitTypeDef USART_InitStruct = {0};
    USART_InitStruct.BaudRate = 115200;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART2, &USART_InitStruct);
    LL_USART_ConfigAsyncMode(USART2);
    LL_USART_Enable(USART2);

    // buffer init
    usart2_rxbuf = ringbuf_new(1024);
    LL_USART_EnableIT_RXNE(USART2);

    // fs init
    uart2.inode = calloc(1, sizeof(*uart2.inode));
    uart2.inode->type = INODE_TYPE_CHAR;
    uart2.inode->ops = uart_ops;
    INIT_LIST_HEAD(&uart2.inode->node);
    struct dentry *den2 = calloc(1, sizeof(*den2));
    snprintf(den2->name, sizeof(den2->name), "%s", "ttyS2");
    den2->type = DENTRY_TYPE_FILE;
    den2->parent = NULL;
    INIT_LIST_HEAD(&den2->childs);
    INIT_LIST_HEAD(&den2->child_node);
    den2->inode = uart2.inode;
    dentry_add("/dev", den2);
}

void uart_init(void)
{
    USART1_init();
    USART2_init();
}

void UART1_write_byte(uint8_t byte)
{
    while (!LL_USART_IsActiveFlag_TXE(USART1));
    LL_USART_TransmitData8(USART1, byte);
}

int UART1_write(const void *buf, size_t len)
{
    for (int i = 0; i < len; i++)
        UART1_write_byte(((uint8_t *)buf)[i]);
    return len;
}

int UART1_read(void *buf, size_t len)
{
    NVIC_DisableIRQ(USART1_IRQn);
    int n = ringbuf_used(usart1_rxbuf);
    if (n > len) n = len;
    ringbuf_read(usart1_rxbuf, buf, len);
    NVIC_EnableIRQ(USART1_IRQn);
    return n;
}

void UART2_write_byte(uint8_t byte)
{
    while (!LL_USART_IsActiveFlag_TXE(USART1));
    LL_USART_TransmitData8(USART1, byte);
}

int UART2_write(const void *buf, size_t len)
{
    for (int i = 0; i < len; i++)
        UART2_write_byte(((uint8_t *)buf)[i]);
    return len;
}

int UART2_read(void *buf, size_t len)
{
    NVIC_DisableIRQ(USART2_IRQn);
    int n = ringbuf_used(usart2_rxbuf);
    if (n > len) n = len;
    ringbuf_read(usart2_rxbuf, buf, len);
    NVIC_EnableIRQ(USART2_IRQn);
    return n;
}
