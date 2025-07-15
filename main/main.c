
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include <nvs_flash.h>
#include "bsp/i2c.h"
#include "bsp/input.h"
#include "bsp/led.h"
#include "effects.h"

#define MAX_SPEED 2.0   // Every 500ms
#define DEF_SPEED 0.25  // Every 4s
#define MIN_SPEED 0.1   // Every 10s
#define INC_SPEED 0.05

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

    ESP_ERROR_CHECK(bsp_input_initialize());
    ESP_ERROR_CHECK(bsp_led_initialize());
    ESP_ERROR_CHECK(bsp_i2c_primary_bus_initialize());

    QueueHandle_t event_queue;
    ESP_ERROR_CHECK(bsp_input_get_queue(&event_queue));

    ESP_LOGI(TAG, "Bornhack 2024 LEDs firmware starting");

    bool do_cycle = true;

    int64_t store_settings_when = INT64_MAX;
    int64_t prev_time           = esp_timer_get_time();
    while (1) {
        if (nvs_res == ESP_OK && esp_timer_get_time() > store_settings_when) {
            store_settings_when = INT64_MAX;
            store_effect_settings(nvs_handle);
        }

        // Check for events.
        bsp_input_event_t event;
        if (xQueueReceive(event_queue, &event, 1) && event.type == INPUT_EVENT_TYPE_NAVIGATION) {
            if (event.args_navigation.key == BSP_INPUT_NAVIGATION_KEY_SELECT) {
                if (event.args_navigation.state) {
                    do_cycle = true;
                } else if (do_cycle) {
                    // If select is released without up/down presses in the mean time, go to next effect.
                    effect_no           = (effect_no + 1) % effects_len;
                    store_settings_when = esp_timer_get_time() + SETTINGS_SAVE_DELAY;
                }
            } else if (event.args_navigation.state && event.args_navigation.key == BSP_INPUT_NAVIGATION_KEY_UP) {
                bool select;
                bsp_input_read_navigation_key(BSP_INPUT_NAVIGATION_KEY_SELECT, &select);
                if (select) {
                    // Increase brightness.
                    brightness = fminf(1, brightness + 0.1);
                    do_cycle   = false;
                } else {
                    // Increase speed.
                    speed = fminf(MAX_SPEED, speed + INC_SPEED);
                }
                store_settings_when = esp_timer_get_time() + SETTINGS_SAVE_DELAY;
            } else if (event.args_navigation.state && event.args_navigation.key == BSP_INPUT_NAVIGATION_KEY_DOWN) {
                bool select;
                bsp_input_read_navigation_key(BSP_INPUT_NAVIGATION_KEY_SELECT, &select);
                if (select) {
                    // Decrease brightness.
                    brightness = fmaxf(0.1, brightness - 0.1);
                    do_cycle   = false;
                } else {
                    // Decrease speed.
                    speed = fmaxf(MIN_SPEED, speed - INC_SPEED);
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
