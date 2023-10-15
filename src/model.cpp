#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "cpp/hsm.hpp"
#include "model.h"

using namespace std;
#define TAG "shutter"

// class Interrupt
// {
// protected:
//     string pin;

// public:
//     Interrupt(string pin)
//     {
//         pin = pin;
//     }
//     string getPin()
//     {
//         return pin;
//     }
// };

// class Port
// {
// protected:
//     string device, pin;

// public:
//     Port(string device, string pin)
//     {
//         device = device;
//         pin = pin;
//     }
//     string getDevice()
//     {
//         return device;
//     }
//     string getPin()
//     {
//         return pin;
//     }
// };

// class Sensor
// {
// protected:
//     Port up, down;
//     Interrupt interrupt;

// public:
//     Sensor(Port up, Port down, Interrupt interrupt)
//     {
//         up = up;
//         down = down;
//         interrupt = interrupt;
//     }
//     Port getUp()
//     {
//         return up;
//     }
//     Port getDown()
//     {
//         return down;
//     }
//     string getInterrupt()
//     {
//         return interrupt;
//     }
// };

// class Actuator
// {
// protected:
//     Port up, down;

// public:
//     Actuator(Port up, Port down)
//     {
//         up = up;
//         down = down;
//     }
//     Port getUp()
//     {
//         return up;
//     }
//     Port getDown()
//     {
//         return down;
//     }
// };

// class Shutter : public Hsm
// {
// protected:
//     string room;
//     string dir;
//     Sensor sensor;
//     Actuator actuator;

//     uint32_t timeout;
//     uint32_t tRunning;
//     uint32_t tDebounce;
//     uint32_t tStopDebounce;

// public:
//     Shutter(string room, string dir, Sensor s, Actuator a);
//     Msg const *topHndlr(Msg const *msg);
//     Msg const *idleHndlr(Msg const *msg);
//     Msg const *stopHndlr(Msg const *msg);
//     Msg const *debounceHndlr(Msg const *msg);
//     Msg const *upHndlr(Msg const *msg);
//     Msg const *downHndlr(Msg const *msg);
// };


static TaskHandle_t taskHandleModel = NULL;
static QueueHandle_t qHandleModel = NULL;

// /*
// Name	    Input				                    Output
//         	(MSP23017	Up	Down	Interrupt)	    (MSP23017	Up	Down)
// Lina Ost	U36	        B0	B1	    B	            U4	        A6	A5
// Lina Süd	U36	        B2	B3	    B	            U4	        A4	A3
// Flur	    U36	        B4	B5	    B	            U4	        A2	A1
// Wäsche	    U36	        B6	B7	    B	            U4	        A0	B6
// Bad Süd	    U36	        A3	A2	    A	            U4	        B5	B4
// Bad Nord	U36	        A1	A0	    A	            U4	        B3	B2
// Julia West	U1	        B0	B1	    B	            U4	        B1	B0
// Julia Ost	U1	        B2	B3	    B	            U37	        A6	A5
// Paul Ost	U1	        B4	B5	    B	            U37	        A4	A3
// Paul Nord	U1	        B6	B7	    B	            U37	        A2	A1
// Büro	    U1	        A7	A6	    A	            U37	        B6	B5
// */

