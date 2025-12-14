# Protocol Description

Protocol is used for bidirectional communication between PC(host) and STM32F411E-disco(target) board using UART

## UART config

- Baud rate: 115200
- Data bits: 8
- Parity: None
- Stop bits: 1
- Flow control: None

## Frame format

Each protocol frame has the following structure:
| SOF | LEN | SIG_ID | PAYLOAD | CRC |

| Field   | Size  | Description                         | 
|---------|-------|-------------------------------------|
| SOF     | 1B    | Start of frame marker (0xAA)        |
| LEN     | 1B    | Number of SIG_ID + PAYLOAD in bytes |
| SIG_ID  | 1B    | Signal ID                           |
| PAYLOAD | NB    | Data                                |
| CRC     | 1B    | CRC-8 over SIG_ID + PAYLOAD         |

## Signals

| SIG_ID  | NAME                  | Direction         | Description                           |
|---------|-----------------------|-------------------|---------------------------------------|
| 0x01    | CONNECT_REQ           | Host  ->  Target  | Start connection                      |
| 0x02    | CONNECT_CFM           | Host  <-  Target  | Confirm connection                    |
| 0x03    | TICK_IND              | Host  ->  Target  | Connection watchdog (1 sec interval)  |
| 0x04    | TICK_CFM              | Host  <-  Target  | Connection confirmation               |
| 0x05    | BUTTON_IND            | Host  <-  Target  | Button pressed indicator              |
| 0x06    | BUTTON_CFM            | Host  ->  Target  | Confrim button pressed event          |
| 0x07    | DISCONNECT_REQ        | Host  ->  Target  | End connection                        |

## Host state machine

| STATE           | Action                                                                            |
|-----------------|-----------------------------------------------------------------------------------|
| INIT            | Init timers, protocol statee reset, COM port closed                               |
| CONNECTING      | Open COM port and send CONNECT_REQ                                                |
| CONNECTED       | Received CONNECT_CFM, sends first TICK_IND, waiting for events from the target    |
| DISCONNECTING   | Send DISCONNECT_REQ and close COM port                                            |

## Target state machine

| STATE           | Action                                                                             |
|-----------------|------------------------------------------------------------------------------------|
| IDLE            | LED1 is blinking every 500ms, LED2 is off                                          |
| CONNECTED       | LED1 is off, LED2 is on                                                            |
| BUTTON_PRESSED  | Waits confirmation from the host, LED1 blinkes 3 times every second and goes off   |
| BUTTON_DISABLED | LED1 is off, LED2 is on, button pushes are ignoring                                |

## Receiving frames state machine

Host and target receiveres use the same rx state machine
| STATE           | Description                                      |
|-----------------|--------------------------------------------------|
| SOF_WAITING     | Wait for SOF byte (0xAA)                         |
| LEN_READING     | Read LEN byte                                    |
| PAYLOAD_READING | Read SIG_ID and PAYLOAD bytes accoriding to LEN  |
| CRC_READING     | Read CRC byte                                    |

Note: On CRC_READING failure, the frame is invalid and state returns to SOF_WAITING

## Connection watchdog

Host sends TICK_IND every second to the target
Target respondes to the host by TICK_CFM
Both host and target maintain a connection timeout
If no valid frame is received within 5 seconds, the connection is lost and then:
  - host returns to the INIT state and print message "Connection is lost"
  - target returns to the IDLE state
Only frames with valid CRC are considered as a valid frame

## Other timeouts

Timeout threshold for CONNECT_CFM is 1 second.
If CONNECT_CFM doesn't received:
  Host:
  - closes the COM port
  - prints "Check connection"
  - goes to the INIT state
