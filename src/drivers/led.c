#include "stm32f1xx_ll_gpio.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fs/fs.h>

static struct led_device {
    struct inode *inode;
    struct dentry *dentry;
    GPIO_TypeDef *gpio;
    int gpio_pin;
} led0, led1;

static int led_open(struct file *filp)
{
    if (strcmp(filp->dentry->name, "led0") == 0) {
        filp->private_data = &led0;
    } else if (strcmp(filp->dentry->name, "led1") == 0) {
        filp->private_data = &led1;
    } else {
        assert(0);
    }

    return 0;
}

static int led_close(struct file *filp)
{
    return 0;
}

static int led_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int led_write(struct file *filp, const void *buf, uint32_t len)
{
    struct led_device *device = filp->private_data;

    if (atoi(buf) == 1) {
        LL_GPIO_ResetOutputPin(device->gpio, device->gpio_pin);
    } else if (atoi(buf) == 0) {
        LL_GPIO_SetOutputPin(device->gpio, device->gpio_pin);
    }
    return 1;
}

static int led_read(struct file *filp, void *buf, uint32_t len)
{
    struct led_device *device = filp->private_data;

    if (LL_GPIO_IsOutputPinSet(device->gpio, device->gpio_pin))
        ((char *)buf)[0] =  0x30;
    else
        ((char *)buf)[0] =  0x31;
    return 1;
}

static struct file_operations led_fops =  {
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

    // fs
    struct inode *inode = calloc(1, sizeof(*inode));
    inode->type = INODE_TYPE_CHAR;
    inode->f_ops = &led_fops;
    struct dentry *den = calloc(1, sizeof(*den));
    snprintf(den->name, sizeof(den->name), "%s", "led0");
    den->type = DENTRY_TYPE_FILE;
    den->inode = inode;
    den->parent = NULL;
    INIT_LIST_HEAD(&den->childs);
    INIT_LIST_HEAD(&den->child_node);
    dentry_add("/dev", den);

    // led0
    led0.inode = inode;
    led0.dentry = den;
    led0.gpio = GPIOC;
    led0.gpio_pin = LL_GPIO_PIN_13;
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

    // fs
    struct inode *inode = calloc(1, sizeof(*inode));
    inode->type = INODE_TYPE_CHAR;
    inode->f_ops = &led_fops;
    struct dentry *den = calloc(1, sizeof(*den));
    snprintf(den->name, sizeof(den->name), "%s", "led1");
    den->type = DENTRY_TYPE_FILE;
    den->inode = inode;
    den->parent = NULL;
    INIT_LIST_HEAD(&den->childs);
    INIT_LIST_HEAD(&den->child_node);
    dentry_add("/dev", den);

    // led1
    led1.inode = inode;
    led1.dentry = den;
    led1.gpio = GPIOA;
    led1.gpio_pin = LL_GPIO_PIN_1;
}

void led_init(void)
{
    led0_init();
    led1_init();
}
