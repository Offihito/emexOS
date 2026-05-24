#pragma once

void taskbar_init(int scr_w, int scr_h);
void taskbar_draw(int mx, int my, int btn_down);
int taskbar_click(int mx, int my);
int taskbar_y(void);