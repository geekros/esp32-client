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

import { clsx, type ClassValue } from "clsx";
import React from "react";
import { twMerge } from "tailwind-merge";

// This file contains utility functions for class name manipulation.
export function cn(...inputs: ClassValue[]) {
    return twMerge(clsx(inputs));
}

// This function removes leading and trailing whitespace from a string and also removes any newline or carriage return characters.
export function RemoveTrim(value: string): string {
    return value.replace(/[\n\r]/g, "").trim();
}

// This function checks if the provided email is in a valid format.
export function CheckEmail(email: string): boolean {
    const regex = /^(([^<>()\[\]\\.,;:\s@"]+(\.[^<>()\[\]\\.,;:\s@"]+)*)|(".+"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$/;
    return regex.test(email);
}

// This hook detects if the current device is a mobile device based on the screen width.
export function useIsMobile() {
    const [isMobile, setIsMobile] = React.useState<boolean | undefined>(undefined);
    const MOBILE_BREAKPOINT = 768;

    React.useEffect(() => {
        const mql = window.matchMedia(`(max-width: ${MOBILE_BREAKPOINT - 1}px)`);
        const onChange = () => {
            setIsMobile(window.innerWidth < MOBILE_BREAKPOINT);
        };
        mql.addEventListener("change", onChange);
        setIsMobile(window.innerWidth < MOBILE_BREAKPOINT);
        return () => mql.removeEventListener("change", onChange);
    }, []);

    return !!isMobile;
}

// This function masks a given key string for security purposes, showing only the first and last few characters.
export function MaskKey(key: string, maxLen = 32) {
    if (!key) return "";
    let str = key.length > maxLen ? key.slice(0, maxLen) : key;
    if (str.length <= 8) {
        return str[0] + "*".repeat(str.length - 2) + str[str.length - 1];
    } else {
        let front = str.slice(0, 4);
        let back = str.slice(-4);
        let middle = "*".repeat(str.length - 8);
        return (front + middle + back).padEnd(maxLen, "*");
    }
}
