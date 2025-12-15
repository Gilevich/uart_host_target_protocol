extern "C" {
  #include "main.h"
}
#include "target.hpp"
extern UART_HandleTypeDef huart1;

void Target::init()
{
  changeState(StateE::IDLE);
  HAL_UART_Receive_IT(&huart1, &rxByte_, 1);
}

void Target::process()
{
  // If no activity for 5 seconds, go to IDLE state
  if (state_ != StateE::IDLE && msCounter_ - lastTickTime_ >= 5000)
  {
    changeState(StateE::IDLE);
  }

  if (state_ == StateE::IDLE && msCounter_ - lastBlinkTime_ >= 500)
  {
    lastBlinkTime_ = msCounter_;
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
  }

  if (state_ == StateE::BUTTON_DISABLED && msCounter_ - lastBlinkTime_ >= 1000)
  {
    if (blinkCounter_++ < 3)
    {
      lastBlinkTime_ = msCounter_;
      HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
      HAL_Delay(200);
      HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    }
  }
}

void Target::changeState(Target::StateE newState)
{
  state_ = newState;
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
  switch (frame.sigId)
  {
  case protocol::signalIdE::CONNECT_REQ:
    sendConnectCfm();
    changeState(StateE::CONNECTED);
    setLedsInConnectedState();
    break;
  case protocol::signalIdE::DISCONNECT_REQ:
    changeState(StateE::IDLE);
    break;
  case protocol::signalIdE::TICK_IND:
    lastTickTime_ = msCounter_;
    sendTickCfm();
    break;
  case protocol::signalIdE::BUTTON_CFM:
    changeState(StateE::BUTTON_DISABLED);
    resetBlinkCounter();
    break;
  default:
    break;
  }
}

void Target::setLedsInConnectedState()
{
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
}

// TODO: one function to send frames
void Target::sendConnectCfm()
{
  auto frame = protocol::encodeFrame(protocol::signalIdE::CONNECT_CFM, {});
  HAL_UART_Transmit(&huart1, frame.data(), frame.size(), HAL_MAX_DELAY);
}

void Target::sendTickCfm()
{
  auto frame = protocol::encodeFrame(protocol::signalIdE::TICK_CFM, {});
  HAL_UART_Transmit(&huart1, frame.data(), frame.size(), HAL_MAX_DELAY);
}

void Target::sendButtonInd()
{
  auto frame = protocol::encodeFrame(protocol::signalIdE::BUTTON_IND, {});
  HAL_UART_Transmit(&huart1, frame.data(), frame.size(), HAL_MAX_DELAY);
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
    sendButtonInd();
  }
}

