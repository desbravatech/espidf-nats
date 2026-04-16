/**
 * Minimal ESP-IDF stubs for host-side unit testing
 * Only stubs the types/functions needed by the pure-logic headers
 */
#ifndef ESP_STUBS_H
#define ESP_STUBS_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>

// ESP log stubs
#define ESP_LOGE(tag, fmt, ...) fprintf(stderr, "E [%s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) fprintf(stderr, "W [%s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) fprintf(stderr, "I [%s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...)

// esp_random stub
inline uint32_t esp_random() {
    return (uint32_t)rand();
}

// FreeRTOS stubs
typedef void* SemaphoreHandle_t;
#define portMAX_DELAY 0xFFFFFFFF
#define pdTRUE 1
#define pdFALSE 0
#define pdMS_TO_TICKS(ms) (ms)

inline SemaphoreHandle_t xSemaphoreCreateMutex() { return (void*)1; }
inline SemaphoreHandle_t xSemaphoreCreateRecursiveMutex() { return (void*)1; }
inline SemaphoreHandle_t xSemaphoreCreateBinary() { return (void*)1; }
inline int xSemaphoreTake(SemaphoreHandle_t, uint32_t) { return pdTRUE; }
inline int xSemaphoreTakeRecursive(SemaphoreHandle_t, uint32_t) { return pdTRUE; }
inline int xSemaphoreGive(SemaphoreHandle_t) { return pdTRUE; }
inline int xSemaphoreGiveRecursive(SemaphoreHandle_t) { return pdTRUE; }
inline void vSemaphoreDelete(SemaphoreHandle_t) {}
inline void vTaskDelay(uint32_t) {}

// cJSON is available via libcjson-dev
#include <cjson/cJSON.h>

#endif // ESP_STUBS_H
