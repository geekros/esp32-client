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

import os from "os";
import path from "path";
import file from "fs";
import * as Electron from "electron";
import * as Updater from "electron-updater";
import Store from "electron-store";
import Debug from "electron-debug";
import Package from "../../package.json";
import { Request, AdmZip } from "../common";

const customUserDataPath = path.join(Electron.app.getPath("appData"), Package.build.appId);
Electron.app.setPath("userData", customUserDataPath);

const store = new Store();

const desktop: any = {
    windows: {
        launch: {
            main: false,
            options: {
                title: Package.build.nsis.shortcutName,
                frame: false,
                center: true,
                width: 1200,
                height: 750,
                minWidth: 1200,
                minHeight: 750,
                useContentSize: false,
                fullscreenable: os.platform() === "darwin",
                hasShadow: os.platform() === "darwin",
                webPreferences: {
                    javascript: true,
                    spellcheck: true,
                    webviewTag: true,
                    webSecurity: false,
                    nodeIntegration: true,
                    contextIsolation: false,
                    nodeIntegrationInWorker: false,
                    nativeWindowOpen: true,
                    preload: path.join(__dirname, "../desktop/preload/index.cjs"),
                },
                fullscreen: false,
                show: false,
                backgroundColor: "rgb(221, 227, 253)",
                titleBarStyle: "hidden",
            },
            display: store.get(Package.env.LOCAL_STORAGE_PREFIX + ":electron:launch:display", 0),
        },
    },
    user_data: {
        language: store.get(Package.env.LOCAL_STORAGE_PREFIX + ":electron:language", ""),
        lock: true,
        sleep: false,
        quit: false,
    },
};

// Prevent the application from opening multiple times
if (!Electron.app.requestSingleInstanceLock()) {
    desktop.user_data.lock = false;
    Electron.app.quit();
} else {
    desktop.user_data.lock = true;
    Electron.app.on("second-instance", () => {});
}

// Default configuration of debugging tool
Debug({
    showDevTools: false,
    devToolsMode: "bottom",
});

// Configuration related to environment variables and startup parameters
process.env["ELECTRON_DISABLE_SECURITY_WARNINGS"] = "true";

// Start parameter configuration
Electron.app.commandLine.appendSwitch("ignore-certificate-errors", "true");
Electron.app.commandLine.appendSwitch("disable-gpu", "false");
Electron.app.commandLine.appendSwitch("enable-unsafe-swiftshader");

console.log("[main:language]", desktop.user_data.language);
if (desktop.user_data.language !== "") {
    Electron.app.commandLine.appendSwitch("--lang", String(desktop.user_data.language)); // zh-CN or en-US
} else {
    desktop.user_data.language = Electron.app.getLocale();
    Electron.app.commandLine.appendSwitch("--lang", String(desktop.user_data.language));
}

if (Package.env.DESKTOP_NETWORK_PAC_URL !== "") {
    console.log("[main:pac_url]", Package.env.DESKTOP_NETWORK_PAC_URL);
    Electron.app.commandLine.appendSwitch("--proxy-pac-url", Package.env.DESKTOP_NETWORK_PAC_URL + "?time=" + Math.floor(Date.now() / 1000));
}

// Initialize the application's root domain and path
const base_url: string = Electron.app.isPackaged ? `file://${path.join(__dirname, "../index.html")}` : `http://localhost:${Package.env.DEV_SERVER_PORT}`;

// Set the user agent for the application
let user_agent: string = Package.env.DESKTOP_USERAGENT.replace("$platform", "Windows NT 10.0; Win64; x64").replace("$version", Package.version);
if (os.platform() === "darwin") {
    user_agent = Package.env.DESKTOP_USERAGENT.replace("$platform", "Macintosh; Intel Mac OS X 10_15_7").replace("$version", Package.version);
}

