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
#include "i2c_device.h"

// I2C device handle
i2c_master_dev_handle_t i2c_device;

// I2C device constructor
void I2cDevice(i2c_master_bus_handle_t i2c_bus, uint8_t addr)
{
    // Configure the I2C device
    i2c_device_config_t i2c_device_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 400 * 1000,
        .scl_wait_us = 0,
        .flags = {
            .disable_ack_check = 0,
        },
    };

    // Add the I2C device to the bus
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &i2c_device_cfg, &i2c_device));
}

// Write a value to a register over I2C
void WriteReg(uint8_t reg, uint8_t value)
{
    // Create a buffer with the register address and value
    uint8_t buffer[2] = {reg, value};

    // Write the buffer to the I2C device
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_device, buffer, 2, 100));
}

// Read a value from a register over I2C
uint8_t ReadReg(uint8_t reg)
{
    // Create a buffer to hold the read value
    uint8_t buffer[1];

    // Read the value from the I2C device
    ESP_ERROR_CHECK(i2c_master_transmit_receive(i2c_device, &reg, 1, buffer, 1, 100));

    // Return the read value
    return buffer[0];
}

// Read multiple values from a register over I2C
void ReadRegs(uint8_t reg, uint8_t *buffer, size_t length)
{
    // Read multiple values from the I2C device
    ESP_ERROR_CHECK(i2c_master_transmit_receive(i2c_device, &reg, 1, buffer, length, 100));
}