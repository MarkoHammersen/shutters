# ESP32 Shutter Control System
This project implements a **ESP32-based shutter control system** designed for controlling shutter motors via relays, with clear separation between hardware abstraction and business logic.
The system combines **ESP32**, **Microchip MCP23017 I/O expanders**, and **ULN2003A Darlington arrays**, and is implemented in **C++ using PlatformIO with the Arduino framework**.

---

## System Overview
The shutter system consists of:

* An **ESP32** as the central controller
* **MCP23017 I/O expanders** for scalable digital input and output via I²C
* **ULN2003A Darlington transistor arrays** for driving relay coils
* A dedicated **external 5 V power supply** for stable I/O operation

All safety-relevant design aspects (galvanic separation, flyback protection, low coil current) are explicitly addressed in the extern hardware design, which is not part of the schematics attached.

---

## Hardware Architecture

### Inputs (Buttons / Wall Switches)

* Digital inputs are handled by an **MCP23017 configured as input expander**
* Button **press and release events** are captured
* Each input event triggers a **hardware interrupt**
* The interrupt line is connected to the ESP32
* Upon interrupt, the ESP32:

  1. Starts an I²C transaction
  2. Reads the MCP23017 input registers
  3. Determines the exact event (press or release)

This approach ensures:

* Low latency
* Minimal polling
* Efficient I²C bus usage

---

### Outputs (Shutter Actuation)

* Outputs are handled by a second **MCP23017 configured as output expander**
* Output states are sent from the ESP32 via **I²C**
* MCP23017 outputs drive **ULN2003A Darlington arrays**
* The Darlington arrays drive the **relay coils**

#### Relay Driving

* Relay coil current is low enough to be **safely driven by ULN2003A**
* **Flyback diodes** are mounted directly at the relay coils
* This protects the Darlington arrays and improves EMC robustness

> ⚠️ **Note:** Mains voltage must only be handled by qualified personnel.
---

### Power Supply

* All MCP23017 inputs and outputs are powered by a **stable external 5 V supply**
* This ensures:

  * Reliable logic levels
  * Noise immunity
  * Stable operation independent of ESP32 load conditions

---

## Software Architecture

The firmware is written in **C++** and built using:

* **PlatformIO**
* **Arduino framework**
* **ESP32 target**

The software is cleanly structured into three main layers:

### 1. Sensors

* Abstraction of all input devices
* MCP23017 input handling
* Interrupt processing
* Debouncing and event detection (press / release)

### 2. Actuators

* Abstraction of all output devices
* MCP23017 output handling
* Relay control logic
* Hardware-independent actuator interface

### 3. Shutter Business Logic

* High-level shutter control logic
* Direction handling (up / down)
* Timing and safety constraints
* Mutual exclusion of motor directions

#### State Machine

The business logic is implemented using a **state machine** based on:

👉 **Quantum Leaps – State-Oriented Programming**
[https://github.com/QuantumLeaps/State-Oriented-Programming](https://github.com/QuantumLeaps/State-Oriented-Programming)

This provides:

* Explicit, deterministic state handling
* Clear separation of states and events
* Improved maintainability and testability
* Robust behavior for asynchronous events (buttons, interrupts)

---

## Documentation & Schematics

* Hardware schematics are included in the repository

---

## Key Features

* ESP32-based central control
* Interrupt-driven input handling
* Scalable I/O via MCP23017
* Safe relay driving using ULN2003A
* Clear separation of concerns in software
* State-machine-driven business logic


⚠️ WARNING

This project uses electromechanical actuators.
Improper handling can result in serious injury, fire, or death.

This project is provided for educational and experimental purposes only.
The author assumes no liability for any damage, injury, or loss resulting
from the use, misuse, or modification of this hardware or software.

Only qualified personnel should work with mains voltage systems.

---
This project is not certified for commercial or residential use.
No compliance with CE, VDE, UL, or any other safety standard is claimed.
--
Use at your own risk!