// When the application is launched
function onLaunch() {
    console.log("[main:on:launch]");

    // Check if the application is running in development mode
    let display: any = null;

    // If the application is running in development mode, use the specified display
    if (desktop.windows.launch.display > 0) {
        const displays = Electron.screen.getAllDisplays();
        display = displays.find((display) => display.id === desktop.windows.launch.display);
    }

    // If no display is found, use the primary display
    if (display) {
        const { bounds } = display;
        const window_width = desktop.windows.launch.options.width;
        const window_height = desktop.windows.launch.options.height;
        const x = bounds.x + Math.round((bounds.width - window_width) / 2);
        const y = bounds.y + Math.round((bounds.height - window_height) / 2);
        desktop.windows.launch.options = { ...desktop.windows.launch.options, x, y };
    }

    // Create the main application window
    desktop.windows.launch.main = new Electron.BrowserWindow(desktop.windows.launch.options);

    // Load the main application URL
    desktop.windows.launch.main.loadURL(base_url).then((res: any) => {
        console.log("[main:on:launch]", res, Electron.app.isPackaged);
        if (!desktop.user_data.quit) {
            const tray_development = os.platform() === "darwin" ? "renderer/public/icons/macos.png" : "renderer/public/icons/windows.ico";
            const tray_production = os.platform() === "darwin" ? path.join(__dirname, "../icons/macos.png") : path.join(__dirname, "../icons/windows.ico");
            const tray_icon = Electron.app.isPackaged
                ? new Electron.Tray(Electron.nativeImage.createFromPath(tray_production))
                : new Electron.Tray(Electron.nativeImage.createFromPath(tray_development));
            const tray_menu = Electron.Menu.buildFromTemplate([
                {
                    label: desktop.user_data.language === "zh-CN" ? "显示窗口" : "Show Window",
                    click: function () {
                        desktop.windows.launch.main.show();
                    },
                },
                {
                    label: desktop.user_data.language === "zh-CN" ? "退出软件" : "Quit Software",
                    click: function () {
                        desktop.user_data.quit = true;
                        desktop.windows.launch.main.close();
                        Electron.app.quit();
                    },
                },
            ]);
            tray_icon.setContextMenu(tray_menu);
            tray_icon.setToolTip(Package.description);
            tray_icon.on("click", function (_event: any) {});
            tray_icon.on("double-click", function () {
                desktop.windows.launch.main.show();
            });
        }
    });

    // Handle blur events
    desktop.windows.launch.main.on("blur", () => {
        desktop.windows.launch.main.setBackgroundColor("rgb(221, 227, 233)");
    });

    // Handle focus events
    desktop.windows.launch.main.on("focus", () => {
        desktop.windows.launch.main.setBackgroundColor("rgb(211, 227, 253)");
    });

    // Handle webview attachment events
    desktop.windows.launch.main.webContents.on("did-attach-webview", (_event: any, wc: any) => {
        wc.setWindowOpenHandler((details: any) => {
            console.log("[main:on:launch:did-attach-webview]", details.url);
            desktop.windows.launch.main.webContents.send("message", { type: "main:browser:open:window", data: details });
            return { action: "deny" };
        });
    });

    // Handle navigation events
    desktop.windows.launch.main.webContents.on("will-navigate", (_event: any, url: any) => {
        console.log("[main:on:launch:will-navigate]", url);
    });

    // Handle new window requests
    desktop.windows.launch.main.webContents.on("new-window", (_event: any, url: any) => {
        console.log("[main:on:launch:new-window]", url);
    });

    // Handle the ready-to-show event
    desktop.windows.launch.main.on("ready-to-show", function () {
        console.log("[main:on:launch:ready-to-show]");
        desktop.windows.launch.main.show();
        desktop.windows.launch.main.webContents.send("message", { type: "main:user:agent", data: user_agent });
    });

    // Handle window moved events
    desktop.windows.launch.main.on("moved", () => {
        console.log("[main:on:launch:moved]");
        DisplayEvent();
    });

    // Implement right-click menu
    desktop.windows.launch.main.webContents.on("context-menu", (e: any, _params: any) => {
        e.preventDefault();
    });

    // Register global shortcuts
    Electron.globalShortcut.register("F11", () => {
        console.log("[main:on:launch:global:shortcut]", "F11");
    });

    // Register global shortcuts for various actions
    Electron.globalShortcut.register("Shift+Alt+L", () => {
        store.set(Package.env.LOCAL_STORAGE_PREFIX + ":electron:language", "");
        if (desktop.user_data.language === "") {
            store.set(Package.env.LOCAL_STORAGE_PREFIX + ":electron:language", "en-US");
        }
        if (desktop.user_data.language !== "") {
            if (desktop.user_data.language === "en-US") {
                desktop.user_data.language = "zh-CN";
                store.set(Package.env.LOCAL_STORAGE_PREFIX + ":electron:language", "zh-CN");
            }
            if (desktop.user_data.language === "zh-CN") {
                desktop.user_data.language = "en-US";
                store.set(Package.env.LOCAL_STORAGE_PREFIX + ":electron:language", "en-US");
            }
        }
        console.log("[main:on:launch:global:shortcut]", "Shift+Alt+L", desktop.user_data.language);
    });

    // Register global shortcuts for various actions
    Electron.globalShortcut.register("Shift+Alt+P", () => {
        console.log("[main:on:launch:global:shortcut]", "Shift+Alt+P");
    });

    // Register global shortcuts for various actions
    Electron.globalShortcut.register("Shift+Alt+H", () => {
        console.log("[main:on:launch:global:shortcut]", "Shift+Alt+H");
        desktop.windows.launch.main.webContents.openDevTools({ mode: "bottom", activate: false });
    });

    // Register global shortcuts for various actions
    Electron.globalShortcut.register("Shift+Alt+V", () => {
        console.log("[main:on:launch:global:shortcut]", "Shift+Alt+V");
        desktop.windows.launch.main.webContents.send("message", { type: "main:browser:tools" });
    });

    // Register global shortcuts for various actions
    Electron.globalShortcut.register("Shift+Alt+S", () => {
        console.log("[main:on:launch:global:shortcut]", "Shift+Alt+S");
        desktop.windows.launch.main.webContents.send("message", { type: "main:display:sleep", status: !desktop.user_data.sleep });
    });

    // Close the main window
    desktop.windows.launch.main.on("close", (event: any) => {
        if (!desktop.user_data.quit) {
            desktop.windows.launch.main.hide();
            event.preventDefault();
        }
    });

    DisplayEvent();

    UpdaterEvent();
}

