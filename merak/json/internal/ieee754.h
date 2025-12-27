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

#ifndef MERAK_JSON_IEEE754_
#define MERAK_JSON_IEEE754_

#include <merak/json/json_internal.h>

namespace merak::json {
namespace internal {

class Double {
public:
    Double() {}
    Double(double d) : d_(d) {}
    Double(uint64_t u) : u_(u) {}

    double Value() const { return d_; }
    uint64_t Uint64Value() const { return u_; }

    double NextPositiveDouble() const {
        MERAK_JSON_ASSERT(!Sign());
        return Double(u_ + 1).Value();
    }

    bool Sign() const { return (u_ & kSignMask) != 0; }
    uint64_t Significand() const { return u_ & kSignificandMask; }
    int Exponent() const { return static_cast<int>(((u_ & kExponentMask) >> kSignificandSize) - kExponentBias); }

    bool IsNan() const { return (u_ & kExponentMask) == kExponentMask && Significand() != 0; }
    bool IsInf() const { return (u_ & kExponentMask) == kExponentMask && Significand() == 0; }
    bool IsNanOrInf() const { return (u_ & kExponentMask) == kExponentMask; }
    bool IsNormal() const { return (u_ & kExponentMask) != 0 || Significand() == 0; }
    bool IsZero() const { return (u_ & (kExponentMask | kSignificandMask)) == 0; }

    uint64_t IntegerSignificand() const { return IsNormal() ? Significand() | kHiddenBit : Significand(); }
    int IntegerExponent() const { return (IsNormal() ? Exponent() : kDenormalExponent) - kSignificandSize; }
    uint64_t ToBias() const { return (u_ & kSignMask) ? ~u_ + 1 : u_ | kSignMask; }

    static int EffectiveSignificandSize(int order) {
        if (order >= -1021)
            return 53;
        else if (order <= -1074)
            return 0;
        else
            return order + 1074;
    }

private:
    static const int kSignificandSize = 52;
    static const int kExponentBias = 0x3FF;
    static const int kDenormalExponent = 1 - kExponentBias;
    static const uint64_t kSignMask = MERAK_JSON_UINT64_C2(0x80000000, 0x00000000);
    static const uint64_t kExponentMask = MERAK_JSON_UINT64_C2(0x7FF00000, 0x00000000);
    static const uint64_t kSignificandMask = MERAK_JSON_UINT64_C2(0x000FFFFF, 0xFFFFFFFF);
    static const uint64_t kHiddenBit = MERAK_JSON_UINT64_C2(0x00100000, 0x00000000);

    union {
        double d_;
        uint64_t u_;
    };
};

} // namespace internal
}  // namespace merak::json

#endif // MERAK_JSON_IEEE754_
