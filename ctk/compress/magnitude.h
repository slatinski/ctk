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

#include <algorithm>
#include <numeric>


namespace ctk { namespace impl {

    // time, one pass
    template<typename I>
    // requirements
    //     - I is ForwardIterator
    auto reduce_row_time(I first, I last, I output) -> I {
        using T = typename std::iterator_traits<I>::value_type;
        static_assert(std::is_unsigned<T>::value, "signed integer underflow");

        return std::adjacent_difference(first, last, output);
    }


    // time, one pass
    template<typename I>
    // requirements
    //     - I is ForwardIterator
    auto restore_row_time(I first, I last) -> I {
        using T = typename std::iterator_traits<I>::value_type;
        static_assert(std::is_unsigned<T>::value, "signed integer overflow");

        return std::partial_sum(first, last, first);
    }





    // time2, from input, 1 pass
    template<typename I>
    // requirements
    //     - I is ForwardIterator
    auto reduce_row_time2_from_input_one_pass(I first, I last, I output) -> I {
        using T = typename std::iterator_traits<I>::value_type;
        static_assert(std::is_unsigned<T>::value, "signed integer underflow");
        assert(first != last && (first + 1) != last);

        I first_minus_one{ first };
        *output = *first;
        ++output;
        ++first;

        *output = static_cast<T>(*first - *first_minus_one);
        I first_minus_two{ first_minus_one };
        ++output;
        ++first;
        ++first_minus_one;

        while(first != last) {
            *output = static_cast<T>((*first - *first_minus_one) - (*first_minus_one - *first_minus_two));

            ++output;
            ++first;
            ++first_minus_one;
            ++first_minus_two;
        }

        return output;
    }


    // time2, from input, 2 passes, with buffer
    template<typename I>
    // requirements
    //     - I is ForwardIterator
    auto reduce_row_time2_from_input(I first, I last, I buffer, I output) -> I {
        using T = typename std::iterator_traits<I>::value_type;
        static_assert(std::is_unsigned<T>::value, "signed integer underflow");
        assert(first != last);

        *output = *first;
        ++output;

        const I buffer_last{ std::adjacent_difference(first, last, buffer) };
        ++buffer;

        return std::adjacent_difference(buffer, buffer_last, output);
    }


    // time2, from time, 1 pass
    template<typename I>
    // requirements
    //     - I is ForwardIterator
    auto reduce_row_time2_from_time(I first_time, I last_time, I output) -> I {
        using T = typename std::iterator_traits<I>::value_type;
        static_assert(std::is_unsigned<T>::value, "signed integer underflow");
        assert(first_time != last_time);

        *output = *first_time;
        return std::adjacent_difference(std::next(first_time), last_time, std::next(output));
    }


    // time2, 2 passes
    template<typename I>
    // requirements
    //     - I is ForwardIterator
    auto restore_row_time2(I first, I last) -> I {
        using T = typename std::iterator_traits<I>::value_type;
        static_assert(std::is_unsigned<T>::value, "signed integer overflow");

        if (first == last) {
            return first;
        }

        const I second{ std::next(first) };
        if (std::partial_sum(second, last, second) != last) {
            throw api::v1::CtkBug{ "restore_row_time2: cannot sum" };
        }

        return std::partial_sum(first, last, first);
    }


    // time2, 1 pass
    template<typename I>
    // requirements
    //     - I is ForwardIterator
    auto restore_row_time2_from_buffer(I first, I last, I output) -> I {
        using T = typename std::iterator_traits<I>::value_type;
        static_assert(std::is_unsigned<T>::value, "signed integer overflow");
        assert(first != output);

        if (first == last) {
            return output;
        }

        I output_minus_two{ output };
        *output = *first;
        ++first;
        ++output;

        if (first == last) {
            return output;
        }

        I output_minus_one{ output };
        *output = static_cast<T>(*output_minus_two + *first);
        ++first;
        ++output;

        while(first != last) {
            *output = static_cast<T>(*output_minus_one + *output_minus_one - *output_minus_two + *first);

            ++output;
            ++output_minus_one;
            ++output_minus_two;
            ++first;
        }

        return output;
    }


