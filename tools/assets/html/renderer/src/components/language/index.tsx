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

import { Button } from "@/components/base/button";
import { useLanguage } from "@/hooks/context/language";
import { Languages } from "lucide-react";

// Define props for the Language component
type LanguageProps = {
    className?: string;
};

// Language switcher component
export function Language({ className }: LanguageProps) {
    // Get language context
    const { change, i18next } = useLanguage();

    // Function to handle language switch
    function onLanguageSwitcher() {
        i18next.language === "zh" ? change("en") : change("zh");
    }

    // Render the language switcher button
    return (
        <Button onClick={onLanguageSwitcher} className={className} variant="outline" size="icon">
            <Languages />
        </Button>
    );
}
