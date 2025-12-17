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
  std::vector<uint8_t> encodeFrame(
    signalIdE sigId,
    const std::vector<uint8_t>& payload)
  {
    std::vector<uint8_t> frame;
    frame.reserve(4 + payload.size()); // SOF + LEN + SIG + PAYLOAD + CRC
  
    frame.push_back(SOF);

    uint8_t len = static_cast<uint8_t>(1 + payload.size());
    frame.push_back(len);

    frame.push_back(static_cast<uint8_t>(sigId));

    frame.insert(frame.end(), payload.begin(), payload.end());

    uint8_t crc = crc8(&frame[2], len);
    frame.push_back(crc);

    return frame;
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
        buffer_.clear();
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
        buffer_.clear();
        state_ = rxStateE::PAYLOAD_READING;
      }
      break;

    case rxStateE::PAYLOAD_READING:
      // Store bytes in buffer
      buffer_.push_back(byte);
      if (buffer_.size() == len_)
      {
        state_ = rxStateE::CRC_READING;
      }
      break;

    case rxStateE::CRC_READING:
    {
      uint8_t received_crc = byte;
      uint8_t calculated_crc = crc8(buffer_.data(), buffer_.size());
      state_ = rxStateE::SOF_WAITING;
      if (received_crc != calculated_crc)
      {
        // CRC error, frame invalid
        res.valid = false;
        return res;
      }

      FrameS frame;
      frame.sigId = static_cast<signalIdE>(buffer_[0]);
      frame.payload.assign(buffer_.begin() + 1, buffer_.end());

      res.valid = true;
      res.frame = frame;
      return res;
    }
    }
    return res;
  }
} // namespace protocol
