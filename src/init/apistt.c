#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <log.h>
#include <atbuf.h>
#include <srrp.h>
#include <crc16.h>
#include <svcx.h>
#include <apix.h>
#include <apix-stm32.h>
#include <net/wizchip_socket.h>

static struct svchub *hub;
static int fd_shell;
static int fd_tty;
static int fd_tcp;
struct apix *ctx;

static int on_echo(struct srrp_packet *req, struct srrp_packet **resp)
{
    uint16_t crc = crc16(req->header, req->header_len);
    crc = crc16_crc(crc, req->data, req->data_len);
    *resp = srrp_write_response(req->srcid, crc, req->header, "{msg:'world'}");
    return 0;
}

#if 0
void __apistt_loop()
{
    int nread = __recv(fd_tcp, (uint8_t *)atbuf_write_pos(rxbuf), atbuf_spare(rxbuf));
    assert(nread >= 0);
    if (nread == 0) return;
    atbuf_write_advance(rxbuf, nread);

    int fd = open("/dev/led0", 0);

    while (atbuf_used(rxbuf)) {
        uint32_t offset = srrp_next_packet_offset(
            atbuf_read_pos(rxbuf), atbuf_used(rxbuf));
        atbuf_read_advance(rxbuf, offset);
        struct srrp_packet *req = srrp_read_one_packet(atbuf_read_pos(rxbuf));
        if (req == NULL) {
            __send(fd_tcp, (uint8_t *)atbuf_read_pos(rxbuf), nread);
            atbuf_read_advance(rxbuf, atbuf_used(rxbuf));
            close(fd);
            return;
        }
        atbuf_read_advance(rxbuf, req->len);

        struct srrp_packet *resp = NULL;
        if (svchub_deal(hub, req, &resp) == 0) {
            assert(resp);
            int nr = __send(fd_tcp, (uint8_t *)resp->raw, resp->len);
            assert(nr != -1);
            assert(nr != 0);

            char tmp[4] = {0};
            read(fd, tmp, 4);
            if (atoi(tmp) == 0) {
                write(fd, "1", 1);
            } else {
                write(fd, "0", 1);
            }

            srrp_free(resp);
        }
        srrp_free(req);
    }

    close(fd);

    if (time(0) % 600 == 0) {
        struct srrp_packet *pac = srrp_write_request(
            8888, "/8888/alive", "{}");
        __send(fd_tcp, (uint8_t *)pac->raw, pac->len);
        srrp_free(pac);
    }
}
#endif

int on_pollin(int fd, const char *buf, size_t len)
{
    write(fd_tty, buf, len);
    LOG_DEBUG("tcp => com, %s", buf);
    return len;
}

int on_close(int fd)
{
    LOG_DEBUG("tcp close: %d", fd_tcp);
    fd_tcp = apix_open_stm32_tcp_server(ctx, "0.0.0.0:824");
    LOG_DEBUG("tcp open: %d", fd_tcp);
    struct apix_events events = {
        .on_close = on_close,
        .on_pollin = on_pollin,
    };
    apix_set_events(ctx, fd_tcp, &events);
    return 0;
}

static void apistt_loop(void)
{
    // fd_tty
    uint8_t buf[256] = {0};
    int nr = read(fd_tty, buf, 256);
    if (nr > 0) {
        __send(fd_tcp, (uint8_t *)buf, nr);
        LOG_DEBUG("com => tcp, %s", buf);
    }

    // fd_tcp
    apix_poll(ctx);
}

static int apistt_init()
{
    hub = svchub_new();
    svchub_add_service(hub, "/8888/echo", on_echo);

    fd_shell = open("/dev/ttyS1", 0);
    assert(fd_shell != -1);
    fd_tty = open("/dev/ttyS2", 0);
    assert(fd_tty != -1);

    ctx = apix_new();
    apix_enable_stm32(ctx);
    fd_tcp = apix_open_stm32_tcp_server(ctx, "0.0.0.0:824");
    LOG_DEBUG("tcp open: %d", fd_tcp);
    struct apix_events events = {
        .on_close = on_close,
        .on_pollin = on_pollin,
    };
    apix_set_events(ctx, fd_tcp, &events);

    return 0;
}

static int apistt_fini()
{
    apix_close(ctx, fd_tcp);
    apix_disable_stm32(ctx);
    apix_destroy(ctx);

    close(fd_tty);
    close(fd_shell);

    svchub_del_service(hub, "/8888/echo");
    svchub_destroy(hub);
    return 0;
}

void apistt_main(void)
{
    apistt_init();
    LOG_INFO("start apistt ...");

    while (1) {
        apistt_loop();
    }

    LOG_INFO("exit apistt ...");
    apistt_fini();
}
