#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_utils.h"
#include "wizchip_conf.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <printk.h>
#include <unistd.h>
#include <drivers/spi.h>

static void cris_en(void)
{
    //__disable_irq();
    __set_PRIMASK(1);
}

static void cris_ex(void)
{
    //__enable_irq();
    __set_PRIMASK(0);
}

static void cs_sel(void)
{
    SPI1_cs_sel();
}

static void cs_desel(void)
{
    SPI1_cs_desel();
}

static uint8_t spi_rb(void)
{
    return SPI1_read_byte();
}

static void spi_wb(uint8_t TxData)
{
    SPI1_write_byte(TxData);
}

static void w5500_reset(void)
{
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_0);
    LL_mDelay(500);
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_0);
    LL_mDelay(1500);
}

static void w5500_init(void)
{
    reg_wizchip_cris_cbfunc(cris_en, cris_ex);
    reg_wizchip_cs_cbfunc(cs_sel, cs_desel);
    reg_wizchip_spi_cbfunc(spi_rb, spi_wb);

    uint8_t memsize[2][8] = {{2,2,2,2,2,2,2,2},{2,2,2,2,2,2,2,2}};
    if (ctlwizchip(CW_INIT_WIZCHIP, (void *)memsize) == -1)
        panic("WIZCHIP Initialized failed.");
}

void w5500_phy_check(void)
{
    printk("check physical connection .");
    while ((0x01 & getPHYCFGR()) == 0) {
        printk(" .");
        LL_mDelay(500);
    }
    printk(" \n");
}

void w5500_set_netinfo(wiz_NetInfo wiz_netinfo)
{
    uint8_t v = getVERSIONR();
    printk("w5500 version: %d\n", v);

    ctlnetwork(CN_SET_NETINFO, (void*)&wiz_netinfo);
    LL_mDelay(1000);
    ctlnetwork(CN_GET_NETINFO, (void*)&wiz_netinfo);
    printk("mac: %02X:%02X:%02X:%02X:%02X:%02X\n",
           wiz_netinfo.mac[0],
           wiz_netinfo.mac[1],
           wiz_netinfo.mac[2],
           wiz_netinfo.mac[3],
           wiz_netinfo.mac[4],
           wiz_netinfo.mac[5]);
    printk("ip: %d.%d.%d.%d\n",
           wiz_netinfo.ip[0],
           wiz_netinfo.ip[1],
           wiz_netinfo.ip[2],
           wiz_netinfo.ip[3]);
    printk("mask: %d.%d.%d.%d\n",
           wiz_netinfo.sn[0],
           wiz_netinfo.sn[1],
           wiz_netinfo.sn[2],
           wiz_netinfo.sn[3]);
    printk("gw: %d.%d.%d.%d\n",
           wiz_netinfo.gw[0],
           wiz_netinfo.gw[1],
           wiz_netinfo.gw[2],
           wiz_netinfo.gw[3]);
    printk("dns: %d.%d.%d.%d\n",
           wiz_netinfo.dns[0],
           wiz_netinfo.dns[1],
           wiz_netinfo.dns[2],
           wiz_netinfo.dns[3]);
}

void net_init(void)
{
    w5500_reset();
    w5500_init();
    w5500_phy_check();

    wiz_NetInfo wiz_netinfo = {
        .mac = { 0x08, 0x33, 0x18, 0xcc, 0x85, 0x24},
        .ip = { 192, 168, 2, 111},
        .sn = { 255, 255, 255, 0},
        .gw = { 192, 168, 2, 1},
        .dns = { 114, 114, 114, 114},
        .dhcp = NETINFO_STATIC,
    };
    w5500_set_netinfo(wiz_netinfo);
}
