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

// Include headers
#include "opus_resampler.h"

// Define log tag
#define TAG "[client:components:opus:resampler]"

// Constructor
OpusResampler::OpusResampler()
{
}

// Destructor
OpusResampler::~OpusResampler()
{
}

// Configure resampler
void OpusResampler::Configure(int input_sample_rate_, int output_sample_rate_)
{
    // Check for valid sample rates
    int encode = input_sample_rate_ > output_sample_rate_ ? 1 : 0;

    // Initialize Silk resampler
    auto ret = silk_resampler_init(&resampler_state, input_sample_rate_, output_sample_rate_, encode);
    if (ret != 0)
    {
        ESP_LOGE(TAG, "Failed to initialize resampler");
        return;
    }

    // Save sample rates
    input_sample_rate = input_sample_rate_;

    // Save output sample rate
    output_sample_rate = output_sample_rate_;
}

// Process resampling
void OpusResampler::Process(const int16_t *input, int input_samples, int16_t *output)
{
    // Call Silk resampler
    auto ret = silk_resampler(&resampler_state, output, input, input_samples);
    if (ret != 0)
    {
        ESP_LOGE(TAG, "Failed to process resampler");
    }
}

// Get output samples count
int OpusResampler::GetOutputSamples(int input_samples) const
{
    // Calculate and return output samples
    return input_samples * output_sample_rate / input_sample_rate;
}