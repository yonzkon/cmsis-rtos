#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <log.h>
#include <modbus.h>

extern int apistt_init();
extern int apistt_fini();
extern void apistt_loop();

void init(void)
{
    log_set_level(LOG_LV_INFO);
    apistt_init();
    LOG_INFO("system initial finished, start main loop ...");

    modbus_t *ctx = modbus_new_rtu("uart2", 115200, 'N', 8, 0);
    modbus_set_slave(ctx, 12);
    modbus_mapping_t *map = modbus_mapping_new(100, 100, 100, 100);
    modbus_connect(ctx);

    while (1) {
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        memset(query, 0, sizeof(query));
        int rc = modbus_receive(ctx, query);
        if (rc >= 0) {
            modbus_reply(ctx, query, rc, map);
            LOG_INFO("modbus reply");
        } else {
            LOG_DEBUG("modbus receive failed");
        }

        apistt_loop();

        usleep(100 * 1000);
    }

    LOG_INFO("exit main loop ...");
    apistt_fini();
}
