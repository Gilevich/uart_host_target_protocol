#pragma once

#include "Protocol.hpp"

#include <array>
#include <cstring>

template<size_t NUM_FRAMES, size_t FRAME_SIZE>
class RingBuffer
{
public:
  RingBuffer() : head_{0}, tail_{0}, count_{0} {};

  bool push(const uint8_t* frame, size_t len)
  {
    if (count_ >= NUM_FRAMES || len > FRAME_SIZE)
      return false;

    std::memcpy(buffer_[head_], frame, len);
    lengths_[head_] = len;

    head_ = (head_ + 1) % NUM_FRAMES;
    ++count_;
    return true;
  }

  bool pop()
  {
    if (count_ == 0)
      return false;

    tail_ = (tail_ + 1) % NUM_FRAMES;
    --count_;
    return true;
  }

  bool front(const uint8_t*& frame, size_t& len) const
  {
    if (count_ == 0)
      return false;

    frame = buffer_[tail_];
    len = lengths_[tail_];
    return true;
  }

  bool empty() const {return count_ == 0;}

private:
  uint8_t buffer_[NUM_FRAMES][FRAME_SIZE];
  size_t lengths_[NUM_FRAMES];
  size_t head_;
  size_t tail_;
  size_t count_;
};
