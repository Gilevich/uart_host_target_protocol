extern "C" {
  #include "main.h"
}

#include "target.hpp"

#include <cstdint>

extern UART_HandleTypeDef huart1;

namespace
{
constexpr uint32_t TICK_TIMEOUT_MS                  = 5000;
constexpr uint32_t LED1_IDLE_INTERVAL_MS            = 500;
constexpr uint32_t LED1_BUTTON_DISABLE_INTERVAL_MS  = 1000;
constexpr uint32_t BUTTON_DISABLE_BLINKS            = 3;
}

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------
void Target::init()
{
  changeState(StateE::IDLE);
  HAL_UART_Receive_IT(&huart1, &rxByte_, 1);
}

// -----------------------------------------------------------------------------
// Main processing loop
// -----------------------------------------------------------------------------
void Target::process()
{
  // Start UART TX if not already in progress
  if (!txBusy_ && !txQueue_.empty())
  {
    tryStartTx();
  }

  // Connecting timeout
  if (state_ != StateE::IDLE &&
     static_cast<uint32_t>(msCounter_ - lastRxTime_) >= TICK_TIMEOUT_MS)
  {
    changeState(StateE::IDLE);
  }

  // LED1 blinking control in IDLE state
  if (state_ == StateE::IDLE &&
      static_cast<uint32_t>(msCounter_ - lastBlinkTime_) >= LED1_IDLE_INTERVAL_MS)
  {
    lastBlinkTime_ = msCounter_;
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
  }

  // LED1 blinking control in BUTTON_DISABLED state
  if (state_ == StateE::BUTTON_DISABLED &&
      static_cast<uint32_t>(msCounter_ - lastBlinkTime_) >= LED1_BUTTON_DISABLE_INTERVAL_MS/2)
  {
    if (blinkCounter_++ < BUTTON_DISABLE_BLINKS * 2)
    {
      lastBlinkTime_ = msCounter_;
      HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    }
  }
}

// -----------------------------------------------------------------------------
// State handling
// -----------------------------------------------------------------------------
void Target::changeState(Target::StateE newState)
{
  state_ = newState;
  switch (state_)
  {
  case StateE::IDLE:
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
    break;
  case StateE::CONNECTED:
    setLedsInConnectedState();
    break;
  case StateE::BUTTON_DISABLED:
    blinkCounter_ = 1;
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    lastBlinkTime_ = msCounter_;
  default:
    break;
  }
}

// -----------------------------------------------------------------------------
// UART RX handling
// -----------------------------------------------------------------------------
// think about to move out of rx interrupt, use queue
void Target::receiver()
{
  auto result = decoder_.processByte(rxByte_);
  if (result.valid)
  {
    handleSignal(result.frame);
  }

  // Restart UART RX interrupt
  HAL_UART_Receive_IT(&huart1, &rxByte_, 1);
}

// -----------------------------------------------------------------------------
// Signal handling
// -----------------------------------------------------------------------------
void Target::handleSignal(const protocol::FrameS& frame)
{
  lastRxTime_ = msCounter_;
  switch (frame.sigId)
  {
  case protocol::signalIdE::CONNECT_REQ:
    sendFrame(protocol::signalIdE::CONNECT_CFM);
    changeState(StateE::CONNECTED);
    break;
  case protocol::signalIdE::DISCONNECT_REQ:
    changeState(StateE::IDLE);
    break;
  case protocol::signalIdE::TICK_IND:
    if (state_ != StateE::IDLE)
    {
      sendFrame(protocol::signalIdE::TICK_CFM);
    }
    break;
  case protocol::signalIdE::BUTTON_CFM:
    changeState(StateE::BUTTON_DISABLED);
    break;
  default:
    break;
  }
}

// -----------------------------------------------------------------------------
// Leds handling in CONNECTED state
// -----------------------------------------------------------------------------
void Target::setLedsInConnectedState()
{
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
}

// -----------------------------------------------------------------------------
// TX handling
// -----------------------------------------------------------------------------
void Target::sendFrame(protocol::signalIdE sig)
{
  auto frame = protocol::encodeFrame(sig, {});
  txQueue_.push(frame);
}

void Target::tryStartTx()
{
  if (txBusy_ || txQueue_.empty())
    return;

  auto& f = txQueue_.front();
  txBusy_ = true;

  HAL_UART_Transmit_IT(&huart1, f.data(), f.size());
}

// -----------------------------------------------------------------------------
// Tx complete callback
// -----------------------------------------------------------------------------
void Target::onTxDone()
{
  txQueue_.pop();
  txBusy_ = false;
  tryStartTx();
}

// -----------------------------------------------------------------------------
// Increment ms counter
// -----------------------------------------------------------------------------
void Target::incTimerMsCounter()
{
  msCounter_++;
}

// -----------------------------------------------------------------------------
// External button press handler
// -----------------------------------------------------------------------------
void Target::handleButtonPress()
{
  if (state_ == Target::StateE::CONNECTED)
  {
    changeState(Target::StateE::BUTTON_PRESSED);
    sendFrame(protocol::signalIdE::BUTTON_IND);
  }
}
