#include "protocol.hpp"

namespace protocol
{
  // ---------------------------------------------------------------------------
  // CRC8
  // Polynomial: x^8 + x^2 + x + 1 (0x07)
  // ---------------------------------------------------------------------------
  uint8_t crc8(const uint8_t* data, size_t len)
  {
    uint8_t crc = 0x00;

    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int b = 0; b < 8; ++b) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
        }
    }
    return crc;
  }

  // ---------------------------------------------------------------------------
  // Frame encoder
  // Format:
  //   [SOF][LEN][SIG][PAYLOAD...][CRC]
  // CRC is calculated over [SIG][PAYLOAD...]
  // ---------------------------------------------------------------------------
  size_t encodeFrame(
    signalIdE sigId,
    const uint8_t* payload,
    size_t payloadLen,
    uint8_t* outFrame)
  {
    size_t byteIndex{0};
    outFrame[byteIndex++] = SOF;
    
    uint8_t len = static_cast<uint8_t>(1U + payloadLen);
    outFrame[byteIndex++] = len;

    outFrame[byteIndex++] = static_cast<uint8_t>(sigId);

    for (size_t i = 0; i < payloadLen; ++i)
    {
      outFrame[byteIndex++] = payload[i];
    }
    uint8_t crc = crc8(&outFrame[2], len); // CRC over [SIG][PAYLOAD...]
    outFrame[byteIndex++] = crc;

    return byteIndex;
  }

  // ---------------------------------------------------------------------------
  // Frame decoder (byte-by-byte)
  // ---------------------------------------------------------------------------  
  frameResult Decoder::processByte(uint8_t byte)
  {
    frameResult res {};

    switch (state_)
    {
    case rxStateE::SOF_WAITING:
      if (byte == SOF)
      {
        bufferIndex_ = 0;
        state_ = rxStateE::LEN_READING;
      }
      break;

    case rxStateE::LEN_READING:
      len_ = byte;
      if (len_ == 0U || len_ > (MAX_PAYLOAD + 1U))
      {
        // Invalid length, reset state
        state_ = rxStateE::SOF_WAITING;
      }
      else
      {
        bufferIndex_ = 0;
        state_ = rxStateE::PAYLOAD_READING;
      }
      break;

    case rxStateE::PAYLOAD_READING:
      // Store bytes in buffer
      buffer_[bufferIndex_++] = byte;
      if (bufferIndex_ >= len_)
      {
        state_ = rxStateE::CRC_READING;
      }
      break;

    case rxStateE::CRC_READING:
    {
      uint8_t received_crc = byte;
      uint8_t calculated_crc = crc8(buffer_.data(), bufferIndex_);
      state_ = rxStateE::SOF_WAITING;
      if (received_crc != calculated_crc)
      {
        // CRC error, frame invalid
        res.valid = false;
        return res;
      }

      FrameS frame;
      frame.sigId = static_cast<signalIdE>(buffer_[0]);

      // Start from index 1 to skip sigId
      for (size_t i = 1; i < bufferIndex_; ++i)
      {
        frame.payload[i - 1] = buffer_[i];
      }

      res.valid = true;
      res.frame = frame;
      return res;
    }
    }
    return res;
  }
} // namespace protocol
