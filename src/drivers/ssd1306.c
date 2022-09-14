#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_utils.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <drivers/spi.h>
#include <fs/fs.h>

static uint8_t ssd1306_buffer[SSD1306_BUFFER_SIZE];
static ssd1306_font_t ssd1306_font;
static uint16_t currentX;
static uint16_t currentY;

static void ssd1306_reset(void)
{
    SPI2_cs_desel();
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_2);
    LL_mDelay(500);
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_2);
    LL_mDelay(1500);
}

static void ssd1306_dc_sel(void)
{
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_1);
}

static void ssd1306_dc_desel(void)
{
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_1);
}

static void ssd1306_write_cmd(uint8_t cmd)
{
    ssd1306_dc_sel();
    SPI2_cs_sel();
    SPI2_write_byte(cmd);
    SPI2_cs_desel();
    ssd1306_dc_desel();
}

static int ssd1306_write_data(const void *buf, uint32_t len)
{
    ssd1306_dc_desel();
    SPI2_cs_sel();
    int rc = SPI2_write(buf, len);
    SPI2_cs_desel();
    return rc;
}

static void ssd1306_display_reset(void)
{
    // reset sequence is fixed according to datasheet, should not change
    ssd1306_write_cmd(SSD1306_DISPLAY_OFF);

    ssd1306_write_cmd(SSD1306_SET_MULTIPLEX_RATIO);
    ssd1306_write_cmd(0x3F);

    ssd1306_write_cmd(SSD1306_SET_DISPLAY_OFFSET);
    ssd1306_write_cmd(0x00);

    ssd1306_write_cmd(SSD1306_SET_DISPLAY_START + 0);

    ssd1306_write_cmd(SSD1306_SET_SEGMENT_REMAP_OFF);

    ssd1306_write_cmd(SSD1306_SET_COMMON_REMAP_OFF);

    ssd1306_write_cmd(SSD1306_SET_COMMON_PINS);
    ssd1306_write_cmd(0x12);

    ssd1306_write_cmd(SSD1306_SET_CONTRAST_CONTROL);
    ssd1306_write_cmd(0x7F);

    ssd1306_write_cmd(SSD1306_ENTIRE_DISPLAY_OFF);

    ssd1306_write_cmd(SSD1306_NORMAL_DISPLAY);

    // Addressing
    ssd1306_write_cmd(SSD1306_SET_MEMORY_MODE);
    ssd1306_write_cmd(0b00);
    ssd1306_write_cmd(SSD1306_SET_COLUMN_ADDRESS);
    ssd1306_write_cmd(0);
    ssd1306_write_cmd(127);
    ssd1306_write_cmd(SSD1306_SET_PAGE_ADDRESS);
    ssd1306_write_cmd(0);
    ssd1306_write_cmd(7);

    ssd1306_write_cmd(SSD1306_SET_DISPLAY_CLOCK);
    ssd1306_write_cmd(0x80);

    ssd1306_write_cmd(SSD1306_SET_PRECHARGE_PERIOD);
    ssd1306_write_cmd(0x22);

    ssd1306_write_cmd(SSD1306_SET_VCOMH_DESELECT_LEVEL);
    ssd1306_write_cmd(0x20);

    ssd1306_write_cmd(SSD1306_CHARGE_PUMP_SETTING);
    ssd1306_write_cmd(0x14);

    ssd1306_write_cmd(SSD1306_DISPLAY_ON);
}

void ssd1306_fill(uint8_t byte)
{
    for (int i = 0; i < sizeof(ssd1306_buffer); i++)
        ssd1306_buffer[i] = byte;
}

void ssd1306_render(void)
{
    for(int i = 0; i < SSD1306_HEIGHT/8; i++) {
        ssd1306_write_cmd(SSD1306_SET_PAGE_START + i);
        ssd1306_write_cmd(SSD1306_SET_LOW_COL_START + SSD1306_X_OFFSET_LOWER);
        ssd1306_write_cmd(SSD1306_SET_HIGH_COL_START + SSD1306_X_OFFSET_HIGHER);
        ssd1306_write_data(&ssd1306_buffer[SSD1306_WIDTH*i], SSD1306_WIDTH);
    }
}

