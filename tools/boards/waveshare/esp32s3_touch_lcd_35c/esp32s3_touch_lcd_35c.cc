/*
Copyright 2025 GEEKROS, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// Include standard libraries
#include <string>

// Include ESP libraries
#include <esp_log.h>
#include <esp_err.h>
#include <esp_lvgl_port.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_touch_ft5x06.h>

// Include board configuration header
#include "board_config.h"

// Include board basic header
#include "board_basic.h"

// Include driver headers
#include "driver/i2c_master.h"

// Include IO expander header
#include "esp_io_expander_tca9554.h"

// Include component headers
#include "power_basic.h"
#include "i2c_device.h"
#include "axp2101_driver.h"
#include "es8311_audio_codec.h"
#include "display_lcd_basic.h"

// Define log tag
#define TAG "[client:waveshare:board]"

// Define lcd init command structure
typedef struct
{
    int cmd;
    const void *data;
    size_t data_bytes;
    unsigned int delay_ms;
} board_lcd_init_cmd_t;

// Define ST7796 vendor configuration structure
typedef struct
{
    const board_lcd_init_cmd_t *init_cmds;
    uint16_t init_cmds_size;
} board_lcd_vendor_config_t;

board_lcd_init_cmd_t st7796_lcd_init_cmds[] = {
    {0x11, (uint8_t[]){0x00}, 0, 120},

    // {0x36, (uint8_t []){ 0x08 }, 1, 0},

    {0x3A, (uint8_t[]){0x05}, 1, 0},
    {0xF0, (uint8_t[]){0xC3}, 1, 0},
    {0xF0, (uint8_t[]){0x96}, 1, 0},
    {0xB4, (uint8_t[]){0x01}, 1, 0},
    {0xB7, (uint8_t[]){0xC6}, 1, 0},
    {0xC0, (uint8_t[]){0x80, 0x45}, 2, 0},
    {0xC1, (uint8_t[]){0x13}, 1, 0},
    {0xC2, (uint8_t[]){0xA7}, 1, 0},
    {0xC5, (uint8_t[]){0x0A}, 1, 0},
    {0xE8, (uint8_t[]){0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33}, 8, 0},
    {0xE0, (uint8_t[]){0xD0, 0x08, 0x0F, 0x06, 0x06, 0x33, 0x30, 0x33, 0x47, 0x17, 0x13, 0x13, 0x2B, 0x31}, 14, 0},
    {0xE1, (uint8_t[]){0xD0, 0x0A, 0x11, 0x0B, 0x09, 0x07, 0x2F, 0x33, 0x47, 0x38, 0x15, 0x16, 0x2C, 0x32}, 14, 0},
    {0xF0, (uint8_t[]){0x3C}, 1, 0},
    {0xF0, (uint8_t[]){0x69}, 1, 120},
    {0x21, (uint8_t[]){0x00}, 0, 0},
    {0x29, (uint8_t[]){0x00}, 0, 0},
};

// Pmic class definition inheriting from AXP2101Driver
class Pmic : public AXP2101Driver
{
public:
    // Constructor
    Pmic(i2c_master_bus_handle_t i2c_bus, uint8_t addr) : AXP2101Driver(i2c_bus, addr)
    {
        WriteReg(0x22, 0b110); // PWRON > OFFLEVEL as POWEROFF Source enable
        WriteReg(0x27, 0x10);  // hold 4s to power off

        // Disable All DCs but DC1
        WriteReg(0x80, 0x01);
        // Disable All LDOs
        WriteReg(0x90, 0x00);
        WriteReg(0x91, 0x00);

        // Set DC1 to 3.3V
        WriteReg(0x82, (3300 - 1500) / 100);

        // Set ALDO1 to 3.3V
        WriteReg(0x92, (3300 - 500) / 100);

        WriteReg(0x96, (1500 - 500) / 100);
        WriteReg(0x97, (2800 - 500) / 100);

        // Enable ALDO1 BLDO1 BLDO2
        WriteReg(0x90, 0x31);

        WriteReg(0x64, 0x02); // CV charger voltage setting to 4.1V

        WriteReg(0x61, 0x02); // set Main battery precharge current to 50mA
        WriteReg(0x62, 0x08); // set Main battery charger current to 400mA ( 0x08-200mA, 0x09-300mA, 0x0A-400mA )
        WriteReg(0x63, 0x01); // set Main battery term charge current to 25mA
    }
};

// CustomBoard class definition
class CustomBoard : public BoardBasic
{
private:
    // Define private members
    Pmic *pmic = nullptr;

    i2c_master_bus_handle_t i2c_bus = nullptr;

    esp_io_expander_handle_t io_expander = nullptr;

    PowerBasic *power_timer_instance = nullptr;

    DisplayLcdBasic *display_ = nullptr;

    // Private method to initialize power timer
    void InitializePower()
    {
        // Get PowerBasic instance
        power_timer_instance = new PowerBasic(-1);

        // Define power timer callbacks
        auto enter_sleep_callback = [this]()
        {
            ESP_LOGI(TAG, "Entering sleep mode...");
        };

        // Define exit sleep callback
        auto exit_sleep_callback = [this]()
        {
            ESP_LOGI(TAG, "Exiting sleep mode...");
        };

        // Define shutdown request callback
        auto shutdown_request_callback = [this]()
        {
            ESP_LOGI(TAG, "Shutdown requested...");

            // Power off the device using PMIC
            if (pmic)
            {
                // Call PMIC PowerOff method
                pmic->PowerOff();
            }
        };

        // Set power timer callbacks
        power_timer_instance->OnEnterSleepMode(enter_sleep_callback);
        power_timer_instance->OnExitSleepMode(exit_sleep_callback);
        power_timer_instance->OnShutdownRequest(shutdown_request_callback);
    }

    // Private method to initialize I2C
    void InitializeI2C()
    {
        // Configure I2C master bus
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = (i2c_port_t)I2C_NUM_0,
            .sda_io_num = BOARD_AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = BOARD_AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };

        // Create I2C master bus
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus));
    }

    // Private method to initialize TCA9554 I/O expander
    void InitializeTCA9554(void)
    {
        // Create TCA9554 I/O expander instance
        esp_err_t ret = esp_io_expander_new_i2c_tca9554(i2c_bus, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000, &io_expander);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to initialize TCA9554 I/O expander: %s", esp_err_to_name(ret));
        }

        // Set pin directions for TCA9554 I/O expander
        ret = esp_io_expander_set_dir(io_expander, IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1, IO_EXPANDER_OUTPUT);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set TCA9554 pin directions: %s", esp_err_to_name(ret));
        }
        vTaskDelay(pdMS_TO_TICKS(100));

        // Set pin levels for TCA9554 I/O expander
        ret = esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1, 0);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set TCA9554 pin levels: %s", esp_err_to_name(ret));
        }
        vTaskDelay(pdMS_TO_TICKS(100));

        // Set pin levels for TCA9554 I/O expander
        ret = esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_1, 1);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set TCA9554 pin levels: %s", esp_err_to_name(ret));
        }
    }

    // Private method to initialize AXP2101 PMIC
    void InitializeAXP2101()
    {
        // Create AXP2101 PMIC instance
        pmic = new Pmic(i2c_bus, 0x34);
    }

    // Private method to initialize SPI (empty for this board)
    void InitializeSPI()
    {
        // Define SPI bus configuration
        spi_bus_config_t buscfg = {};

        // Configure SPI bus pins
        buscfg.mosi_io_num = BOARD_DISPLAY_MOSI_PIN;
        buscfg.miso_io_num = BOARD_DISPLAY_MISO_PIN;
        buscfg.sclk_io_num = BOARD_DISPLAY_CLK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = BOARD_DISPLAY_WIDTH * BOARD_DISPLAY_HEIGHT * sizeof(uint16_t);

        // Initialize SPI bus
        ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    // Private method to initialize display (empty for this board)
    void InitializeDisplay()
    {
        // Define panel IO and panel handles
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        // Configure panel IO for SPI
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = BOARD_DISPLAY_CS_PIN;
        io_config.dc_gpio_num = BOARD_DISPLAY_DC_PIN;
        io_config.spi_mode = BOARD_DISPLAY_SPI_MODE;
        io_config.pclk_hz = 40 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI3_HOST, &io_config, &panel_io));

        // Configure panel device
        board_lcd_vendor_config_t st7796_vendor_config = {
            .init_cmds = st7796_lcd_init_cmds,
            .init_cmds_size = sizeof(st7796_lcd_init_cmds) / sizeof(board_lcd_init_cmd_t),
        };

        // Create new ST7789 panel
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = BOARD_DISPLAY_RST_PIN;
        panel_config.rgb_ele_order = BOARD_DISPLAY_RGB_ORDER;
        panel_config.bits_per_pixel = 16;
        panel_config.vendor_config = &st7796_vendor_config;

        // Create new panel instance
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));

        // Initialize panel
        esp_lcd_panel_reset(panel);

        // Initialize panel
        esp_lcd_panel_init(panel);
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
        esp_lcd_panel_invert_color(panel, BOARD_DISPLAY_INVERT_COLOR);
        esp_lcd_panel_swap_xy(panel, BOARD_DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, BOARD_DISPLAY_MIRROR_X, BOARD_DISPLAY_MIRROR_Y);

        ESP_LOGI(TAG, "Test draw: white screen");

        std::vector<uint16_t> line(BOARD_DISPLAY_WIDTH, 0xFFFF);
        for (int y = 0; y < BOARD_DISPLAY_HEIGHT; y++)
        {
            esp_lcd_panel_draw_bitmap(panel, 0, y, BOARD_DISPLAY_WIDTH, y + 1, line.data());
        }

        // Set the display to on
        ESP_LOGI(TAG, "Turning display on");
        {
            esp_err_t __err = esp_lcd_panel_disp_on_off(panel, true);
            if (__err == ESP_ERR_NOT_SUPPORTED)
            {
                ESP_LOGW(TAG, "Panel does not support disp_on_off; assuming ON");
            }
            else
            {
                ESP_ERROR_CHECK(__err);
            }
        }

        // Create DisplayLcdBasic instance
        display_ = new DisplayLcdBasic(panel_io, panel, BOARD_DISPLAY_WIDTH, BOARD_DISPLAY_HEIGHT);
    }

    // Private method to initialize display and touch (empty for this board)
    void InitializeDisplayTouch()
    {
        // Create touch controller instance
        esp_lcd_touch_handle_t tp;
        esp_lcd_touch_config_t tp_cfg = {};
        tp_cfg.x_max = BOARD_DISPLAY_WIDTH;
        tp_cfg.y_max = BOARD_DISPLAY_HEIGHT;
        tp_cfg.rst_gpio_num = GPIO_NUM_NC;
        tp_cfg.int_gpio_num = GPIO_NUM_NC;
        tp_cfg.levels.reset = 0;
        tp_cfg.levels.interrupt = 0;
        tp_cfg.flags.swap_xy = 1;
        tp_cfg.flags.mirror_x = 1;
        tp_cfg.flags.mirror_y = 1;

        // Create I2C panel IO for touch controller
        esp_lcd_panel_io_handle_t tp_io_handle = NULL;
        esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
        tp_io_config.scl_speed_hz = 400 * 1000;

        // Create new panel IO for I2C
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &tp_io_config, &tp_io_handle));

        // Create new FT5x06 touch controller instance
        ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &tp));

        // Configure LVGL touch input device
        const lvgl_port_touch_cfg_t touch_cfg = {
            .disp = lv_display_get_default(),
            .handle = tp,
        };

        // Add touch input device to LVGL
        lvgl_port_add_touch(&touch_cfg);
    }

public:
    // Override Initialization method
    void Initialization() override
    {
        ESP_LOGI(TAG, "%s %s", BOARD_NAME, CONFIG_IDF_TARGET);

        // Initialize power timer
        InitializePower();

        // Initialize I2C
        InitializeI2C();

        // Initialize TCA9554 I/O expander
        InitializeTCA9554();

        // Initialize AXP2101 PMIC
        InitializeAXP2101();

        // Initialize SPI
        InitializeSPI();

        // Initialize display
        InitializeDisplay();

        // Initialize display touch
        // InitializeDisplayTouch();
    }

    // Override GetAudioCodec method
    virtual AudioCodec *GetAudioCodec() override
    {
        // Create ES8311 audio codec instance
        static ES8311AudioCodec audio_codec(i2c_bus, I2C_NUM_0, BOARD_AUDIO_INPUT_SAMPLE_RATE, BOARD_AUDIO_OUTPUT_SAMPLE_RATE, BOARD_I2S_MCLK_GPIO, BOARD_I2S_BCLK_GPIO, BOARD_I2S_WS_GPIO, BOARD_I2S_DOUT_GPIO, BOARD_I2S_DIN_GPIO, BOARD_AUDIO_CODEC_PA_PIN, BOARD_AUDIO_CODEC_ES8311_ADDR);

        // Return audio codec instance
        return &audio_codec;
    }

    // Override GetDisplay method
    virtual DisplayBasic *GetDisplay() override
    {
        // Return display instance
        return display_;
    }
};

// Factory function to create a BoardBasic instance
BoardBasic *CreateBoard()
{
    // Return a static CustomBoard instance
    static CustomBoard instance;

    // Return the instance
    return &instance;
}