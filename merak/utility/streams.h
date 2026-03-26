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
#pragma once

#include <google/protobuf/io/zero_copy_stream.h> // ZeroCopyOutputStream
#include <iostream>
#include <turbo/strings/cord.h>
#include <turbo/strings/cord_buffer.h>

namespace merak {

    // A ZeroCopyInputStream backed by a Cord.  This stream implements ReadCord()
    // in a way that can share memory between the source and destination cords
    // rather than copying.
    class TURBO_EXPORT CordInputStream final : public google::protobuf::io::ZeroCopyInputStream {
    public:
        // Creates an InputStream that reads from the given Cord. `cord` must
        // not be null and must outlive this CordInputStream instance. `cord` must
        // not be modified while this instance is actively being used: any change
        // to `cord` will lead to undefined behavior on any subsequent call into
        // this instance.
        explicit CordInputStream(
                const turbo::Cord *cord TURBO_ATTRIBUTE_LIFETIME_BOUND);


        // `CordInputStream` is neither copiable nor assignable
        CordInputStream(const CordInputStream &) = delete;

        CordInputStream &operator=(const CordInputStream &) = delete;

        // implements ZeroCopyInputStream ----------------------------------
        bool Next(const void **data, int *size) override;

        void BackUp(int count) override;

        bool Skip(int count) override;

        int64_t ByteCount() const override;

        bool ReadCord(absl::Cord *cord, int count) override;

        bool read_cord(turbo::Cord *cord, int count);


    private:
        // Moves `it_` to the next available chunk skipping `skip` extra bytes
        // and updates the chunk data pointers.
        bool NextChunk(size_t skip);

        // Updates the current chunk data context `data_`, `size_` and `available_`.
        // If `bytes_remaining_` is zero, sets `size_` and `available_` to zero.
        // Returns true if more data is available, false otherwise.
        bool LoadChunkData();

        turbo::Cord::CharIterator it_;
        size_t length_;
        size_t bytes_remaining_;
        const char *data_;
        size_t size_;
        size_t available_;
    };

    // A ZeroCopyOutputStream that writes to a Cord.  This stream implements
    // WriteCord() in a way that can share memory between the source and
    // destination cords rather than copying.
    class TURBO_EXPORT CordOutputStream final : public google::protobuf::io::ZeroCopyOutputStream {
    public:
        // Creates an OutputStream streaming serialized data into a Cord. `size_hint`,
        // if given, is the expected total size of the resulting Cord. This is a hint
        // only, used for optimization. Callers can obtain the generated Cord value by
        // invoking `Consume()`.
        explicit CordOutputStream(size_t size_hint = 0);

        // Creates an OutputStream with an initial Cord value. This constructor can be
        // used by applications wanting to directly append serialization data to a
        // given cord. In such cases, donating the existing value as in:
        //
        //   CordOutputStream stream(std::move(cord));
        //   message.SerializeToZeroCopyStream(&stream);
        //   cord = std::move(stream.Consume());
        //
        // is more efficient then appending the serialized cord in application code:
        //
        //   CordOutputStream stream;
        //   message.SerializeToZeroCopyStream(&stream);
        //   cord.Append(stream.Consume());
        //
        // The former allows `CordOutputStream` to utilize pre-existing privately
        // owned Cord buffers from the donated cord where the latter does not, which
        // may lead to more memory usage when serialuzing data into existing cords.
        explicit CordOutputStream(turbo::Cord cord, size_t size_hint = 0);

        // Creates an OutputStream with an initial Cord value and initial buffer.
        // This donates both the preexisting cord in `cord`, as well as any
        // pre-existing data and additional capacity in `buffer`.
        // This function is mainly intended to be used in internal serialization logic
        // using eager buffer initialization in EpsCopyOutputStream.
        // The donated buffer can be empty, partially empty or full: the outputstream
        // will DTRT in all cases and preserve any pre-existing data.
        explicit CordOutputStream(turbo::Cord cord, turbo::CordBuffer buffer,
                                  size_t size_hint = 0);

        // Creates an OutputStream with an initial buffer.
        // This method is logically identical to, but more efficient than:
        //   `CordOutputStream(turbo::Cord(), std::move(buffer), size_hint)`
        explicit CordOutputStream(turbo::CordBuffer buffer, size_t size_hint = 0);

        // `CordOutputStream` is neither copiable nor assignable
        CordOutputStream(const CordOutputStream &) = delete;

        CordOutputStream &operator=(const CordOutputStream &) = delete;

        // implements `ZeroCopyOutputStream` ---------------------------------
        bool Next(void **data, int *size) final;

        void BackUp(int count) final;

        int64_t ByteCount() const final;

        bool write_cord(const turbo::Cord &cord);

        // Consumes the serialized data as a cord value. `Consume()` internally
        // flushes any pending state 'as if' BackUp(0) was called. While a final call
        // to BackUp() is generally required by the `ZeroCopyOutputStream` contract,
        // applications using `CordOutputStream` directly can call `Consume()` without
        // a preceding call to `BackUp()`.
        //
        // While it will rarely be useful in practice (and especially in the presence
        // of size hints) an instance is safe to be used after a call to `Consume()`.
        // The only logical change in state is that all serialized data is extracted,
        // and any new serialization calls will serialize into new cord data.

        turbo::Cord consume();

    private:
        // State of `buffer_` and 'cord_. As a default CordBuffer instance always has
        // inlined capacity, we track state explicitly to avoid returning 'existing
        // capacity' from the default or 'moved from' CordBuffer. 'kSteal' indicates
        // we should (attempt to) steal the next buffer from the cord.
        enum class State {
            kEmpty, kFull, kPartial, kSteal
        };

        turbo::Cord cord_;
        size_t size_hint_;
        State state_ = State::kEmpty;
        turbo::CordBuffer buffer_;
    };

}  // namespace merak
