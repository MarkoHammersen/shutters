#ifndef __MESSAGE_H__
#define __MESSAGE_H__

typedef enum 
{
    NONE,
    RUN,
    TIMEOUT,
    STOP,
}appEvent_t;

typedef struct
{ 
    appEvent_t evt;
    uint8_t i2cAddr;
    int data;
}appMessage_t;

extern QueueHandle_t qHandleActuators;
extern QueueHandle_t qHandleShutters;


#endif // __MESSAGE_H__