/*
Copyright 2025 GEEKROS, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef OPUS_RESAMPLER_H
#define OPUS_RESAMPLER_H

// Include standard headers
#include <string>
#include <cstdint>

// Include ESP headers
#include <esp_log.h>
#include <esp_err.h>

// Include Opus headers
#include "opus.h"
#include "resampler_structs.h"
#include "resampler_silk.h"

// Opus Resampler Class
class OpusResampler
{
private:
    // Member variables
    silk_resampler_state_struct resampler_state;
    int input_sample_rate;
    int output_sample_rate;

public:
    // Constructor and Destructor
    OpusResampler();
    ~OpusResampler();

    // Configure resampler
    void Configure(int input_sample_rate_, int output_sample_rate_);
    void Process(const int16_t *input, int input_samples, int16_t *output);
    int GetOutputSamples(int input_samples) const;

    // Getters for sample rates
    int InputSampleRate() const { return input_sample_rate; }
    int OutputSampleRate() const { return output_sample_rate; }
};

#endif