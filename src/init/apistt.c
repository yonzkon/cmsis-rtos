#include "stm32f1xx_hal.h"
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <log.h>
#include <atbuf.h>
#include <srrp.h>
#include <crc16.h>
#include <svcx.h>

static atbuf_t *rxbuf;
static struct svchub *hub;
static int fd_stt;

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
    fd_stt = open("uart1", 0);
    assert(fd_stt);

    svchub_add_service(hub, "/8888/echo", on_echo);

    struct srrp_packet *pac = srrp_write_request(
        8888, "/8888/online", "{}");
    write(fd_stt, pac->raw, pac->len);
    srrp_free(pac);

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

void apistt_loop()
{
    int nread = read(fd_stt, atbuf_write_pos(rxbuf), atbuf_spare(rxbuf));
    assert(nread >= 0);
    if (nread == 0) return;
    atbuf_write_advance(rxbuf, nread);

    while (atbuf_used(rxbuf)) {
        uint32_t offset = srrp_next_packet_offset(
            atbuf_read_pos(rxbuf), atbuf_used(rxbuf));
        atbuf_read_advance(rxbuf, offset);
        struct srrp_packet *req = srrp_read_one_packet(atbuf_read_pos(rxbuf));
        if (req == NULL) {
            write(fd_stt, atbuf_read_pos(rxbuf), nread);
            atbuf_read_advance(rxbuf, atbuf_used(rxbuf));
            return;
        }
        atbuf_read_advance(rxbuf, req->len);

        struct srrp_packet *resp = NULL;
        if (svchub_deal(hub, req, &resp) == 0) {
            assert(resp);
            int nr = write(fd_stt, resp->raw, resp->len);
            assert(nr != -1);
            assert(nr != 0);
            if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET) {
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
            } else {
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
            }
            srrp_free(resp);
        }
        srrp_free(req);
    }

    if ((HAL_GetTick() / 1000) % 600 == 0) {
        struct srrp_packet *pac = srrp_write_request(
            8888, "/8888/alive", "{}");
        write(fd_stt, pac->raw, pac->len);
        srrp_free(pac);
    }
}
