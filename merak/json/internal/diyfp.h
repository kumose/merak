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

// This is a C++ header-only implementation of Grisu2 algorithm from the publication:
// Loitsch, Florian. "Printing floating-point numbers quickly and accurately with
// integers." ACM Sigplan Notices 45.6 (2010): 233-243.

#ifndef MERAK_JSON_DIYFP_H_
#define MERAK_JSON_DIYFP_H_

#include <merak/json/json_internal.h>
#include <merak/json/internal/clzll.h>
#include <limits>

#if defined(_MSC_VER) && defined(_M_AMD64) && !defined(__INTEL_COMPILER)
#include <intrin.h>
#if !defined(_ARM64EC_)
#pragma intrinsic(_umul128)
#else
#pragma comment(lib,"softintrin")
#endif
#endif

namespace merak::json {
namespace internal {

#ifdef __GNUC__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(effc++)
#endif

#ifdef __clang__
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(padded)
#endif

struct DiyFp {
    DiyFp() : f(), e() {}

    DiyFp(uint64_t fp, int exp) : f(fp), e(exp) {}

    explicit DiyFp(double d) {
        union {
            double d;
            uint64_t u64;
        } u = { d };

        int biased_e = static_cast<int>((u.u64 & kDpExponentMask) >> kDpSignificandSize);
        uint64_t significand = (u.u64 & kDpSignificandMask);
        if (biased_e != 0) {
            f = significand + kDpHiddenBit;
            e = biased_e - kDpExponentBias;
        }
        else {
            f = significand;
            e = kDpMinExponent + 1;
        }
    }

    DiyFp operator-(const DiyFp& rhs) const {
        return DiyFp(f - rhs.f, e);
    }

    DiyFp operator*(const DiyFp& rhs) const {
#if defined(_MSC_VER) && defined(_M_AMD64)
        uint64_t h;
        uint64_t l = _umul128(f, rhs.f, &h);
        if (l & (uint64_t(1) << 63)) // rounding
            h++;
        return DiyFp(h, e + rhs.e + 64);
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && defined(__x86_64__)
        __extension__ typedef unsigned __int128 uint128;
        uint128 p = static_cast<uint128>(f) * static_cast<uint128>(rhs.f);
        uint64_t h = static_cast<uint64_t>(p >> 64);
        uint64_t l = static_cast<uint64_t>(p);
        if (l & (uint64_t(1) << 63)) // rounding
            h++;
        return DiyFp(h, e + rhs.e + 64);
#else
        const uint64_t M32 = 0xFFFFFFFF;
        const uint64_t a = f >> 32;
        const uint64_t b = f & M32;
        const uint64_t c = rhs.f >> 32;
        const uint64_t d = rhs.f & M32;
        const uint64_t ac = a * c;
        const uint64_t bc = b * c;
        const uint64_t ad = a * d;
        const uint64_t bd = b * d;
        uint64_t tmp = (bd >> 32) + (ad & M32) + (bc & M32);
        tmp += 1U << 31;  /// mult_round
        return DiyFp(ac + (ad >> 32) + (bc >> 32) + (tmp >> 32), e + rhs.e + 64);
#endif
    }

    DiyFp Normalize() const {
        int s = static_cast<int>(clzll(f));
        return DiyFp(f << s, e - s);
    }

    DiyFp NormalizeBoundary() const {
        DiyFp res = *this;
        while (!(res.f & (kDpHiddenBit << 1))) {
            res.f <<= 1;
            res.e--;
        }
        res.f <<= (kDiySignificandSize - kDpSignificandSize - 2);
        res.e = res.e - (kDiySignificandSize - kDpSignificandSize - 2);
        return res;
    }

    void NormalizedBoundaries(DiyFp* minus, DiyFp* plus) const {
        DiyFp pl = DiyFp((f << 1) + 1, e - 1).NormalizeBoundary();
        DiyFp mi = (f == kDpHiddenBit) ? DiyFp((f << 2) - 1, e - 2) : DiyFp((f << 1) - 1, e - 1);
        mi.f <<= mi.e - pl.e;
        mi.e = pl.e;
        *plus = pl;
        *minus = mi;
    }

