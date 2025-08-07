#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _EMSCRIPTEN_
#include <emscripten/emscripten.h>
#endif

#define MOD_FREQ 20.0f
#define PI 3.14159265359f

typedef struct {
    float modPhase;
    float modPhaseIncr;

    ma_int16* delayBuffer;
    ma_uint32 delayBufferFrames;
    ma_uint32 delayBufferChannels;
    ma_uint32 writePos;
    ma_uint32 sampleDelayFrames;
} EffectData;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    EffectData* effect = (EffectData*)pDevice->pUserData;

    if (pInput == NULL || pOutput == NULL) return;

    ma_int16* in  = (ma_int16*)pInput;
    ma_int16* out = (ma_int16*)pOutput;
    ma_uint32 channels = pDevice->capture.channels;

    for (ma_uint32 i = 0; i < frameCount; ++i) {
        for (ma_uint32 c = 0; c < channels; ++c) {
            // Write incoming sample into buffer
            effect->delayBuffer[effect->writePos] = *in++;

            // Calculate delayed read position
            ma_uint32 readPos = (effect->writePos + effect->delayBufferFrames * channels - (effect->sampleDelayFrames * channels)) % (effect->delayBufferFrames * channels);

            ma_int16 delayedSample = effect->delayBuffer[readPos];

            // Optional: add alien modulation
            float mod = sinf(effect->modPhase);
            effect->modPhase += effect->modPhaseIncr;
            if (effect->modPhase > 2 * PI) effect->modPhase -= 2 * PI;

            float modulated = (float)delayedSample * mod;

            // Clamp
            if (modulated > 32767.0f) modulated = 32767.0f;
            if (modulated < -32768.0f) modulated = -32768.0f;

            *out++ = (ma_int16)modulated;

            // Advance write pointer
            effect->writePos++;
            if (effect->writePos >= effect->delayBufferFrames * channels) {
                effect->writePos = 0;
            }
        }
    }
}

#ifdef _EMSCRIPTEN_
void main_loop__em() {}
#endif

int main(int argc, char** argv) {
    ma_result result;
    ma_device_config deviceConfig;
    ma_device device;
    EffectData effect;
    memset(&effect, 0, sizeof(effect));

    deviceConfig = ma_device_config_init(ma_device_type_duplex);
    deviceConfig.capture.format = ma_format_s16;
    deviceConfig.capture.channels = 2;
    deviceConfig.capture.pDeviceID = NULL;
    deviceConfig.capture.shareMode = ma_share_mode_shared;

    deviceConfig.playback.format = ma_format_s16;
    deviceConfig.playback.channels = 2;
    deviceConfig.playback.pDeviceID = NULL;
    deviceConfig.playback.shareMode = ma_share_mode_shared;

    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = &effect;

    result = ma_device_init(NULL, &deviceConfig, &device);
    if (result != MA_SUCCESS) {
        printf("Failed to initialize audio device: %d\n", result);
        return -1;
    }

    effect.modPhase = 0.0f;
    effect.modPhaseIncr = (2.0f * PI * MOD_FREQ) / (float)device.sampleRate;

    effect.delayBufferChannels = device.capture.channels;
    effect.delayBufferFrames = (ma_uint32)(device.sampleRate * 4); // Make it larger than delay time
    effect.sampleDelayFrames = (ma_uint32)(device.sampleRate * 2); // 2 seconds delay

    effect.delayBuffer = (ma_int16*)malloc(effect.delayBufferFrames * effect.delayBufferChannels * sizeof(ma_int16));
    if (effect.delayBuffer == NULL) {
        printf("Failed to allocate delay buffer.\n");
        ma_device_uninit(&device);
        return -1;
    }
    memset(effect.delayBuffer, 0, effect.delayBufferFrames * effect.delayBufferChannels * sizeof(ma_int16));

    effect.writePos = 0;

    result = ma_device_start(&device);
    if (result != MA_SUCCESS) {
        printf("Failed to start audio device: %d\n", result);
        free(effect.delayBuffer);
        ma_device_uninit(&device);
        return -1;
    }

#ifdef _EMSCRIPTEN_
    emscripten_set_main_loop(main_loop__em, 0, 1);
#else
    printf("🎙 Continuous 2s Delayed Voice Active! Press Enter to stop...\n");
    getchar();
#endif

    ma_device_uninit(&device);
    free(effect.delayBuffer);
    return 0;
}
