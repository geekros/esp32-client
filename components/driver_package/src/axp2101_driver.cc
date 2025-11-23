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
#include "axp2101_driver.h"

// Define log tag
#define TAG "[client:components:driver:axp2101]"

// Constructor
AXP2101Driver::AXP2101Driver(i2c_master_bus_handle_t i2c_bus, uint8_t addr) : I2CDevice(i2c_bus, addr)
{
}

// Private method to read battery current direction
int AXP2101Driver::GetBatteryCurrentDirection()
{
    return (ReadReg(0x01) & 0b01100000) >> 5;
}

// Public method to check if the battery is charging
bool AXP2101Driver::IsCharging()
{
    return GetBatteryCurrentDirection() == 1;
}

// Public method to check if the battery is discharging
bool AXP2101Driver::IsDischarging()
{
    return GetBatteryCurrentDirection() == 2;
}

// Public method to check if charging is done
bool AXP2101Driver::IsChargingDone()
{
    uint8_t value = ReadReg(0x01);
    return (value & 0b00000111) == 0b00000100;
}

// Public method to get battery level
int AXP2101Driver::GetBatteryLevel()
{
    return ReadReg(0xA4);
}

// Public method to get temperature
float AXP2101Driver::GetTemperature()
{
    return ReadReg(0xA5);
}

// Public method to power off the device
void AXP2101Driver::PowerOff()
{
    uint8_t value = ReadReg(0x10);
    value = value | 0x01;
    WriteReg(0x10, value);
}