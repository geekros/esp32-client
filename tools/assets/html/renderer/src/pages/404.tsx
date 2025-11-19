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
import { Link, useNavigate } from "react-router-dom";

// A 404 Not Found page that redirects to home after 5 seconds.
export function NotFound(_props: any) {
    // Redirect to home after 5 seconds.
    const navigate = useNavigate();

    // Countdown state.
    const [countdown, setCountdown] = useState(5);

    // Start the countdown.
    useEffect(() => {
        const interval = setInterval(() => {
            setCountdown((prev) => {
                if (prev == 0) {
                    clearInterval(interval);
                    navigate("/");
                    return 0;
                }
                return prev - 1;
            });
        }, 1000);

        return () => {
            clearInterval(interval);
        };
    }, [navigate]);

    // Render the 404 page.
    return (
        <div className="w-full h-full flex items-center justify-center">
            <div className="w-auto space-y-2">
                <div className="w-full text-xl">Not Found</div>
                <div className="w-full text-sm">This page doesn’t exist.</div>
                <div className="w-full text-xs">
                    <span className="text-gray-500">Redirecting to </span>
                    <Link to="/" className="text-blue-500 hover:underline">
                        Home
                    </Link>
                    <span className="text-gray-500"> in {countdown} seconds...</span>
                </div>
            </div>
        </div>
    );
}
