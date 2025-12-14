#include "host.hpp"
#include <iostream>
#include <atomic>

std::atomic<bool> connectCfmReceived{false};

bool Host::openPort()
{
  serial_ = CreateFileA(
    comPort_.c_str(),
    GENERIC_READ | GENERIC_WRITE,
    0,
    nullptr,
    OPEN_EXISTING,
    0,
    nullptr);

  if (serial_ == INVALID_HANDLE_VALUE)
  {
    std::cout << "Failed to open " << comPort_ << std::endl;
    return false;
  }

  std::cout << "[host] Port is opened" << std::endl;
  DCB dcb{};
  dcb.DCBlength = sizeof(dcb);
  GetCommState(serial_, &dcb);

  dcb.BaudRate = CBR_115200;
  dcb.ByteSize = 8;
  dcb.StopBits = ONESTOPBIT;
  dcb.Parity   = NOPARITY;

  SetCommState(serial_, &dcb);

  COMMTIMEOUTS timeouts{};
  timeouts.ReadIntervalTimeout = 50;
  timeouts.ReadTotalTimeoutConstant = 50;
  timeouts.ReadTotalTimeoutMultiplier = 10;
  SetCommTimeouts(serial_, &timeouts);

  return true;
}

void Host::closePort()
{
  if (serial_ != INVALID_HANDLE_VALUE)
  {
    CloseHandle(serial_);
    serial_ = INVALID_HANDLE_VALUE;
  }
}

void Host::sendSignal(protocol::signalIdE sig,
                      const std::vector<uint8_t>& payload)
{
  auto frame = protocol::encodeFrame(sig, payload);
  
  DWORD written = 0;
  WriteFile(serial_, frame.data(), static_cast<DWORD>(frame.size()), &written, nullptr);
}

void Host::connect()
{
  changeState(StateE::INIT);
  waitingConnectCfm();
  mainLoop();
}

void Host::changeState(StateE newState)
{
  state_ = newState;
  switch (state_)
  {
  case StateE::INIT:
    std::cout << "[host] Initialization" << std::endl;
    init();
    break;

  case StateE::CONNECTING:
    std::cout << "[host] Connectin to the target ..." << std::endl;
    break;

  case StateE::CONNECTED:
    std::cout << "[host] Received CONNECT_CFM" << std::endl;
    std::cout << "[host] Connected" << std::endl;
    break;

  case StateE::DISCONNECTING:
    std::cout << "[host] Disconnecting" << std::endl;
    disconnect();
    break;
  }
}

void Host::waitingConnectCfm()
{
  using namespace std::chrono_literals;
  while(state_ == StateE::CONNECTING &&
        !connectCfmReceived.load())
  {
    auto now = std::chrono::steady_clock::now();
    auto diff = now - lastRxTime_;
    if (diff > 3s)
    {
      std::cout << "[host] Connection timeout" << std::endl;
      changeState(StateE::DISCONNECTING);
    }
    std::this_thread::sleep_for(1s);
  }
}

void Host::mainLoop()
{
  using namespace std::chrono_literals;
  while(state_ == StateE::CONNECTED)
  {
    sendTickInd();
    auto now = std::chrono::steady_clock::now();
    auto diff = now - lastRxTime_;
    if (diff > 5s)
    {
      std::cout << "Connection lost" << std::endl;
      changeState(StateE::DISCONNECTING);
    }
    std::this_thread::sleep_for(1s);
  }
}

void Host::rxThread()
{
  uint8_t byte;
  DWORD read;

  while (portOpened_)
  {
    if (!ReadFile(serial_, &byte, 1, &read, nullptr))
      continue;

    if (read == 1)
    {
      auto res = decoder_.processByte(byte);
      // check if frame is valid
      if (res.valid)
      {
        lastRxTime_ = std::chrono::steady_clock::now();
        handleSignal(res.frame);
      }
    }
  }
}

void Host::handleSignal(const protocol::FrameS& frame)
{
  switch (frame.sigId)
  {
  case protocol::signalIdE::CONNECT_CFM:
    if (state_ == StateE::CONNECTING)
    {
      connectCfmReceived.store(true);
      changeState(StateE::CONNECTED);
    }
    break;
  case protocol::signalIdE::TICK_CFM:
    break;
  case protocol::signalIdE::BUTTON_IND:
    sendButtonCfm();
    break;
  default:
    break;
  }
}

bool Host::init()
{
  if (!openPort())
    return false;

  portOpened_ = true;
  lastRxTime_ = std::chrono::steady_clock::now();
  changeState(StateE::CONNECTING);
  rxThread_ = std::thread(&Host::rxThread, this);
  sendConnectReq();

  return true;
}

void Host::disconnect()
{
  portOpened_ = false;
  if (rxThread_.joinable())
    rxThread_.join();

  closePort();
}

void Host::sendConnectReq()
{
  std::cout << "[host] Send CONNECT_REQ" << std::endl;
  sendSignal(protocol::signalIdE::CONNECT_REQ);
  std::cout << "[host] Waiting for CONNECT_CFM from target" << std::endl;
}

void Host::sendDisconnectReq()
{
  std::cout << "[host] Send DISCONNECT_REQ" << std::endl;
  sendSignal(protocol::signalIdE::DISCONNECT_REQ);
}

void Host::sendTickInd()
{
  sendSignal(protocol::signalIdE::TICK_IND);
}

void Host::sendButtonCfm()
{
  std::cout << "[host] Button pressed" << std::endl;
  std::cout << "[host] Send BUTTON_CFM" << std::endl;
  sendSignal(protocol::signalIdE::BUTTON_CFM);
}
