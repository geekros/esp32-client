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

import { useTheme } from "@/hooks/context/theme";
import { Button } from "@/components/base/button";

type ThemeProps = {
    className?: string;
};

export function Theme({ className }: ThemeProps) {
    const { setTheme } = useTheme();

    function onThemeSwitcher(theme: any) {
        setTheme(theme);
    }

    return (
        <Button className={"w-auto " + className} variant="secondary">
            <div className="w-auto flex items-center justify-center space-x-2">
                <div className="size-[16px] theme-default">
                    <div
                        onClick={() => {
                            onThemeSwitcher("default");
                        }}
                        className="w-full h-full bg-[var(--color-neutral-950)] rounded-[100%]"
                    ></div>
                </div>
                <div className="size-[16px]">
                    <div
                        onClick={() => {
                            onThemeSwitcher("blue");
                        }}
                        className="w-full h-full bg-[var(--color-blue-600)] rounded-[100%]"
                    ></div>
                </div>
                <div className="size-[16px]">
                    <div
                        onClick={() => {
                            onThemeSwitcher("red");
                        }}
                        className="w-full h-full bg-[var(--color-red-600)] rounded-[100%]"
                    ></div>
                </div>
                <div className="size-[16px]">
                    <div
                        onClick={() => {
                            onThemeSwitcher("rose");
                        }}
                        className="w-full h-full bg-[var(--color-rose-600)] rounded-[100%]"
                    ></div>
                </div>
                <div className="size-[16px]">
                    <div
                        onClick={() => {
                            onThemeSwitcher("orange");
                        }}
                        className="w-full h-full bg-[var(--color-orange-600)] rounded-[100%]"
                    ></div>
                </div>
                <div className="size-[16px]">
                    <div
                        onClick={() => {
                            onThemeSwitcher("green");
                        }}
                        className="w-full h-full bg-[var(--color-green-600)] rounded-[100%]"
                    ></div>
                </div>
                <div className="size-[16px]">
                    <div
                        onClick={() => {
                            onThemeSwitcher("teal");
                        }}
                        className="w-full h-full bg-[var(--color-teal-600)] rounded-[100%]"
                    ></div>
                </div>
                <div className="size-[16px]">
                    <div
                        onClick={() => {
                            onThemeSwitcher("amber");
                        }}
                        className="w-full h-full bg-[var(--color-amber-600)] rounded-[100%]"
                    ></div>
                </div>
                <div className="size-[16px]">
                    <div
                        onClick={() => {
                            onThemeSwitcher("yellow");
                        }}
                        className="w-full h-full bg-[var(--color-yellow-600)] rounded-[100%]"
                    ></div>
                </div>
                <div className="size-[16px]">
                    <div
                        onClick={() => {
                            onThemeSwitcher("violet");
                        }}
                        className="w-full h-full bg-[var(--color-violet-600)] rounded-[100%]"
                    ></div>
                </div>
                <div className="size-[16px]">
                    <div
                        onClick={() => {
                            onThemeSwitcher("purple");
                        }}
                        className="w-full h-full bg-[var(--color-purple-600)] rounded-[100%]"
                    ></div>
                </div>
            </div>
        </Button>
    );
}
