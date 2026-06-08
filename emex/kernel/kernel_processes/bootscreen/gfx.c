#include "gfx.h"

#include "boot.h"
#include "print.h"

#include "info.h"

#include <kernel/graph/graphics.h>
#include <kernel/include/reqs.h>

//from info.c
static struct limine_file *find_logo_module(void)
{
    if (!module_request.response) return NULL;
    if (module_request.response->module_count == 0) return NULL;

    struct limine_module_response *resp = module_request.response;

    for (u64 i = 0; i < resp->module_count; i++) {
        const char *path = resp->modules[i]->path;

        // extract filename from path
        const char *fname = path;
        for (const char *p = path; *p; p++) {
            if (*p == '/') fname = p + 1;
        }

        if (str_equals(fname, LOGO_MODULE_NAME)) {
            return resp->modules[i];
        }
    }
    return NULL;
}

void loading_screen(void)
{
    bs_switch(BS4);

    bs_clear_screen(BS4, 0xFF008080);

    int old = bs_active;
    bs_active = BS4;
    bs_screen_t *scr = &bs_screens[BS4];

    u32 len = scr->width * scr->height;
    if (scr->buffer) memset(scr->buffer, 0, len * sizeof(u32));

    u32 text_y = 8;

    struct limine_file *logo_mod = find_logo_module();

    if (logo_mod && logo_mod->size >= LOGO_MIN_SIZE)
    {
        const u8 *data = (const u8 *)logo_mod->address;

        // read header
        u32 logo_w = (u32)data[0] | ((u32)data[1] << 8) |
                     ((u32)data[2] << 16) | ((u32)data[3] << 24);
        u32 logo_h = (u32)data[4] | ((u32)data[5] << 8) |
                     ((u32)data[6] << 16) | ((u32)data[7] << 24);

        u64 expected = 8 + (u64)logo_w * logo_h * 4;

        if (logo_w > 0 && logo_h > 0 && logo_mod->size >= expected)
        {
            const u32 *pixels = (const u32 *)(data + 8);

            // center horizontally in BS2
            //u32 lx = (scr->width > logo_w) ? (scr->width - logo_w) / 2 : 0;   // centered
            //u32 lx = (scr->width > logo_w) ? (scr->width - logo_w) / : 0; 	// right
            u32 lx = 16; // left; 16p padding
            u32 ly = 8;

            u32 draw_w = logo_w;
            u32 draw_h = logo_h;
            if (lx + draw_w > scr->width)  draw_w = scr->width  - lx;
            if (ly + draw_h > scr->height) draw_h = scr->height - ly;

            for (u32 dy = 0; dy < draw_h; dy++) {
                for (u32 dx = 0; dx < draw_w; dx++) {
                    u32 c = pixels[dy * logo_w + dx];
                    if ((c >> 24) == 0) continue; // transparent
                    bs_setpixel(scr, lx + dx, ly + dy, c);
                }
            }

            text_y = ly + draw_h + 8;
        }
    }

    // position text cursor below logo
    scr->cursor_x = 4;
    scr->cursor_y = text_y;

    u32 w = bs_screens[BS4].width;
    u32 h = bs_screens[BS4].height;

    u32 box_w = 360;
    u32 box_h = 80;

    u32 box_x = (w - box_w) / 2;
    u32 box_y = (h - box_h) / 2;

    draw_rect_both(
        box_x,
        box_y,
        box_w,
        box_h,
        0xFFC0C0C0,
        0xFF000000
    );

    bs_screens[BS4].cursor_x = box_x + 40;
    bs_screens[BS4].cursor_y = box_y + 30;

    print_to(
        BS4,
        "Loading emexOS...",
        0xFFFFFFFF
    );

    bs_backbuf_flush_all();
}