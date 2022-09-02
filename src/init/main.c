#include "stm32f1xx_hal.h"
#include <printk.h>
#include <fs/fs.h>
#include <drivers/gpio.h>
#include <drivers/led.h>
#include <drivers/uart.h>
#include <syscall/syscall.h>

extern void SystemClock_Config(void);
extern void init(void);

#define move_to_user_mode()      \
    __asm__("MRS r0, CONTROL;"); \
    __asm__("ORR r0, #1;");      \
    __asm__("MSR CONTROL, r0;");

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    fs_init();

    gpio_init();
    led_init();
    uart_init();
    sys_init();

    printk("fenix init finished, start user init ...");

    move_to_user_mode();
    init();
}
