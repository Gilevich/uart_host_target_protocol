#pragma once

#include "protocol.hpp"
#include "ringBuffer.hpp"

#include <cstdint>
#include <cstring>

/**
 * @brief Target device application logic.
 *
 * All time-related logic is driven by a 1 ms system tick
 */
class Target
{
public:
/**
 * @brief Target FSM states.
 */
  enum class StateE
  {
    IDLE,
    CONNECTED,
    BUTTON_PRESSED,
    BUTTON_DISABLED
  };
  Target() = default;
  ~Target() = default;


  /**
   * @brief Initialize target logic (UART RX, initial state).
   * Call once during system startup.
   */
  void init();

  /**
   * @brief Main non-blocking processing loop.
   * Call periodically from main while(1).
   */
  void process();

  /**
   * @brief UART RX byte handler.
   * Call from HAL_UART_RxCpltCallback().
   */
  void receiver();

  /**
   * @brief Increment millisecond counter.
   * Call from timer ISR (1 ms tick).
   */
  void incTimerMsCounter();

  // --- Externall events for ISR handlers -----------------------------------------

  /**
   * @brief Handle HW button press event.
   * Call from HAL_GPIO_EXTI_Callback().
   */
  void handleButtonPress();

  /**
   * @brief UART TX complete callback.
   * Call from HAL_UART_TxCpltCallback().
   */
  void onTxDone();

  // --- Public data -----------------------------------------------------------

  /**
   * @brief Single-byte UART RX buffer
   * Used with HAL_UART_Receive_IT.
   */
  uint8_t rxByte_ {0};

private:
  /**
   * @brief Handle received signal frame.
   */
  void handleSignal(const protocol::FrameS& frame);

  /**
   * @brief Change internal FSM state.
   */
  void changeState(StateE newState);

  /**
   * @brief Push a frame to UART TX queue.
   */
  void sendFrame(protocol::signalIdE sig);

  /**
   * @brief Start UART transmission if not already in progress.
   */
  void tryStartTx();

  /**
   * @brief Configure LEDs for CONNECTED state.
   */
  void setLedsInConnectedState();

  // --- Internal state -----------------------------------------------------------

  protocol::Decoder decoder_;
  StateE state_ = StateE::IDLE;

  // Time tracking (in ms)
  volatile uint32_t msCounter_ {0};
  volatile uint32_t lastRxTime_ {0};

  // LED1 blinking control
  uint32_t lastBlinkTime_ {0};
  uint32_t blinkCounter_ {0};

  // UART TX queue
  constexpr static size_t NUM_FRAMES = 4;
  RingBuffer<NUM_FRAMES, protocol::MAX_FRAME_SIZE> txQueue_;
  bool txBusy_ {false};
};
