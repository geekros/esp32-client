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
import { useTheme } from "@/hooks/context/theme";

export function ThemeRadius() {
    const { setRadius } = useTheme();

    function onRadiusSwitcher(radius: any) {
        setRadius(radius);
    }

    return (
        <Button variant="secondary" className="w-auto">
            <div className="w-auto flex items-center justify-center space-x-2">
                {["0", "0.3", "0.5", "0.75", "1.0"].map((value: any) => {
                    return (
                        <div key={value} className="size-[16px]">
                            <div
                                onClick={() => {
                                    onRadiusSwitcher(value);
                                }}
                                className="w-full h-full bg-primary"
                                style={{ borderRadius: `${value / 2.5}rem` }}
                            ></div>
                        </div>
                    );
                })}
            </div>
        </Button>
    );
}
