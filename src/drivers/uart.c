#include "stm32f1xx_hal.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_usart.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <printk.h>
#include <ringbuf.h>
#include <fs/fs.h>

static UART_HandleTypeDef huart2;
static struct ringbuf *usart1_rxbuf;
static struct ringbuf *huart2_rxbuf;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    printk("cplt\n");
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    printk("halfcplt\n");
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart2)
        printk("huart2\n");

    if (huart == &huart2) {
        if (ringbuf_spare_right(huart2_rxbuf) > 0) {
            HAL_UARTEx_ReceiveToIdle_IT(
                &huart2, (uint8_t *)ringbuf_write_pos(huart2_rxbuf),
                ringbuf_spare_right(huart2_rxbuf));
        } else {
            HAL_UARTEx_ReceiveToIdle_IT(
                &huart2, (uint8_t *)ringbuf_write_pos(huart2_rxbuf),
                ringbuf_spare_left(huart2_rxbuf));
        }
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart == &huart2) {
        ringbuf_write_advance(huart2_rxbuf, Size);
        if (ringbuf_spare_right(huart2_rxbuf) > 0) {
            HAL_UARTEx_ReceiveToIdle_IT(
                &huart2, (uint8_t *)ringbuf_write_pos(huart2_rxbuf),
                ringbuf_spare_right(huart2_rxbuf));
        } else {
            HAL_UARTEx_ReceiveToIdle_IT(
                &huart2, (uint8_t *)ringbuf_write_pos(huart2_rxbuf),
                ringbuf_spare_left(huart2_rxbuf));
        }
    }
}

void USART1_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_RXNE(USART1))
        ringbuf_write_byte(usart1_rxbuf, USART1->DR & USART_DR_DR);
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
    if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /*
         * USART2 GPIO Configuration
         * PA2     ------> USART2_TX
         * PA3     ------> USART2_RX
         */

        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
    if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_DISABLE();

        /*
         * USART2 GPIO Configuration
         *  PA2     ------> USART2_TX
         *  PA3     ------> USART2_RX
         */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);
        HAL_NVIC_DisableIRQ(USART2_IRQn);
    }
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
        for (int i = 0; i < len; i++) {
            while (!LL_USART_IsActiveFlag_TXE(USART1));
            LL_USART_TransmitData8(USART1, ((uint8_t *)buf)[i]);
        }
        return len;
    } else if (inode == uart2.inode) {
        HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, HAL_MAX_DELAY);
        return len - huart2.TxXferCount;
    }

    return -1;
}

static int uart_read(struct inode *inode, void *buf, uint32_t size)
{
    if (inode == uart1.inode) {
        NVIC_DisableIRQ(USART1_IRQn);
        int n = ringbuf_used(usart1_rxbuf);
        if (n > size) n = size;
        ringbuf_read(usart1_rxbuf, buf, size);
        NVIC_EnableIRQ(USART1_IRQn);
        return n;
    } else if (inode == uart2.inode) {
        HAL_NVIC_DisableIRQ(USART2_IRQn);
        int n = ringbuf_used(huart2_rxbuf);
        if (n > size) n = size;
        ringbuf_read(huart2_rxbuf, buf, size);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
        return n;
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
    usart1_rxbuf = ringbuf_new(0);
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

void uart_init(void)
{
    USART1_init();

    // huart2 init
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
        panic("init huart2 failed");
    huart2_rxbuf = ringbuf_new(0);
    HAL_UARTEx_ReceiveToIdle_IT(
        &huart2, (uint8_t *)ringbuf_write_pos(huart2_rxbuf),
        ringbuf_spare(huart2_rxbuf));

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
