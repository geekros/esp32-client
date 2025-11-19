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

import i18next from "i18next";
import Backend from "i18next-http-backend";
import { createContext, useContext, useEffect, useState } from "react";
import { initReactI18next, useTranslation, Trans } from "react-i18next";

type LanguageProps = {
    children: React.ReactNode;
};

type LanguageType = {
    i18next: typeof i18next;
    language: string;
    trans: typeof Trans;
    lang: (key: string) => string;
    format: (value: string) => string;
    change: (_lang: string) => void;
};

const local_storage_name = "language";
const language = (typeof window !== "undefined" && localStorage.getItem(local_storage_name)) || navigator.language.slice(0, 2).toLowerCase();

const Context = createContext<LanguageType>({
    i18next: i18next,
    language: language,
    trans: Trans,
    lang: (key: string) => key,
    format: (value: string) => value,
    change: (_lang: any) => {},
});

i18next
    .use(initReactI18next)
    .use(Backend)
    .init({
        lng: language,
        fallbackLng: "en",
        debug: false,
        backend: {
            loadPath: ((window as any).extensionUri ? (window as any).extensionUri : "") + "/locales/{{lng}}/{{ns}}.json",
        },
        ns: ["common"],
        defaultNS: "common",
        interpolation: {
            escapeValue: true,
        },
    });

export const useLanguage = () => useContext<LanguageType>(Context);

export const LanguageContext = ({ children, ...props }: LanguageProps) => {
    const [language, setLanguage] = useState(i18next.language);

    const { t } = useTranslation();

    const value = {
        i18next,
        language,
        trans: Trans,
        lang: (key: string) => t(key),
        format: (value: string) => value.toLowerCase().replace(/\s+/g, "_"),
        change: (lang: any) => {
            i18next.changeLanguage(lang);
            setLanguage(lang);
            localStorage.setItem(local_storage_name, lang);
        },
    };

    useEffect(() => {
        localStorage.setItem(local_storage_name, language);
        const savedLanguage = localStorage.getItem(local_storage_name);
        if (savedLanguage) {
            i18next.changeLanguage(savedLanguage);
            setLanguage(savedLanguage);
        } else {
            const browserLanguage = i18next.language;
            i18next.changeLanguage(browserLanguage);
            setLanguage(browserLanguage);
        }
    }, []);

    return (
        <Context {...props} value={value}>
            {children}
        </Context>
    );
};
