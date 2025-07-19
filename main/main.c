#include <math.h>
#include <stdbool.h>
#include "bsp/device.h"
#include "bsp/i2c.h"
#include "bsp/input.h"
#include "bsp/led.h"
#include "custom_certificates.h"
#include "effects.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "wifi_connection.h"
#include "wifi_ota.h"
#include "wifi_settings.h"

#define MAX_SPEED 2.0   // Every 500ms
#define DEF_SPEED 0.25  // Every 4s
#define MIN_SPEED 0     // Stopped
#define INC_SPEED 0.05

#define OTA_BASE_URL "https://selfsigned.ota.badge.team/bornhack2024-"

#define MAX_BRIGHTNESS 1.0
#define MIN_BRIGHTNESS 0.1
#define INC_BRIGHTNESS 0.1

#define SETTINGS_SAVE_DELAY 1000000

static uint32_t   effect_no = 0;
static float      speed     = DEF_SPEED;
static char const TAG[]     = "main";

static void load_effect_settings(nvs_handle_t nvs_handle) {
    uint32_t speed_proxy = UINT32_MAX, brightness_proxy = UINT32_MAX;
    nvs_get_u32(nvs_handle, "effect_no", &effect_no);
    if (effect_no >= effects_len) {
        effect_no = 0;
    }
    nvs_get_u32(nvs_handle, "speed", &speed_proxy);
    nvs_get_u32(nvs_handle, "brightness", &brightness_proxy);
    if (speed_proxy != UINT32_MAX) {
        speed = fminf(MAX_SPEED, speed_proxy * INC_SPEED + MIN_SPEED);
    }
    if (brightness_proxy != UINT32_MAX) {
        brightness = fminf(MAX_BRIGHTNESS, brightness_proxy * INC_BRIGHTNESS + MIN_BRIGHTNESS);
    }
}

static void store_effect_settings(nvs_handle_t nvs_handle) {
    nvs_set_u32(nvs_handle, "effect_no", effect_no);
    nvs_set_u32(nvs_handle, "speed", (speed - MIN_SPEED + 0.001) / INC_SPEED);
    nvs_set_u32(nvs_handle, "brightness", (brightness - MIN_BRIGHTNESS + 0.001) / INC_BRIGHTNESS);
    nvs_commit(nvs_handle);
}

bool wifi_stack_get_initialized(void) {
    return true;
}

static void firmware_update_callback(const char* status_text, uint8_t progress) {
    printf("OTA status changed [%u%%]: %s\r\n", progress, status_text);
    double  progress_leds            = (progress * 16.0f) / 100.0f;
    double  progress_leds_integer    = 0;
    double  progress_leds_fractional = modf(progress_leds, &progress_leds_integer);
    uint8_t led_data[3 * 16]         = {0};
    for (uint8_t led = 0; led < 16; led++) {
        if (led < (uint8_t)(progress_leds_integer)) {
            led_data[3 * led + 0] = 0;
            led_data[3 * led + 1] = 64;
            led_data[3 * led + 2] = 0;
        } else {
            led_data[3 * led + 0] = 64;
            led_data[3 * led + 1] = 0;
            led_data[3 * led + 2] = 0;
        }
    }
    led_data[3 * (uint8_t)(progress_leds_integer) + 0] = 64 * (1.0f - progress_leds_fractional);
    led_data[3 * (uint8_t)(progress_leds_integer) + 1] = 64 * (progress_leds_fractional);
    led_data[3 * (uint8_t)(progress_leds_integer) + 2] = 16;
    bsp_led_write(led_data, sizeof(led_data));
}

