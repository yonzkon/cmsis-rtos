#include <string.h>
#include <modbus.h>
#include <log.h>

static modbus_t *ctx;
static modbus_mapping_t *map;

int modbus_slave_init()
{
    ctx = modbus_new_rtu("/dev/ttyS2", 115200, 'N', 8, 0);
    modbus_set_slave(ctx, 12);
    map = modbus_mapping_new(10, 10, 10, 10);
    modbus_connect(ctx);
    return 0;
}

int modbus_slave_fini()
{
    modbus_mapping_free(map);
    modbus_close(ctx);
    modbus_free(ctx);
    return 0;
}

void modbus_slave_loop()
{
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
    memset(query, 0, sizeof(query));
    int rc = modbus_receive(ctx, query);
    if (rc >= 0) {
        modbus_reply(ctx, query, rc, map);
        LOG_DEBUG("modbus reply");
    } else {
        LOG_DEBUG("modbus receive failed");
    }
}
