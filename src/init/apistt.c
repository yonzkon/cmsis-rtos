#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <net/socket.h>
#include <log.h>
#include <atbuf.h>
#include <srrp.h>
#include <crc16.h>
#include <svcx.h>

static atbuf_t *rxbuf;
static struct svchub *hub;
static int fd_stt = 1;
static uint16_t local_port = 8080;

static int on_echo(struct srrp_packet *req, struct srrp_packet **resp)
{
    uint16_t crc = crc16(req->header, req->header_len);
    crc = crc16_crc(crc, req->data, req->data_len);
    *resp = srrp_write_response(req->srcid, crc, req->header, "{msg:'world'}");
    return 0;
}

int apistt_init()
{
    rxbuf = atbuf_new(0);
    hub = svchub_new();
    svchub_add_service(hub, "/8888/echo", on_echo);

    return 0;
}

int apistt_fini()
{
    close(fd_stt);

    svchub_del_service(hub, "/8888/echo");
    svchub_destroy(hub);
    atbuf_delete(rxbuf);
    return 0;
}

void __apistt_loop()
{
    int nread = recv(fd_stt, (uint8_t *)atbuf_write_pos(rxbuf), atbuf_spare(rxbuf));
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
            send(fd_stt, (uint8_t *)atbuf_read_pos(rxbuf), nread);
            atbuf_read_advance(rxbuf, atbuf_used(rxbuf));
            close(fd);
            return;
        }
        atbuf_read_advance(rxbuf, req->len);

        struct srrp_packet *resp = NULL;
        if (svchub_deal(hub, req, &resp) == 0) {
            assert(resp);
            int nr = send(fd_stt, (uint8_t *)resp->raw, resp->len);
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
        send(fd_stt, (uint8_t *)pac->raw, pac->len);
        srrp_free(pac);
    }
}

void apistt_loop(void)
{
    uint8_t sr = getSn_SR(fd_stt);

    if (sr == SOCK_CLOSED) {
        socket(fd_stt, Sn_MR_TCP, local_port++, Sn_MR_ND);
    } else if (sr == SOCK_INIT) {
        uint8_t remote_ip[] = { 192, 168, 2, 132};
        connect(fd_stt, remote_ip, 12248);

        struct srrp_packet *pac = srrp_write_request(
            8888, "/8888/alive", "{}");
        send(fd_stt, (uint8_t *)pac->raw, pac->len);
        srrp_free(pac);
    } else if (sr == SOCK_ESTABLISHED) {
        // 清除接收中断标志位
        if (getSn_IR(fd_stt) & Sn_IR_CON)
            setSn_IR(fd_stt, Sn_IR_CON);

        // 定义len为已接收数据的长度
        uint16_t len = getSn_RX_RSR(fd_stt);
        if (len > 0)
            __apistt_loop();
    } else if (sr == SOCK_CLOSE_WAIT) {
        disconnect(fd_stt);
        close_socket(fd_stt);
    }
}