void app_main() {
    esp_err_t nvs_res = nvs_flash_init();
    if (nvs_res == ESP_ERR_NVS_NO_FREE_PAGES || nvs_res == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_res = nvs_flash_init();
    }

    float coeff = 0;

    nvs_handle_t nvs_handle;
    if (nvs_res != ESP_OK) {
        ESP_LOGW(TAG, "Failed to init NVS; settings cannot be stored");
    } else {
        nvs_res = nvs_open("bh24effect", NVS_READWRITE, &nvs_handle);
    }
    if (nvs_res == ESP_OK) {
        load_effect_settings(nvs_handle);
    }

    ESP_ERROR_CHECK(bsp_device_initialize());

    if (bsp_device_get_initialized_without_coprocessor()) {
        ESP_LOGE(TAG, "Coprocessor not initialized");
        return;
    }

    QueueHandle_t event_queue;
    ESP_ERROR_CHECK(bsp_input_get_queue(&event_queue));

    ESP_LOGI(TAG, "Bornhack 2024 LEDs firmware starting");

    bool do_cycle = true;

    ESP_ERROR_CHECK(initialize_custom_ca_store());

#ifdef CONFIG_BSP_TARGET_BORNHACK_2024_POV
    bool do_update = false;
    bsp_input_read_navigation_key(BSP_INPUT_NAVIGATION_KEY_UP, &do_update);

    if (do_update) {
        wifi_connection_init_stack();

        wifi_settings_t settings = {
            .ssid     = "bornhack-nat",
            .authmode = WIFI_AUTH_OPEN,
        };
        wifi_settings_set(0, &settings);

        bool do_unstable = false;
        bsp_input_read_navigation_key(BSP_INPUT_NAVIGATION_KEY_DOWN, &do_unstable);

        ota_update(do_unstable ? OTA_BASE_URL "staging.bin" : OTA_BASE_URL "stable.bin", firmware_update_callback);
        esp_restart();
    }
#endif

    int64_t prev_time           = esp_timer_get_time();
    int64_t store_settings_when = INT64_MAX;
    while (1) {
        if (nvs_res == ESP_OK && esp_timer_get_time() > store_settings_when) {
            store_settings_when = INT64_MAX;
            store_effect_settings(nvs_handle);
        }

        // Check for events.
        bsp_input_event_t event;
        if (xQueueReceive(event_queue, &event, pdMS_TO_TICKS(portTICK_PERIOD_MS * 2)) &&
            event.type == INPUT_EVENT_TYPE_NAVIGATION) {
            if (event.args_navigation.key == BSP_INPUT_NAVIGATION_KEY_SELECT ||
                event.args_navigation.key == BSP_INPUT_NAVIGATION_KEY_RETURN) {
                if (event.args_navigation.state) {
                    do_cycle = true;
                } else if (do_cycle) {
                    // If select is released without up/down presses in the mean time, go to next effect.
                    effect_no           = (effect_no + 1) % effects_len;
                    store_settings_when = esp_timer_get_time() + SETTINGS_SAVE_DELAY;
                    ESP_LOGI(TAG, "Effect changed to %u", effect_no);
                }
            } else if (event.args_navigation.state &&
                       (event.args_navigation.key == BSP_INPUT_NAVIGATION_KEY_UP ||
                        event.args_navigation.key == BSP_INPUT_NAVIGATION_KEY_VOLUME_UP)) {
                bool select;
                bsp_input_read_navigation_key(BSP_INPUT_NAVIGATION_KEY_SELECT, &select);
                if (!select) {
                    bsp_input_read_navigation_key(BSP_INPUT_NAVIGATION_KEY_RETURN, &select);
                }
                if (select) {
                    // Increase brightness.
                    brightness = fminf(1, brightness + 0.1);
                    do_cycle   = false;
                    ESP_LOGI(TAG, "Brightness increased to %.1f", brightness);
                } else {
                    // Increase speed.
                    speed = fminf(MAX_SPEED, speed + INC_SPEED);
                    ESP_LOGI(TAG, "Speed increased to %.1f", speed);
                }
                store_settings_when = esp_timer_get_time() + SETTINGS_SAVE_DELAY;
            } else if (event.args_navigation.state &&
                       (event.args_navigation.key == BSP_INPUT_NAVIGATION_KEY_DOWN ||
                        event.args_navigation.key == BSP_INPUT_NAVIGATION_KEY_VOLUME_DOWN)) {
                bool select;
                bsp_input_read_navigation_key(BSP_INPUT_NAVIGATION_KEY_SELECT, &select);
                if (!select) {
                    bsp_input_read_navigation_key(BSP_INPUT_NAVIGATION_KEY_RETURN, &select);
                }
                if (select) {
                    // Decrease brightness.
                    brightness = fmaxf(0.1, brightness - 0.1);
                    do_cycle   = false;
                    ESP_LOGI(TAG, "Brightness decreased to %.1f", brightness);
                } else {
                    // Decrease speed.
                    speed = fmaxf(MIN_SPEED, speed - INC_SPEED);
                    ESP_LOGI(TAG, "Speed decreased to %.1f", speed);
                }
                store_settings_when = esp_timer_get_time() + SETTINGS_SAVE_DELAY;
            }
        }

        int64_t time  = esp_timer_get_time();
        coeff        += speed * 0.000001 * (time - prev_time);
        prev_time     = time;
        effects[effect_no](coeff);
    }
}
