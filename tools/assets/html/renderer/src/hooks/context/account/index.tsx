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

import { createContext, useContext, useEffect, useState } from "react";

// Define the context for account information.
type AccountProps = {
    children: React.ReactNode;
};

// Define the context type
export type AccountType = {
    account: AccountStructure;
    update_account: (value: AccountStructure) => void;
};

// Define the default value
export interface AccountStructure {
    token: string;
    nickname: string;
    email: string;
    avatar: string;
}

// Initial state for the context
const account_state: AccountType = {
    account: {} as AccountStructure,
    update_account: (_value: AccountStructure) => {},
};

// Create the context
const Context = createContext<AccountType>(account_state);

// Custom hook to use the account context
export const useAccount = () => useContext<AccountType>(Context);

// Account context provider component
export const AccountContext = ({ children, ...props }: AccountProps) => {
    // State to hold the context value
    const [account, setAccount] = useState<AccountStructure>(account_state.account);

    // Function to update the context state
    const update_account = (value: AccountStructure) => {
        setAccount((prev: any) => ({ ...prev, ...value }));
    };

    const value = {
        account: account,
        update_account: update_account,
    };

    // Side effect to run on component mount
    useEffect(() => {}, []);

    // Render the context provider with the given children
    return (
        <Context {...props} value={value}>
            {children}
        </Context>
    );
};
