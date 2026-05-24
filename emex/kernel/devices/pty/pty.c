#include "pty.h"
#include <kernel/file_systems/vfs/vfs.h>
#include <kernel/mem/klime/klime.h>
#include <kernel/mem/lib/main.h>
#include <kernel/graph/lib/string.h>
#include <kernel/communication/serial.h>




static pty_pair_t pty_table[PTY_MAX];

void pty_init(void)
{
    memset(pty_table, 0, sizeof(pty_table));
    log("[PTY]", "subsystem initialised\n", d);
}

int pty_alloc(void)
{
    for (int i = 0; i < PTY_MAX; i++)
    {
        if (!pty_table[i].active)
        {
            memset(&pty_table[i], 0, sizeof(pty_pair_t));
            pty_table[i].active = 1;
            printf("[PTY] allocated pair %d\n", i);
            return i;
        }
    }
    return -1;
}

void pty_free(int idx)
{
    if (idx < 0 || idx >= PTY_MAX) return;
    pty_table[idx].active      	  = 0;
    pty_table[idx].slave_open  	  = 0;
    pty_table[idx].master_open 	  = 0;
}

pty_pair_t *pty_get(int idx)
{
    if (idx < 0 || idx >= PTY_MAX) return NULL;
    if (!pty_table[idx].active) return NULL;
    return &pty_table[idx];
}



static ssize_t ring_write(
	char *buf, u32 *head, u32 *tail,
    u32 *count, const char *src, size_t n
){
    size_t written = 0;
    while (written < n && *count < PTY_BUF_SIZE)
    {
        buf[*tail] = src[written++];
        *tail = (*tail + 1) % PTY_BUF_SIZE;
        (*count)++;
    }
    return (ssize_t)written;
}

static ssize_t ring_read(
	char *buf, u32 *head, u32 *tail,
    u32 *count, char *dst, size_t n
){
    (void)tail;
    size_t rd = 0;
    while (rd < n && *count > 0)
    {
        dst[rd++] = buf[*head];
        *head = (*head + 1) % PTY_BUF_SIZE;
        (*count)--;
    }
    return (ssize_t)rd;
}



ssize_t pty_master_read(int idx, void *buf, size_t count)
{
    pty_pair_t *p = pty_get(idx);
    if (!p) return -1;


    return ring_read(
    	p->s2m_buf, &p->s2m_head, &p->s2m_tail,
        &p->s2m_count, (char *)buf, count
    );
}

ssize_t pty_master_write(int idx, const void *buf, size_t count)
{
    pty_pair_t *p = pty_get(idx);
    if (!p) return -1;


    return ring_write(
    	p->m2s_buf, &p->m2s_head, &p->m2s_tail,
        &p->m2s_count, (const char *)buf, count
    );
}

ssize_t pty_slave_read(int idx, void *buf, size_t count)
{
    pty_pair_t *p = pty_get(idx);
    if (!p) return -1;

    while (p->m2s_count == 0)
    {
        __asm__ volatile("sti");
        __asm__ volatile("hlt");
    }

    ssize_t n = ring_read(
    	p->m2s_buf, &p->m2s_head, &p->m2s_tail,
        &p->m2s_count, (char *)buf, count);
    return n;
}

ssize_t pty_slave_write(int idx, const void *buf, size_t count)
{
    pty_pair_t *p = pty_get(idx);
    if (!p) return -1;


    return ring_write(
    	p->s2m_buf, &p->s2m_head, &p->s2m_tail,
        &p->s2m_count, (const char *)buf, count
    );
}

static int ptmx_init_fn(void)
{
    pty_init();
    log("[PTMX]", "init /dev/ptmx\n", d);
    return 0;
}

static void ptmx_fini_fn(void) {}

static void *ptmx_open_fn(const char *path)
{
    (void)path;

    if (pty_table[0].active)
    {
        pty_table[0].master_open = 1;
        return (void *)(u64)(0 + 1);   /* handle = idx(0) + 1 */
    }
    int idx = pty_alloc();
    if (idx < 0) return NULL;
    pty_table[idx].master_open = 1;
    return (void *)(u64)(idx + 1);
}

static int ptmx_read_fn(void *handle, void *buf, size_t count, u64 offset)
{
    (void)offset;
    int idx = (int)((u64)handle) - 1;
    return (int)pty_master_read(idx, buf, count);
}

