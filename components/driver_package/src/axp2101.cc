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
#include "axp2101.h"

// Include ESP libraries
static bool axp2101_initialized = false;

// AXP2101 initialization function
void Axp2101Init(i2c_master_bus_handle_t i2c_bus)
{
    // Check if already initialized
    if (axp2101_initialized)
    {
        return;
    }

    // Initialize the I2C device with AXP2101 address 0x34
    I2cDevice(i2c_bus, 0x34);

    // Update initialization flag
    axp2101_initialized = true;
}

// Function to get battery current direction
int GetBatteryCurrentDirection()
{
    // Read the battery current direction from register 0x01
    return (ReadReg(0x01) & 0b01100000) >> 5;
}

// Function to check if the battery is charging
bool IsCharging()
{
    // Return true if the battery current direction indicates charging
    return GetBatteryCurrentDirection() == 1;
}

// Function to check if the battery is discharging
bool IsDischarging()
{
    // Return true if the battery current direction indicates discharging
    return GetBatteryCurrentDirection() == 2;
}

// Function to check if charging is done
bool IsChargingDone()
{
    // Read the charging status from register 0x01
    uint8_t value = ReadReg(0x01);

    // Return true if charging is done
    return (value & 0b00000111) == 0b00000100;
}

// Function to get battery level percentage
int GetBatteryLevel()
{
    // Read and return the battery level from register 0xA4
    return ReadReg(0xA4);
}

// Function to get temperature
float GetTemperature()
{
    // Read and return the temperature from register 0xA5
    return ReadReg(0xA5);
}

// Function to power off the device
void PowerOff()
{
    // Set the power off bit in register 0x10
    uint8_t value = ReadReg(0x10);

    // Set bit 0 to 1 to power off
    value = value | 0x01;

    // Write back the modified value to register 0x10
    WriteReg(0x10, value);
}