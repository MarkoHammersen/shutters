#pragma once

typedef enum
{
    ACTUATOR_EVT_STOP,
    ACTUATOR_EVT_RUN,
    SENSOR_EVT_USR_INPUT
}event_t;

typedef struct messages
{
    event_t evt;
    uint8_t device;
    uint8_t pin;
}message_t;
