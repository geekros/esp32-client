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

// Include headers
#include "display_lvgl_basic.h"

// Define log tag
#define TAG "[client:components:display:lvgl:basic]"

// Constructor
DisplayLvglBasic::DisplayLvglBasic()
{
    // Initialize event group
    event_group = xEventGroupCreate();
}

// Destructor
DisplayLvglBasic::~DisplayLvglBasic()
{
    // Delete event group
    if (event_group)
    {
        vEventGroupDelete(event_group);
        event_group = NULL;
    }
}

// Initialize LVGL with the given display
void DisplayLvglBasic::Initialize(DisplayBasic *display)
{
    if (!display)
    {
        ESP_LOGE(TAG, "Display is null");
        return;
    }

    auto *lcd = dynamic_cast<DisplayLcdBasic *>(display);
    if (!lcd)
    {
        ESP_LOGE(TAG, "Display is not LCD");
        return;
    }

    ESP_LOGI(TAG, "LVGL init");
    lv_init();

    ESP_LOGI(TAG, "LVGL port init");
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_priority = 1;
#if CONFIG_SOC_CPU_CORES_NUM > 1
    port_cfg.task_affinity = 1;
#endif

    lvgl_port_init(&port_cfg);

    ESP_LOGI(TAG, "Add LVGL display");

    lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd->GetPanelIO(),
        .panel_handle = lcd->GetPanel(),
        .control_handle = nullptr,
        .buffer_size = static_cast<uint32_t>(lcd->width() * 20),
        .double_buffer = false,
        .trans_size = 0,
        .hres = static_cast<uint32_t>(lcd->width()),
        .vres = static_cast<uint32_t>(lcd->height()),
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = 1,
            .buff_spiram = 0,
            .sw_rotate = 0,
            .swap_bytes = 1,
            .full_refresh = 0,
            .direct_mode = 0,
        },
    };

    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
    if (!disp)
    {
        ESP_LOGE(TAG, "Failed to add LVGL display");
        return;
    }

    ESP_LOGI(TAG, "LVGL display added");

    lvgl_port_lock(0);

    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "GEEKROS");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);

    lvgl_port_unlock();

    ESP_LOGI(TAG, "LVGL initialized successfully");
}