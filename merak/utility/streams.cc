// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <merak/utility/streams.h>
#include <turbo/log/logging.h>

namespace merak {


    // ===================================================================
    CordInputStream::CordInputStream(const turbo::Cord* cord)
            : it_(cord->char_begin()),
              length_(cord->size()),
              bytes_remaining_(length_) {
        LoadChunkData();
    }

    bool CordInputStream::LoadChunkData() {
        if (bytes_remaining_ != 0) {
            std::string_view sv = turbo::Cord::ChunkRemaining(it_);
            data_ = sv.data();
            size_ = available_ = sv.size();
            return true;
        }
        size_ = available_ = 0;
        return false;
    }

    bool CordInputStream::NextChunk(size_t skip) {
        // `size_ == 0` indicates we're at EOF.
        if (size_ == 0) return false;

        // The caller consumed 'size_ - available_' bytes that are not yet accounted
        // for in the iterator position to get to the start of the next chunk.
        const size_t distance = size_ - available_ + skip;
        turbo::Cord::advance(&it_, distance);
        bytes_remaining_ -= skip;

        return LoadChunkData();
    }

    bool CordInputStream::Next(const void** data, int* size) {
        if (available_ > 0 || NextChunk(0)) {
            *data = data_ + size_ - available_;
            *size = available_;
            bytes_remaining_ -= available_;
            available_ = 0;
            return true;
        }
        return false;
    }

    void CordInputStream::BackUp(int count) {
        // Backup is only allowed on last returned chunk from `Next()`.
        KCHECK_LE(static_cast<size_t>(count), size_ - available_);

        available_ += count;
        bytes_remaining_ += count;
    }

    bool CordInputStream::Skip(int count) {
        // Short circuit if we stay inside the current chunk.
        if (static_cast<size_t>(count) <= available_) {
            available_ -= count;
            bytes_remaining_ -= count;
            return true;
        }

        // Sanity check the skip count.
        if (static_cast<size_t>(count) <= bytes_remaining_) {
            // Skip to end: do not return EOF condition: skipping into EOF is ok.
            NextChunk(count);
            return true;
        }
        NextChunk(bytes_remaining_);
        return false;
    }

    int64_t CordInputStream::ByteCount() const {
        return length_ - bytes_remaining_;
    }

    bool CordInputStream::ReadCord(absl::Cord* cord, int count) {
        // Advance the iterator to the current position
        const size_t used = size_ - available_;
        turbo::Cord::advance(&it_, used);

        // Read the cord, adjusting the iterator position.
        // Make sure to cap at available bytes to avoid hard crashes.
        const size_t n = std::min(static_cast<size_t>(count), bytes_remaining_);
        auto c = turbo::Cord::advance_and_read(&it_, n);
        auto sv = c.flatten();
        cord->Append(absl::string_view(sv.data(), sv.size()));

        // Update current chunk data.
        bytes_remaining_ -= n;
        LoadChunkData();

        return n == static_cast<size_t>(count);
    }

    bool CordInputStream::read_cord(turbo::Cord* cord, int count) {
        // Advance the iterator to the current position
        const size_t used = size_ - available_;
        turbo::Cord::advance(&it_, used);

        // Read the cord, adjusting the iterator position.
        // Make sure to cap at available bytes to avoid hard crashes.
        const size_t n = std::min(static_cast<size_t>(count), bytes_remaining_);
        cord->append(turbo::Cord::advance_and_read(&it_, n));

        // Update current chunk data.
        bytes_remaining_ -= n;
        LoadChunkData();

        return n == static_cast<size_t>(count);
    }


    CordOutputStream::CordOutputStream(size_t size_hint) : size_hint_(size_hint) {}

    CordOutputStream::CordOutputStream(turbo::Cord cord, size_t size_hint)
            : cord_(std::move(cord)),
              size_hint_(size_hint),
              state_(cord_.empty() ? State::kEmpty : State::kSteal) {}

    CordOutputStream::CordOutputStream(turbo::CordBuffer buffer, size_t size_hint)
            : size_hint_(size_hint),
              state_(buffer.length() < buffer.capacity() ? State::kPartial
                                                         : State::kFull),
              buffer_(std::move(buffer)) {}

