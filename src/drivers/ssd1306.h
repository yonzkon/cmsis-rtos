#ifndef __SSD1306_H
#define __SSD1306_H

#include "ssd1306_fonts.h"
#include <stdint.h>

#define SSD1306_IOCTL_WRITE_CMD 0

// Fundamental Commands
#define SSD1306_SET_CONTRAST_CONTROL 0x81 // RESET = 0x7F, two bytes cmd: A[7:0]
#define SSD1306_ENTIRE_DISPLAY_OFF   0xA4 // RESET
#define SSD1306_ENTIRE_DISPLAY_ON    0xA5
#define SSD1306_NORMAL_DISPLAY       0xA6 // RESET
#define SSD1306_INVERSE_DISPLAY      0xA7
#define SSD1306_DISPLAY_OFF          0xAE // RESET, sleep mode
#define SSD1306_DISPLAY_ON           0xAF

// Addressing Setting Commands, PAGE|HORIZONTAL|VERTICAL_(ADDRESSING)_MODE
#define SSD1306_SET_LOW_COL_START  0x00 // PAGE only, RESET = 0, 0b0000____, last 4bits is lower col
#define SSD1306_SET_HIGH_COL_START 0x10 // PAGE only, RESET = 0x10, 0b0001____, last 4bits is higher col
#define SSD1306_SET_PAGE_START     0xB0 // PAGE only, RESET = 0xB0, 0b10110___, last 3bits is page
#define SSD1306_SET_MEMORY_MODE    0x20 // RESET = 01b, two bytes cmd: A[1:0]
                                        // 00b: Horizontal, 01b: Vertical, 10b: Page
#define SSD1306_SET_COLUMN_ADDRESS 0x21 // HORIZONTAL|VERTIAL, three bytes cmd
                                        // RESET = [0,127], A[6:0], B[6:0]
#define SSD1306_SET_PAGE_ADDRESS   0x22 // HORIZONTAL|VERTIAL, three bytes cmd
                                        // RESET = [0,7], A[2:0], B[2:0]

// Hardware Configuration (Panel resolution & layout related) Commands
// SEGMENT: 128 bits
// COMMON: 64 bits
#define SSD1306_SET_DISPLAY_START     0x40 // RESET = 0x40, 0b01______, last 6bits is start line
#define SSD1306_SET_SEGMENT_REMAP_OFF 0xA0 // RESET, col address 0 is mapped to SEG0
#define SSD1306_SET_SEGMENT_REMAP_ON  0xA1 //        col address 127 is mapped to SEG0
#define SSD1306_SET_MULTIPLEX_RATIO   0xA8 // RESET = 111111b, two bytes cmd: A[5:0]
                                           // A[5:0]: 0~15 is invalid, 16~64(auto +1) is valid
#define SSD1306_SET_COMMON_REMAP_OFF  0xC0 // RESET, scan from COM0 to COM[N–1]
#define SSD1306_SET_COMMON_REMAP_ON   0xC8 //        scan from COM[N-1] to COM0
#define SSD1306_SET_DISPLAY_OFFSET    0xD3 // RESET = 0, two bytes cmd: A[5:0]
                                           // Set vertical shift by COM from 0d~63d
#define SSD1306_SET_COMMON_PINS       0xDA // RESET = 010010b, two bytes cmd: A[5:4], 0b00__0010
                                           // A[4]=0b, Sequential COM pin configuration
                                           // A[4]=1b(RESET), Alternative COM pin configuration
                                           // A[5]=0b(RESET), Disable COM Left/Right remap
                                           // A[5]=1b, Enable COM Left/Right remap

// Timing & Driving Scheme Setting Commands
#define SSD1306_SET_DISPLAY_CLOCK 0xD5 // set display clock divide ratio/oscillator frequency
                                       // two bytes cmd
                                       // A[3:0]: Define the divide ratio of the display clocks
                                       // Divide ratio= A[3:0]+1, RESET = 0000b (divide ratio = 1)
                                       // A[7:4] : Set the Oscillator Frequency, FOSC
                                       // RESET = 1000b
#define SSD1306_SET_PRECHARGE_PERIOD 0xD9 // A[3:0]: Phase 1 period of up to 15 DCLK
                                          // clocks 0 is invalid entry
                                          // RESET = 0x2
                                          // A[7:4] : Phase 2 period of up to 15 DCLK
                                          // clocks 0 is invalid entry
                                          // RESET = 0x2
#define SSD1306_SET_VCOMH_DESELECT_LEVEL 0xDB // Set VCOMH Deselect Level
                                              // A[6:4] | Hex code | VCOMH deselect level
                                              // 000b   | 00h      | ~ 0.65 x VCC
                                              // 010b   | 20h      | ~ 0.77 x VCC (RESET)
                                              // 011b   | 30h      | ~ 0.83 x VCC

// Charge Pump Commands
#define SSD1306_CHARGE_PUMP_SETTING 0x8D // two bytes cmd: A[7:0], 0b010_00
                                         // A[2] = 0b, Disable charge pump(RESET)
                                         // A[2] = 1b, Enable charge pump during display on
                                         // The Charge Pump must be enabled by the following
                                         // command: 8Dh; Charge Pump Setting 14h;
                                         // Enable Charge Pump AFh; Display ON

#define SSD1306_HEIGHT 64
#define SSD1306_WIDTH  128
#define SSD1306_BUFFER_SIZE (SSD1306_WIDTH * SSD1306_HEIGHT / 8)
#define SSD1306_X_OFFSET_LOWER 0
#define SSD1306_X_OFFSET_HIGHER 0

void ssd1306_fill(uint8_t byte);
void ssd1306_render(void);
void ssd1306_set_font(ssd1306_font_t font);
void ssd1306_set_cursor(uint8_t x, uint8_t y);
void ssd1306_next_line(void);
int ssd1306_write_char(char ch);
int ssd1306_write_str(const char *str);

void ssd1306_init(void);

#endif
