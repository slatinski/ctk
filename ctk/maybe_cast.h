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

#include <cstdint>
#include <limits>
#include <optional>
#include <cassert>

namespace ctk { namespace impl {

    template<typename U, typename S>
    constexpr
    auto valid_upper_bound(U x, S) -> bool {
        constexpr const bool both_signed{ std::is_signed<U>::value && std::is_signed<S>::value };
        constexpr const bool both_unsigned{ std::is_unsigned<U>::value && std::is_unsigned<S>::value };
        static_assert(sizeof(S) <= sizeof(U) && (both_signed || both_unsigned));

        return x <= static_cast<U>(std::numeric_limits<S>::max());
    }


    template<typename U, typename S>
    constexpr
    auto valid_lower_bound(U x, S) -> bool {
        static_assert(std::is_signed<U>::value);
        static_assert(std::is_signed<S>::value);
        return std::numeric_limits<S>::min() <= static_cast<intmax_t>(x);
    }

    template<bool>
    struct wide_enough_signed2signed;

    template<>
    struct wide_enough_signed2signed<true>
    {
        template<typename S, typename D>
        static
        auto convert(S s, D) -> std::optional<D> {
            static_assert(std::is_signed<S>::value);
            static_assert(std::is_signed<D>::value);
            static_assert(sizeof(S) <= sizeof(D));

            return static_cast<D>(s);
        }
    };

    template<>
    struct wide_enough_signed2signed<false>
    {
        template<typename S, typename D>
        static
        auto convert(S s, D d) -> std::optional<D> {
            static_assert(std::is_signed<S>::value);
            static_assert(std::is_signed<D>::value);
            static_assert(sizeof(D) < sizeof(S));

            if (!valid_upper_bound(s, d) || !valid_lower_bound(s, d)) {
                return std::nullopt;
            }

            return static_cast<D>(s);
        }
    };


    template<bool>
    struct wide_enough_unsigned2unsigned;

    template<>
    struct wide_enough_unsigned2unsigned<true>
    {
        template<typename S, typename D>
        static
        auto convert(S s, D) -> std::optional<D> {
            static_assert(std::is_unsigned<S>::value);
            static_assert(std::is_unsigned<D>::value);
            static_assert(sizeof(S) <= sizeof(D));
            return static_cast<D>(s);
        }
    };

    template<>
    struct wide_enough_unsigned2unsigned<false>
    {
        template<typename S, typename D>
        static
        auto convert(S s, D d) -> std::optional<D> {
            static_assert(std::is_unsigned<S>::value);
            static_assert(std::is_unsigned<D>::value);
            static_assert(sizeof(D) < sizeof(S));

            if (!valid_upper_bound(s, d)) {
                return std::nullopt;
            }

            return static_cast<D>(s);
        }
    };


    template<bool>
    struct wide_enough_unsigned2signed;

    template<>
    struct wide_enough_unsigned2signed<true>
    {
        template<typename S, typename D>
        static
        auto convert(S s, D) -> std::optional<D> {
            static_assert(std::is_unsigned<S>::value);
            static_assert(std::is_signed<D>::value);
            static_assert(sizeof(S) < sizeof(D));

            return static_cast<D>(s);
        }
    };

    template<>
    struct wide_enough_unsigned2signed<false>
    {
        template<typename S, typename D>
        static
        auto convert(S s, D) -> std::optional<D> {
            static_assert(std::is_unsigned<S>::value);
            static_assert(std::is_signed<D>::value);
            static_assert(sizeof(D) <= sizeof(S));

            const uintmax_t umax{ std::numeric_limits<D>::max() };
            if (umax < s) {
                return std::nullopt;
            }

            return static_cast<D>(s);
        }
    };

    template<bool>
    struct wide_enough_signed2unsigned;

    template<>
    struct wide_enough_signed2unsigned<true>
    {
        template<typename S, typename D>
        static
        auto convert(S s, D) -> std::optional<D> {
            static_assert(std::is_signed<S>::value);
            static_assert(std::is_unsigned<D>::value);
            static_assert(sizeof(S) <= sizeof(D));
            assert(0 <= s);

            return static_cast<D>(s);
        }
    };

    template<>
    struct wide_enough_signed2unsigned<false>
    {
        template<typename S, typename D>
        static
        auto convert(S s, D) -> std::optional<D> {
            static_assert(std::is_signed<S>::value);
            static_assert(std::is_unsigned<D>::value);
            static_assert(sizeof(D) < sizeof(S));

            const uintmax_t umax{ std::numeric_limits<D>::max() };
            if (umax < static_cast<uintmax_t>(s)) {
                return std::nullopt;
            }

            return static_cast<D>(s);
        }
    };

    template<bool/* is source signed? */, bool/* is dest signed? */>
    struct cast_to;

    template<>
    struct cast_to<true/* source signed */, true/* dest signed */>
    {
        template<typename S, typename D>
        static
        auto cast(S s, D d) -> std::optional<D> {
            static_assert(std::is_integral<S>::value);
            static_assert(std::is_integral<D>::value);

            using op = wide_enough_signed2signed<sizeof(S) <= sizeof(D)>;
            return op::convert(s, d);
        }
    };

    template<>
    struct cast_to<true/* source signed */, false/* dest unsigned */>
    {
        template<typename S, typename D>
        static
        auto cast(S s, D d) -> std::optional<D> {
            static_assert(std::is_integral<S>::value);
            static_assert(std::is_integral<D>::value);

            if (s < 0) {
                return std::nullopt;
            }

            using op = wide_enough_signed2unsigned<sizeof(S) <= sizeof(D)>;
            return op::convert(s, d);
        }
    };

    template<>
    struct cast_to<false/* source unsigned */, true/* dest signed */>
    {
        template<typename S, typename D>
        static
        auto cast(S s, D d) -> std::optional<D> {
            static_assert(std::is_integral<S>::value);
            static_assert(std::is_integral<D>::value);

            using op = wide_enough_unsigned2signed<sizeof(S) < sizeof(D)>;
            return op::convert(s, d);
        }
    };

    template<>
    struct cast_to<false/* source unsigned */, false/* dest unsigned */>
    {
        template<typename S, typename D>
        static
        auto cast(S s, D d) -> std::optional<D> {
            static_assert(std::is_integral<S>::value);
            static_assert(std::is_integral<D>::value);

            using op = wide_enough_unsigned2unsigned<sizeof(S) <= sizeof(D)>;
            return op::convert(s, d);
        }
    };

    template<typename X, typename Y>
    auto maybe_cast(X x, Y type_tag) -> std::optional<Y> {
        using op = cast_to<std::is_signed<X>::value, std::is_signed<Y>::value>;
        return op::cast(x, type_tag);
    }

} /* namespace impl */ } /* namespace ctk */

