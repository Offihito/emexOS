#ifndef BOOT_H
#define BOOT_H

#include <kernel/kernel_processes/loader.h>
#include <kernel/data/images/bmp.h>

#include "info.h"

extern kproc_t bootscreen_proc;

// bootscreen vt's
#define BS_MAX_SCREENS 4
#define BS_LOG_MAX 65536
#define BS1 	0
#define BS2 	1
#define BS3 	2
#define BS4 	3

typedef struct {
	u32 cursor_x;
	u32 cursor_y;
	u32 x;
	u32 y;
	u32 width;
	u32 height;
	u32 *buffer; // opt. buffering
} bs_screen_t;

extern bs_screen_t bs_screens[BS_MAX_SCREENS];
extern int bs_active;

extern bmp_image_t bs_logo;
extern int bs_logo_loaded;

void init_bootscreen(void);
void uninit_bootscreen(void);

void bs_init_screens(void);
void bs_switch(int id);
void bs_set_region(int id, u32 x, u32 y, u32 w, u32 h);
bs_screen_t* bs_get_active(void);

/* backbuffer API used by print.c */
u32 *bs_backbuf_get(void);
u32 bs_backbuf_pitch_dw(void);
u32 bs_backbuf_height(void);

void bs_flush_rows(u32 y, u32 row_count);
void bs_flush_rect(u32 x, u32 y, u32 w, u32 h);
void bs_backbuf_flush_all(void);
void bs_backbuf_clear(u32 color);
void bs_clear_screen(int id, u32 color);
// probably not needed
void print_to(int screen, const char *str, u32 color);

/* logger */
void bs_log_start(int screen);
void bs_log_stop(int screen);
const char *bs_log_read(int screen);
void bs_log_write(int screen, const char *text);

#endif