// static vector<Shutter> shutters =
// {
//     Shutter("Lina", "Ost",      Sensor(Port("U36",  "B0"), Port("U36",  "B1"), Interrupt("B")), Actuator(Port("U4",  "A6"), Port("A6", "A5"))),
//     Shutter("Lina", "Sued",     Sensor(Port("U36",  "B2"), Port("U36",  "B3"), Interrupt("B")), Actuator(Port("U4",  "A4"), Port("A4", "A3"))),
//     Shutter("Flur", "Sued",     Sensor(Port("U36",  "B4"), Port("U36",  "B5"), Interrupt("B")), Actuator(Port("U4",  "A2"), Port("A2", "A1"))),
//     Shutter("Waesche", "Sued",  Sensor(Port("U36",  "B6"), Port("U36",  "B7"), Interrupt("B")), Actuator(Port("U4",  "A0"), Port("A0", "B6"))),
//     Shutter("Bad", "Sued",      Sensor(Port("U36",  "A3"), Port("U36",  "A2"), Interrupt("A")), Actuator(Port("U4",  "A0"), Port("B5", "B4"))),
//     Shutter("Bad", "Nord",      Sensor(Port("U36",  "A1"), Port("U36",  "A0"), Interrupt("A")), Actuator(Port("U4",  "B5"), Port("B3", "B2"))),
//     Shutter("Julia", "West",    Sensor(Port("U1",   "B0"), Port("U1",   "B1"), Interrupt("B")), Actuator(Port("U4",  "B3"), Port("B1", "B0"))),
//     Shutter("Julia", "Nord",    Sensor(Port("U1",   "B2"), Port("U1",   "B3"), Interrupt("B")), Actuator(Port("U37", "B1"), Port("A6", "A5"))),
//     Shutter("Paul", "Nord",     Sensor(Port("U1",   "B4"), Port("U1",   "B5"), Interrupt("B")), Actuator(Port("U37", "B1"), Port("A4", "A3"))),
//     Shutter("Paul", "Ost",      Sensor(Port("U1",   "B6"), Port("U1",   "B7"), Interrupt("B")), Actuator(Port("U37", "B1"), Port("A2", "A1"))),
//     Shutter("Buero", "Ost",     Sensor(Port("U1",   "A7"), Port("U1",   "A6"), Interrupt("A")), Actuator(Port("U37", "B1"), Port("B6", "B5")))
// };

// enum ShutterEvents
// {
//     SHUTTER_UP_EVT,
//     SHUTTER_DOWN_EVT,
//     SHUTTER_IDLE_EVT,
//     SHUTTER_TIMEOUT_EVT
// };

// const Msg shutterMsg[] = {
//     (Event)SHUTTER_UP_EVT,
//     (Event)SHUTTER_DOWN_EVT,
//     (Event)SHUTTER_IDLE_EVT,
//     (Event)SHUTTER_TIMEOUT_EVT};

// Msg const *Shutter::downHndlr(Msg const *msg)
// {
//   switch (msg->evt)
//   {
//   case START_EVT:
//     trace("down-INIT\n");
//     return 0;

//   case ENTRY_EVT:
//     trace("down-ENTRY\n");
//     if (NULL != relayModule)
//     {
//       relayModule->changeMode(relayUp, OFF);
//       relayModule->changeMode(relayDown, ON);
//       tDebounce = 0u;
//     }
//     return 0;

//   case EXIT_EVT:
//     trace("down-EXIT\n");
//     return 0;

//   default:
//     if (ButtonEvent::UpEvt == currentBtnEvt)
//     {
//       STATE_TRAN(&stStop);
//       return 0;
//     }
//     tDebounce += timeBase;
//     if (tDebounce >= timeMaxDebounce)
//     {
//       STATE_TRAN(&stRunning);
//     }
//     return 0;
//   }
//   return msg;
// }

// Msg const* Shutter::stopHndlr(Msg const* msg)
// {
//   switch (msg->evt)
//   {
//   case START_EVT:
//     trace("idle-INIT\n");
//     return 0;

//   case ENTRY_EVT:
//     trace("idle-ENTRY\n");
//     relayModule->changeMode(relayUp, OFF);
//     relayModule->changeMode(relayDown, OFF);
//     tStopDebounce = 0;
//     return 0;

//   case EXIT_EVT:
//     trace("idle-EXIT\n");
//     return 0;

//   case static_cast <Event>(ShutterEvent::TickEvt):
//     if (ButtonEvent::ReleaseEvt == currentBtnEvt)
//     {
//       if (tStopDebounce > timeMaxDebounce) // ticks
//       {
//         STATE_TRAN(&stIdle);
//       }
//       else
//       {
//         tStopDebounce += timeBase;
//       }
//     }
//     return 0;
//   }
//   return msg;
// }

// Msg const *Shutter::idleHndlr(Msg const *msg)
// {
//   switch (msg->evt)
//   {
//   case START_EVT:
//     trace("idle-INIT\n");
//     return 0;

//   case ENTRY_EVT:
//     trace("idle-ENTRY\n");
//     return 0;

