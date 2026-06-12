#ifndef SOUND_LAYER_H
#define SOUND_LAYER_H

/*
 * drivers/sound/layer.h
 *
 * Universal audio driver layer — mirrors drivers/net/layers.h
 *
 * The device node (/dev/audio0) and any kernel audio consumer
 * talk exclusively to this API.  The layer selects and delegates
 * to a concrete hardware driver (AC97, HDA, SB16, …) at init time.
 * Adding a new hardware driver only requires changes in layer.c.
 */

#include <types.h>

/* ------------------------------------------------------------------ */
/* Hardware-driver vtable                                              */
/* ------------------------------------------------------------------ */

typedef struct {
    /*
     * Probe and initialise the hardware.
     * Return 0 on success, -1 if the device is not present.
     */
    int  (*init)(void);

    /*
     * Write PCM audio data.
     * buf  : signed 16-bit stereo samples, little-endian, interleaved
     * len  : byte count
     * Returns bytes written, or -1 on error.
     */
    int  (*write)(const void *buf, size_t len);

    /*
     * Set output sample rate in Hz.
     * Returns 0 on success, -1 if the codec rejects the rate.
     */
    int  (*set_rate)(u32 hz);

    /*
     * Set attenuation (0 = full volume, 63 = near-silent).
     */
    void (*set_volume)(u8 master_atten, u8 pcm_atten);

    /*
     * Returns 1 if the driver was successfully initialised, 0 otherwise.
     */
    int  (*present)(void);

    /*
     * Synthesise and play a pure sine tone.
     * freq_hz    : frequency in Hz  (e.g. 440, 1000)
     * duration_ms: duration in milliseconds
     * Returns total bytes written, or -1 on error.
     */
    int  (*beep)(u32 freq_hz, u32 duration_ms);
} audio_driver_t;

/* ------------------------------------------------------------------ */
/* Public API — used by kernel/devices/audio/audio0.c and             */
/* any other kernel code that needs audio output.                     */
/* Never include an ac97 header outside of layer.c.                  */
/* ------------------------------------------------------------------ */

/*
 * Probe all known drivers in order and latch the first one that works.
 * Called once from kernel.c (or from audio0 init).
 * Returns 0 on success, -1 if no audio hardware was found.
 */
int  audiodrv_init(void);

/*
 * Write PCM data through the active driver.
 * Returns bytes written, or -1 if no driver is active.
 */
int  audiodrv_write(const void *buf, size_t len);

/*
 * Set the sample rate on the active driver.
 * Returns 0 on success, -1 on failure or if no driver is active.
 */
int  audiodrv_set_rate(u32 hz);

/*
 * Adjust output volume.
 */
void audiodrv_set_volume(u8 master_atten, u8 pcm_atten);

/*
 * Returns 1 if an audio driver is active, 0 otherwise.
 * Used by audio0 open() to deny access when there is no hardware.
 */
int  audiodrv_available(void);

/*
 * Synthesise and play a pure sine tone through the active driver.
 * freq_hz    : tone frequency in Hz  (e.g. 440 = concert A, 1000 = UI beep)
 * duration_ms: duration in milliseconds
 * Returns total bytes written, or -1 if no driver is active.
 */
int  audiodrv_beep(u32 freq_hz, u32 duration_ms);

#endif /* SOUND_LAYER_H */
