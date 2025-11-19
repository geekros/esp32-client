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

import { createContext, useContext, useEffect, useState } from "react";

// Define types for mode, theme, and radius
type Mode = "dark" | "light" | "system";

// Define types for theme and radius
type Theme = "default" | "red" | "rose" | "orange" | "green" | "teal" | "blue" | "amber" | "yellow" | "violet" | "purple";

// Define types for radius
type Radius = "0" | "0.3" | "0.5" | "0.75" | "1.0";

// Define the props for the ThemeContext provider
type ThemeProps = {
    children: React.ReactNode;
    defaultTheme?: Theme;
    defaultMode?: Mode;
    defaultRadius?: Radius;
};

// Define the context type
type ThemeType = {
    mode: Mode;
    theme?: Theme;
    radius?: Radius;
    setMode: (_mode: Mode) => void;
    setTheme: (_theme: Theme) => void;
    setRadius: (_radius: Radius) => void;
};

// Define local storage keys
const local_storage_mode_name = "mode";
const local_storage_theme_name = "theme";
const local_storage_radius_name = "radius";

// Create the context with default values
const Context = createContext<ThemeType>({
    mode: "dark",
    theme: "default",
    radius: "0.75",
    setMode: (_mode: Mode) => null,
    setTheme: (_theme: Theme) => null,
    setRadius: (_radius: Radius) => null,
});

// Create a custom hook to use the ThemeContext
export const useTheme = () => useContext<ThemeType>(Context);

// Create the ThemeContext provider component
export const ThemeContext = ({ children, defaultMode = "system", defaultTheme = "default", defaultRadius = "0.75", ...props }: ThemeProps) => {
    // Initialize state from local storage or use default values
    const [mode, setMode] = useState<Mode>(() => (localStorage.getItem(local_storage_mode_name) as Mode) || defaultMode);
    const [theme, setTheme] = useState<Theme>(() => (localStorage.getItem(local_storage_theme_name) as Theme) || defaultTheme);
    const [radius, setRadius] = useState<Radius>(() => (localStorage.getItem(local_storage_radius_name) as Radius) || defaultRadius);

    // Update the document classes and styles based on mode, theme, and radius
    function updateMode() {
        const root = window.document.documentElement;
        const body = document.body;

        root.classList.remove("light", "dark");

        root.classList.add("[&_*]:!transition-none");

        if (mode === "system") {
            const systemMode = window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "dark";
            root.classList.add(systemMode);
            localStorage.setItem(local_storage_mode_name, systemMode);
            setMode(systemMode);
        }

        root.classList.add(mode);

        const vscodeTheme = body.dataset["vscodeThemeKind"];
        if (vscodeTheme) {
            const themeMode = vscodeTheme === "vscode-dark" ? "dark" : "dark";
            root.classList.add(themeMode);
            localStorage.setItem(local_storage_mode_name, themeMode);
            setMode(themeMode);
        }

        setTimeout(() => {
            root.classList.remove("[&_*]:!transition-none");
        }, 50);
    }

    // Update the document classes based on theme
    function updateTheme() {
        const root = window.document.documentElement;

        root.classList.remove("theme-default", "theme-red", "theme-rose", "theme-orange", "theme-teal", "theme-green", "theme-blue", "theme-amber", "theme-yellow", "theme-violet", "theme-purple");

        root.classList.add("[&_*]:!transition-none");

        root.classList.add("theme-" + theme);

        localStorage.setItem(local_storage_theme_name, theme);

        setTimeout(() => {
            root.classList.remove("[&_*]:!transition-none");
        }, 50);
    }

    // Update the document styles based on radius
    function updateRadius() {
        const root = window.document.documentElement;

        root.classList.add("[&_*]:!transition-none");

        root.style.setProperty("--radius", `${radius}rem`);

        localStorage.setItem(local_storage_radius_name, radius);

        setTimeout(() => {
            root.classList.remove("[&_*]:!transition-none");
        }, 50);
    }

    // Define the context value
    const value = {
        mode,
        setMode: (mode: Mode) => {
            localStorage.setItem(local_storage_mode_name, mode);
            setMode(mode);
        },
        theme,
        setTheme: (theme: Theme) => {
            localStorage.setItem(local_storage_theme_name, theme);
            setTheme(theme);
        },
        radius,
        setRadius: (radius: Radius) => {
            localStorage.setItem(local_storage_radius_name, radius);
            setRadius(radius);
        },
    };

    // Apply side effects when mode, theme, or radius changes
    useEffect(() => {
        updateMode();
        updateTheme();
        updateRadius();
    }, [mode, theme, radius]);

    return (
        <Context {...props} value={value}>
            {children}
        </Context>
    );
};
