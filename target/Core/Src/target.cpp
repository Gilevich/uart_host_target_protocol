extern "C" {
  #include "main.h"
}
#include <cstring>
#include <cstdio>
#include "target.hpp"
extern UART_HandleTypeDef huart1;

#define TICK_TIMEOUT_MS 5000
#define LED1_IDLE_INTERVAL_MS 500
#define LED1_BUTTON_DISABLE_INTERVAL_MS 1000
#define BUTTON_DISABLE_BLINKS 3

void Target::init()
{
  changeState(StateE::IDLE);
  HAL_UART_Receive_IT(&huart1, &rxByte_, 1);
}

void Target::process()
{
  if (!txBusy_ && !txQueue_.empty())
  {
    tryStartTx();
  }

  // If no activity for 5 seconds, go to IDLE state
  if (state_ != StateE::IDLE && msCounter_ - lastRxTime_ >= TICK_TIMEOUT_MS)
  {
    changeState(StateE::IDLE);
  }

  if (state_ == StateE::IDLE && msCounter_ - lastBlinkTime_ >= LED1_IDLE_INTERVAL_MS)
  {
    lastBlinkTime_ = msCounter_;
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
  }


  if (state_ == StateE::BUTTON_DISABLED && msCounter_ - lastBlinkTime_ >= LED1_BUTTON_DISABLE_INTERVAL_MS/2)
  {
    if (blinkCounter_++ < BUTTON_DISABLE_BLINKS * 2)
    {
      lastBlinkTime_ = msCounter_;
      HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    }
  }
}

void Target::changeState(Target::StateE newState)
{
  state_ = newState;
  switch (state_)
  {
  case StateE::IDLE:
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
    break;
  default:
    break;
  }
}

// move out of rx interrupt, use queue
void Target::receiver()
{
  auto result = decoder_.processByte(rxByte_);
  if (result.valid)
  {
    handleSignal(result.frame);
  }
  HAL_UART_Receive_IT(&huart1, &rxByte_, 1);
}

void Target::handleSignal(const protocol::FrameS& frame)
{
  lastRxTime_ = msCounter_;
  switch (frame.sigId)
  {
  case protocol::signalIdE::CONNECT_REQ:
    sendFrame(protocol::signalIdE::CONNECT_CFM);
    changeState(StateE::CONNECTED);
    setLedsInConnectedState();
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
    blinkCounter_ = 1;
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    led1State_ = true;
    lastBlinkTime_ = msCounter_;
    break;
  default:
    break;
  }
}

void Target::setLedsInConnectedState()
{
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
}

void Target::sendFrame(protocol::signalIdE sig)
{
  auto frame = protocol::encodeFrame(sig, {});

  txQueue_.push(frame);
  // tryStartTx();
}

void Target::tryStartTx()
{
  if (txBusy_ || txQueue_.empty())
    return;

  auto& f = txQueue_.front();
  txBusy_ = true;

  HAL_UART_Transmit_IT(&huart1, f.data(), f.size());
}

void Target::onTxDone()
{
  txQueue_.pop();
  txBusy_ = false;
  tryStartTx();
}

void Target::incTimerMsCounter()
{
  msCounter_++;
}

void Target::handleButtonPress()
{
  if (state_ == Target::StateE::CONNECTED)
  {
    changeState(Target::StateE::BUTTON_PRESSED);
    sendFrame(protocol::signalIdE::BUTTON_IND);
  }
}

void Target::printState(Target::StateE state)
{
  static char buf[32];  // must persist after function returns

  int len = snprintf(buf, sizeof(buf),
                     "state: %d\r\n",
                     static_cast<int>(state));

  if (len > 0)
  {
    HAL_UART_Transmit_IT(&huart1,
                         reinterpret_cast<uint8_t*>(buf),
                         len);
  }
}

void Target::printMsg(const char* msg)
{
  HAL_UART_Transmit_IT(&huart1,
                       reinterpret_cast<uint8_t*>(const_cast<char*>(msg)),
                       strlen(msg));
}