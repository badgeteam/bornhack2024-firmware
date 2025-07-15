
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

static char const TAG[] = "main";

void app_main() {
    esp_err_t nvs_res = nvs_flash_init();
    if (nvs_res == ESP_ERR_NVS_NO_FREE_PAGES || nvs_res == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_res = nvs_flash_init();
    }

    size_t effect_no = 0;
    float  speed     = DEF_SPEED;
    float  coeff     = 0;

    nvs_handle_t nvs_handle;
    if (nvs_res != ESP_OK) {
        ESP_LOGW(TAG, "Failed to init NVS; settings cannot be stored");
    } else {
        nvs_res = nvs_open("bh24effect", NVS_READWRITE, &nvs_handle);
    }
    if (nvs_res == ESP_OK) {
        nvs_get_u32(nvs_handle, "effect_no", (uint32_t*)&effect_no);
        nvs_get_u32(nvs_handle, "speed", (uint32_t*)&speed);
        nvs_get_u32(nvs_handle, "brightness", (uint32_t*)&brightness);
    }

    ESP_ERROR_CHECK(bsp_input_initialize());
    ESP_ERROR_CHECK(bsp_led_initialize());
    ESP_ERROR_CHECK(bsp_i2c_primary_bus_initialize());

    QueueHandle_t event_queue;
    ESP_ERROR_CHECK(bsp_input_get_queue(&event_queue));

    ESP_LOGI(TAG, "Bornhack 2024 LEDs firmware starting");

    bool do_cycle = true;

    int64_t prev_time = esp_timer_get_time();
    while (1) {
        // Check for events.
        bsp_input_event_t event;
        if (xQueueReceive(event_queue, &event, 1) && event.type == INPUT_EVENT_TYPE_NAVIGATION) {
            if (event.args_navigation.key == BSP_INPUT_NAVIGATION_KEY_SELECT) {
                if (event.args_navigation.state) {
                    do_cycle = true;
                } else if (do_cycle) {
                    // If select is released without up/down presses in the mean time, go to next effect.
                    effect_no = (effect_no + 1) % effects_len;
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
            }
        }

        int64_t time  = esp_timer_get_time();
        coeff        += speed * 0.000001 * (time - prev_time);
        prev_time     = time;
        effects[effect_no](coeff);
    }
}
