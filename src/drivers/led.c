#include "stm32f1xx_hal.h"
#include "led.h"
#include <stdlib.h>
#include <stdio.h>
#include <fs/fs.h>

#define LED0 0
#define LED1 1

struct led_struct {
    struct inode *inode;
    int state;
    GPIO_TypeDef *gpio;
    int gpio_pin;
    int on;
    int off;
};

struct led_struct led0;
struct led_struct led1;

static void led_on(struct led_struct *led)
{
    if (led->state == 0) {
        HAL_GPIO_WritePin(led->gpio, led->gpio_pin, led->on);
        led->state = 1;
    }
}

static void led_off(struct led_struct *led)
{
    if (led->state == 1) {
        HAL_GPIO_WritePin(led->gpio, led->gpio_pin, led->off);
        led->state = 0;
    }
}

static int led_open(struct inode *inode)
{
    return 0;
}

static int led_close(struct inode *inode)
{
    return 0;
}

static int led_ioctl(struct inode *inode, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int led_write(struct inode *inode, const void *buf, uint32_t len)
{
    if (atoi(buf) == 1) {
        if (inode == led0.inode)
            led_on(&led0);
        else if (inode == led1.inode)
            led_on(&led1);
    } else if (atoi(buf) == 0) {
        if (inode == led0.inode)
            led_off(&led0);
        else if (inode == led1.inode)
            led_off(&led1);
    }
    return -1;
}

static int led_read(struct inode *inode, void *buf, uint32_t size)
{
    if (inode == led0.inode) {
        ((char *)buf)[0] = led0.state + 0x30;
        return led0.state;
    } else if (inode == led1.inode) {
        ((char *)buf)[0] = led0.state + 0x30;
        return led1.state;
    }
    return -1;
}

static inode_ops_t led_ops =  {
    .open = led_open,
    .close = led_close,
    .ioctl = led_ioctl,
    .write = led_write,
    .read = led_read,
};

void led_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // led0
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    led0.gpio = GPIOC;
    led0.gpio_pin = GPIO_PIN_13;
    led0.on = GPIO_PIN_RESET;
    led0.off = GPIO_PIN_SET;
    led0.state = 0;
    led0.inode = calloc(1, sizeof(*led0.inode));
    led0.inode->type = INODE_TYPE_CHAR;
    led0.inode->ops = led_ops;
    INIT_LIST_HEAD(&led0.inode->node);
    struct dentry *den0 = calloc(1, sizeof(*den0));
    snprintf(den0->name, sizeof(den0->name), "%s", "led0");
    den0->type = DENTRY_TYPE_FILE;
    den0->parent = NULL;
    INIT_LIST_HEAD(&den0->childs);
    INIT_LIST_HEAD(&den0->child_node);
    den0->inode = led0.inode;
    dentry_add("/dev", den0);

    // led1
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    led1.gpio = GPIOA;
    led1.gpio_pin = GPIO_PIN_1;
    led1.on = GPIO_PIN_SET;
    led1.off = GPIO_PIN_RESET;
    led1.state = 0;
    led1.inode = calloc(1, sizeof(*led1.inode));
    led1.inode->type = INODE_TYPE_CHAR;
    led1.inode->ops = led_ops;
    INIT_LIST_HEAD(&led1.inode->node);
    struct dentry *den1 = calloc(1, sizeof(*den1));
    snprintf(den1->name, sizeof(den1->name), "%s", "led1");
    den1->type = DENTRY_TYPE_FILE;
    den1->parent = NULL;
    INIT_LIST_HEAD(&den1->childs);
    INIT_LIST_HEAD(&den1->child_node);
    den1->inode = led1.inode;
    dentry_add("/dev", den1);
}
