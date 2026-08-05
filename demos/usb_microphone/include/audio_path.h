#ifndef AUDIO_PATH_H
#define AUDIO_PATH_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Bring up the PDM RX (mic) on I2S0, a ring buffer, and the reader task that
// drains the mic into the ring for the USB UAC input callback.
void audio_path_init(void);

// Drain up to max_len bytes of mono 16-bit PCM from the mic ring into out,
// zero-padding any shortfall. Returns the number of real (non-pad) bytes,
// which the UAC callback reports back as bytes_read.
size_t audio_path_pull_mic(uint8_t *out, size_t max_len);

// Host feature-unit mute — when set, audio_path_pull_mic returns silence.
void audio_path_set_mute_mic(bool muted);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_PATH_H
