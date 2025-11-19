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

import "./assets/styles/base.css";
import "./assets/styles/tailwind.css";
import "./assets/styles/themes.css";
import "./assets/styles/animation.css";

import { createRoot } from "react-dom/client";
import { RouterProvider } from "react-router-dom";
import { Router } from "./router";
import { LanguageContext } from "./hooks/context/language";
import { Toaster } from "./components/base/sonner";
import { AccountContext } from "./hooks/context/account";

// Render the app.
createRoot(document.getElementById("root")!).render(
    <LanguageContext>
        <AccountContext>
            <div className="page theme-container">
                <RouterProvider router={Router} />
            </div>
        </AccountContext>
        <Toaster position="top-center" />
    </LanguageContext>
);
