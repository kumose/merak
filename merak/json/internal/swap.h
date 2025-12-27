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

#ifndef MERAK_JSON_INTERNAL_SWAP_H_
#define MERAK_JSON_INTERNAL_SWAP_H_

#include <merak/json/json_internal.h>

#if defined(__clang__)
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(c++98-compat)
#endif

namespace merak::json {
namespace internal {

//! Custom swap() to avoid dependency on C++ <algorithm> header
/*! \tparam T Type of the arguments to swap, should be instantiated with primitive C++ types only.
    \note This has the same semantics as std::swap().
*/
template <typename T>
inline void swap(T& a, T& b) MERAK_JSON_NOEXCEPT {
    T tmp = a;
        a = b;
        b = tmp;
}

} // namespace internal
}  // namespace merak::json

#if defined(__clang__)
MERAK_JSON_DIAG_POP
#endif

#endif // MERAK_JSON_INTERNAL_SWAP_H_
