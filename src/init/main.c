#include <printk.h>
#include <syscall/syscall.h>
#include <fs/fs.h>
#include <drivers/gpio.h>
#include <drivers/led.h>
#include <drivers/uart.h>
#include <drivers/spi.h>

extern void board_init(void);
extern void init(void);

#define move_to_user_mode()      \
    __asm__("MRS r0, CONTROL;"); \
    __asm__("ORR r0, #1;");      \
    __asm__("MSR CONTROL, r0;");

int main(void)
{
    board_init();

    sys_init();
    fs_init();

    gpio_init();
    led_init();
    uart_init();
    spi_init();

    printk("fenix init finished, start user init ...");

    move_to_user_mode();
    init();
}
