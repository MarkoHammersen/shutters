#ifndef __MESSAGE_H__
#define __MESSAGE_H__

struct AppEvents
{
    enum Names
    {
        NONE = 0,
        TOUCH, 
        RUN,
        TIMEOUT,
        STOP
    };
};



typedef struct
{
    AppEvents::Names evt;
    uint8_t i2cAddr;
    uint16_t data;
} appMessage_t;

extern QueueHandle_t qHandleActuators;
extern QueueHandle_t qHandleShutters;

#endif // __MESSAGE_H__