// When the application is ready
Electron.app.on("ready", () => {
    console.log("[main:ready]");
    if (desktop.user_data.lock) {
        InitUnpacked();
    }
});

// When the application is ready
Electron.app.whenReady().then(() => {
    console.log("[main:whenready]");
    // Initialize the application
    onLaunch();
    // Switch the sleep configuration once to ensure the status is not 0
    desktop.user_data.sleep = Electron.powerSaveBlocker.start("prevent-display-sleep");
    Electron.powerSaveBlocker.stop(desktop.user_data.sleep);
    desktop.user_data.sleep = false;
    // Active state monitoring
    Electron.app.on("activate", () => {
        console.log("[main:whenready:activate]");
        if (Electron.BrowserWindow.getAllWindows().length === 0) {
            console.log("[main:whenready:activate:length]", 0);
            onLaunch();
        }
    });
    // Handle the user agent for specific URLs
    Electron.session.defaultSession.webRequest.onBeforeSendHeaders((details: any, callback: any) => {
        if (details.url.includes("google.com") || details.url.includes("chatgpt.com") || details.url.includes("openai.com") || details.url.includes("geekllm.com")) {
            details.requestHeaders["User-Agent"] = user_agent;
        }
        callback({ cancel: false, requestHeaders: details.requestHeaders });
    });
    // listens for power suspend and screen lock
    Electron.powerMonitor.on("suspend", () => {
        console.log("[main:whenready:powerMonitor:suspend]");
        if (desktop.windows.launch.main) {
            desktop.windows.launch.main.webContents.send("message", { type: "main:power:lock" });
        }
    });
    Electron.powerMonitor.on("resume", () => {
        console.log("[main:whenready:powerMonitor:resume]");
        if (desktop.windows.launch.main) {
            desktop.windows.launch.main.webContents.send("message", { type: "main:power:unlock" });
        }
    });
    Electron.powerMonitor.on("lock-screen", () => {
        console.log("[main:whenready:powerMonitor:lock-screen]");
        if (desktop.windows.launch.main) {
            desktop.windows.launch.main.webContents.send("message", { type: "main:screen:lock" });
        }
    });
    Electron.powerMonitor.on("unlock-screen", () => {
        console.log("[main:whenready:powerMonitor:unlock-screen]");
        if (desktop.windows.launch.main) {
            desktop.windows.launch.main.webContents.send("message", { type: "main:screen:unlock" });
        }
    });
});