    template<typename I>
    // requirements
    //     - I is ForwardIterator
    auto reduce_row_chan_from_input(I previous, I first, I last, I output) -> I {
        using T = typename std::iterator_traits<I>::value_type;
        static_assert(std::is_unsigned<T>::value, "signed integer underflow");
        assert(first != last);

        I first_minus_one{ first };
        I previous_minus_one{ previous };

        *output = *first;
        ++output;
        ++first;
        ++previous;

        while (first != last) {
            *output = static_cast<T>(*first - *first_minus_one + *previous_minus_one - *previous);

            ++output;
            ++first;
            ++first_minus_one;
            ++previous;
            ++previous_minus_one;
        }

        return output;
    }


    template<typename I>
    // requirements
    //     - I is ForwardIterator
    auto reduce_row_chan_from_time(I previous, I first, I first_time, I last_time, I output) -> I {
        using T = typename std::iterator_traits<I>::value_type;
        static_assert(std::is_unsigned<T>::value, "signed integer underflow");

        I previous_minus_one = previous;
        *output = *first;
        ++output;
        ++first_time;
        ++previous;

        while (first_time != last_time) {
            *output = static_cast<T>(*first_time + *previous_minus_one - *previous);

            ++output;
            ++first_time;
            ++previous;
            ++previous_minus_one;
        }

        return output;
    }


    template<typename T>
    struct addition_with_constant
    {
        static_assert(std::is_unsigned<T>::value, "signed integer overflow");

        const T c;

        constexpr
        auto operator()(T x, T y) const -> T {
            return static_cast<T>(x + y + c);
        }
    };


    template<typename T>
    struct subtraction_with_constant
    {
        static_assert(std::is_unsigned<T>::value, "signed integer underflow");

        const T c;

        constexpr
        auto operator()(T x, T y) const -> T {
            return static_cast<T>(x - y - c);
        }
    };


    template<typename I>
    // requirements
    //     - I is ForwardIterator
    auto reduce_row_chan_from_input(I previous, I first, I last, I buffer, I output) -> I {
        using T = typename std::iterator_traits<I>::value_type;
        using ternary_minus = subtraction_with_constant<T>;
        static_assert(std::is_unsigned<T>::value, "signed integer underflow");
        assert(first != last);

        *output = *first;
        ++output;

        const T constant{ static_cast<T>(*first - *previous) };
        ++first;
        ++previous;

        const ternary_minus minus_constant{ constant };
        const I buffer_last{ std::transform(first, last, previous, buffer, minus_constant) };

        return std::adjacent_difference(buffer, buffer_last, output);
    }

    template<typename I>
    // requirements
    //     - I is ForwardIterator
    auto restore_row_chan(I previous, I first, I last, I buffer) -> I {
        using T = typename std::iterator_traits<I>::value_type;
        using ternary_plus = addition_with_constant<T>;
        static_assert(std::is_unsigned<T>::value, "signed integer overflow");

        if (first == last) {
            return first;
        }

        const T constant{ static_cast<T>(*first - *previous) };
        ++first;
        ++previous;

        const I buffer_last{ std::partial_sum(first, last, buffer) };

        const ternary_plus plus_constant{ constant };
        return std::transform(buffer, buffer_last, previous, first, plus_constant);
    }


    template<typename I>
    // requirements
    //     - I is ForwardIterator
    auto restore_row_chan_from_buffer(I first, I last, I previous, I output) -> I {
        using T = typename std::iterator_traits<I>::value_type;
        static_assert(std::is_unsigned<T>::value, "signed integer overflow");

        if (first == last) {
            return output;
        }

        I output_minus_one{ output };
        I previous_minus_one{ previous };

        *output = *first;
        ++first;
        ++output;
        ++previous;

        while(first != last) {
            *output = static_cast<T>(*output_minus_one + *previous - *previous_minus_one + *first);

            ++output;
            ++output_minus_one;
            ++previous;
            ++previous_minus_one;
            ++first;
        }

        return output;
    }

} /* namespace impl */ } /* namespace ctk */