static int ptmx_write_fn(void *handle, const void *buf, size_t count, u64 offset)
{
    (void)offset;
    int idx = (int)((u64)handle) - 1;
    return (int)pty_master_write(idx, buf, count);
}

driver_module ptmx_module =
{
    .name    = "dev_ptmx",
    .mount   = "/dev/ptmx",
    .version = VERSION_NUM(0, 1, 0, 0),
    .init    = ptmx_init_fn,
    .fini    = ptmx_fini_fn,
    .open    = ptmx_open_fn,
    .read    = ptmx_read_fn,
    .write   = ptmx_write_fn,
};



static int pts0_init_fn(void)
{
    log("[PTS0]", "init /dev/pts/0\n", d);
    return 0;
}

static void pts0_fini_fn(void) {}


static void *pts0_open_fn(const char *path)
{
    (void)path;

    if (!pty_table[0].active) return NULL;
    pty_table[0].slave_open++;
    return (void *)1;    /* handle = idx(0) + 1 */
}

static int pts0_read_fn(void *handle, void *buf, size_t count, u64 offset)
{
    (void)offset;
    int idx = (int)((u64)handle) - 1;
    return (int)pty_slave_read(idx, buf, count);
}

static int pts0_write_fn(void *handle, const void *buf, size_t count, u64 offset)
{
    (void)offset;
    int idx = (int)((u64)handle) - 1;
    return (int)pty_slave_write(idx, buf, count);
}

driver_module pts0_module =
{
    .name    = "dev_pts0",
    .mount   = "/dev/pts/0",
    .version = VERSION_NUM(0, 1, 0, 0),
    .init    = pts0_init_fn,
    .fini    = pts0_fini_fn,
    .open    = pts0_open_fn,
    .read    = pts0_read_fn,
    .write   = pts0_write_fn,
};


static fs_file *stdio_files[3] = { NULL, NULL, NULL };

static int wire_fd_to_pts(int fd_slot, int pty_idx)
{
    extern void *fs_klime;

    /* resolve the pts node in devfs */
    char pts_path[32];

    str_copy(pts_path, "/dev/pts/0");
    (void)pty_idx; /* TODO: support >0 */

    fs_node *node = fs_resolve(pts_path);
    if (!node) {
        printf("[PTY] wire_fd: cannot resolve %s\n", pts_path);
        return -1;
    }

    /* allocate an fs_file */
    fs_file *file = (fs_file *)klime_create((klime_t *)fs_klime, sizeof(fs_file));
    if (!file) return -1;

    memset(file, 0, sizeof(fs_file));
    file->node  = node;
    file->pos   = 0;
    file->flags = O_RDWR;

    /* open the device */
    if (node->ops && node->ops->open)
    {
        if (node->ops->open(node, file) != 0)
        {
            klime_free((klime_t *)fs_klime, (u64 *)file);
            return -1;
        }
    }


    extern fs_file *fs_get_file(int fd);
    extern void fs_free_fd(int fd);

    /* free old entry if present */
    if (fs_get_file(fd_slot)) {
        fs_free_fd(fd_slot);
    }


    extern int fs_set_fd(int fd, fs_file *f);
    if (fs_set_fd(fd_slot, file) < 0)
    {
        klime_free((klime_t *)fs_klime, (u64 *)file);
        return -1;
    }

    stdio_files[fd_slot] = file;
    return 0;
}

int pty_attach_slave_fds(int pty_idx)
{
    /* stdin  = fd 0 */
    if (wire_fd_to_pts(0, pty_idx) < 0) return -1;
    /* stdout = fd 1 */
    if (wire_fd_to_pts(1, pty_idx) < 0) return -1;
    /* stderr = fd 2 */
    if (wire_fd_to_pts(2, pty_idx) < 0) return -1;

    printf("[PTY] attached slave fds 0/1/2 to pty %d\n", pty_idx);
    return 0;
}

int pty_open_master(int pty_idx)
{

    int fd = fs_open("/dev/ptmx", O_RDWR);
    if (fd < 0) {
        printf("[PTY] pty_open_master: fs_open(/dev/ptmx) failed\n");
        return -1;
    }

    (void)pty_idx;
    return fd;
}
