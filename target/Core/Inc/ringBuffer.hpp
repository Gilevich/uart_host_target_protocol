#pragma once

#include "Protocol.hpp"

#include <cstring>
#include <cstddef>
#include <cstdint>

template<size_t NUM_FRAMES, size_t FRAME_SIZE>
class RingBuffer
{
public:
  RingBuffer() = default;

  bool push(const uint8_t* frame, size_t len)
  {
    if (count_ >= NUM_FRAMES || len > FRAME_SIZE)
      return false;

    std::memcpy(buffer_[head_], frame, len);

    head_ = (head_ + 1U) % NUM_FRAMES;
    ++count_;
    return true;
  }

  bool pop()
  {
    if (count_ == 0U)
      return false;

    tail_ = (tail_ + 1U) % NUM_FRAMES;
    --count_;
    return true;
  }

  bool front(const uint8_t*& frame) const
  {
    if (count_ == 0U)
      return false;

    frame = buffer_[tail_];
    return true;
  }

  bool empty() const {return count_ == 0U;}

private:
  uint8_t buffer_[NUM_FRAMES][FRAME_SIZE] {};
  size_t head_ {0};
  size_t tail_ {0};
  size_t count_ {0};
};
