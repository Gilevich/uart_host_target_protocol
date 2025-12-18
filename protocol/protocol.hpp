#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

namespace protocol
{
  // --- Protocol definitions -----------------------------------------------------

  // Start of Frame Marker
  constexpr uint8_t SOF = 0xAA;
  constexpr size_t MAX_PAYLOAD = 32;
  constexpr size_t MAX_FRAME_SIZE = 4 + MAX_PAYLOAD; // SOF + LEN + SIG + PAYLOAD + CRC

  // Signal IDs
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

  // --- Frame structure -----------------------------------------------------

  // Protocol frame structure
  struct FrameS
  {
    signalIdE sigId {};
    std::array<uint8_t, MAX_PAYLOAD> payload {}; ///< optional payload data
  };

  // Result of frame decoding
  struct frameResult
  {
    bool  valid {false};  ///< True if a complete and valid frame was decoded
    FrameS frame {};        ///< Decoded frame (valid only if valid == true)
  };

  // --- Protocol encoding/decoding ------------------------------------------------

  /**
   * @brief Encode a protocol frame into a byte stream.
   *
   * Frame format:
   *   [SOF][LEN][SIG][PAYLOAD...][CRC]
   *
   * Where:
   * - SOF = Start of Frame marker (0xAA)
   * - LEN = number of bytes: SIG + PAYLOAD
   * - SIG = Signal ID
   * - PAYLOAD = optional payload data
   * - CRC = CRC8 over [LEN][SIG][PAYLOAD...]
   */
  size_t encodeFrame(
    signalIdE sigId,
    const uint8_t* payload,
    size_t payloadLen,
    uint8_t* outFrame);
  
  // CRC8 calculation
  uint8_t crc8(const uint8_t* data, size_t);
  
  // Byte-wise frame decoder with interanl state machine
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
    rxStateE state_ {rxStateE::SOF_WAITING};
    uint8_t len_ {0};
    std::array<uint8_t, MAX_PAYLOAD + 1> buffer_ {}; // +1 for sigId
    size_t bufferIndex_ {0};
  };
} // namespace protocol
