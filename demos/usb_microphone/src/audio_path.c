// ============================================================
// Audio path (mic only) — LMD4030T261 PDM MEMS mic → ring buffer.
//
// ESP-IDF 5.x i2s_pdm driver. PDM RX runs on I2S port 0. A reader task
// continuously pulls mono 16-bit PCM at 16 kHz into a stream buffer that the
// USB UAC input callback drains on demand.
// ============================================================

#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"
#include "driver/i2s_pdm.h"
#include "esp_log.h"

#include "pin_definitions.h"
#include "audio_path.h"

static const char *TAG = "audio_path";

// ---- Configuration -----------------------------------------
#define MIC_SAMPLE_RATE   16000
#define MIC_FRAME_SAMPLES 160                 // 10 ms @ 16 kHz
#define MIC_FRAME_BYTES   (MIC_FRAME_SAMPLES * 2)
#define MIC_RING_BYTES    (MIC_FRAME_BYTES * 20)   // ~200 ms cushion

// ---- State -------------------------------------------------
static i2s_chan_handle_t    rx_chan  = NULL;
static StreamBufferHandle_t mic_ring = NULL;
static volatile bool        mic_muted = false;

// ============================================================
// I2S PDM RX init
// ============================================================
static void mic_i2s_init(void) {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_chan));

    i2s_pdm_rx_config_t pdm_cfg = {
        .clk_cfg  = I2S_PDM_RX_CLK_DEFAULT_CONFIG(MIC_SAMPLE_RATE),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = MIC_CLK,
            .din = MIC_DATA,
            .invert_flags = { .clk_inv = false },
        },
    };
    // L/R strap = left on the OpenMote PCB.
    pdm_cfg.slot_cfg.slot_mask = I2S_PDM_SLOT_LEFT;

    ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_chan, &pdm_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
    ESP_LOGI(TAG, "PDM mic up: %d Hz mono 16-bit (clk=%d din=%d)", MIC_SAMPLE_RATE, MIC_CLK, MIC_DATA);
}

// ============================================================
// Reader task: I2S → ring
// ============================================================
static void mic_task(void *arg) {
    (void)arg;
    uint8_t buf[MIC_FRAME_BYTES];
    size_t  bytes_read;

    for (;;) {
        esp_err_t err = i2s_channel_read(rx_chan, buf, sizeof(buf), &bytes_read, pdMS_TO_TICKS(100));
        if (err != ESP_OK || bytes_read == 0) {
            continue;
        }

        // Drop oldest data if the host isn't draining (ring full).
        size_t space = xStreamBufferSpacesAvailable(mic_ring);
        if (bytes_read > space) {
            uint8_t scratch[64];
            size_t drop = bytes_read - space;
            while (drop > 0) {
                size_t chunk = drop > sizeof(scratch) ? sizeof(scratch) : drop;
                size_t n = xStreamBufferReceive(mic_ring, scratch, chunk, 0);
                if (n == 0) break;
                drop -= n;
            }
        }
        xStreamBufferSend(mic_ring, buf, bytes_read, 0);
    }
}

// ============================================================
// Public API
// ============================================================
void audio_path_init(void) {
    mic_ring = xStreamBufferCreate(MIC_RING_BYTES, MIC_FRAME_BYTES);
    configASSERT(mic_ring);

    mic_i2s_init();

    xTaskCreatePinnedToCore(mic_task, "mic", 4096, NULL, 5, NULL, 1);
}

size_t audio_path_pull_mic(uint8_t *out, size_t max_len) {
    if (!mic_ring || mic_muted) {
        memset(out, 0, max_len);
        return mic_muted ? max_len : 0;
    }
    size_t got = xStreamBufferReceive(mic_ring, out, max_len, 0);
    if (got < max_len) {
        memset(out + got, 0, max_len - got);
    }
    return max_len;
}

void audio_path_set_mute_mic(bool muted) { mic_muted = muted; }
