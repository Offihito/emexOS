#include "boot.h"
#include "print.h"
#include <kernel/communication/serial.h>
#include <kernel/graph/graphics.h>
#include <kernel/data/images/bmp.h>
#include <kernel/graph/lib/string.h>
// all the print function will be migrated to this file
// because currently the console is a part of the screen (like console_scrollup)
// and this is bad thats why i create kernel processes and the bootscreen process
// which will handle all bootup messages and also the scroll up (and mb login)

bs_screen_t bs_screens[BS_MAX_SCREENS];
int bs_active = 0;

bmp_image_t bs_logo;
int bs_logo_loaded = 0;

#define BS_MAX_PIX (1920 * 1200)
static u32 bs_buf_left [BS_MAX_PIX / 2]; // BS1
static u32 bs_buf_right[BS_MAX_PIX / 2]; // BS2
static u32 bs_buf_full [BS_MAX_PIX];     // BS3
static u32 bs_buf_bs4  [BS_MAX_PIX];     // BS4 (unused but allocated)

static u32 bs_pdw = 0; /* fb_pitch : 4 */
static u32 bs_fbh = 0; /* real fbh */

/*logger */
static int bs_log_enabled[BS_MAX_SCREENS];
static u32 bs_log_lengths[BS_MAX_SCREENS];
static char bs_log_buffers[BS_MAX_SCREENS][BS_LOG_MAX];

static void bs_blit_screen(int id)
{
    if (id < 0 || id >= BS_MAX_SCREENS) return;
    bs_screen_t *scr = &bs_screens[id];
    u32 *fb = get_framebuffer();
    if (!fb || !scr->buffer || !bs_pdw) return;

    u32 sx = scr->x;
    u32 sy = scr->y;
    u32 sw = scr->width;
    u32 sh = scr->height;

    for (u32 row = 0; row < sh; row++) {
        u32 *dst = fb + (sy + row) * bs_pdw + sx;
        u32 *src = scr->buffer + row * sw;
        memcpy(dst, src, sw * sizeof(u32));
    }
}

void bs_init_screens(void)
{
	bs_pdw = get_fb_pitch() / 4;
	bs_fbh = get_fb_height();

	u32 fw   = get_fb_width();
	u32 fh   = get_fb_height();
	u32 half = fw / 2;

	// BS1: left half
	bs_screens[BS1].cursor_x = 0;
	bs_screens[BS1].cursor_y = 0;
	bs_screens[BS1].buffer   = bs_buf_left;
	bs_screens[BS1].x        = 0;
	bs_screens[BS1].y        = 0;
	bs_screens[BS1].width    = half;
	bs_screens[BS1].height   = fh;

	// BS2: right half
	bs_screens[BS2].cursor_x = 0;
	bs_screens[BS2].cursor_y = 0;
	bs_screens[BS2].buffer   = bs_buf_right;
	bs_screens[BS2].x        = half;
	bs_screens[BS2].y        = 0;
	bs_screens[BS2].width    = fw - half;
	bs_screens[BS2].height   = fh;

	// BS3: full screen
	bs_screens[BS3].cursor_x = 0;
	bs_screens[BS3].cursor_y = 0;
	bs_screens[BS3].buffer   = bs_buf_full;
	bs_screens[BS3].x        = 0;
	bs_screens[BS3].y        = 0;
	bs_screens[BS3].width    = fw;
	bs_screens[BS3].height   = fh;

	// BS4: full screen (unused)
	bs_screens[BS4].cursor_x = 0;
	bs_screens[BS4].cursor_y = 0;
	bs_screens[BS4].buffer   = bs_buf_bs4;
	bs_screens[BS4].x        = 0;
	bs_screens[BS4].y        = 0;
	bs_screens[BS4].width    = fw;
	bs_screens[BS4].height   = fh;

	// clear all buffers
	memset(bs_buf_left,  0, sizeof(bs_buf_left));
	memset(bs_buf_right, 0, sizeof(bs_buf_right));
	memset(bs_buf_full,  0, sizeof(bs_buf_full));
	memset(bs_buf_bs4,   0, sizeof(bs_buf_bs4));
}

