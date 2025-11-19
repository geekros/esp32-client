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

// I2C device handle
extern i2c_master_dev_handle_t i2c_device;

// I2C device constructor
void I2cDevice(i2c_master_bus_handle_t i2c_bus, uint8_t addr);

// Function to write a value to a register over I2C
void WriteReg(uint8_t reg, uint8_t value);

// Function to read a value from a register over I2C
uint8_t ReadReg(uint8_t reg);

// Function to read multiple values from a register over I2C
void ReadRegs(uint8_t reg, uint8_t *buffer, size_t length);

#endif