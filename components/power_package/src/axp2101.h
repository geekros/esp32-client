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

#ifndef POWER_AXP2101_H
#define POWER_AXP2101_H

// Include standard libraries
#include <stdio.h>
#include <string.h>

// Include ESP libraries
#include <esp_log.h>
#include <esp_err.h>

// Include headers
#include "i2c_device.h"

// AXP2101 initialization function
void Axp2101Init(i2c_master_bus_handle_t i2c_bus);

// Function to get battery current direction
int GetBatteryCurrentDirection();

// Function to check if the battery is charging
bool IsCharging();

// Function to check if the battery is discharging
bool IsDischarging();

// Function to check if charging is done
bool IsChargingDone();

// Function to get battery level percentage
int GetBatteryLevel();

// Function to get temperature
float GetTemperature();

// Function to power off the device
void PowerOff();

#endif