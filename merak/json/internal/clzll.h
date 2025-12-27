// Copyright (C) 2024 Kumo inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
