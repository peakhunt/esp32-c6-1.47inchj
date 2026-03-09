#pragma once

extern void st7789_init_lcd(void);
extern void st7789_fill_screen(uint16_t color);
extern void st7789_disp_on_off(bool on);
extern void st7789_backlight_on_off(bool on);
extern void st7789_fill_screen_pat(uint16_t pat1, uint16_t pat2, uint16_t pat3, uint16_t pat4, uint8_t offset);
