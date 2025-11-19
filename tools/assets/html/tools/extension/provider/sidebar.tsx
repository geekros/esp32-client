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

import * as vscode from "vscode";

export class SideBarProvider implements vscode.WebviewViewProvider {
    constructor(private readonly context: vscode.ExtensionContext) {}
    resolveWebviewView(webviewView: vscode.WebviewView, context: vscode.WebviewViewResolveContext, token: vscode.CancellationToken): Thenable<void> | void {
        webviewView.webview.options = {
            enableScripts: true,
            localResourceRoots: [
                vscode.Uri.joinPath(this.context.extensionUri, "release", "dist"),
                vscode.Uri.joinPath(this.context.extensionUri, "release", "dist", "assets"),
                vscode.Uri.joinPath(this.context.extensionUri, "release", "dist", "script"),
            ],
        };

        const extensionUri = webviewView.webview.asWebviewUri(vscode.Uri.joinPath(this.context.extensionUri, "release", "dist"));

        const joystickScriptUri = webviewView.webview.asWebviewUri(vscode.Uri.joinPath(this.context.extensionUri, "release", "dist", "script", "joystick.js"));

        const indexScriptUri = webviewView.webview.asWebviewUri(vscode.Uri.joinPath(this.context.extensionUri, "release", "dist", "assets", "index.js"));

        const indexStyleUri = webviewView.webview.asWebviewUri(vscode.Uri.joinPath(this.context.extensionUri, "release", "dist", "assets", "index.css"));

        const viteClientUri = `http://${__DEV_SERVER_HOST__}:${__DEV_SERVER_PORT__}/@vite/client`;

        const mainScriptUri = `http://${__DEV_SERVER_HOST__}:${__DEV_SERVER_PORT__}/src/main.tsx`;

        const isDevelopment = this.context.extensionMode === vscode.ExtensionMode.Development;

        webviewView.webview.html = `
        <!DOCTYPE html>
        <html lang="en" class="vscode-extension">
            <head>
                <meta charset="UTF-8" />
                <meta name="viewport" content="width=device-width, initial-scale=1.0" />
                <meta name="build-time" content="${Date.now()}">
                <title>GEEKSTUDIO</title>
                <script>
                    window.extensionUri = "${extensionUri}";
                    (function () {
                        try {
                            const mode = localStorage.getItem("mode");
                            const prefersDark = window.matchMedia("(prefers-color-scheme: dark)").matches;
                            if (mode === "dark" || (!mode && prefersDark)) {
                                document.documentElement.classList.add("dark");
                            }
                        } catch (e) {}
                    })();
                </script>
                <script type="module" src="${isDevelopment ? viteClientUri : ""}"></script>
                <script type="module" src="${joystickScriptUri}"></script>
                <script type="module" crossorigin src="${!isDevelopment ? indexScriptUri : mainScriptUri}"></script>
                <link rel="stylesheet" crossorigin href="${!isDevelopment ? indexStyleUri : ""}" />
            </head>
            <body class="w-full h-full bg-background text-foreground font-sans font-medium antialiased font-gest-sans font-gest-mono">
                <div id="root"></div>
            </body>
        </html>
        `;
    }
}
