#include "stm32f1xx_ll_gpio.h"
#include "led.h"
#include <stdlib.h>
#include <stdio.h>
#include <fs/fs.h>

struct led_struct {
    struct inode *inode;
    GPIO_TypeDef *gpio;
    int gpio_pin;
};

struct led_struct led0;
struct led_struct led1;

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
            LL_GPIO_ResetOutputPin(led0.gpio, led0.gpio_pin);
        else if (inode == led1.inode)
            LL_GPIO_ResetOutputPin(led1.gpio, led1.gpio_pin);
    } else if (atoi(buf) == 0) {
        if (inode == led0.inode)
            LL_GPIO_SetOutputPin(led0.gpio, led0.gpio_pin);
        else if (inode == led1.inode)
            LL_GPIO_SetOutputPin(led1.gpio, led1.gpio_pin);
    }
    return -1;
}

static int led_read(struct inode *inode, void *buf, uint32_t len)
{
    if (inode == led0.inode) {
        if (LL_GPIO_IsOutputPinSet(led0.gpio, led0.gpio_pin))
            ((char *)buf)[0] =  0x30;
        else
            ((char *)buf)[0] =  0x31;
        return 1;
    } else if (inode == led1.inode) {
        if (LL_GPIO_IsOutputPinSet(led1.gpio, led1.gpio_pin))
            ((char *)buf)[0] =  0x30;
        else
            ((char *)buf)[0] =  0x31;
        return 1;
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

static void led0_init(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_13);
    GPIO_InitStruct.Pin = LL_GPIO_PIN_13;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    led0.gpio = GPIOC;
    led0.gpio_pin = LL_GPIO_PIN_13;
    led0.inode = calloc(1, sizeof(*led0.inode));
    led0.inode->type = INODE_TYPE_CHAR;
    led0.inode->ops = led_ops;
    INIT_LIST_HEAD(&led0.inode->node);

    // fs
    struct dentry *den0 = calloc(1, sizeof(*den0));
    snprintf(den0->name, sizeof(den0->name), "%s", "led0");
    den0->type = DENTRY_TYPE_FILE;
    den0->parent = NULL;
    INIT_LIST_HEAD(&den0->childs);
    INIT_LIST_HEAD(&den0->child_node);
    den0->inode = led0.inode;
    dentry_add("/dev", den0);
}

static void led1_init(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_1);
    GPIO_InitStruct.Pin = LL_GPIO_PIN_1;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    led1.gpio = GPIOA;
    led1.gpio_pin = LL_GPIO_PIN_1;
    led1.inode = calloc(1, sizeof(*led1.inode));
    led1.inode->type = INODE_TYPE_CHAR;
    led1.inode->ops = led_ops;
    INIT_LIST_HEAD(&led1.inode->node);

    // fs
    struct dentry *den1 = calloc(1, sizeof(*den1));
    snprintf(den1->name, sizeof(den1->name), "%s", "led1");
    den1->type = DENTRY_TYPE_FILE;
    den1->parent = NULL;
    INIT_LIST_HEAD(&den1->childs);
    INIT_LIST_HEAD(&den1->child_node);
    den1->inode = led1.inode;
    dentry_add("/dev", den1);
}

void led_init(void)
{
    led0_init();
    led1_init();
}
