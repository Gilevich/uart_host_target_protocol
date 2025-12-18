#pragma once

#include "../protocol/protocol.hpp"

#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <cstdint>

#include <windows.h>


/**
 * @brief Host application logic.
 *
 * All time-related logic is driven by a 1 ms system tick
 */
class Host{
public:
  explicit Host(const std::string& comPort);
  ~Host();

  void connect();
private:
  // --- Constants ------------------------------------------------
  static constexpr auto CONNECT_TIMEOUT    = std::chrono::seconds{5};
  static constexpr auto TICK_PERIOD        = std::chrono::seconds{1};
  static constexpr auto CONNECT_POLL_DELAY = std::chrono::seconds{1};
  static constexpr auto RX_IDLE_SLEEP      = std::chrono::milliseconds{10};

  enum class StateE
  {
    INIT,
    CONNECTING,
    CONNECTED,
    DISCONNECTING
  };

  // --- State machine ------------------------------------------------
  void changeState(StateE newState);
  void waitingConnectCfm();
  void mainLoop();

  // --- RX handling ------------------------------------------------
  void rxThread();
  void handleSignal(const protocol::FrameS& frame);

  // --- Initialization and disconnection --------------------------
  bool init();
  void disconnect();

  // --- Signal sending ------------------------------------------------
  void sendConnectReq();
  void sendDisconnectReq();
  void sendTickInd();
  void sendButtonCfm();

  // --- Serial port handling ----------------------------------------
  bool openPort();
  void closePort();
  void sendSignal(protocol::signalIdE sigId,
                  const std::vector<uint8_t>& payload = {});

  // --- Internal data members ------------------------------------------------
  // COM port
  std::string comPort_;
  HANDLE serial_ {INVALID_HANDLE_VALUE};
  std::atomic<bool> portOpened_ {false};

  // Protocol
  protocol::Decoder decoder_;

  // RX thread
  std::thread rxThread_;

  // State
  std::atomic<StateE> state_ {StateE::INIT};
  std::atomic<bool> connectCfmReceived_ {false};

  // Time tracking
  std::chrono::steady_clock::time_point lastRxTime_ {};

  // Tick handling
  std::atomic<bool> tickCfmPending_ {false};
};