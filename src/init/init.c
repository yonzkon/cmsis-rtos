#include <unistd.h>
#include <malloc.h>
#include <log.h>

extern int shell_init();
extern int shell_fini();
extern void shell_loop();

extern int apistt_init();
extern int apistt_fini();
extern void apistt_loop();

extern int modbus_slave_init();
extern int modbus_slave_fini();
extern void modbus_slave_loop();

void init(void)
{
    log_set_level(LOG_LV_INFO);

    shell_init();
    apistt_init();
    //modbus_slave_init();
    LOG_INFO("system initial finished, start main loop ...");

    while (1) {
        apistt_loop();
        //modbus_slave_loop();

        shell_loop();
        usleep(100 * 1000);

        struct mallinfo info = mallinfo();
        LOG_DEBUG("arena: %d, ordblks: %d, uordblks: %d, fordblks: %d, keepcost: %d",
                  info.arena, info.ordblks, info.uordblks, info.fordblks, info.keepcost);
    }

    LOG_INFO("exit main loop ...");
    //modbus_slave_fini();
    apistt_fini();
    shell_fini();
}
