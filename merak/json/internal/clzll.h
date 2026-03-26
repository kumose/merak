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
// Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip.

#ifndef MERAK_JSON_CLZLL_H_
#define MERAK_JSON_CLZLL_H_

#include <merak/json/json_internal.h>

#if defined(_MSC_VER) && !defined(UNDER_CE)
#include <intrin.h>
#if defined(_WIN64)
#pragma intrinsic(_BitScanReverse64)
#else
#pragma intrinsic(_BitScanReverse)
#endif
#endif

namespace merak::json {
namespace internal {

inline uint32_t clzll(uint64_t x) {
    // Passing 0 to __builtin_clzll is UB in GCC and results in an
    // infinite loop in the software implementation.
    MERAK_JSON_ASSERT(x != 0);

#if defined(_MSC_VER) && !defined(UNDER_CE)
    unsigned long r = 0;
#if defined(_WIN64)
    _BitScanReverse64(&r, x);
#else
    // Scan the high 32 bits.
    if (_BitScanReverse(&r, static_cast<uint32_t>(x >> 32)))
        return 63 - (r + 32);

    // Scan the low 32 bits.
    _BitScanReverse(&r, static_cast<uint32_t>(x & 0xFFFFFFFF));
#endif // _WIN64

    return 63 - r;
#elif (defined(__GNUC__) && __GNUC__ >= 4) || MERAK_JSON_HAS_BUILTIN(__builtin_clzll)
    // __builtin_clzll wrapper
    return static_cast<uint32_t>(__builtin_clzll(x));
#else
    // naive version
    uint32_t r = 0;
    while (!(x & (static_cast<uint64_t>(1) << 63))) {
        x <<= 1;
        ++r;
    }

    return r;
#endif // _MSC_VER
}

#define MERAK_JSON_CLZLL merak::json::internal::clzll

} // namespace internal
}  // namespace merak::json

#endif // MERAK_JSON_CLZLL_H_
