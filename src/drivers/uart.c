#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <printk.h>
#include <ringbuf.h>
#include <fs/fs.h>

UART_HandleTypeDef huart1;
static UART_HandleTypeDef huart2;
static struct ringbuf *huart1_rxbuf;
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
    if (huart == &huart1)
        printk("huart1\n");
    else if (huart == &huart2)
        printk("huart2\n");

    if (huart == &huart1) {
        if (ringbuf_spare_right(huart1_rxbuf) > 0) {
            HAL_UARTEx_ReceiveToIdle_IT(
                &huart1, (uint8_t *)ringbuf_write_pos(huart1_rxbuf),
                ringbuf_spare_right(huart1_rxbuf));
        } else {
            HAL_UARTEx_ReceiveToIdle_IT(
                &huart1, (uint8_t *)ringbuf_write_pos(huart1_rxbuf),
                ringbuf_spare_left(huart1_rxbuf));
        }
    }

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
    if (huart == &huart1) {
        ringbuf_write_advance(huart1_rxbuf, Size);
        if (ringbuf_spare_right(huart1_rxbuf) > 0) {
            HAL_UARTEx_ReceiveToIdle_IT(
                &huart1, (uint8_t *)ringbuf_write_pos(huart1_rxbuf),
                ringbuf_spare_right(huart1_rxbuf));
        } else {
            HAL_UARTEx_ReceiveToIdle_IT(
                &huart1, (uint8_t *)ringbuf_write_pos(huart1_rxbuf),
                ringbuf_spare_left(huart1_rxbuf));
        }
    }

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
    HAL_UART_IRQHandler(&huart1);
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /*
         * USART1 GPIO Configuration
         * PA9     ------> USART1_TX
         * PA10    ------> USART1_RX
         */

        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    } else if (huart->Instance == USART2) {
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
    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_DISABLE();

        /*
         * USART1 GPIO Configuration
         *  PA9     ------> USART1_TX
         *  PA10    ------> USART1_RX
         */

        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);
        HAL_NVIC_DisableIRQ(USART1_IRQn);
    } else if (huart->Instance == USART2) {
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
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, HAL_MAX_DELAY);
        return len - huart1.TxXferCount;
    } else if (inode == uart2.inode) {
        HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, HAL_MAX_DELAY);
        return len - huart2.TxXferCount;
    }

    return -1;
}

static int uart_read(struct inode *inode, void *buf, uint32_t size)
{
    if (inode == uart1.inode) {
        HAL_NVIC_DisableIRQ(USART1_IRQn);
        int n = ringbuf_used(huart1_rxbuf);
        if (n > size) n = size;
        ringbuf_read(huart1_rxbuf, buf, size);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
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

void uart_init(void)
{
    // huart1 init
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
        panic("init huart1 failed");
    huart1_rxbuf = ringbuf_new(0);
    HAL_UARTEx_ReceiveToIdle_IT(
        &huart1, (uint8_t *)ringbuf_write_pos(huart1_rxbuf),
        ringbuf_spare(huart1_rxbuf));

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