void ssd1306_set_font(ssd1306_font_t font)
{
    memcpy(&ssd1306_font, &font, sizeof(font));
}

void ssd1306_set_cursor(uint8_t x, uint8_t y)
{
    currentX= x;
    currentY = y;
}

void ssd1306_next_line(void)
{
    currentX = 0;
    currentY += ssd1306_font.height;
}

static void ssd1306_set_pixel(uint8_t x, uint8_t y)
{
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
        return;

    ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
}

static void ssd1306_unset_pixel(uint8_t x, uint8_t y)
{
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
        return;

    ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
}

int ssd1306_write_char(char ch)
{
    if (ch < 32 || ch > 126)
        return -1;

    if (SSD1306_WIDTH < (currentX + ssd1306_font.width) ||
        SSD1306_HEIGHT < (currentY + ssd1306_font.height))
        return -1;

    for(int i = 0; i < ssd1306_font.height; i++) {
        int b = ssd1306_font.data[(ch - 32) * ssd1306_font.height + i];
        for(int j = 0; j < ssd1306_font.width; j++) {
            if((b << j) & 0x8000)  {
                ssd1306_set_pixel(currentX + j, (currentY + i));
            } else {
                ssd1306_unset_pixel(currentX + j, (currentY + i));
            }
        }
    }

    currentX += ssd1306_font.width;
    return 0;
}

int ssd1306_write_str(const char *str)
{
    int len = strlen(str);

    for (int i = 0; i < len; i++) {
        if (ssd1306_write_char(str[i]) != 0)
            return i;
    }
    return len;
}

static struct ssd1306_struct {
    struct inode *inode;
} ssd1306;

static int ssd1306_open(struct inode *inode)
{
    return 0;
}

static int ssd1306_close(struct inode *inode)
{
    return 0;
}

static int ssd1306_ioctl(struct inode *inode, unsigned int cmd, unsigned long arg)
{
    if (cmd == SSD1306_IOCTL_WRITE_CMD) {
        if (inode == ssd1306.inode) {
            ssd1306_write_cmd(arg);
            return 0;
        } else {
            errno = ENODEV;
            return -1;
        }
    }

    errno = EINVAL;
    return -1;
}

static int ssd1306_write(struct inode *inode, const void *buf, uint32_t len)
{
    int rc = 0;
    if (inode == ssd1306.inode) {
        ssd1306_dc_desel();
        SPI2_cs_sel();
        rc = SPI2_write(buf, len);
        SPI2_cs_desel();
        return rc;
    }
    errno = ENODEV;
    return -1;
}

static int ssd1306_read(struct inode *inode, void *buf, uint32_t len)
{
    int rc = 0;
    if (inode == ssd1306.inode) {
        ssd1306_dc_desel();
        SPI2_cs_sel();
        rc = SPI2_read(buf, len);
        SPI2_cs_desel();
        return rc;
    }
    errno = ENODEV;
    return -1;
}

static inode_ops_t ssd1306_ops =  {
    .open = ssd1306_open,
    .close = ssd1306_close,
    .ioctl = ssd1306_ioctl,
    .write = ssd1306_write,
    .read = ssd1306_read,
};

void ssd1306_init(void)
{
    ssd1306_reset();
    ssd1306_display_reset();
    ssd1306_fill(0x00);
    ssd1306_set_font(Font_6x8);
    ssd1306_render();

    // fs
    ssd1306.inode = calloc(1, sizeof(*ssd1306.inode));
    ssd1306.inode->type = INODE_TYPE_CHAR;
    ssd1306.inode->ops = ssd1306_ops;
    INIT_LIST_HEAD(&ssd1306.inode->node);
    struct dentry *den1 = calloc(1, sizeof(*den1));
    snprintf(den1->name, sizeof(den1->name), "%s", "ssd1306");
    den1->type = DENTRY_TYPE_FILE;
    den1->parent = NULL;
    INIT_LIST_HEAD(&den1->childs);
    INIT_LIST_HEAD(&den1->child_node);
    den1->inode = ssd1306.inode;
    dentry_add("/dev", den1);
}