//   case EXIT_EVT:
//     trace("idle-EXIT\n");
//     return 0;

//     case static_cast<Event>(ShutterEvent::TickEvt) :
//     {
//       if (ButtonEvent::UpEvt == currentBtnEvt)
//       {
//         STATE_TRAN(&stUp);
//         return 0;
//       }

//       if (ButtonEvent::DownEvt == currentBtnEvt)
//       {
//         STATE_TRAN(&stDown);
//         return 0;
//       }
//     }
//   }
//   return msg;
// }

// Msg const *Shutter::upHndlr(Msg const *msg)
// {
//   switch (msg->evt)
//   {
//   case START_EVT:
//     trace("up-INIT\n");
//     return 0;

//   case ENTRY_EVT:
//     trace("up-ENTRY\n");
//     // relayModule->changeMode(relayDown, OFF);
//     // relayModule->changeMode(relayUp, ON);
//     tDebounce = 0u;
//     return 0;

//   case EXIT_EVT:
//     trace("up-EXIT\n");
//     return 0;

//   default:
//   {
//     if (ButtonEvent::DownEvt == currentBtnEvt)
//     {
//       STATE_TRAN(&stStop);
//       return 0;
//     }

//     tDebounce += timeBase;
//     if (tDebounce >= timeMaxDebounce ) // ticks
//     {
//       STATE_TRAN(&stRunning);
//     }
//   }
//     return 0;
//   }
//   return msg;
// }

// Msg const *Shutter::topHndlr(Msg const *msg)
// {
//   switch (msg->evt) {
//   case START_EVT:
//     trace("top-INIT\n");
//     if (NULL != relayModule)
//     {
//       STATE_START(&stStop);
//     }
//     return 0;
//   case ENTRY_EVT:
//     trace("top-ENTRY\n");
//     return 0;
//   case EXIT_EVT:
//     trace("top-EXIT\n");
//     return 0;
//   }
//   return msg;
// }

// Msg const* Shutter::runningHndlr(Msg const* msg)
// {
//   switch (msg->evt) {
//   case START_EVT:
//     trace("running-INIT\n");
//     return 0;
//   case ENTRY_EVT:
//     trace("running-ENTRY\n");
//     tRunning = 0u;
//     return 0;
//   case EXIT_EVT:
//     trace("running-EXIT\n");
//     return 0;
//   default:
//     if (ButtonEvent::ReleaseEvt != currentBtnEvt)
//     {
//       STATE_TRAN(&stStop);
//       return 0;
//     }

//     tRunning = tRunning + timeBase;
//     if (tRunning >= timeout)
//     {
//       STATE_TRAN(&stStop);
//     }
//     return 0;
//   }
//   return msg;
// }
// Shutter::Shutter(string room,
//     string dir,
//     Sensor sensor,
//     Actuator actuator)
//     : Hsm("shutter", (EvtHndlr)&Shutter::topHndlr),
//       idle("idle", &top, (EvtHndlr)&Shutter::idleHndlr),
//       up("up", &up, (EvtHndlr)&Shutter::upHndlr),
//       down("down", &down, (EvtHndlr)&Shutter::downHndlr),
//       debounce("debounce", &debounce, (EvtHndlr)&Shutter::debounceHndlr),
//       stop("stop", &top, (EvtHndlr)&Shutter::stopHndlr){
//     room = room;
//     dir = dir;
//     sensor = sensor;
//     actuator = actuator;
// }

static void modelTask(void *args)
{
  (void *)args;
  uint8_t buffer[32];
  qHandleModel = xQueueCreate(128, sizeof(buffer));
  if (NULL == qHandleModel)
  {
    ESP_LOGE(TAG, "q creation failed");
    return;
  }

  while (xQueueReceive(qHandleModel, buffer, portMAX_DELAY))
  {
  }
}
void initializeModel()
{
  if (pdPASS != xTaskCreate(modelTask, "modelTask", 4096, NULL, 10, &taskHandleModel))
  {
    ESP_LOGE(TAG, "task setup failed");
    return;
  }
  ESP_LOGI(TAG, "task setup done!");
}