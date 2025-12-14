# STM32F411E Host-Target UART Communication

This project demonstrates a **bidirectional UART communication protocol** between a PC (host) and an STM32F411E-DISCO board (target).
It is implemented in **C++11** and uses a simple protocol with error detection and connection watchdog.

## Features

- Host communicates with target using **UART** (115200, 8N1, no flow control)
- Protocol includes:
  - Connection request/confirmation
  - Heartbeat (TICK_IND / TICK_CFM)
  - Button press notification
  - Disconnection
  - CRC-8 error detection
- Host monitors connection and handles timeouts
- Target handles LEDs and button events

## Target Behavior

The STM32F411E-DISCO target implements the following behavior:

- **After booting:**
  - LED1 blinks every 500 ms
  - LED2 is off

- **After establishing communication with the host:**
  - LED2 is on
  - LED1 is off

- **After closing communication with the host:**
  - LED2 is off
  - LED1 blinks every 500 ms

- **When LED2 is on and HWButton1 is pressed:**
  1. Target sends button event information to the host
  2. Host confirms the received information
  3. After receiving the confirmation:
     - LED1 blinks three times per second and then goes off
     - Further button presses are ignored until the next connection

## Build instruction

## Protocol

## License