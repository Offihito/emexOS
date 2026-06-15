#include "sound.h"

int sound_available(void)
{
    return audiodrv_available();
}
