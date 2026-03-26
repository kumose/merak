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
