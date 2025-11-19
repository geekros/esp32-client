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
import file from "fs";
import path from "path";
import chalk from "chalk";
import yargs from "yargs/yargs";
import electron from "electron";
import { spawn } from "child_process";
import { hideBin } from "yargs/helpers";
import { build, createServer } from "vite";
import { chunkSplitPlugin } from "vite-plugin-chunk-split";

const argvs = yargs(hideBin(process.argv)).argv;
const modes = ["browser", "desktop", "extension"];
const scripts = ["dev", "build"];

if (!argvs.mode || argvs.mode === "" || !modes.includes(argvs.mode)) {
    console.log(chalk.red("Please specify mode: --mode=<browser|desktop|extension>"));
    process.exit(0);
}

if (!process.env.SCRIPT_MODE || process.env.SCRIPT_MODE === "" || !scripts.includes(process.env.SCRIPT_MODE)) {
    console.log(chalk.red("Please specify script: <dev|build>"));
    process.exit(0);
}

if (!file.existsSync("release")) {
    file.mkdirSync("release");
    file.writeFileSync("release/.gitkeep", "");
}

const tools = {
    server: null,
};

function onBuildDesktopBrowser() {
    return build({
        configFile: "tools/desktop/browser/vite.config.ts",
        mode: "development",
        plugins: [
            {
                name: "electron-browser-watcher",
                writeBundle() {
                    tools.server.ws.send({ type: "full-reload" });
                },
            },
        ],
        build: {
            watch: {},
        },
    });
}

function onBuildDesktopPreload() {
    return build({
        configFile: "tools/desktop/preload/vite.config.ts",
        mode: "development",
        plugins: [
            {
                name: "electron-preload-watcher",
                writeBundle() {
                    tools.server.ws.send({ type: "full-reload" });
                },
            },
        ],
        build: {
            watch: {},
        },
    });
}

function onBuildDesktopMain() {
    const address = tools.server.httpServer.address();
    const env = Object.assign(process.env, {
        VITE_DEV_SERVER_HOST: address.address,
        VITE_DEV_SERVER_PORT: address.port,
    });
    return build({
        configFile: "tools/desktop/vite.config.ts",
        mode: "development",
        plugins: [
            {
                name: "electron-main-watcher",
                writeBundle(command, options) {
                    if (process.electronApp) {
                        process.electronApp.removeAllListeners();
                        process.electronApp.kill();
                    }
                    process.electronApp = spawn(electron, [".", "--no-sandbox"], { stdio: "inherit", env });
                    process.electronApp.once("exit", process.exit);
                },
            },
        ],
        build: {
            watch: {},
        },
    });
}

function onBuildExtension() {
    return build({
        configFile: "tools/extension/vite.config.ts",
        mode: "development",
        plugins: [
            {
                name: "extension-watcher",
                writeBundle() {
                    tools.server.ws.send({ type: "full-reload" });
                },
            },
        ],
        build: {
            watch: {},
        },
    });
}

function onCopyIcons() {
    const icons = file.readdirSync("renderer/public/icons", {
        withFileTypes: true,
    });
    file.mkdirSync("release/dist/icons", { recursive: true });
    for (let item of icons) {
        let srcPath = path.join("renderer/public/icons", item.name);
        let destPath = path.join("release/dist/icons", item.name);
        file.copyFileSync(srcPath, destPath);
    }
}

function onCopySoftwares() {
    file.mkdirSync("release/dist/software", { recursive: true });
    const nets = file.readdirSync("tools/software", { withFileTypes: true });
    for (let item of nets) {
        let srcPath = path.join("tools/software", item.name);
        let destPath = path.join("release/dist/software", item.name);
        file.copyFileSync(srcPath, destPath);
    }
}

async function onStartServer() {
    tools.server = await createServer({
        configFile: "renderer/vite.config.ts",
        plugins: [
            {
                name: "server-watcher",
                writeBundle() {},
            },
        ],
    });
    tools.server.listen().then(() => {
        if (argvs.mode === "desktop") {
            onBuildDesktopBrowser();
            onBuildDesktopPreload();
            onBuildDesktopMain();
        }
        if (argvs.mode === "extension") {
            onBuildExtension();
        }
        const address = tools.server.httpServer.address();
        if (address && typeof address !== "string") {
            const host = address.address === "0.0.0.0" || address.address === "::1" ? "localhost" : address.address;
            const port = address.port;
            console.log(chalk.green("-----------------------------------------------"));
            console.log(chalk.gray("Current mode"), chalk.green(argvs.mode));
            console.log(chalk.gray("Server running at"), chalk.green(`http://${host}:${port}`));
            const networkInterfaces = os.networkInterfaces();
            for (let interfaceName in networkInterfaces) {
                networkInterfaces[interfaceName].forEach((network) => {
                    if (network.family === "IPv4" && !network.internal) {
                        console.log(chalk.gray("Server running at"), chalk.green(`http://${network.address}:${port}`));
                    }
                });
            }
            console.log(chalk.green("-----------------------------------------------"));
        }
    });
}

if (process.env.SCRIPT_MODE === "dev") {
    if (argvs.mode === "desktop") {
        await build({ configFile: "renderer/vite.config.ts" });
        await build({ configFile: "tools/desktop/browser/vite.config.ts" });
        await build({ configFile: "tools/desktop/preload/vite.config.ts" });
        await build({ configFile: "tools/desktop/vite.config.ts" });
        onCopyIcons();
        onCopySoftwares();
    }
    if (argvs.mode === "extension") {
        await build({ configFile: "tools/extension/vite.config.ts" });
        await build({
            configFile: "renderer/vite.config.ts",
            base: "./",
            plugins: [
                chunkSplitPlugin({
                    strategy: "default",
                    customChunk: (args) => {
                        return null;
                    },
                    customSplitting: {
                        "react-vendor": ["react", "react-dom"],
                    },
                }),
            ],
            build: {
                rollupOptions: {
                    output: {
                        entryFileNames: `assets/[name].js`,
                        chunkFileNames: `assets/[name].js`,
                        assetFileNames: `assets/[name].[ext]`,
                    },
                },
            },
        });
    }
    onStartServer();
}

if (process.env.SCRIPT_MODE === "build") {
    if (argvs.mode === "desktop") {
        await build({ configFile: "renderer/vite.config.ts" });
        await build({ configFile: "tools/desktop/vite.config.ts" });
        await build({ configFile: "tools/desktop/browser/vite.config.ts" });
        await build({ configFile: "tools/desktop/preload/vite.config.ts" });
        onCopyIcons();
        onCopySoftwares();
    }
    if (argvs.mode === "extension") {
        await build({
            configFile: "renderer/vite.config.ts",
            base: "./",
            plugins: [
                chunkSplitPlugin({
                    strategy: "all-in-one",
                    customChunk: (args) => {
                        return null;
                    },
                    customSplitting: {
                        "react-vendor": ["react", "react-dom"],
                    },
                }),
            ],
            build: {
                rollupOptions: {
                    output: {
                        entryFileNames: `assets/[name].js`,
                        chunkFileNames: `assets/[name].js`,
                        assetFileNames: `assets/[name].[ext]`,
                    },
                },
            },
        });
        await build({ configFile: "tools/extension/vite.config.ts" });
    }
    if (argvs.mode === "browser") {
        await build({
            configFile: "renderer/vite.config.ts",
            build: { outDir: "../release/dist/" },
        });

        file.rmSync("release/dist/icons", { recursive: true, force: true });
    }
}