// Listen for main process messages
Electron.ipcMain.on("message", (event: any, args: any) => {
    if (args.type === "template:select:folder:path") {
        console.log("[main:message:select:folder:path]");
        if (args.callback && args.callback !== "") {
            Electron.dialog
                .showOpenDialog(desktop.windows.launch.main, {
                    properties: ["openDirectory"],
                })
                .then((r: any) => {
                    event.sender.send("message", { type: "main:select:folder:path", callback: args.callback, data: r });
                });
        }
    }
    if (args.type === "template:select:folder:file") {
        console.log("[main:message:select:folder:file]", args.callback);
        if (args.callback && args.callback !== "") {
            Electron.dialog
                .showOpenDialog(desktop.windows.launch.main, {
                    properties: ["openFile"],
                    filters: [{ name: "Images", extensions: ["jpg", "jpeg", "png", "gif", "bmp"] }],
                })
                .then((r: any) => {
                    event.sender.send("message", { type: "main:select:folder:file", callback: args.callback, data: r });
                });
        }
    }
    if (args.type === "template:refresh:account:data") {
        console.log("[main:message:refresh:account:data]");
        event.sender.send("message", { type: "main:refresh:account:data" });
    }
    if (args.type === "template:markdown:link") {
        console.log("[main:message:markdown:link]", args.data);
        desktop.windows.launch.main.webContents.send("message", { type: "main:browser:open:window", data: { url: args.data } });
    }
    if (args.type === "template:updater:app") {
        console.log("[main:message:updater:app]", args.data);
        if (Electron.app.isPackaged) {
            let updater_app_path = path.resolve(__dirname, "../../../../");
            let updater_app_download_length = 0;
            let updater_app_download_received = 0;
            let updater_app_file = file.createWriteStream(updater_app_path + "/updater_app.zip");
            let updater_app_download = new Request().getApi({
                method: "GET",
                uri: args.url,
                headers: {
                    "User-Agent": user_agent,
                },
            });
            updater_app_download.pipe(updater_app_file);
            updater_app_download.on("response", function (data: any) {
                updater_app_download_length = data.headers["content-length"];
            });
            updater_app_download.on("data", function (chunk: any) {
                updater_app_download_received += chunk.length;
                event.sender.send("message", { type: "main:updater:app:progress", data: parseFloat(((updater_app_download_received * 100) / updater_app_download_length).toFixed(2)) });
            });
            updater_app_download.on("end", function () {
                event.sender.send("message", { type: "main:updater:app:end", data: false });
                setTimeout(() => {
                    const zip = new AdmZip().getApi(updater_app_path + "/updater_app.zip");
                    zip.extractAllTo(updater_app_path, true);
                    setTimeout(() => {
                        file.unlinkSync(updater_app_path + "/updater_app.zip");
                        UpdateAsar();
                        desktop.user_data.quit = true;
                        desktop.windows.launch.main.close();
                        Electron.app.relaunch();
                        Electron.app.quit();
                    }, 1000);
                }, 1000);
            });
        }
    }
    if (args.type === "template:updater") {
        console.log("[main:message:updater]");
        Updater.autoUpdater.checkForUpdates().then((_r: any) => {
            if (!Electron.app.isPackaged) {
                desktop.windows.launch.main.webContents.send("message", {
                    type: "main:updater:update:not:available",
                });
            }
        });
    }
    if (args.type === "template:header:right:button") {
        console.log("[main:message:header:right:button]");
        if (args.data === "close") {
            if (!desktop.user_data.quit) {
                desktop.windows.launch.main.hide();
            } else {
                desktop.windows.launch.main.close();
                Electron.app.quit();
            }
        }
        if (args.data === "min") {
            desktop.windows.launch.main.minimize();
        }
        if (args.data === "size") {
            if (desktop.windows.launch.main.isMaximized()) {
                desktop.windows.launch.main.unmaximize();
            } else {
                desktop.windows.launch.main.maximize();
            }
        }
    }
    if (args.type === "template:window:resize") {
        console.log("[main:message:window:resize]");
        if (args.data === "resize") {
            if (desktop.windows.launch.main.isFullScreen()) {
                event.sender.send("message", { type: "main:window:resize", data: "full" });
            } else {
                if (desktop.windows.launch.main.isMaximized()) {
                    event.sender.send("message", { type: "main:window:resize", data: "max" });
                } else {
                    event.sender.send("message", { type: "main:window:resize", data: "restore" });
                }
            }
        }
    }
    if (args.type === "template:cache:clear") {
        console.log("[main:message:cache:clear]");
        Electron.session.defaultSession
            .clearCache()
            .then(() => {
                Electron.session.defaultSession
                    .clearStorageData({
                        storages: ["cookies", "indexdb", "websql", "filesystem", "shadercache", "websql", "serviceworkers", "cachestorage"],
                    })
                    .then(() => {
                        event.sender.send("message", { type: "main:cache:clear" });
                    })
                    .catch((_e: any) => {
                        event.sender.send("message", { type: "main:cache:clear" });
                    });
            })
            .catch(() => {
                event.sender.send("message", { type: "main:cache:clear" });
            });
    }
    if (args.type === "template:refresh:account:data") {
        console.log("[main:message:refresh:account:data]");
        event.sender.send("message", { type: "main:refresh:account:data" });
    }
});