    double ToDouble() const {
        union {
            double d;
            uint64_t u64;
        }u;
        MERAK_JSON_ASSERT(f <= kDpHiddenBit + kDpSignificandMask);
        if (e < kDpDenormalExponent) {
            // Underflow.
            return 0.0;
        }
        if (e >= kDpMaxExponent) {
            // Overflow.
            return std::numeric_limits<double>::infinity();
        }
        const uint64_t be = (e == kDpDenormalExponent && (f & kDpHiddenBit) == 0) ? 0 :
            static_cast<uint64_t>(e + kDpExponentBias);
        u.u64 = (f & kDpSignificandMask) | (be << kDpSignificandSize);
        return u.d;
    }

    static const int kDiySignificandSize = 64;
    static const int kDpSignificandSize = 52;
    static const int kDpExponentBias = 0x3FF + kDpSignificandSize;
    static const int kDpMaxExponent = 0x7FF - kDpExponentBias;
    static const int kDpMinExponent = -kDpExponentBias;
    static const int kDpDenormalExponent = -kDpExponentBias + 1;
    static const uint64_t kDpExponentMask = MERAK_JSON_UINT64_C2(0x7FF00000, 0x00000000);
    static const uint64_t kDpSignificandMask = MERAK_JSON_UINT64_C2(0x000FFFFF, 0xFFFFFFFF);
    static const uint64_t kDpHiddenBit = MERAK_JSON_UINT64_C2(0x00100000, 0x00000000);

