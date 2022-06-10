#include <sdkconfig.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <soc/gpio_reg.h>
#include <soc/gpio_sig_map.h>
#include <soc/gpio_struct.h>
#include <soc/spi_reg.h>
#include <esp_err.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/gpio.h>

#include "include/driver_ili9341.h"
#include "esp32-component-spi-ili9341/include/ili9341.h"

#define TAG "ILI9341"

static ILI9341 dev_ili9341 = {0};

void ili9341_set_lcd_mode(bool mode) {
    ESP_LOGI(TAG, "LCD mode switch to %s", mode ? "Parallel" : "SPI");
    esp_err_t res = gpio_set_level(CONFIG_PIN_NUM_ILI9341_MODE, mode);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Setting LCD mode failed");
    }
}

esp_err_t driver_ili9341_init(void)
{
	static bool driver_ili9341_init_done = false;
	if (driver_ili9341_init_done) return ESP_OK;
	ESP_LOGD(TAG, "init called");
	
	dev_ili9341.spi_bus               = VSPI_HOST;
    dev_ili9341.pin_cs                = CONFIG_PIN_NUM_ILI9341_CS;
    dev_ili9341.pin_dcx               = CONFIG_PIN_NUM_ILI9341_DCX;
    dev_ili9341.pin_reset             = CONFIG_PIN_NUM_ILI9341_RESET;
    dev_ili9341.rotation              = CONFIG_ILI9341_ORIENTATION;
    dev_ili9341.color_mode            = CONFIG_ILI9341_COLOR_SWAP;      // Blue and red channels are swapped
    dev_ili9341.spi_speed             = 40000000;  // 40MHz
    dev_ili9341.spi_max_transfer_size = CONFIG_BUS_VSPI_MAX_TRANSFERSIZE;
    #if CONFIG_PIN_NUM_ILI9341_MODE >= 0
	dev_ili9341.callback              = ili9341_set_lcd_mode;  // Callback for changing LCD mode between ESP32 and FPGA
	#endif
	esp_err_t res;
	
	//Init mode pin
	#if CONFIG_PIN_NUM_ILI9341_MODE >= 0
		res = gpio_set_direction(CONFIG_PIN_NUM_ILI9341_MODE, GPIO_MODE_OUTPUT);
		if (res != ESP_OK) return res;
	#endif

	//Initialize backlight GPIO pin
	#if CONFIG_PIN_NUM_ILI9341_BACKLIGHT >= 0
		res = gpio_set_direction(CONFIG_PIN_NUM_ILI9341_BACKLIGHT, GPIO_MODE_OUTPUT);
		if (res != ESP_OK) return res;
	#endif

	//Turn off backlight
	res = driver_ili9341_set_backlight(false);
	if (res != ESP_OK) return res;
	
	res = ili9341_init(&dev_ili9341);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing LCD failed");
        return res;
    }

	//Turn on backlight
	res = driver_ili9341_set_backlight(true);
	if (res != ESP_OK) return res;

	driver_ili9341_init_done = true;
	ESP_LOGD(TAG, "init done");
	return ESP_OK;
}

esp_err_t driver_ili9341_set_backlight(bool state)
{
	#if CONFIG_PIN_NUM_ILI9341_BACKLIGHT >= 0
		return gpio_set_level(CONFIG_PIN_NUM_ILI9341_BACKLIGHT, !state);
	#else
		return ESP_OK;
	#endif
}

esp_err_t driver_ili9341_write_partial(const uint8_t *buffer, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
	return ili9341_write_partial(&dev_ili9341, buffer, x0, y0, x1-x0, y1-y0);
}