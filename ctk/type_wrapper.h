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

#include <limits>
#include <ostream>

namespace ctk { namespace impl {
	
    template<typename Repr, typename Tag>
    class incompatible_integral
    {
        static_assert(std::is_integral<Repr>::value);

        Repr count;

    public:

        using value_type = Repr;

        constexpr
        incompatible_integral()
            : count{0} {
        }

        explicit constexpr
        incompatible_integral(Repr x)
            : count{x} {
        }

        constexpr
        incompatible_integral& operator++() {
            ++count;
            return *this;
        }

        incompatible_integral operator++(int) {
            const auto x{ *this };
            ++(*this);
            return x;
        }

        constexpr
        incompatible_integral& operator--() {
            --count;
            return *this;
        }

        incompatible_integral operator--(int) {
            const auto x{ *this };
            --(*this);
            return x;
        }

        constexpr
        incompatible_integral& operator+=(const incompatible_integral& x) {
            count += x.count;
            return *this;
        }

        constexpr
        incompatible_integral& operator-=(const incompatible_integral& x) {
            count -= x.count;
            return *this;
        }

        constexpr
        incompatible_integral& operator*=(const incompatible_integral& x) {
            count *= x.count;
            return *this;
        }

        constexpr
        incompatible_integral& operator/=(const incompatible_integral& x) {
            count /= x.count;
            return *this;
        }

        explicit constexpr
        operator Repr() const {
            return count;
        }

        friend
        auto operator<<(std::ostream& os, const incompatible_integral& x) -> std::ostream& {
            os << x.count;
            return os;
        }
    };

    template<typename Repr, typename Tag>
    constexpr
    auto operator==(const incompatible_integral<Repr, Tag>& x, const incompatible_integral<Repr, Tag>& y) -> bool {
        return static_cast<Repr>(x) == static_cast<Repr>(y);
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator==(const incompatible_integral<Repr, Tag>& x, int y) -> bool {
        return static_cast<Repr>(x) == y;
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator==(int x, const incompatible_integral<Repr, Tag>& y) -> bool {
        return x == static_cast<Repr>(y);
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator!=(const incompatible_integral<Repr, Tag>& x, const incompatible_integral<Repr, Tag>& y) -> bool {
        return static_cast<Repr>(x) != static_cast<Repr>(y);
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator!=(const incompatible_integral<Repr, Tag>& x, int y) -> bool {
        return static_cast<Repr>(x) != y;
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator!=(int x, const incompatible_integral<Repr, Tag>& y) -> bool {
        return x != static_cast<Repr>(y);
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator<(const incompatible_integral<Repr, Tag>& x, const incompatible_integral<Repr, Tag>& y) -> bool {
        return static_cast<Repr>(x) < static_cast<Repr>(y);
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator<(const incompatible_integral<Repr, Tag>& x, int y) -> bool {
        return static_cast<Repr>(x) < y;
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator<(int x, const incompatible_integral<Repr, Tag>& y) -> bool {
        return x < static_cast<Repr>(y);
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator>(const incompatible_integral<Repr, Tag>& x, const incompatible_integral<Repr, Tag>& y) -> bool {
        return static_cast<Repr>(x) > static_cast<Repr>(y);
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator>(const incompatible_integral<Repr, Tag>& x, int y) -> bool {
        return static_cast<Repr>(x) > y;
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator>(int x, const incompatible_integral<Repr, Tag>& y) -> bool {
        return x > static_cast<Repr>(y);
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator<=(const incompatible_integral<Repr, Tag>& x, const incompatible_integral<Repr, Tag>& y) -> bool {
        return static_cast<Repr>(x) <= static_cast<Repr>(y);
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator<=(const incompatible_integral<Repr, Tag>& x, int y) -> bool {
        return static_cast<Repr>(x) <= y;
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator<=(int x, const incompatible_integral<Repr, Tag>& y) -> bool {
        return x <= static_cast<Repr>(y);
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator>=(const incompatible_integral<Repr, Tag>& x, const incompatible_integral<Repr, Tag>& y) -> bool {
        return static_cast<Repr>(x) >= static_cast<Repr>(y);
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator>=(const incompatible_integral<Repr, Tag>& x, int y) -> bool {
        return static_cast<Repr>(x) >= y;
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator>=(int x, const incompatible_integral<Repr, Tag>& y) -> bool {
        return x >= static_cast<Repr>(y);
    }

    template<typename Repr, typename Tag>
    constexpr
    auto operator+(const incompatible_integral<Repr, Tag>& x, const incompatible_integral<Repr, Tag>& y) -> incompatible_integral<Repr, Tag> {
        using N = incompatible_integral<Repr, Tag>;
        return N(static_cast<Repr>(x) + static_cast<Repr>(y));
    }


    template<typename Repr, typename Tag>
    constexpr
    auto operator-(const incompatible_integral<Repr, Tag>& x, const incompatible_integral<Repr, Tag>& y) -> incompatible_integral<Repr, Tag> {
        using N = incompatible_integral<Repr, Tag>;
        return N(static_cast<Repr>(x) - static_cast<Repr>(y));
    }


    template<typename Repr, typename Tag>
    constexpr
    auto operator*(const incompatible_integral<Repr, Tag>& x, const incompatible_integral<Repr, Tag>& y) -> incompatible_integral<Repr, Tag> {
        using N = incompatible_integral<Repr, Tag>;
        return N(static_cast<Repr>(x) * static_cast<Repr>(y));
    }


    template<typename Repr, typename Tag>
    constexpr
    auto operator/(const incompatible_integral<Repr, Tag>& x, const incompatible_integral<Repr, Tag>& y) -> incompatible_integral<Repr, Tag> {
        using N = incompatible_integral<Repr, Tag>;
        return N(static_cast<Repr>(x) / static_cast<Repr>(y));
    }


    enum class encoding_size{ one_byte, two_bytes, four_bytes, eight_bytes, length };
    std::ostream& operator<<(std::ostream&, const encoding_size&);

    enum class encoding_method{ copy, time, time2, chan, length };
    std::ostream& operator<<(std::ostream&, const encoding_method&);


    //using sint = int16_t;
    using sint = int64_t;

    struct tag_bits;
    struct tag_bytes;
    struct tag_sensors;
    struct tag_measurements;
    struct tag_epochs;
    struct tag_segments;

    using bit_count         = incompatible_integral<sint, tag_bits>;
    using byte_count        = incompatible_integral<sint, tag_bytes>;
    using sensor_count      = incompatible_integral<sint, tag_sensors>;
    using measurement_count = incompatible_integral<sint, tag_measurements>;
    using epoch_count       = incompatible_integral<sint, tag_epochs>;
    using segment_count     = incompatible_integral<sint, tag_segments>;

} /* namespace impl */ } /* namespace ctk */