    uint64_t f;
    int e;
};

inline DiyFp GetCachedPowerByIndex(size_t index) {
    // 10^-348, 10^-340, ..., 10^340
    static const uint64_t kCachedPowers_F[] = {
        MERAK_JSON_UINT64_C2(0xfa8fd5a0, 0x081c0288), MERAK_JSON_UINT64_C2(0xbaaee17f, 0xa23ebf76),
        MERAK_JSON_UINT64_C2(0x8b16fb20, 0x3055ac76), MERAK_JSON_UINT64_C2(0xcf42894a, 0x5dce35ea),
        MERAK_JSON_UINT64_C2(0x9a6bb0aa, 0x55653b2d), MERAK_JSON_UINT64_C2(0xe61acf03, 0x3d1a45df),
        MERAK_JSON_UINT64_C2(0xab70fe17, 0xc79ac6ca), MERAK_JSON_UINT64_C2(0xff77b1fc, 0xbebcdc4f),
        MERAK_JSON_UINT64_C2(0xbe5691ef, 0x416bd60c), MERAK_JSON_UINT64_C2(0x8dd01fad, 0x907ffc3c),
        MERAK_JSON_UINT64_C2(0xd3515c28, 0x31559a83), MERAK_JSON_UINT64_C2(0x9d71ac8f, 0xada6c9b5),
        MERAK_JSON_UINT64_C2(0xea9c2277, 0x23ee8bcb), MERAK_JSON_UINT64_C2(0xaecc4991, 0x4078536d),
        MERAK_JSON_UINT64_C2(0x823c1279, 0x5db6ce57), MERAK_JSON_UINT64_C2(0xc2109436, 0x4dfb5637),
        MERAK_JSON_UINT64_C2(0x9096ea6f, 0x3848984f), MERAK_JSON_UINT64_C2(0xd77485cb, 0x25823ac7),
        MERAK_JSON_UINT64_C2(0xa086cfcd, 0x97bf97f4), MERAK_JSON_UINT64_C2(0xef340a98, 0x172aace5),
        MERAK_JSON_UINT64_C2(0xb23867fb, 0x2a35b28e), MERAK_JSON_UINT64_C2(0x84c8d4df, 0xd2c63f3b),
        MERAK_JSON_UINT64_C2(0xc5dd4427, 0x1ad3cdba), MERAK_JSON_UINT64_C2(0x936b9fce, 0xbb25c996),
        MERAK_JSON_UINT64_C2(0xdbac6c24, 0x7d62a584), MERAK_JSON_UINT64_C2(0xa3ab6658, 0x0d5fdaf6),
        MERAK_JSON_UINT64_C2(0xf3e2f893, 0xdec3f126), MERAK_JSON_UINT64_C2(0xb5b5ada8, 0xaaff80b8),
        MERAK_JSON_UINT64_C2(0x87625f05, 0x6c7c4a8b), MERAK_JSON_UINT64_C2(0xc9bcff60, 0x34c13053),
        MERAK_JSON_UINT64_C2(0x964e858c, 0x91ba2655), MERAK_JSON_UINT64_C2(0xdff97724, 0x70297ebd),
        MERAK_JSON_UINT64_C2(0xa6dfbd9f, 0xb8e5b88f), MERAK_JSON_UINT64_C2(0xf8a95fcf, 0x88747d94),
        MERAK_JSON_UINT64_C2(0xb9447093, 0x8fa89bcf), MERAK_JSON_UINT64_C2(0x8a08f0f8, 0xbf0f156b),
        MERAK_JSON_UINT64_C2(0xcdb02555, 0x653131b6), MERAK_JSON_UINT64_C2(0x993fe2c6, 0xd07b7fac),
        MERAK_JSON_UINT64_C2(0xe45c10c4, 0x2a2b3b06), MERAK_JSON_UINT64_C2(0xaa242499, 0x697392d3),
        MERAK_JSON_UINT64_C2(0xfd87b5f2, 0x8300ca0e), MERAK_JSON_UINT64_C2(0xbce50864, 0x92111aeb),
        MERAK_JSON_UINT64_C2(0x8cbccc09, 0x6f5088cc), MERAK_JSON_UINT64_C2(0xd1b71758, 0xe219652c),
        MERAK_JSON_UINT64_C2(0x9c400000, 0x00000000), MERAK_JSON_UINT64_C2(0xe8d4a510, 0x00000000),
        MERAK_JSON_UINT64_C2(0xad78ebc5, 0xac620000), MERAK_JSON_UINT64_C2(0x813f3978, 0xf8940984),
        MERAK_JSON_UINT64_C2(0xc097ce7b, 0xc90715b3), MERAK_JSON_UINT64_C2(0x8f7e32ce, 0x7bea5c70),
        MERAK_JSON_UINT64_C2(0xd5d238a4, 0xabe98068), MERAK_JSON_UINT64_C2(0x9f4f2726, 0x179a2245),
        MERAK_JSON_UINT64_C2(0xed63a231, 0xd4c4fb27), MERAK_JSON_UINT64_C2(0xb0de6538, 0x8cc8ada8),
        MERAK_JSON_UINT64_C2(0x83c7088e, 0x1aab65db), MERAK_JSON_UINT64_C2(0xc45d1df9, 0x42711d9a),
        MERAK_JSON_UINT64_C2(0x924d692c, 0xa61be758), MERAK_JSON_UINT64_C2(0xda01ee64, 0x1a708dea),
        MERAK_JSON_UINT64_C2(0xa26da399, 0x9aef774a), MERAK_JSON_UINT64_C2(0xf209787b, 0xb47d6b85),
        MERAK_JSON_UINT64_C2(0xb454e4a1, 0x79dd1877), MERAK_JSON_UINT64_C2(0x865b8692, 0x5b9bc5c2),
        MERAK_JSON_UINT64_C2(0xc83553c5, 0xc8965d3d), MERAK_JSON_UINT64_C2(0x952ab45c, 0xfa97a0b3),
        MERAK_JSON_UINT64_C2(0xde469fbd, 0x99a05fe3), MERAK_JSON_UINT64_C2(0xa59bc234, 0xdb398c25),
        MERAK_JSON_UINT64_C2(0xf6c69a72, 0xa3989f5c), MERAK_JSON_UINT64_C2(0xb7dcbf53, 0x54e9bece),
        MERAK_JSON_UINT64_C2(0x88fcf317, 0xf22241e2), MERAK_JSON_UINT64_C2(0xcc20ce9b, 0xd35c78a5),
        MERAK_JSON_UINT64_C2(0x98165af3, 0x7b2153df), MERAK_JSON_UINT64_C2(0xe2a0b5dc, 0x971f303a),
        MERAK_JSON_UINT64_C2(0xa8d9d153, 0x5ce3b396), MERAK_JSON_UINT64_C2(0xfb9b7cd9, 0xa4a7443c),
        MERAK_JSON_UINT64_C2(0xbb764c4c, 0xa7a44410), MERAK_JSON_UINT64_C2(0x8bab8eef, 0xb6409c1a),
        MERAK_JSON_UINT64_C2(0xd01fef10, 0xa657842c), MERAK_JSON_UINT64_C2(0x9b10a4e5, 0xe9913129),
        MERAK_JSON_UINT64_C2(0xe7109bfb, 0xa19c0c9d), MERAK_JSON_UINT64_C2(0xac2820d9, 0x623bf429),
        MERAK_JSON_UINT64_C2(0x80444b5e, 0x7aa7cf85), MERAK_JSON_UINT64_C2(0xbf21e440, 0x03acdd2d),
        MERAK_JSON_UINT64_C2(0x8e679c2f, 0x5e44ff8f), MERAK_JSON_UINT64_C2(0xd433179d, 0x9c8cb841),
        MERAK_JSON_UINT64_C2(0x9e19db92, 0xb4e31ba9), MERAK_JSON_UINT64_C2(0xeb96bf6e, 0xbadf77d9),
        MERAK_JSON_UINT64_C2(0xaf87023b, 0x9bf0ee6b)
    };
    static const int16_t kCachedPowers_E[] = {
        -1220, -1193, -1166, -1140, -1113, -1087, -1060, -1034, -1007,  -980,
        -954,  -927,  -901,  -874,  -847,  -821,  -794,  -768,  -741,  -715,
        -688,  -661,  -635,  -608,  -582,  -555,  -529,  -502,  -475,  -449,
        -422,  -396,  -369,  -343,  -316,  -289,  -263,  -236,  -210,  -183,
        -157,  -130,  -103,   -77,   -50,   -24,     3,    30,    56,    83,
        109,   136,   162,   189,   216,   242,   269,   295,   322,   348,
        375,   402,   428,   455,   481,   508,   534,   561,   588,   614,
        641,   667,   694,   720,   747,   774,   800,   827,   853,   880,
        907,   933,   960,   986,  1013,  1039,  1066
    };
    MERAK_JSON_ASSERT(index < 87);
    return DiyFp(kCachedPowers_F[index], kCachedPowers_E[index]);
}

inline DiyFp GetCachedPower(int e, int* K) {

    //int k = static_cast<int>(ceil((-61 - e) * 0.30102999566398114)) + 374;
    double dk = (-61 - e) * 0.30102999566398114 + 347;  // dk must be positive, so can do ceiling in positive
    int k = static_cast<int>(dk);
    if (dk - k > 0.0)
        k++;

    unsigned index = static_cast<unsigned>((k >> 3) + 1);
    *K = -(-348 + static_cast<int>(index << 3));    // decimal exponent no need lookup table

    return GetCachedPowerByIndex(index);
}

inline DiyFp GetCachedPower10(int exp, int *outExp) {
    MERAK_JSON_ASSERT(exp >= -348);
    unsigned index = static_cast<unsigned>(exp + 348) / 8u;
    *outExp = -348 + static_cast<int>(index) * 8;
    return GetCachedPowerByIndex(index);
}

#ifdef __GNUC__
MERAK_JSON_DIAG_POP
#endif

#ifdef __clang__
MERAK_JSON_DIAG_POP
MERAK_JSON_DIAG_OFF(padded)
#endif

} // namespace internal
}  // namespace merak::json

#endif // MERAK_JSON_DIYFP_H_
