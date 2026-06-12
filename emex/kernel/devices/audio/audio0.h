#ifndef DEVICE_AUDIO0_H
#define DEVICE_AUDIO0_H

/*
 * kernel/devices/audio/audio0.h
 *
 * /dev/audio0 — generic PCM output device.
 *
 * The module talks exclusively to the audio layer (audiodrv_*).
 * It has zero knowledge of AC97, HDA, or any other hardware driver.
 */

#include <kernel/module/module.h>

extern driver_module audio0_module;

#endif /* DEVICE_AUDIO0_H */
