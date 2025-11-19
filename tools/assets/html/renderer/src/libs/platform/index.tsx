// Copyright 2025 GEEKROS, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This file contains utility functions for platform detection and manipulation.
export function IsElectron() {
    return typeof process !== "undefined" && process.versions && process.versions.electron;
}

// Function to check if the platform is a specific type
export function IsVsCode() {
    return typeof acquireVsCodeApi === "function";
}

// Function to check if the platform is a specific type
export function GetPlatform() {
    const userAgent = (navigator as any).userAgent;
    if (userAgent.includes("Windows")) {
        return "windows";
    }
    if (userAgent.includes("Mac")) {
        return "mac";
    }
    if (userAgent.includes("Linux")) {
        return "linux";
    }
    return "-";
}

// Function to check if the platform is a specific target
export function CheckPlatform(target: string) {
    const userAgent: string = (navigator as any).userAgent;
    return userAgent.toLocaleLowerCase().includes(target.toLocaleLowerCase());
}
