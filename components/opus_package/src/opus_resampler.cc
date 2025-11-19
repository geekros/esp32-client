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

// Function to create and initialize an Opus resampler
opus_resampler_t *opus_resampler_create(void)
{
    // Allocate memory for the resampler
    opus_resampler_t *r = (opus_resampler_t *)calloc(1, sizeof(opus_resampler_t));
    if (!r)
    {
        return NULL;
    }

    // Return the created resampler
    return r;
}

// Function to configure the Opus resampler with input and output sample rates
bool opus_resampler_configure(opus_resampler_t *res, int input_sample_rate, int output_sample_rate)
{
    // Check for null pointer
    if (!res)
    {
        return false;
    }

    // Initialize the Silk resampler state
    int encode = input_sample_rate > output_sample_rate ? 1 : 0;

    // Initialize Silk resampler
    int ret = silk_resampler_init(&res->state, input_sample_rate, output_sample_rate, encode);
    if (ret != 0)
    {
        return false;
    }

    // Set sample rates
    res->input_sample_rate = input_sample_rate;
    res->output_sample_rate = output_sample_rate;

    // Return success
    return true;
}

// Function to process audio samples through the Opus resampler
bool opus_resampler_process(opus_resampler_t *res, const int16_t *input, int input_samples, int16_t *output)
{
    // Check for null pointer
    if (!res)
    {
        return false;
    }

    // Perform resampling
    int ret = silk_resampler(&res->state, output, input, input_samples);
    if (ret != 0)
    {
        return false;
    }

    // Return success
    return true;
}

// Function to get the number of output samples for given input samples
int opus_resampler_get_output_samples(opus_resampler_t *res, int input_samples)
{
    // Check for null pointer
    if (!res || res->input_sample_rate == 0)
    {
        return 0;
    }

    // Calculate and return output samples
    return (input_samples * res->output_sample_rate) / res->input_sample_rate;
}

// Function to destroy and free an Opus resampler
void opus_resampler_destroy(opus_resampler_t *res)
{
    // Check for null pointer
    if (!res)
    {
        return;
    }

    // Free the resampler structure
    free(res);
}