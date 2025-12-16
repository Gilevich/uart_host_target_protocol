#pragma once

#include "protocol.hpp"
#include <queue>

class Target
{
private:
  enum class StateE
  {
    IDLE,
    CONNECTED,
    BUTTON_PRESSED,
    BUTTON_DISABLED
  };
public:
  Target(){};
  ~Target(){};
  void incTimerMsCounter();
  void init();
  void process();
  void receiver();
  void handleButtonPress();
  void sendButtonInd();
  void onTxDone();
  uint8_t rxByte_ = 0;
private:
  void handleSignal(const protocol::FrameS& frame);
  void changeState(StateE newState);
  void printState(StateE msg);
  void printMsg(const char* msg);

  void setLedsInConnectedState();

  void sendConnectCfm();
  void sendTickCfm();
  protocol::Decoder decoder_;
  StateE state_ = StateE::IDLE;
  volatile uint32_t msCounter_ = 0;
  volatile uint32_t lastRxTime_ = 0;
  uint32_t lastBlinkTime_ = 0;
  uint32_t blinkCounter_ = 0;

  std::queue<std::vector<uint8_t>> txQueue_;
  bool txBusy_ = false;
  void sendFrame(protocol::signalIdE sig);
  void tryStartTx();
  bool led1State_ = false;
};