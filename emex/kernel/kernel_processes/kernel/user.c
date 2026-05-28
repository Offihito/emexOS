#include "gen.h"
#include "../shells/shells.h"
#include "../bootscreen/boot.h"

#include <kernel/include/assembly.h>
#include <kernel/packages/emex/emex.h>

#include <kernel/mem/memlog.h>

#include <kernel/devices/pty/pty.h>
#include <kernel/file_systems/vfs/vfs.h>


// path to the init app in the VFS (loaded from initrd.cpio)
#define SYSTEMLOCATE "/emr/system/system.emx"
//mod
//#define LOGINLOCATE "/emr/bin/login.elf"


static void setup_stdio(void)
{
    pty_init();
    int pty_idx = pty_alloc();
    if (pty_idx < 0) {
        log("[INIT]", "WARN: pty_alloc failed\n", warning);
    }

    int tty_fd = fs_open("/dev/tty0", O_RDWR);
    if (tty_fd >= 0) {
        fs_file *f = fs_get_file(tty_fd);
        if (f) fs_set_fd(0, f); /* stdin */
    }

    int pts_fd = fs_open("/dev/pts/0", O_RDWR);
    if (pts_fd >= 0) {
        fs_file *f = fs_get_file(pts_fd);
        if (f) {
            fs_set_fd(1, f);
            fs_set_fd(2, f);
        }
        log("[INIT]", "stdio: fd0->tty0, fd1/2->pts/0\n", d);
    } else {
        if (tty_fd >= 0) {
            fs_file *f = fs_get_file(tty_fd);
            if (f) {
                fs_set_fd(1, f);
                fs_set_fd(2, f);
            }
        }
        log("[INIT]", "stdio: fd0/1/2->tty0 (PTY fallback)\n", warning);
    }
}

void uproc(void) {
    #if ENABLE_ULIME
        if (!proc_mgr || !ulime) {
            log("[INIT]", "proc_mgr or ulime not ready\n", error);
            return;
        }
        setup_stdio();

        log("[INIT]", "loading " SYSTEMLOCATE "...\n", d);

        ulime_proc_t *init_proc = NULL;
        int result = emex_launch_app(SYSTEMLOCATE, &init_proc);

        if (result != EMEX_OK || !init_proc) {
            log("[INIT]", "failed to load " SYSTEMLOCATE "\n", error);
            log("[INIT]", "system cannot continue without init\n", error);
            //while (1) __asm__ volatile("cli; hlt");
            // hcf();
            return;
        }

        log("[INIT]", "jumping to userspace\n", success);

        //memlog_print_map();

        bootscreen_services: {
        	u32 fw     = get_fb_width();
	        u32 fh     = get_fb_height();
	        u32 half_w     = fw / 2;
	        u32 half_h     = fh / 2;

		    clear(BS1, 0xff000000);
		    clear(BS2, 0xff000000);
			clear(BS3, 0xff000000);
      		bs_backbuf_clear(0x00000000);

	        uninit_bootscreen();
	        bs_set_region(BS1, 0, 0,      half_w,half_h);
	        bs_set_region(BS2, 0, half_h, half_w,fh - half_h);

			/*userspace */
	        //bs_set_region(BS3, half_w, 0, fw - half_w,fh);
	        //bs_set_region(BS4, half_w, 0, fw - half_w,fh);
			bs_set_region(USER_SCREEN_MODE, 0, 0, fw, fh);
			bs_set_region(BS4, 0, 0, fw, fh);

	        bs_switch(USER_SCREEN_MODE);
			//clear(BS3, 0xff000033); // blue ig?

   			dump_kprocesses();
        	proc_list_procs(proc_mgr);
        }

        ulime->ptr_proc_curr = init_proc;

        //hcf();

        log("[INIT]", "jumping to init_proc\n", d);

        //clear(0xff000000);

        #if JUMPTOUSER == 1

        	if (mt) {
	            mt_add_task(mt, init_proc); // register init with mt
	            mt_start(mt); // and launch
				//never returns
	        }
            JumpToUserspace(init_proc); // fallback if mt is not up

        #endif

        emergency_shell();
        recovery_shell();
        __builtin_unreachable();

        //proc_list_procs(proc_mgr);
    #endif
}