void bs_switch(int id)
{
	if (id < 0 || id >= BS_MAX_SCREENS) return;
	if (!bs_screens[id].buffer) return;

    int old = bs_active;
    bs_active = id;

    char buffer[64];
    buffer[0] = '\0';
    char num[16];
    str_append(buffer, "bootscreen switched to ");
    str_from_int(num, id);
    str_append(buffer, num);
    str_append(buffer, "\n");

    u32 *fb = get_framebuffer();
    if (!fb) return;
    // blit the newly active screen to fb
    bs_blit_screen(id);
    log("[BOOTSCREEN]", buffer, d);
    (void)old;
}

void bs_set_region(int id, u32 x, u32 y, u32 w, u32 h)
{
    if (id < 0 || id >= BS_MAX_SCREENS) return;

    bs_screens[id].x = x;
    bs_screens[id].y = y;
    bs_screens[id].width = w;
    bs_screens[id].height = h;
}

void bs_log_start(int screen)
{
	if (screen < 0 || screen >= BS_MAX_SCREENS) return;

	bs_log_enabled[screen] = 1;
	bs_log_lengths[screen] = 0;

	memset(bs_log_buffers[screen], 0, BS_LOG_MAX);
}

void bs_log_stop(int screen)
{
	if (screen < 0 || screen >= BS_MAX_SCREENS) return;

	bs_log_enabled[screen] = 0;
}

const char *bs_log_read(int screen)
{
	if (screen < 0 || screen >= BS_MAX_SCREENS) return NULL;

	return bs_log_buffers[screen];
}

void bs_log_write(int screen, const char *text)
{
	u32 len = str_len(text);

	if (screen < 0 || screen >= BS_MAX_SCREENS) return;
	if (!bs_log_enabled[screen]) return;
	if (!text) return;
	if ((bs_log_lengths[screen] + len ) >= BS_LOG_MAX) { len = BS_LOG_MAX - bs_log_lengths[screen] - 1 ; }
	if ((int)len <= 0) return;

	memcpy(
		&bs_log_buffers[screen][bs_log_lengths[screen]],
		text,
		len
	);
	bs_log_lengths[screen] += len;
	bs_log_buffers[screen][bs_log_lengths[screen]] = '\0';
}

bs_screen_t* bs_get_active(void)
{
	return &bs_screens[bs_active];
}

// api
u32 *bs_backbuf_get(void)
{
	return bs_get_active()->buffer;
}
u32 bs_backbuf_pitch_dw(void)
{
	return bs_get_active()->width;
}
u32 bs_backbuf_height(void)
{
	return bs_get_active()->height;
}

void bs_flush_rows(u32 y, u32 row_count)
{
    bs_screen_t *scr = bs_get_active();
    u32 *fb = get_framebuffer();
    if (!fb || !scr->buffer || !bs_pdw) return;

    u32 end = y + row_count;
    if (end > scr->height) end = scr->height;
    if (end <= y) return;

    u32 sx = scr->x;
    u32 sy = scr->y;
    u32 sw = scr->width;

    for (u32 row = y; row < end; row++) {
        u32 *dst = fb + (sy + row) * bs_pdw + sx;
        u32 *src = scr->buffer + row * sw;
        memcpy(dst, src, sw * sizeof(u32));
    }
}

/* from active backbuffer to framebuffer
 * row by wor
 */
void bs_flush_rect(u32 x, u32 y, u32 w, u32 h)
{
    bs_screen_t *scr = bs_get_active();
    u32 *fb = get_framebuffer();
    if (!fb || !scr->buffer || !bs_pdw) return;

    u32 end_y = y + h;
    if (end_y > scr->height) end_y = scr->height;
    u32 end_x = x + w;
    if (end_x > scr->width) end_x = scr->width;
    u32 copy_w = end_x - x;
    if (!copy_w) return;

    u32 sx = scr->x;
    u32 sy = scr->y;
    u32 sw = scr->width;

    for (u32 row = y; row < end_y; row++) {
        u32 *dst = fb + (sy + row) * bs_pdw + (sx + x);
        u32 *src = scr->buffer + row * sw + x;
        memcpy(dst, src, copy_w * sizeof(u32));
    }
}

