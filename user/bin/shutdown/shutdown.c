#include <emx/system.h>
#include <unistd.h>

int main(void) {
    reboot(RSYSTEM_CMD_POWEROFF);
    _exit(1);
}