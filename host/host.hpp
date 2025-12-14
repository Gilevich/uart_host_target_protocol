#pragma once

#include "../protocol/protocol.hpp"
#include <string>
#include <windows.h>
#include <thread>

class Host{
public:
  explicit Host(const std::string& comPort)
  : comPort_{comPort}{};
  ~Host();

  void connect();
private:
  enum class StateE
  {
    INIT,
    CONNECTING,
    CONNECTED,
    DISCONNECTING
  };

  void changeState(StateE newState);
  void waitingConnectCfm();
  void mainLoop();

  void rxThread();
  void handleSignal(const protocol::FrameS& frame);
  bool init();
  void disconnect();

  void sendConnectReq();
  void sendDisconnectReq();
  void sendTickInd();
  void sendButtonCfm();

  bool openPort();
  void closePort();
  void sendSignal(protocol::signalIdE sigId,
                  const std::vector<uint8_t>& payload = {});

  
  std::string comPort_;
  HANDLE serial_ = INVALID_HANDLE_VALUE;
  protocol::Decoder decoder_;
  std::thread rxThread_;
  StateE state_ = StateE::INIT;
  std::chrono::steady_clock::time_point lastRxTime_;
  bool portOpened_ = false;
};