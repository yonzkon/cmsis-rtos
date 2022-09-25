#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <modbus.h>
#include <log.h>

static int fd_shell;
static modbus_t *ctx;
static modbus_mapping_t *map;

static int modbus_slave_init()
{
    fd_shell = open("/dev/ttyS1", 0);
    assert(fd_shell != -1);

    //ctx = modbus_new_rtu("/dev/ttyS2", 115200, 'N', 8, 0);
    ctx = modbus_new_tcp("0.0.0.0", 502);
    int s = modbus_tcp_listen(ctx, 4);
    modbus_tcp_accept(ctx, &s);
    modbus_set_slave(ctx, 12);
    map = modbus_mapping_new(10, 10, 10, 10);
    //modbus_connect(ctx);
    return 0;
}

static int modbus_slave_fini()
{
    modbus_mapping_free(map);
    modbus_close(ctx);
    modbus_free(ctx);
    close(fd_shell);
    return 0;
}

static void modbus_slave_loop()
{
    static int sleep_value;
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
    memset(query, 0, sizeof(query));
    int rc = modbus_receive(ctx, query);
    if (rc == -1) {
        if (errno == ETIMEDOUT) {
            LOG_DEBUG("modbus receive timeout");
            if (sleep_value < 1000 * 1000)
                sleep_value += 100 * 1000;
            usleep(sleep_value);
        } else {
            modbus_close(ctx);
            int s = modbus_tcp_listen(ctx, 4);
            modbus_tcp_accept(ctx, &s);
        }
    } else {
        modbus_reply(ctx, query, rc, map);
        LOG_DEBUG("modbus reply");
        sleep_value = 0;
    }
}

void modbus_slave_main(void)
{
    modbus_slave_init();
    LOG_INFO("start modbus slave ...");

    while (1) {
        modbus_slave_loop();
    }

    LOG_INFO("exit modbus slave ...");
    modbus_slave_fini();
}
