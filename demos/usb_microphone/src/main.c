// ============================================================
// OpenMote Gen 3.3 — USB Microphone firmware entry point (ESP-IDF 5.x)
//
// usb_device_uac builds the whole UAC device and calls us back to fill the
// mic buffer. We just bridge the PDM mic ring buffer into input_cb.
//
// Ported from the Gen 3.2 build. The only hardware difference that reaches
// this firmware is the MAX98357A SD_MODE pin (GPIO21), which is software
// controlled on Gen 3.3 — see amp handling in app_main().
//
// Diagnostic LED milestones (so progress is visible without a serial log):
//   LED 1 solid : reached app_main (GPIO up)
//   LED 2 solid : audio_path_init() returned (PDM mic I2S up)
//   LED 3 solid : uac_device_init() returned ESP_OK (USB stack up)
//   LED 4 blink : heartbeat — main reached the end and scheduler is running
//   LED 4 fast-blink only (LED3 off) : uac_device_init() FAILED
// ============================================================

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "usb_device_uac.h"

#include "pin_definitions.h"
#include "audio_path.h"

static const char *TAG = "usb_mic";

// ============================================================
// UAC callbacks
// ============================================================

// Host is reading a mic frame: fill up to len bytes of mono 16-bit PCM.
static esp_err_t uac_input_cb(uint8_t *buf, size_t len, size_t *bytes_read, void *arg) {
    (void)arg;
    audio_path_pull_mic(buf, len);   // zero-pads internally
    *bytes_read = len;               // always hand back a full frame
    return ESP_OK;
}

static void uac_set_mute_cb(uint32_t mute, void *arg) {
    (void)arg;
    audio_path_set_mute_mic(mute != 0);
    ESP_LOGI(TAG, "mic mute = %u", (unsigned)mute);
}

static void uac_set_volume_cb(uint32_t volume, void *arg) {
    (void)arg;
    ESP_LOGI(TAG, "mic volume = %u", (unsigned)volume);
}

static void init_out_low(int pin) {
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
}

// ============================================================
// app_main
// ============================================================
void app_main(void) {
    init_out_low(LED_1);
    init_out_low(LED_2);
    init_out_low(LED_3);
    init_out_low(LED_4);

    // Rumble motor (GPIO39) powers on by default (it's a JTAG pin at reset);
    // claim it as a GPIO and drive it low so the haptic stays off.
    init_out_low(RMBL);

    // Gen 3.3: the MAX98357A SD_MODE pin is driven from GPIO21 (it was hard-wired
    // on in Gen 3.2). This build is mic-only, so hold the amp in shutdown rather
    // than leaving it in whatever state the R17/JP1 bias network powers up in —
    // an enabled amp with no I2S clocks just burns current and can hiss.
    // If a speaker path is ever added here, drive AMP_SD HIGH before streaming.
    init_out_low(AMP_SD);

    gpio_set_level(LED_1, 1);                 // milestone: app_main reached
    ESP_LOGI(TAG, "boot: app_main");

    audio_path_init();
    gpio_set_level(LED_2, 1);                 // milestone: PDM mic up
    ESP_LOGI(TAG, "boot: audio_path_init done");

    uac_device_config_t cfg = {
        .output_cb     = NULL,                // mic-only: no speaker
        .input_cb      = uac_input_cb,
        .set_mute_cb   = uac_set_mute_cb,
        .set_volume_cb = uac_set_volume_cb,
        .cb_ctx        = NULL,
    };
    esp_err_t err = uac_device_init(&cfg);
    bool uac_ok = (err == ESP_OK);
    ESP_LOGI(TAG, "boot: uac_device_init -> %s", esp_err_to_name(err));

    if (uac_ok) {
        gpio_set_level(LED_3, 1);             // milestone: USB stack up
        ESP_LOGI(TAG, "USB microphone ready");
    }

    // Heartbeat so liveness is visible. Slow blink if all OK, fast if UAC failed.
    int period_ms = uac_ok ? 500 : 100;
    bool on = false;
    for (;;) {
        on = !on;
        gpio_set_level(LED_4, on ? 1 : 0);
        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
}
