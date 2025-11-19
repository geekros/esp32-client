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

import { useEffect, useState } from "react";
import { toast } from "sonner";
import { useLanguage } from "@/hooks/context/language";
import { useRequest } from "@/libs/request";
import { Loading } from "@/components/base/loading";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/base/tabs";
import { CpuIcon, LoaderIcon, LockKeyholeIcon, LockKeyholeOpen, RefreshCw, WifiIcon } from "lucide-react";
import { Button } from "@/components/base/button";
import { Input } from "@/components/base/input";
import { Drawer, DrawerClose, DrawerContent, DrawerDescription, DrawerFooter, DrawerHeader, DrawerTitle, DrawerTrigger } from "@/components/base/drawer";
import { Empty } from "@/components/base/empty";
import { Language } from "@/components/language";

export function PageIndex() {
    // Get the current language context.
    const { lang } = useLanguage();

    // Get the request and token management functions.
    const { request } = useRequest();

    // Define the data state.
    const data_structure: any = {
        loading: true,
        tabs: {
            current: "wifi",
        },
        config: {
            max_tx_power: 0,
            remember_bssid: false,
            sleep_mode: true,
        },
        scan: {
            button_loading: false,
            items: [],
            drawer: {
                open: false,
            },
        },
        reboot: {
            button_loading: false,
        },
        form: {
            ssid: "",
            password: "",
            button_loading: false,
        },
    };

    // Initialize the docs state.
    const [data, setdata] = useState(data_structure);

    // Function to update the data state.
    function updateData(data: any) {
        setdata((prev: any) => ({ ...prev, ...data }));
    }

    // Function to scan for available WIFI networks.
    function OnScan() {
        if (data.scan.button_loading) {
            return;
        }
        data.form.ssid = "";
        data.form.password = "";
        data.scan.button_loading = true;
        updateData(data);
        setTimeout(() => {
            request("GET", "/scan", {}, {}).then((request: any) => {
                if (request.status === 200) {
                    if (request.data.aps) {
                        data.scan.items = request.data.aps;
                    }
                    data.scan.button_loading = false;
                    updateData(data);
                } else {
                    data.scan.button_loading = false;
                    updateData(data);
                    toast.error(lang("common.tips.request_failed"));
                }
            });
        }, 2000);
    }

    // Function to choose a WIFI network.
    function OnChoice(item: any) {
        data.form.ssid = item.ssid;
        data.form.password = "";
        data.scan.drawer.open = false;
        updateData(data);
    }

    // Function to submit the WIFI connection.
    function OnSubmit() {
        if (data.form.ssid === "") {
            toast.warning(lang("form.select_wifi_placeholder"));
            return;
        }
        if (data.form.button_loading) {
            return;
        }
        data.form.button_loading = true;
        updateData(data);
        request("POST", "/submit", {}, { ssid: data.form.ssid, password: data.form.password }).then((request: any) => {
            if (request.status === 200) {
                if (request.data.success) {
                    data.form.button_loading = false;
                    updateData(data);
                    toast.success(lang("common.tips.wifi_connected_successfully"));
                } else {
                    data.form.button_loading = false;
                    updateData(data);
                    toast.warning(lang("common.tips.wifi_connected_failed"));
                }
            } else {
                data.form.button_loading = false;
                updateData(data);
                toast.warning(lang("common.tips.wifi_connected_failed"));
            }
        });
    }

    // Function to fetch data from the backend service.
    function onService() {
        request("GET", "/config", {}, {}).then((request: any) => {
            if (request.status === 200) {
                OnScan();
                data.config.max_tx_power = request.data.max_tx_power;
                data.config.remember_bssid = request.data.remember_bssid;
                data.config.sleep_mode = request.data.sleep_mode;
                data.loading = false;
                updateData(data);
            } else {
                toast.error(lang("common.tips.request_failed"));
            }
        });
    }

    // After the initial render
    useEffect(() => {
        onService();
    }, []);

    // Render the example page
    return (
        <>
            {!data.loading ? (
                <div className="page max-w-[480px] mx-auto">
                    <div className="w-full p-5 flex justify-end">
                        <Language className="hover:cursor-pointer" />
                    </div>
                    <header className="w-40 m-auto mb-10">
                        <img src="/images/logo.png" />
                    </header>
                    <div className="w-[80%] m-auto">
                        <div className="w-full">
                            <Tabs className="w-full" defaultValue={data.tabs.current}>
                                <TabsList className="w-full h-auto p-2">
                                    <TabsTrigger value="wifi">
                                        <div className="w-full pb-2">
                                            <div className="w-9 h-9 m-auto flex items-center justify-center">
                                                <WifiIcon className="size-6" />
                                            </div>
                                            <div className="w-full text-xs">{lang("form.wifi")}</div>
                                        </div>
                                    </TabsTrigger>
                                    <TabsTrigger value="manage">
                                        <div className="w-full pb-2">
                                            <div className="w-9 h-9 m-auto flex items-center justify-center">
                                                <CpuIcon className="size-6" />
                                            </div>
                                            <div className="w-full text-xs">{lang("form.device")}</div>
                                        </div>
                                    </TabsTrigger>
                                </TabsList>
                                <TabsContent value="wifi">
                                    <div className="w-full pt-5 space-y-6">
                                        <div className="w-full flex items-center gap-2">
                                            <Drawer
                                                open={data.scan.drawer.open}
                                                onOpenChange={(open: boolean) => {
                                                    data.scan.drawer.open = open;
                                                    updateData(data);
                                                }}
                                            >
                                                <DrawerTrigger asChild>
                                                    <Button className="w-full h-12 flex-5" variant="outline">
                                                        {data.form.ssid === "" ? (
                                                            <span className="text-muted-foreground line-clamp-1">{lang("form.select_wifi_placeholder")}</span>
                                                        ) : (
                                                            <span className="text-muted-foreground line-clamp-1">{data.form.ssid}</span>
                                                        )}
                                                    </Button>
                                                </DrawerTrigger>
                                                <DrawerContent className="w-full">
                                                    <DrawerHeader className="hidden">
                                                        <DrawerTitle></DrawerTitle>
                                                        <DrawerDescription></DrawerDescription>
                                                    </DrawerHeader>
                                                    <div className="w-full py-5 px-5 space-y-2">
                                                        <div className="w-full h-[380px] overflow-y-auto no-scrollbar space-y-2">
                                                            {data.scan.items.length === 0 ? (
                                                                <div className="w-full py-20 bg-muted/20 rounded-md">
                                                                    <Empty />
                                                                </div>
                                                            ) : (
                                                                <div className="w-full">
                                                                    {data.scan.items.map((item: any, index: number) => (
                                                                        <div
                                                                            onClick={() => {
                                                                                OnChoice(item);
                                                                            }}
                                                                            key={index}
                                                                            className="w-full h-12 flex items-center"
                                                                        >
                                                                            <div className="w-9 h-9 flex items-center justify-center">
                                                                                {item.authmode === 0 && <LockKeyholeOpen className="w-4 h-4" />}
                                                                                {item.authmode !== 0 && <LockKeyholeIcon className="w-4 h-4" />}
                                                                            </div>
                                                                            <div className="w-full line-clamp-1">{item.ssid}</div>
                                                                        </div>
                                                                    ))}
                                                                </div>
                                                            )}
                                                        </div>
                                                    </div>
                                                    <DrawerFooter>
                                                        <DrawerClose asChild></DrawerClose>
                                                    </DrawerFooter>
                                                </DrawerContent>
                                            </Drawer>
                                            <Button onClick={OnScan} disabled={data.scan.button_loading} className="w-full h-12 flex-1" variant="secondary" size="icon" aria-label="Submit">
                                                {!data.scan.button_loading ? <RefreshCw /> : <RefreshCw className="animate-spin" />}
                                            </Button>
                                        </div>
                                        <div className="w-full">
                                            <Input
                                                type="password"
                                                defaultValue={data.form.password}
                                                onChange={(e) => {
                                                    data.form.password = e.target.value;
                                                    updateData(data);
                                                }}
                                                className="w-full h-12 placeholder:text-sm"
                                                placeholder={lang("form.select_wifi_password_placeholder")}
                                            />
                                        </div>
                                        <div className="w-full">
                                            <Button onClick={OnSubmit} disabled={data.form.button_loading} className="w-full h-12 text-xs">
                                                {!data.form.button_loading ? null : <LoaderIcon className="size-4 mr-2 animate-spin inline-block" />}
                                                <span>{lang("form.button.connect")}</span>
                                            </Button>
                                        </div>
                                    </div>
                                </TabsContent>
                                <TabsContent value="manage">
                                    <div className="w-full grid grid-cols-3 gap-3 pt-5">
                                        <div className="col-span-3 py-20 bg-muted/20 rounded-md">
                                            <Empty />
                                        </div>
                                    </div>
                                </TabsContent>
                            </Tabs>
                        </div>
                    </div>
                </div>
            ) : (
                <div className="w-full h-screen flex items-center justify-center">
                    <Loading />
                </div>
            )}
        </>
    );
}
