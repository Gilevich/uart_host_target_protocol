#pragma once

#include <cstdint>
#include <vector>

namespace protocol
{
  constexpr uint8_t SOF = 0xAA;
  constexpr size_t MAX_PAYLOAD = 32;

  enum class signalIdE : uint8_t
  {
    CONNECT_REQ     = 0x01,
    CONNECT_CFM     = 0x02,
    TICK_IND        = 0x03,
    TICK_CFM        = 0x04,
    BUTTON_IND      = 0x05,
    BUTTON_CFM      = 0x06,
    DISCONNECT_REQ  = 0x07
  };

  struct FrameS
  {
    signalIdE sigId;
    std::vector<uint8_t> payload;
  };

  struct frameResult
  {
    bool valid;
    FrameS frame;
  };

  std::vector<uint8_t> encodeFrame(
    signalIdE sigId,
    const std::vector<uint8_t>& payload);
  
  uint8_t crc8(const uint8_t* data, size_t);

  class Decoder
  {
  public:
    frameResult processByte(uint8_t byte);
  private:
    enum class rxStateE
    {
      SOF_WAITING,
      LEN_READING,
      PAYLOAD_READING,
      CRC_READING
    };
    rxStateE state_ = rxStateE::SOF_WAITING;
    uint8_t len_ = 0;
    std::vector<uint8_t> buffer_;
  };
}