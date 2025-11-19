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

import { builtinModules } from "module";
import { defineConfig } from "vite";
import Package from "../../package.json";

export default defineConfig({
    root: __dirname,
    define: {
        __APP_NAME__: JSON.stringify(Package.title),
        __APP_VERSION__: JSON.stringify(Package.version),
        __DEV_SERVER_HOST__: JSON.stringify(Package.env.DEV_SERVER_HOST),
        __DEV_SERVER_PORT__: JSON.stringify(Package.env.DEV_SERVER_PORT),
        __DEV_BACKEND_HOST__: JSON.stringify(Package.env.DEV_BACKEND_HOST),
        __APP_LOCAL_STORAGE_PREFIX__: JSON.stringify(Package.env.LOCAL_STORAGE_PREFIX),
    },
    build: {
        outDir: "../../release/dist/main",
        emptyOutDir: true,
        minify: "terser",
        sourcemap: false,
        lib: {
            entry: "index.tsx",
            formats: ["cjs"],
            fileName: () => "[name].cjs",
        },
        rollupOptions: {
            external: ["vscode", ...builtinModules, ...builtinModules.map((e) => `node:${e}`), ...Object.keys(Package.dependencies || {})],
        },
    },
});
