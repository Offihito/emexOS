#include "tty_render.h"
#include "tty.h"
#include <kernel/graph/graphics.h>
#include <kernel/graph/theme.h>
#include <kernel/kernel_processes/fm/fm.h>
#include <kernel/kernel_processes/bootscreen/console/console.h>
#include <kernel/kernel_processes/bootscreen/boot.h>

#include <kernel/graph/lib/string.h>

#define TTY_RENDER_BATCH 64

/* rrender backup for TTYNOGUI when set 1*/
static void tty_render_cell(tty_cell_t *cell)
{
    switch (cell->cmd)
    {
        case TTY_CELL_CHAR:
        {
            char tmp[2] = { cell->c, '\0' };
            cprintf(tmp, cell->fg);
            break;
        }
        case TTY_CELL_BACKSPACE:
        {
            u32 cw = fm_get_char_width()  * font_scale;
            u32 ch = fm_get_char_height() * font_scale;
            if (bs_get_active()->cursor_x >= cw)
            {
                bs_get_active()->cursor_x -= cw;
                draw_rect(
                	bs_get_active()->cursor_x,
                    bs_get_active()->cursor_y,
                    cw, ch, cell->bg
                );
            }
            break;
        }
        case TTY_CELL_CLEAR:
	    {
		    clear(BS1, bg());
		    clear(BS2, bg());
		    clear(BS3, bg());
		    clear(BS4, bg());
	    }
            break;
        case TTY_CELL_HOME:
            bs_get_active()->cursor_x = 0;
            bs_get_active()->cursor_y = 0;
            break;
        default:
            break;
    }
}

void tty_flush(int id)
{
    tty_cell_t batch[TTY_RENDER_BATCH];
    int n;
    while ((n = tty_read_output(id, batch, TTY_RENDER_BATCH)) > 0)
    {
        for (int i = 0; i < n; i++)
        {
            tty_render_cell(&batch[i]);
        }
    }
}