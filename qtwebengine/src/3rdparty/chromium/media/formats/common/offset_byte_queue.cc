// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/formats/common/offset_byte_queue.h"

#include "base/logging.h"

namespace media {

OffsetByteQueue::OffsetByteQueue() : buf_(NULL), size_(0), head_(0) {}
OffsetByteQueue::~OffsetByteQueue() = default;

void OffsetByteQueue::Reset() {
  queue_.Reset();
  buf_ = NULL;
  size_ = 0;
  head_ = 0;
}

void OffsetByteQueue::Push(const uint8_t* buf, int size) {
  queue_.Push(buf, size);
  Sync();
  DVLOG(4) << "Buffer pushed. head=" << head() << " tail=" << tail();
}

void OffsetByteQueue::Peek(const uint8_t** buf, int* size) {
  *buf = size_ > 0 ? buf_ : NULL;
  *size = size_;
}

void OffsetByteQueue::Pop(int count) {
  queue_.Pop(count);
  head_ += count;
  Sync();
}

void OffsetByteQueue::PeekAt(int64_t offset, const uint8_t** buf, int* size) {
  DCHECK(offset >= head());
  if (offset < head() || offset >= tail()) {
    *buf = NULL;
    *size = 0;
    return;
  }
  *buf = &buf_[offset - head()];
  *size = tail() - offset;
}

bool OffsetByteQueue::Trim(int64_t _max_offset) {
  if (_max_offset < head_) return true;
  if (_max_offset > tail()) {
    Pop(size_);
    return false;
  }
  Pop(_max_offset - head_);
  return true;
}

void OffsetByteQueue::Sync() {
  queue_.Peek(&buf_, &size_);
}

}  // namespace media