// Extract built-in software packages
function InitUnpacked() {
    console.log("[main:init:unpacked]");
    // Copy Pack Files
    file.mkdirSync(path.join(__dirname, Electron.app.isPackaged ? "../../../../software" : "../software"), { recursive: true });
    const net_files = file.readdirSync(path.join(__dirname, "../software"), { withFileTypes: true });
    if (net_files.length > 0) {
        console.log("[main:init:unpacked:files]", net_files.length);
        for (let item of net_files) {
            if (item.name !== ".gitkeep") {
                let srcPath = path.join(path.join(__dirname, "../software/"), item.name);
                let destPath = path.join(path.join(__dirname, Electron.app.isPackaged ? "../../../../software/" : "../software/"), item.name);
                file.copyFileSync(srcPath, destPath);
            }
        }
        const child_process = require("child_process");
        if (os.platform() !== "win32") {
            child_process.spawn("chmod", ["-R", "777", path.join(__dirname, Electron.app.isPackaged ? "../../../../software/" : "../software/")]);
        }
    }
}

// Hot Update
function UpdateAsar() {
    console.log("[main:update:asar]");
    const { execSync, spawnSync } = require("child_process");
    const updater_app_path = path.resolve(__dirname, "../../../../");
    const asarPath = path.join(updater_app_path, "/app.asar");
    const updaterAsarPath = path.join(updater_app_path, "/app.temp");
    if (file.existsSync(updaterAsarPath)) {
        if (os.platform() === "win32") {
            // Check if the updater app has write permissions
            function checkWritePermission(folderPath: any) {
                try {
                    const testFile = path.join(folderPath, ".test");
                    file.writeFileSync(testFile, "test");
                    file.unlinkSync(testFile);
                    return true;
                } catch (error) {
                    return false;
                }
            }
            if (!checkWritePermission(updater_app_path)) {
                const args: any = ["-FilePath", "cmd", "-WindowStyle", "hidden", "-ArgumentList", `"/c xcopy \\"${updaterAsarPath}\\" \\"${asarPath}\\" /y"`, "-Verb", "RunAs"];
                spawnSync("powershell", args, { stdio: "ignore" });
            } else {
                const cmd = `xcopy "${updaterAsarPath}" "${asarPath}" /y`;
                execSync(cmd, { stdio: "ignore" });
            }
        } else {
            const cmd = `cp -r -f "${updaterAsarPath}" "${asarPath}"`;
            execSync(cmd, { stdio: "ignore" });
        }

        setTimeout(() => {
            file.unlinkSync(updater_app_path + "/app.temp");
        }, 500);
    }
}

// Display screen events
function DisplayEvent() {
    // Get the display nearest to the center of the main window
    const bounds: any = desktop.windows.launch.main.getBounds();
    // If the display is not found, use the primary display
    const display = Electron.screen.getDisplayNearestPoint({
        x: bounds.x + bounds.width / 2,
        y: bounds.y + bounds.height / 2,
    });
    // Set the display ID in the store
    store.set(Package.env.LOCAL_STORAGE_PREFIX + ":electron:launch:display", display.id);
}

// Updater app events
function UpdaterEvent() {
    console.log("[main:updater:event]");

    // Check if the updater URL is empty
    if (Package.build.publish[0].url === "") {
        console.log("[main:updater:event]", "off");
        return;
    }

    console.log("[main:updater:event]", "on");

    // Check if the application is running in development mode
    Updater.autoUpdater.setFeedURL(Package.build.publish[0].url);

    // Check for updates
    Updater.autoUpdater.on("error", function (error: any) {
        desktop.windows.launch.main.webContents.send("message", {
            type: "main:updater:error",
            error: error,
        });
    });

    // Check for updates
    Updater.autoUpdater.on("checking-for-update", function () {
        desktop.windows.launch.main.webContents.send("message", {
            type: "main:updater:checking:for:update",
        });
    });

    // Update available
    Updater.autoUpdater.on("update-available", function (_info: any) {
        desktop.windows.launch.main.webContents.send("message", {
            type: "main:updater:update:available",
        });
    });

    // Update not available
    Updater.autoUpdater.on("update-not-available", function (_info: any) {
        desktop.windows.launch.main.webContents.send("message", {
            type: "main:updater:update:not:available",
        });
    });

    // Downloading progress
    Updater.autoUpdater.on("download-progress", function (progress: any) {
        desktop.windows.launch.main.webContents.send("message", {
            type: "main:updater:download:progress",
            progress: progress,
        });
    });

    // Update downloaded
    Updater.autoUpdater.on("update-downloaded", function () {
        desktop.windows.launch.main.webContents.send("message", {
            cmd: "main:updater:update:downloaded",
        });
        desktop.uaer_data.quit = true;
        Updater.autoUpdater.quitAndInstall();
    });
}
