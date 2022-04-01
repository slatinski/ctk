/*
Copyright 2015-2021 Velko Hristov
This file is part of CntToolKit.
SPDX-License-Identifier: LGPL-3.0+

CntToolKit is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CntToolKit is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with CntToolKit.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <cassert>

#include "type_wrapper.h"
#include "maybe_cast.h"
#include "exception.h"


namespace ctk { namespace impl {

    template<typename Source, typename Dest>
    auto invalid_cast(Source a, Dest) -> std::string {
        constexpr const auto min_b{ std::numeric_limits<Dest>::min() };
        constexpr const auto max_b{ std::numeric_limits<Dest>::max() };

        std::ostringstream oss;
        oss << "invalid cast: " << a << " to [" << min_b << ", " << max_b << "]";
        return oss.str();
    }


    enum class arithmetic_error{ none, addition_0, addition_1, subtraction_0, subtraction_1, multiplication_0, multiplication_1, multiplication_2, multiplication_3, division_0, division_1 };


    template<typename T>
    constexpr
    auto signed_addition(T a, T b) -> std::pair<T, arithmetic_error> {
        static_assert(std::numeric_limits<T>::is_signed);

        constexpr const T int_max{ std::numeric_limits<T>::max() };
        constexpr const T int_min{ std::numeric_limits<T>::min() };

        if ((b > T{ 0 }) && (a > (int_max - b))) {
            return { 0, arithmetic_error::addition_0 };
        }

        if ((b < T{ 0 }) && (a < (int_min - b))) {
            return { 0, arithmetic_error::addition_1 };
        }

        return { static_cast<T>(a + b), arithmetic_error::none };
    }


    template<typename T>
    auto invalid_addition(T a, T b, arithmetic_error cause) -> std::string {
        constexpr const T int_min{ std::numeric_limits<T>::min() };
        constexpr const T int_max{ std::numeric_limits<T>::max() };

        std::ostringstream oss;
        oss << "signed integer addition: " << a << " + " << b << ", ";

        switch (cause) {
            case arithmetic_error::addition_0:
                oss << a << " > (" << int_max << " - " << b << ")";
                break;
            case arithmetic_error::addition_1:
                oss << a << " < (" << int_min << " - " << b << ")";
                break;
            default: abort();
        }

        return oss.str();
    }


    template<typename T>
    constexpr
    auto signed_subtraction(T a, T b) -> std::pair<T, arithmetic_error> {
        static_assert(std::numeric_limits<T>::is_signed);

        constexpr const T int_max{ std::numeric_limits<T>::max() };
        constexpr const T int_min{ std::numeric_limits<T>::min() };

        if (b > T{ 0 } && a < int_min + b) {
            return { 0, arithmetic_error::subtraction_0 };
        }

        if (b < T{ 0 } && a > int_max + b) {
            return { 0, arithmetic_error::subtraction_1 };
        }

        return { static_cast<T>(a - b), arithmetic_error::none };
    }


    template<typename T>
    auto invalid_subtraction(T a, T b, arithmetic_error cause) -> std::string {
        constexpr const T int_min{ std::numeric_limits<T>::min() };
        constexpr const T int_max{ std::numeric_limits<T>::max() };

        std::ostringstream oss;
        oss << "signed integer subtraction: " << a << " - " << b << ", ";

        switch (cause) {
            case arithmetic_error::subtraction_0:
                oss << a << " < " << int_min << " + " << b;
                break;
            case arithmetic_error::subtraction_1:
                oss << a << " > " << int_max << " + " << b;
                break;
            default: abort();
        }

        return oss.str();
    }


    template<typename T>
    constexpr
    auto signed_multiplication_impl(T a, T b) -> std::pair<T, arithmetic_error> {
        static_assert(std::numeric_limits<T>::is_signed);

        constexpr const T int_max{ std::numeric_limits<T>::max() };
        constexpr const T int_min{ std::numeric_limits<T>::min() };

        if (a > T{ 0 }) {  // a is positive
            if (b > T{ 0 }) {  // a and b are positive
                if (a > (int_max / b)) {
                    // a > (int_max / b), a > 0, b > 0
                    return { 0, arithmetic_error::multiplication_0 };
                }
            } else { // a positive, b nonpositive
                if (b < (int_min / a)) {
                    // b < (int_min / a), a > 0, b <= 0
                    return { 0, arithmetic_error::multiplication_1 };
                }
            } // a positive, b nonpositive
        } else { // a is nonpositive
            if (b > T{ 0 }) { // a is nonpositive, b is positive
                if (a < (int_min / b)) {
                    // (a < (int_min / b)), a <= 0, b > 0
                    return { 0, arithmetic_error::multiplication_2 };
                }
            } else { // a and b are nonpositive
                if ((a != T{ 0 }) && (b < (int_max / a))) {
                    // (a != 0) && (b < (int_max / a)), a <= 0, b <= 0
                    return { 0, arithmetic_error::multiplication_3 };
                }
            }
        }

        return { static_cast<T>(a * b), arithmetic_error::none };
    }


    template<typename T>
    auto invalid_multiplication(T a, T b, arithmetic_error cause) -> std::string {
        constexpr const T int_min{ std::numeric_limits<T>::min() };
        constexpr const T int_max{ std::numeric_limits<T>::max() };
        std::ostringstream oss;
        oss << "signed integer multiplication: " << a << " * " << b << ", ";

        switch(cause) {
            case arithmetic_error::multiplication_0:
                oss << a << " > " << int_max << " / " << b;
                break;
            case arithmetic_error::multiplication_1:
                oss << b << " < " << int_min << " / " << a;
                break;
            case arithmetic_error::multiplication_2:
                oss << a << " < " << int_min << " / " << b;
                break;
            case arithmetic_error::multiplication_3:
                oss << a << " != 0 && " << b << " < (" << int_max << " / " << a << ")";
                break;
            default: abort();
        }

        return oss.str();
    }


    template<typename T>
    constexpr
    auto signed_division(T a, T b) -> std::pair<T, arithmetic_error> {
        static_assert(std::numeric_limits<T>::is_signed);

        constexpr const T int_min{ std::numeric_limits<T>::min() };

        if (b == T{ 0 }) {
            return { 0, arithmetic_error::division_0 };
        }

        if ((a == int_min) && (b == T{ -1 })) {
            return { 0, arithmetic_error::division_1 };
        }

        return { static_cast<T>(a / b), arithmetic_error::none };
    }


    template<typename T>
    auto invalid_division(T a, T b, arithmetic_error cause) -> std::string {
        constexpr const T int_min{ std::numeric_limits<T>::min() };
        std::ostringstream oss;
        oss << "signed integer division: " << a << " / " << b << ", ";

        switch (cause) {
            case arithmetic_error::division_0:
                oss << "division by zero";
                break;
            case arithmetic_error::division_1:
                oss << a << " == " << int_min << " && " << b << " == -1";
                break;
            default: abort();
        }

        return oss.str();
    }



    template<typename Repr, typename T>
    auto as_intmax(impl::incompatible_integral<Repr, T> x) -> intmax_t {
        static_assert(std::numeric_limits<T>::is_signed);
        return intmax_t{ static_cast<Repr>(x) };
    }

    template<typename T>
    auto as_intmax(T x) -> intmax_t {
        static_assert(std::numeric_limits<T>::is_signed);
        return intmax_t{ x };
    }


    struct unguarded
    {
        template<typename T, typename U>
        constexpr
        auto cast(T x, U) const -> U {
            return static_cast<U>(x);
        }

        template<typename T>
        constexpr
        auto plus(T a, T b) const -> T {
            return static_cast<T>(a + b);
        }

        template<typename T>
        constexpr
        auto minus(T a, T b) const -> T {
            return static_cast<T>(a - b);
        }

        template<typename T>
        constexpr
        auto mul(T a, T b) const -> T {
            return static_cast<T>(a * b);
        }

        template<typename T>
        constexpr
        auto div(T a, T b) const -> T {
            return static_cast<T>(a / b);
        }
    };


    // TODO: better name
    struct guarded
    {
        template<typename T, typename U>
        auto cast(T x, U type_tag) const -> U {
            const auto maybe_y{ maybe_cast(x, type_tag) };
            if (!maybe_y) {
                const auto e{ invalid_cast(x, type_tag) };
                throw api::v1::CtkBug{ e };
            }

            return *maybe_y;
        }

        template<typename T>
        constexpr
        auto plus(T a, T b) const -> T {
            const auto [result, valid]{ signed_addition(a, b) };
            if (valid != arithmetic_error::none) {
                throw api::v1::CtkBug{ invalid_addition(a, b, valid) };
            }

            return result;
        }

        template<typename T>
        auto minus(T a, T b) const -> T {
            const auto [result, valid]{ signed_subtraction(a, b) };
            if (valid != arithmetic_error::none) {
                throw api::v1::CtkBug{ invalid_subtraction(a, b, valid) };
            }

            return result;
        }

        template<typename T>
        constexpr
        auto mul(T a, T b) const -> T {
            const auto [result, valid]{ signed_multiplication_impl(a, b) };
            if (valid != arithmetic_error::none) {
                throw api::v1::CtkBug{ invalid_multiplication(a, b, valid) };
            }

            return result;
        }

        template<typename T>
        auto div(T a, T b) const -> T {
            const auto [result, valid]{ signed_division(a, b) };
            if (valid != arithmetic_error::none) {
                throw api::v1::CtkBug{ invalid_division(a, b, valid) };
            }

            return result;
        }
    };


    // TODO: better name
    struct ok
    {
        template<typename T, typename U>
        auto cast(T x, U type_tag) const -> U {
            const auto maybe_y{ maybe_cast(x, type_tag) };
            if (!maybe_y) {
                const auto e{ invalid_cast(x, type_tag) };
                throw api::v1::CtkLimit{ e };
            }

            return *maybe_y;
        }

        template<typename T>
        constexpr
        auto plus(T a, T b) const -> T {
            const auto [result, valid]{ signed_addition(a, b) };
            if (valid != arithmetic_error::none) {
                throw api::v1::CtkLimit{ invalid_addition(a, b, valid) };
            }

            return result;
        }

        template<typename T>
        auto minus(T a, T b) const -> T {
            const auto [result, valid]{ signed_subtraction(a, b) };
            if (valid != arithmetic_error::none) {
                throw api::v1::CtkLimit{ invalid_subtraction(a, b, valid) };
            }

            return result;
        }

        template<typename T>
        constexpr
        auto mul(T a, T b) const -> T {
            const auto [result, valid]{ signed_multiplication_impl(a, b) };
            if (valid != arithmetic_error::none) {
                throw api::v1::CtkLimit{ invalid_multiplication(a, b, valid) };
            }

            return result;
        }

        template<typename T>
        auto div(T a, T b) const -> T {
            const auto [result, valid]{ signed_division(a, b) };
            if (valid != arithmetic_error::none) {
                throw api::v1::CtkLimit{ invalid_division(a, b, valid) };
            }

            return result;
        }
    };


    template<typename T, typename U, typename Guard>
    constexpr
    auto cast(T a, U b, Guard guard) -> U {
        return guard.cast(a, b);
    }


    template<typename T, typename Guard>
    constexpr
    auto plus(T a, T b, Guard guard) -> T {
        return guard.plus(a, b);
    }


    template<typename T, typename Guard>
    constexpr
    auto minus(T a, T b, Guard guard) -> T {
        return guard.minus(a, b);
    }


    template<typename T, typename Guard>
    constexpr
    auto multiply(T a, T b, Guard guard) -> T {
        return guard.mul(a, b);
    }


    template<typename T, typename Guard>
    constexpr
    auto divide(T a, T b, Guard guard) -> T {
        return guard.div(a, b);
    }


    template<typename Guard>
    constexpr
    auto scale(bit_count x, bit_count::value_type length, Guard guard) -> bit_count {
        const bit_count::value_type ix{ x };
        return bit_count{ multiply(ix, length, guard) };
    }

    template<typename Guard>
    constexpr
    auto scale(measurement_count x, measurement_count::value_type length, Guard guard) -> measurement_count {
        const measurement_count::value_type ix{ x };
        return measurement_count{ multiply(ix, length, guard) };
    }

    template<typename Guard>
    constexpr
    auto scale(bit_count x, measurement_count length, Guard guard) -> bit_count {
        const bit_count::value_type ix{ x };
        const measurement_count::value_type iy{ length };
        return bit_count{ multiply(ix, iy, guard) };
    }


    static constexpr
    sint bits_per_byte = 8;


    template<typename Guard>
    constexpr
    auto as_bits(byte_count x, Guard guard) -> bit_count {
        const byte_count::value_type ix{ x };
        return bit_count{ multiply(ix, bits_per_byte, guard) };
    }

    struct btb_ceil
    {
        constexpr
        auto bytes(byte_count::value_type x) const -> byte_count {
            return x ? byte_count{ 1 } : byte_count{ 0 };
        }
    };

    struct btb_floor
    {
        constexpr
        auto bytes(byte_count::value_type) const -> byte_count {
            return byte_count{ 0 };
        }
    };

    template<typename Rounding = btb_ceil>
    constexpr
    auto as_bytes(bit_count x, Rounding rounding = btb_ceil{}) -> byte_count {
        assert(0 <= x);

        const bit_count::value_type ix{ x };
        const auto[quot, rem]{ std::div(ix, bits_per_byte) };
        const byte_count result{ cast(quot, byte_count::value_type{}, guarded{}) };
        return result + rounding.bytes(rem);
    }


    constexpr
    auto one_byte() -> bit_count {
        return bit_count{ bits_per_byte };
    }


    template<typename T>
    constexpr
    auto size_in_bits() -> bit_count {
        return as_bits( byte_count{ static_cast<bit_count::value_type>(sizeof(T)) }, unguarded{} );
    }


    template<typename I, typename Guard>
    constexpr
        auto size_in_bits(I first, I last, Guard guard) -> bit_count {
        using T = typename std::iterator_traits<I>::value_type;

        const auto l{ std::distance(first, last) };
        const auto length{ cast(l, bit_count::value_type{}, guard) };
        return scale(size_in_bits<T>(), length, guard);
    }


    template<typename U, typename S, typename Guard>
    constexpr
    auto in_signed_range(U x, S, Guard guard) -> bool {
        static_assert(std::is_unsigned<U>::value);
        static_assert(std::is_signed<S>::value);

        return x <= cast(std::numeric_limits<S>::max(), U{}, guard);
    }


    constexpr
    auto as_sizet_unchecked(sint x) -> size_t {
        assert(0 <= x);
        return static_cast<size_t>(x);
    }

    constexpr
    auto as_sizet_unchecked(sensor_count x) -> size_t {
        const sensor_count::value_type ix{ x };
        return as_sizet_unchecked(ix);
    }

    constexpr
    auto as_sizet_unchecked(measurement_count x) -> size_t {
        const measurement_count::value_type ix{ x };
        return as_sizet_unchecked(ix);
    }

    constexpr
    auto as_sizet_unchecked(epoch_count x) -> size_t {
        const epoch_count::value_type ix{ x };
        return as_sizet_unchecked(ix);
    }

    constexpr
    auto as_sizet_unchecked(bit_count x) -> size_t {
        const bit_count::value_type ix{ x };
        return as_sizet_unchecked(ix);
    }

    constexpr
    auto as_sizet_unchecked(byte_count x) -> size_t {
        const byte_count::value_type ix{ x };
        return as_sizet_unchecked(ix);
    }


    template<typename T>
    auto vsize(const std::vector<T>& v) -> sint {
        return cast(v.size(), sint{}, ok{});
    }

} /* namespace impl */ } /* namespace ctk */

