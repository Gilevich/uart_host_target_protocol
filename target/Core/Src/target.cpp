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
  // If no activity for 5 seconds, go to IDLE state
  // printState(state_);
  if (state_ != StateE::IDLE && msCounter_ - lastRxTime_ >= TICK_TIMEOUT_MS)
  {
    changeState(StateE::IDLE);
  }

  if (state_ == StateE::IDLE && msCounter_ - lastBlinkTime_ >= LED1_IDLE_INTERVAL_MS)
  {
    lastBlinkTime_ = msCounter_;
    // printMsg("IDLE: Toggle LED1 in IDLE\r\n");
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
  }

  if (!txBusy_ && !txQueue_.empty())
  {
    tryStartTx();
  }

  if (state_ == StateE::BUTTON_DISABLED && msCounter_ - lastBlinkTime_ >= LED1_BUTTON_DISABLE_INTERVAL_MS)
  {
    if (blinkCounter_++ < BUTTON_DISABLE_BLINKS)
    {
      lastBlinkTime_ = msCounter_;
      // printMsg("IDLE: Toggle LED1 in BUTTON_DISABLED\r\n");
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
    // printMsg("CONNECT_REQ received\r\n");
    sendFrame(protocol::signalIdE::CONNECT_CFM);
    changeState(StateE::CONNECTED);
    setLedsInConnectedState();
    break;
  case protocol::signalIdE::DISCONNECT_REQ:
    // printMsg("DISCONNECT_REQ received\r\n");
    changeState(StateE::IDLE);
    break;
  case protocol::signalIdE::TICK_IND:
    // printMsg("TICK_IND received\r\n");
    if (state_ == StateE::CONNECTED)
    {
      sendFrame(protocol::signalIdE::TICK_CFM);
    }
    break;
  case protocol::signalIdE::BUTTON_CFM:
    // printMsg("BUTTON_CFM received\r\n");
    changeState(StateE::BUTTON_DISABLED);
    resetBlinkCounter();
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

// TODO: one function to send frames
// void Target::sendConnectCfm()
// {
//   auto frame = protocol::encodeFrame(protocol::signalIdE::CONNECT_CFM, {});
//   HAL_UART_Transmit(&huart1, frame.data(), frame.size(), HAL_MAX_DELAY);
// }

// void Target::sendTickCfm()
// {
//   auto frame = protocol::encodeFrame(protocol::signalIdE::TICK_CFM, {});
//   HAL_UART_Transmit(&huart1, frame.data(), frame.size(), HAL_MAX_DELAY);
// }

// void Target::sendButtonInd()
// {
//   auto frame = protocol::encodeFrame(protocol::signalIdE::BUTTON_IND, {});
//   HAL_UART_Transmit(&huart1, frame.data(), frame.size(), HAL_MAX_DELAY);
// }

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

void Target::resetBlinkCounter()
{
  blinkCounter_ = 0;
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