    CordOutputStream::CordOutputStream(turbo::Cord cord, turbo::CordBuffer buffer,
                                       size_t size_hint)
            : cord_(std::move(cord)),
              size_hint_(size_hint),
              state_(buffer.length() < buffer.capacity() ? State::kPartial
                                                         : State::kFull),
              buffer_(std::move(buffer)) {}

    bool CordOutputStream::Next(void** data, int* size) {
        // Use 128 bytes as a minimum buffer size if we don't have any application
        // provided size hints. This number is picked somewhat arbitrary as 'small
        // enough to avoid excessive waste on small data, and large enough to not
        // waste CPU and memory on tiny buffer overhead'.
        // It is worth noting that absent size hints, we pick 'current size' as
        // the default buffer size (capped at max flat size), which means we quickly
        // double the buffer size. This is in contrast to `Cord::Append()` functions
        // accepting strings which use a conservative 10% growth.
        static const size_t kMinBlockSize = 128;

        size_t desired_size, max_size;
        const size_t cord_size = cord_.size() + buffer_.length();
        if (size_hint_ > cord_size) {
            // Try to hit size_hint_ exactly so the caller doesn't receive a larger
            // buffer than indicated, requiring a non-zero call to BackUp() to undo
            // the buffer capacity we returned beyond the indicated size hint.
            desired_size = size_hint_ - cord_size;
            max_size = desired_size;
        } else {
            // We're past the size hint or don't have a size hint.  Try to allocate a
            // block as large as what we have so far, or at least kMinBlockSize bytes.
            // CordBuffer will truncate this to an appropriate size if it is too large.
            desired_size = std::max(cord_size, kMinBlockSize);
            max_size = std::numeric_limits<size_t>::max();
        }

        switch (state_) {
            case State::kSteal:
                // Steal last buffer from Cord if available.
                assert(buffer_.length() == 0);
                buffer_ = cord_.get_append_buffer(desired_size);
                break;
            case State::kPartial:
                // Use existing capacity in 'buffer_`
                assert(buffer_.length() < buffer_.capacity());
                break;
            case State::kFull:
                assert(buffer_.length() > 0);
                cord_.append(std::move(buffer_));
                ABSL_FALLTHROUGH_INTENDED;
            case State::kEmpty:
                assert(buffer_.length() == 0);
                buffer_ = turbo::CordBuffer::create_with_default_limit(desired_size);
                break;
        }

        // Get all available capacity from the buffer.
        turbo::span<char> span = buffer_.available();
        assert(!span.empty());
        *data = span.data();

        // Only hand out up to 'max_size', which is limited if there is a size hint
        // specified, and we have more available than the size hint.
        if (span.size() > max_size) {
            *size = static_cast<int>(max_size);
            buffer_.increase_length_by(max_size);
            state_ = State::kPartial;
        } else {
            *size = static_cast<int>(span.size());
            buffer_.increase_length_by(span.size());
            state_ = State::kFull;
        }

        return true;
    }

    void CordOutputStream::BackUp(int count) {
        // Check if something to do, else state remains unchanged.
        assert(0 <= count && count <= ByteCount());
        if (count == 0) return;

        // Backup() is not supposed to backup beyond last Next() call
        const int buffer_length = static_cast<int>(buffer_.length());
        assert(count <= buffer_length);
        if (count <= buffer_length) {
            buffer_.set_length(static_cast<size_t>(buffer_length - count));
            state_ = State::kPartial;
        } else {
            buffer_ = {};
            cord_.remove_suffix(static_cast<size_t>(count));
            state_ = State::kSteal;
        }
    }

    int64_t CordOutputStream::ByteCount() const {
        return static_cast<int64_t>(cord_.size() + buffer_.length());
    }

    bool CordOutputStream::write_cord(const turbo::Cord &cord) {
        cord_.append(std::move(buffer_));
        cord_.append(cord);
        state_ = State::kSteal;  // Attempt to utilize existing capacity in `cord'
        return true;
    }

    turbo::Cord CordOutputStream::consume() {
        cord_.append(std::move(buffer_));
        state_ = State::kEmpty;
        return std::move(cord_);
    }
}  // namespace merak