/* entire backbuffer copy */
void bs_backbuf_flush_all(void)
{
	if (!bs_pdw) {
		bs_pdw = get_fb_pitch() / 4;
		bs_fbh = get_fb_height();
	}
	bs_blit_screen(bs_active);
}

/* clears both backbuffer and framebuffer */
void bs_backbuf_clear(u32 color)
{
	if (!bs_pdw) {
		bs_pdw = get_fb_pitch() / 4;
		bs_fbh = get_fb_height();
	}
	bs_screen_t *scr = bs_get_active();
	u32 len = scr->width * scr->height;
	if (!len || !scr->buffer) return;

	if (color == 0) {
		memset(scr->buffer, 0, len * sizeof(u32));
	} else
	{
		u64 fill = ((u64)color << 32) | color;
		u64 *p64 = (u64 *)scr->buffer;
		u32 n64 = len / 2;
		for (u32 i = 0; i < n64; i++) p64[i] = fill;
		/* handle odd trailing pixel */
		if (len & 1) scr->buffer[len - 1] = color;
	}

	bs_blit_screen(bs_active);
}

// clear a specific screen to a color, never touches other screens
void bs_clear_screen(int id, u32 color)
{
    if (id < 0 || id >= BS_MAX_SCREENS) return;
    bs_screen_t *scr = &bs_screens[id];
    u32 len = scr->width * scr->height;
    if (!len || !scr->buffer) return;

    if (color == 0) {
        memset(scr->buffer, 0, len * sizeof(u32));
    } else {
        u64 fill = ((u64)color << 32) | color;
        u64 *p64 = (u64 *)scr->buffer;
        u32 n64 = len / 2;
        for (u32 i = 0; i < n64; i++) p64[i] = fill;
        if (len & 1) scr->buffer[len - 1] = color;
    }

    bs_blit_screen(id);
}

void bs_log_save(int screen, const char *path)
{
	const char *log = bs_log_read(screen);
	int fd = fs_open(path, O_CREAT | O_WRONLY);

	if(!log) return;
	if (fd <0) return;

	fs_write(
		fd,
		log,
		str_len(log)
	);
	fs_close(fd);
}

static int bs_init_proc(kproc_t *self) {
    (void)self;
    bs_switch(BS1);
    log("[BOOTSCREEN]","init bootscreen\n", d);
    return KPROC_EFINE;
}

static int bs_tick(kproc_t *self) {
    (void)self;
    return KPROC_EFINE;
}

static void bs_fini(kproc_t *self) {
    (void)self;
}

kproc_t bootscreen_proc =
{
    .name     = "bootscreen",
    .state    = KPROC_STATE_READY,
    .flags    = KPROC_FLAG_EARLY | KPROC_FLAG_CRITICAL | KPROC_FLAG_PERMANENT,
    .priority = 128,
    .init     = bs_init_proc,
    .tick     = bs_tick,
    .fini     = bs_fini,
};

void init_bootscreen(void)
{
    bs_init_screens();
    bs_logo_loaded = 0;

    u32 fw   = get_fb_width();
    u32 fh   = get_fb_height();
    u32 half = fw / 2;

    bs_set_region(BS1, 0,    0, half,      fh);
    bs_set_region(BS2, half, 0, fw - half, fh);
    bs_set_region(BS3, 0,    0, fw,        fh);
    bs_set_region(BS4, 0,    0, fw,        fh);

    bs_switch(BS2);
    bs2_draw_info();
    //print("test", white());
    bs_switch(BS1);

    bs_log_start(BS1);

    kproc_register_and_start(&bootscreen_proc);
}

void uninit_bootscreen(void)
{
	log("[BS]", "stopping bootscreen services...\n", d);
	bs_log_stop(BS1);
	log("[BS]", "writing logs...\n", d);
	bs_log_save(BS1, "/emr/system/logs/boot.log");
}