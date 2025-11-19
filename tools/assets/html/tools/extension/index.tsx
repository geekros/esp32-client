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

import * as path from "path";
import * as vscode from "vscode";
import { SideBarProvider } from "./provider/sidebar";

export function activate(context: vscode.ExtensionContext) {
    console.log("[GEEKSTUDIO]", "activate...", vscode.env.language, context.extensionPath);

    const isDevelopment = context.extensionMode === vscode.ExtensionMode.Development;

    vscode.commands.registerCommand("geekstudio:start", () => {
        vscode.window.showInformationMessage("Hello!!!");
    });

    context.subscriptions.push(vscode.window.registerWebviewViewProvider("geekstudio.sidebar", new SideBarProvider(context)));

    // Auto reload window when .tsx files change in development mode
    if (isDevelopment) {
        const pattern = "**/*.tsx";
        const watchPaths = [
            { path: path.join(context.extensionPath, "renderer/src"), name: "renderer" },
            { path: path.join(context.extensionPath, "tools/extension"), name: "extension" },
        ];

        watchPaths.forEach(({ path: watchPath, name }) => {
            const watcher = vscode.workspace.createFileSystemWatcher(new vscode.RelativePattern(watchPath, pattern));
            watcher.onDidChange((uri) => {
                vscode.commands.executeCommand("workbench.action.reloadWindow");
            });

            context.subscriptions.push(watcher);
        });
    }
}

export function deactivate() {
    console.log("[GEEKSTUDIO]", "deactivate...");
}
