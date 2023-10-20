#pragma once
#include "gtest/gtest.h"
#define ESP_LOGI(TAG, msg, ...) printf("%s: ", TAG); printf(msg, __VA_ARGS__);printf("\n");
