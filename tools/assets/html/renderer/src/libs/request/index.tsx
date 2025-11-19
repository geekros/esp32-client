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

import axios from "axios";
import FingerprintJS from "@fingerprintjs/fingerprintjs";

// State interface for the request hook
export interface State {
    instance: any;
    local_storage_name: string;
}

// Actions interface for the request hook
interface Actions {
    setToken: (token: string) => void;
    getToken: () => string;
    clearToken: () => void;
    request: (method: string, path: string, params: object, data: object) => Promise<any>;
}

// Combined Props interface for the request hook
interface Props extends State, Actions {}

// Default properties for the request hook
const default_props: Props = {
    instance: axios.create({
        baseURL: "",
        timeout: 60000,
    }),
    local_storage_name: "login:token",
    setToken: (token: string) => {
        localStorage.setItem(default_props.local_storage_name, token);
    },
    getToken() {
        return localStorage.getItem(default_props.local_storage_name) || "";
    },
    clearToken: () => {
        localStorage.removeItem(default_props.local_storage_name);
    },
    request: (method: string, path: string, params: object, data: object) => {
        return FingerprintJS.load()
            .then((fp: any) => {
                return fp.get().then((result: any) => {
                    // Configure and send the HTTP request using axios instance.
                    return default_props.instance({
                        headers: {
                            "Content-Type": "application/json",
                            "Content-X-Source": "browser",
                            "Content-X-Device": result.visitorId,
                            "Content-X-Language": localStorage.getItem("language") || "",
                            "Content-X-Time": Date.now().toString(),
                            "Content-X-Sign": default_props.getToken(),
                        },
                        url: path,
                        method: method,
                        params: params ? params : {},
                        data: data ? data : {},
                    });
                });
            })
            .catch((_error: any) => {
                // Configure and send the HTTP request using axios instance.
                return default_props.instance({
                    headers: {
                        "Content-Type": "application/json",
                        "Content-X-Source": "browser",
                        "Content-X-Device": "",
                        "Content-X-Language": localStorage.getItem("language") || "",
                        "Content-X-Time": Date.now().toString(),
                        "Content-X-Sign": default_props.getToken(),
                    },
                    url: path,
                    method: method,
                    params: params ? params : {},
                    data: data ? data : {},
                });
            });
    },
};

// Response interceptor to handle responses and errors
default_props.instance.interceptors.response.use(
    (response: any) => {
        if (response.status === 200) {
            return Promise.resolve(response);
        } else {
            return Promise.reject(response);
        }
    },
    (error: any) => {
        if (error.response) {
            if (error.response.status) {
                return false;
            }
        }
        return false;
    }
);

// Custom hook to use the request functionalities
export const useRequest = () => {
    return default_props;
};
