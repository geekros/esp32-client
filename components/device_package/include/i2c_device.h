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

#ifndef I2C_DEVICE_H
#define I2C_DEVICE_H

// Include standard headers
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include driver headers
#include <driver/i2c_master.h>

// Define I2C device class
class I2CDevice
{
protected:
    // Define I2C device handle
    i2c_master_dev_handle_t i2c_device_;

    // Private methods
    void WriteReg(uint8_t reg, uint8_t value);
    uint8_t ReadReg(uint8_t reg);
    void ReadRegs(uint8_t reg, uint8_t *buffer, size_t length);

public:
    // Public methods
    I2CDevice(i2c_master_bus_handle_t i2c_bus, uint8_t addr);
};

#endif