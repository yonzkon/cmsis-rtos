#include <printk.h>
#include <syscall/syscall.h>
#include <fs/fs.h>
#include <drivers/gpio.h>
#include <drivers/led.h>
#include <drivers/spi.h>
#include <drivers/i2c.h>
#include <drivers/uart.h>
#include <net/net.h>

extern void board_init(void);
extern void init(void);

#define move_to_user_mode()      \
    __asm__("MRS r0, CONTROL;"); \
    __asm__("ORR r0, #1;");      \
    __asm__("MSR CONTROL, r0;");

int main(void)
{
    board_init();

    // sys
    sys_init();
    fs_init();

    // drivers
    gpio_init();
    led_init();
    spi_init();
    i2c_init();
    uart_init();

    // w5500
    net_init();

    printk("fenix init finished, start user init ...");

    move_to_user_mode();
    init();
}
