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

#include <algorithm>

#include "evt/event_lib.h"
#include "container/io.h"


namespace ctk { namespace impl {

    namespace {

        static
        auto wide2narrow(const std::wstring& xs) -> std::string {
            const auto size{ xs.size() };

            std::string ys;
            ys.resize(size * 2);
            for (size_t i{ 0 }; i < size; ++i) {
                const wchar_t x{ xs[i] };
                ys[i * 2] = static_cast<char>((x >> 8) & 0xff);
                ys[i * 2 + 1] = static_cast<char>(x & 0xff);
            }
            return ys;
        }

        static
        auto narrow2wide(const std::string& xs) -> std::wstring {
            auto size{ xs.size() };
            assert((size & 1) == 0);
            size /= 2;

            std::wstring ys;
            ys.resize(size);
            for (size_t i{ 0 }; i < size; ++i) {
                ys[i] = static_cast<wchar_t>((xs[i * 2] << 8) | xs[i * 2 + 1]);
            }
            return ys;
        }



        template<typename T>
        auto value2string(const T& x) -> std::string {
            // TODO: to_chars/from_chars
            return std::to_string(x);
        }


        static
        auto value2strings(const std::wstring& x) -> std::vector<std::string> {
            return { wide2narrow(x) };
        }

        template<typename T>
        auto value2strings(T x) -> std::vector<std::string> {
            return { value2string(x) };
        }


        static
        auto values2strings(const std::vector<std::wstring>& xs) -> std::vector<std::string> {
            std::vector<std::string> ys(xs.size());
            std::transform(std::begin(xs), std::end(xs), std::begin(ys), wide2narrow);
            return ys;
        }

        template<typename T>
        auto values2strings(const std::vector<T>& xs) -> std::vector<std::string> {
            const auto bin2str = [](const T& x) -> std::string { return value2string(x); };

            std::vector<std::string> ys(xs.size());
            std::transform(std::begin(xs), std::end(xs), std::begin(ys), bin2str);
            return ys;
        }


        static
        auto str2bin(const std::string& x, float) -> float {
            return std::stof(x);
        }

        static
        auto str2bin(const std::string& x, double) -> double {
            return std::stod(x);
        }

        static
        auto str2bin(const std::string& x, bool) -> bool {
            return std::stoi(x) != 0; // assumes that std::to_string(bool) writes out 0 for false
        }

        // TODO: add functions for integral types larger than what stoi handles; use from_chars
        template<typename T>
        auto str2bin(const std::string& x, T) -> T {
            return static_cast<T>(std::stoi(x));
        }


        template<typename T>
        auto strings2values(const std::vector<std::string>& xs, T type_tag) -> std::vector<T> {
            std::vector<T> ys(xs.size());

            std::transform(std::begin(xs), std::end(xs), std::begin(ys), [type_tag](const std::string& x) { return str2bin(x, type_tag); });
            return ys;
        }


    } // anonymous namespace



    auto is_int8(const str_variant& x) -> bool { return !x.is_array && x.type == vt_i1 && x.data.size() == 1; }
    auto is_int16(const str_variant& x) -> bool { return !x.is_array && x.type == vt_i2 && x.data.size() == 1; }
    auto is_int32(const str_variant& x) -> bool { return !x.is_array && x.type == vt_i4 && x.data.size() == 1; }
    //auto is_int64(const str_variant& x) -> bool { return !x.is_array && x.type == vt_i8 && x.data.size() == 1; }
    auto is_uint8(const str_variant& x) -> bool { return !x.is_array && x.type == vt_u1 && x.data.size() == 1; }
    auto is_uint16(const str_variant& x) -> bool { return !x.is_array && x.type == vt_u2 && x.data.size() == 1; }
    auto is_uint32(const str_variant& x) -> bool { return !x.is_array && x.type == vt_u4 && x.data.size() == 1; }
    //auto is_uint64(const str_variant& x) -> bool { return !x.is_array && x.type == vt_u8 && x.data.size() == 1; }
    auto is_float(const str_variant& x) -> bool { return !x.is_array && x.type == vt_r4 && x.data.size() == 1; }
    auto is_double(const str_variant& x) -> bool { return !x.is_array && x.type == vt_r8 && x.data.size() == 1; }
    auto is_bool(const str_variant& x) -> bool { return !x.is_array && x.type == vt_bool && x.data.size() == 1; }
    auto is_wstring(const str_variant& x) -> bool { return !x.is_array && x.type == vt_bstr && x.data.size() == 1; }
    auto is_float_array(const str_variant& x) -> bool { return x.is_array && x.type == vt_r4; }

    auto as_int8(const str_variant& x) -> int8_t { assert(is_int8(x)); return str2bin(x.data[0], int8_t{}); }
    auto as_int16(const str_variant& x) -> int16_t { assert(is_int16(x)); return str2bin(x.data[0], int16_t{}); }
    auto as_int32(const str_variant& x) -> int32_t { assert(is_int32(x)); return str2bin(x.data[0], int32_t{}); }
    //auto as_int64(const str_variant& x) -> int64_t { assert(is_int64(x)); return str2bin(x.data[0], int64_t{}); }
    auto as_uint8(const str_variant& x) -> uint8_t { assert(is_uint8(x)); return str2bin(x.data[0], uint8_t{}); }
    auto as_uint16(const str_variant& x) -> uint16_t { assert(is_uint16(x)); return str2bin(x.data[0], uint16_t{}); }
    auto as_uint32(const str_variant& x) -> uint32_t { assert(is_uint32(x)); return str2bin(x.data[0], uint32_t{}); }
    //auto as_uint64(const str_variant& x) -> uint64_t { assert(is_uint64(x)); return str2bin(x.data[0], uint64_t{}); }
    auto as_float(const str_variant& x) -> float { assert(is_float(x)); return str2bin(x.data[0], float{}); }
    auto as_double(const str_variant& x) -> double { assert(is_double(x)); return str2bin(x.data[0], double{}); }
    auto as_bool(const str_variant& x) -> bool { assert(is_bool(x)); return str2bin(x.data[0], bool{}); }
    auto as_wstring(const str_variant& x) -> std::wstring { assert(is_wstring(x)); return narrow2wide(x.data[0]); }
    auto as_float_array(const str_variant& x) -> std::vector<float> { assert(is_float_array(x)); return strings2values(x.data, float{}); }


    str_variant::str_variant()
    : type{ vt_empty }
    , is_array{ false } {
    }

    str_variant::str_variant(int8_t x)
    : type{ vt_i1 }
    , is_array{ false }
    , data{ value2strings(x) } {
    }

    str_variant::str_variant(int16_t x)
    : type{ vt_i2 }
    , is_array{ false }
    , data{ value2strings(x) } {
    }

    str_variant::str_variant(int32_t x)
    : type{ vt_i4 }
    , is_array{ false }
    , data{ value2strings(x) } {
    }

    /*
    str_variant::str_variant(int64_t x)
    : type{ vt_i8 }
    , is_array{ false }
    , data{ value2strings(x) } {
    }
    */

    str_variant::str_variant(uint8_t x)
    : type{ vt_u1 }
    , is_array{ false }
    , data{ value2strings(x) } {
    }

    str_variant::str_variant(uint16_t x)
    : type{ vt_u2 }
    , is_array{ false }
    , data{ value2strings(x) } {
    }

    str_variant::str_variant(uint32_t x)
    : type{ vt_u4 }
    , is_array{ false }
    , data{ value2strings(x) } {
    }

    /*
    str_variant::str_variant(uint64_t x)
    : type{ vt_u8 }
    , is_array{ false }
    , data{ value2strings(x) } {
    }
    */

    str_variant::str_variant(float x)
    : type{ vt_r4 }
    , is_array{ false }
    , data{ value2strings(x) } {
    }

    str_variant::str_variant(double x)
    : type{ vt_r8 }
    , is_array{ false }
    , data{ value2strings(x) } {
    }

    str_variant::str_variant(bool x)
    : type{ vt_bool }
    , is_array{ false }
    , data{ value2strings(x) } {
    }

    str_variant::str_variant(const std::wstring& x)
    : type{ vt_bstr }
    , is_array{ false }
    , data{ { value2strings(x) } } {
    }

    str_variant::str_variant(const std::vector<int8_t>& xs)
    : type{ vt_i1 }
    , is_array{ true }
    , data{ values2strings(xs) } {
    }

    str_variant::str_variant(const std::vector<int16_t>& xs)
    : type{ vt_i2 }
    , is_array{ true }
    , data{ values2strings(xs) } {
    }

    str_variant::str_variant(const std::vector<int32_t>& xs)
    : type{ vt_i4 }
    , is_array{ true }
    , data{ values2strings(xs) } {
    }

    /*
    str_variant::str_variant(const std::vector<int64_t>& xs)
    : type{ vt_i8 }
    , is_array{ true }
    , data{ values2strings(xs) } {
    }
    */

    str_variant::str_variant(const std::vector<uint8_t>& xs)
    : type{ vt_u1 }
    , is_array{ true }
    , data{ values2strings(xs) } {
    }

    str_variant::str_variant(const std::vector<uint16_t>& xs)
    : type{ vt_u2 }
    , is_array{ true }
    , data{ values2strings(xs) } {
    }

    str_variant::str_variant(const std::vector<uint32_t>& xs)
    : type{ vt_u4 }
    , is_array{ true }
    , data{ values2strings(xs) } {
    }

    /*
    str_variant::str_variant(const std::vector<uint64_t>& xs)
    : type{ vt_u8 }
    , is_array{ true }
    , data{ values2strings(xs) } {
    }
    */

    str_variant::str_variant(const std::vector<float>& xs)
    : type{ vt_r4 }
    , is_array{ true }
    , data{ values2strings(xs) } {
    }

    str_variant::str_variant(const std::vector<double>& xs)
    : type{ vt_r8 }
    , is_array{ true }
    , data{ values2strings(xs) } {
    }

    str_variant::str_variant(const std::vector<bool>& xs)
    : type{ vt_bool }
    , is_array{ true }
    , data{ values2strings(xs) } {
    }

    str_variant::str_variant(const std::vector<std::wstring>& xs)
    : type{ vt_bstr }
    , is_array{ true }
    , data{ values2strings(xs) } {
    }



    event_descriptor::event_descriptor() {
    }

    event_descriptor::event_descriptor(const str_variant& value)
    : value{ value } {
    }

    event_descriptor::event_descriptor(const str_variant& value, const std::string& name)
    : name{ name }
    , value{ value } {
    }

    event_descriptor::event_descriptor(const str_variant& value, const std::string& name, const std::string& unit)
    : name{ name }
    , unit{ unit }
    , value{ value } {
    }


    guid::guid()
    : data1{ 0 }
    , data2{ 0 }
    , data3{ 0 }
    , data4{ 0, 0, 0, 0, 0, 0, 0, 0 } {
    }


    base_event::base_event()
    : visible_id{ 0 }
    , type{ 0 }
    , state{ 0 }
    , original{ 1 }
    , duration{ 0 }
    , duration_offset{ 0 }
    , stamp{ api::dcdate2timepoint({ 0, 0 }) } {
    }


    base_event::base_event(const std::chrono::system_clock::time_point& stamp, int32_t type, const std::string& name, const std::vector<event_descriptor>& descriptors, double duration, double offset)
    : visible_id{ 0 }
    , name{ name }
    , type{ type }
    , state{ 0 }
    , original{ 1 }
    , duration{ duration }
    , duration_offset{ offset }
    , stamp{ stamp }
    , descriptors{ descriptors } {
    }


    marker_event::marker_event()
    : show_amplitude{ 0 }
    , show_duration{ 0 } {
    }

    marker_event::marker_event(const base_event& common, const std::string& description)
    : common{ common }
    , description{ description }
    , show_amplitude{ 0 }
    , show_duration{ 0 } {
    }

    event_library::event_library()
    : version{ default_output_file_version() } {
    }


    namespace sizes {
        const uint8_t max_byte{ std::numeric_limits<uint8_t>::max() };
        const uint16_t max_word{ std::numeric_limits<uint16_t>::max() };
        const uint32_t max_dword{ std::numeric_limits<uint32_t>::max() };
    } // namespace sizes


    namespace tags {
        const int32_t null{ 0 };
        const int32_t name{ -1 };
        const uint16_t unicode{ sizes::max_word - 1 };
    } // namespace tags



    // reads the length and the character width of a string stored in a file.
    // the size might be encoded in 1, 2, 4 or 8 bytes.
    // the valid character widths are 1 or 2 bytes per character.
    static
    auto read_string_properties(FILE* f) -> std::pair<size_t, unsigned> {
        static_assert(sizeof(uint8_t) == 1, "assumed by this function");
        static_assert(sizeof(uint16_t) == 2, "assumed by this function");
        static_assert(sizeof(uint32_t) == 4, "assumed by this function");
        static_assert(sizeof(uint64_t) == 8, "assumed by this function");

        size_t length{ 0 };
        unsigned character_width{ sizeof(uint8_t) };

        // for short strings (< 0xff characters) the length is encoded in a byte
        uint8_t byte{ read(f, uint8_t{}) };
        if (byte < sizes::max_byte) {
            length = byte;
            return { length, character_width };
        }

        // string length >= 0xff. the size might be encoded in the next word or the next word might contain a "unicode marker" instead.
        // if the word contains a "unicode marker" then the character data of the string is encoded in 2 bytes per character.
        // the string length might be encoded either in a word or in a dword or in a qword.
        uint16_t word{ read(f, uint16_t{}) };

        if (word == tags::unicode) {
            // unicode marker encountered. the string consists of two bytes per character.
            character_width = sizeof(uint16_t);

            // for short strings (< 0xff characters) the size is encoded in a byte
            byte = read(f, uint8_t{});
            if (byte < sizes::max_byte) {
                length = byte;
                return { length, character_width };
            }

            // string length >= 0xff. the string length might be encoded either in a word or in a dword or in a qword.
            word = read(f, uint16_t{});
        }

        // character width 1 or 2, string length < 0xffff
        if (word < sizes::max_word) {
            length = word;
            return { length, character_width };
        }

        const uint32_t dword{ read(f, uint32_t{}) };

        // character width 1 or 2, string length < 0xffffffff 
        if (dword < sizes::max_dword) {
            length = dword;
            return { length, character_width };
        }

        const uint64_t qword{ read(f, uint64_t{}) };
        length = qword;
        return { length, character_width };
    }


    static
    auto write_string_properties(FILE* f, size_t length, bool unicode) -> void {
        if (unicode) {
            write(f, sizes::max_byte);
            write(f, tags::unicode);
        }

        if (length < sizes::max_byte) {
            write(f, static_cast<uint8_t>(length));
        }
        else if (length < (sizes::max_word)) {
            write(f, sizes::max_byte);
            write(f, static_cast<uint16_t>(length));
        }
        else if (length < sizes::max_dword) {
            write(f, sizes::max_byte);
            write(f, sizes::max_word);
            write(f, static_cast<uint32_t>(length));
        }
        else {
            write(f, sizes::max_byte);
            write(f, sizes::max_word);
            write(f, sizes::max_dword);
            write(f, static_cast<uint64_t>(length));
        }
    }


    static
    auto read_archive_string(FILE* f) -> std::string {
        static_assert(sizeof(std::string::value_type) == 1, "assumed by this function");

        const auto [length, character_width]{ read_string_properties(f) };
        if (character_width != 1 && character_width != 2) {
            throw api::v1::ctk_data{ "read_archive_string: character width not equal to 1 or 2" };
        }

        std::string x;
        x.resize(length * character_width);
        read(f, std::begin(x), std::end(x));
        return x;
    }


    static
    auto write_archive_string(FILE* f, const std::string& input) -> void {
        static_assert(sizeof(std::string::value_type) == 1, "assumed by this function");

        constexpr const bool unicode{ false };
        write_string_properties(f, input.size(), unicode);
        write(f, std::begin(input), std::end(input));
    }


    static
    auto read_bstring(FILE* f) -> std::wstring {
        const int32_t size{ read(f, int32_t{}) }; // in bytes
        if ((size & 1) != 0) {
            throw api::v1::ctk_data{ "read_bstring: odd byte string size" };
        }

        constexpr const int32_t unit{ sizeof(int16_t) };
        const int32_t length{ size / unit };
        if (length < 0) {
            throw api::v1::ctk_data{ "read_bstring: negative length" };
        }

        std::wstring xs;
        xs.reserve(static_cast<size_t>(length));
        for (int32_t i{ 0 }; i < length; ++i) {
            const int16_t x{ read(f, int16_t{}) };
            xs.push_back(static_cast<wchar_t>(x));
        }

        return xs;
    }


    static
    auto write_bstring(FILE* f, const std::wstring& xs) -> void {
        constexpr const int32_t unit{ sizeof(int16_t) };

        const int32_t length{ cast(xs.length(), int32_t{}, ok{}) };
        const int32_t size{ multiply(length, unit, ok{}) }; // in bytes
        write(f, size);

        for (wchar_t x : xs) {
            const auto y{ static_cast<int16_t>(x & 0xffff) };
            write(f, y);
        }
    }


    static
    auto read_class(FILE* f, int32_t& class_tag, std::string& class_name) -> bool {
        class_tag = read(f, int32_t{});

        // object without class name
        if (class_tag == tags::null) {
            return true;
        }

        if (class_tag == tags::name) {
            class_name = read_archive_string(f);
            return true;
        }

        return false;
    }


    static
    auto write_class(FILE* f, int32_t class_tag, std::string class_name) -> void {
        if (class_tag == tags::null) {
            write(f, tags::null);
        }
        else if (class_tag == tags::name) {
            write(f, tags::name);
            write_archive_string(f, class_name);
        }
        else {
            assert(false);
        }
    }


    static
    auto read_guid(FILE* f) -> guid {
        guid x;
        x.data1 = read(f, uint32_t{});
        x.data2 = read(f, uint16_t{});
        x.data3 = read(f, uint16_t{});

        for (unsigned i{ 0 }; i < 8; ++i) {
            x.data4[i] = read(f, uint8_t{});
        }

        return x;
    }


    static
    auto write_guid(FILE* f, const guid& x) -> void {
        write(f, x.data1);
        write(f, x.data2);
        write(f, x.data3);

        for (unsigned i{ 0 }; i < 8; ++i) {
            write(f, x.data4[i]);
        }
    }


    // reads a value of type T from a file and converts it to string.
    template<typename T>
    auto bin2str(FILE* f, T type_tag) -> std::string {
        return value2string(read(f, type_tag));
    }


    // reads a value of type bstring from a file.
    static
    auto bin2str(FILE* f, std::wstring) -> std::string {
        return wide2narrow(read_bstring(f));
    }


    // reads a utility vector with elements of type T from a file and converts them to strings.
    template<typename T>
    auto read_archive_vector(FILE* f, T type_tag) -> std::vector<std::string> {
        const uint32_t size{ read(f, uint32_t{}) };

        std::vector<std::string> xs;
        xs.reserve(size);

        for (uint32_t i{ 0 }; i < size; ++i) {
            xs.push_back(bin2str(f, type_tag));
        }
        return xs;
    }


    static
    auto write_value(FILE* f, const std::string& x, vt_e type) -> void {
        switch (type) {
        case vt_empty:
        case vt_null:
            return;

        case vt_i1: write(f, str2bin(x, int8_t{})); return;
        case vt_i2: write(f, str2bin(x, int16_t{})); return;
        case vt_i4: write(f, str2bin(x, int32_t{})); return;
        //case vt_i8: write(f, str2bin(x, int64_t{})); return;
        case vt_u1: write(f, str2bin(x, uint8_t{})); return;
        case vt_u2: write(f, str2bin(x, uint16_t{})); return;
        case vt_u4: write(f, str2bin(x, uint32_t{})); return;
        //case vt_u8: write(f, str2bin(x, uint64_t{})); return;
        case vt_r4: write(f, str2bin(x, float{})); return;
        case vt_r8: write(f, str2bin(x, double{})); return;
        case vt_bstr: write_bstring(f, narrow2wide(x)); return;
        case vt_bool: write(f, str2bin(x, bool{})); return;

        // intentional fall trough: these types cannot be written out as a simple value
        case vt_variant:
        case vt_array:
        case vt_byref:
            break;
        }

        throw api::v1::ctk_data{ "write_value: invalid input" };
    }



    static
    auto write_archive_vector(FILE* f, const str_variant& v) -> void {
        const uint32_t size{ cast(v.data.size(), uint32_t{}, ok{}) };
        write(f, size);

        for (const std::string& x : v.data) {
            write_value(f, x, v.type);
        }
    }


    static
    auto read_simple_variant(FILE* f) -> std::pair<str_variant, bool> {
        bool valid{ false };
        str_variant x;
        x.type = static_cast<vt_e>(read(f, int16_t{}));

        // vt_byref, vt_array and vt_variant are intentionally not handled here
        if (x.type == vt_empty || x.type == vt_null) {
            valid = true;
        }
        else if (x.type == vt_i1) {
            x.data.push_back(bin2str(f, int8_t{}));
            valid = true;
        }
        else if (x.type == vt_i2) {
            x.data.push_back(bin2str(f, int16_t{}));
            valid = true;
        }
        else if (x.type == vt_i4) {
            x.data.push_back(bin2str(f, int32_t{}));
            valid = true;
        }
        /*
        else if (x.type == vt_i8) {
            x.data.push_back(bin2str(f, int64_t{}));
            valid = true;
        }
        */
        else if (x.type == vt_u1) {
            x.data.push_back(bin2str(f, uint8_t{}));
            valid = true;
        }
        else if (x.type == vt_u2) {
            x.data.push_back(bin2str(f, uint16_t{}));
            valid = true;
        }
        else if (x.type == vt_u4) {
            x.data.push_back(bin2str(f, uint32_t{}));
            valid = true;
        }
        /*
        else if (x.type == vt_u8) {
            x.data.push_back(bin2str(f, uint64_t{}));
            valid = true;
        }
        */
        else if (x.type == vt_r4) {
            x.data.push_back(bin2str(f, float{}));
            valid = true;
        }
        else if (x.type == vt_r8) {
            x.data.push_back(bin2str(f, double{}));
            valid = true;
        }
        else if (x.type == vt_bstr) {
            x.data.push_back(bin2str(f, std::wstring{}));
            valid = true;
        }
        else if (x.type == vt_bool) {
            x.data.push_back(bin2str(f, bool{}));
            valid = true;
        }

        return { x, valid };
    }


    static
    auto write_simple_variant(FILE* f, const str_variant& v) -> void {
        const auto count{ v.data.size() };
        if (count != 1) {
            throw api::v1::ctk_bug{ "write_simple_variant: not a simple variant" };
        }

        write(f, static_cast<int16_t>(v.type));
        write_value(f, v.data[0], v.type);
    }


    static
    auto read_variant_array(FILE* f, str_variant& v) -> void {
        const auto [array_type, valid]{ read_simple_variant(f) };
        if (!valid) {
            throw api::v1::ctk_data{ "read_variant_array: invalid array type" };
        }

        v.type = array_type.type;
        v.is_array = true;

        if (array_type.type == vt_i1) {
            v.data = read_archive_vector(f, int8_t{});
        }
        else if (array_type.type == vt_i2) {
            v.data = read_archive_vector(f, int16_t{});
        }
        else if (array_type.type == vt_i4) {
            v.data = read_archive_vector(f, int32_t{});
        }
        /*
        else if (array_type.type == vt_i8) {
            v.data = read_archive_vector(f, int64_t{});
        }
        */
        else if (array_type.type == vt_u1) {
            v.data = read_archive_vector(f, uint8_t{});
        }
        else if (array_type.type == vt_u2) {
            v.data = read_archive_vector(f, uint16_t{});
        }
        else if (array_type.type == vt_u4) {
            v.data = read_archive_vector(f, uint32_t{});
        }
        /*
        else if (array_type.type == vt_u8) {
            v.data = read_archive_vector(f, uint64_t{});
        }
        */
        else if (array_type.type == vt_r4) {
            v.data = read_archive_vector(f, float{});
        }
        else if (array_type.type == vt_r8) {
            v.data = read_archive_vector(f, double{});
        }
        else if (array_type.type == vt_bool) {
            v.data = read_archive_vector(f, bool{});
        }
        else if (array_type.type == vt_bstr) {
            v.data = read_archive_vector(f, std::wstring{});
        }
        else {
            throw api::v1::ctk_data{ "read_variant_array: invalid element type" };
        }
    }


    static
    auto make_dummy_variant(vt_e t) -> str_variant {
        str_variant x;
        x.type = t;

        switch (t) {
        case vt_empty:
        case vt_null:
        case vt_bstr:
            x.data.push_back("");
            break;

        case vt_i1:
        case vt_i2:
        case vt_i4:
        //case vt_i8:
        case vt_u1:
        case vt_u2:
        case vt_u4:
        //case vt_u8:
        case vt_r4:
        case vt_r8:
        case vt_bool:
            x.data.push_back("0");
            break;

        case vt_variant:
        case vt_array:
        case vt_byref:
            throw api::v1::ctk_bug{ "make_dummy_variant: invalid input" };
        }

        return x;
    }


    static
    auto write_variant_array(FILE* f, const str_variant& v) -> void {
        constexpr const int16_t array_of_variants{ vt_array | vt_variant };

        write(f, array_of_variants);
        write_simple_variant(f, make_dummy_variant(v.type));
        write_archive_vector(f, v);
    }



    static
    auto read_variant(FILE* f) -> str_variant {
        auto [x, valid]{ read_simple_variant(f) };
        if (valid) {
            return x;
        }

        if (!(x.type & vt_byref) && !(x.type & vt_array)) {
            throw api::v1::ctk_data{ "read_variant: invalid variant type" };
        }

        read_variant_array(f, x);
        return x;
    }


    static
    auto write_variant(FILE* f, const str_variant& v) -> void {
        if (v.data.empty() && v.type != vt_empty) {
            throw api::v1::ctk_bug{ "write_variant: invalid input" };
        }

        if (v.is_array) {
            write_variant_array(f, v);
        }
        else {
            write_simple_variant(f, v);
        }
    }


    static
    auto read_dcdate(FILE* f) -> std::chrono::system_clock::time_point {
        const double date{ read(f, double{}) };
        const double fraction{ read(f, double{}) };
        return api::dcdate2timepoint({ date, fraction });
    }


    static
    auto write_dcdate(FILE* f, const std::chrono::system_clock::time_point& x) -> void {
        const auto y{ api::timepoint2dcdate(x) };
        write(f, y.date);
        write(f, y.fraction);
    }


    static
    auto read_channel_info(FILE* f) -> channel_info {
        const std::string active{ read_archive_string(f) };
        const std::string reference{ read_archive_string(f) };
        return { active, reference };
    }


    static
    auto write_channel_info(FILE* f, const channel_info& channel) -> void {
        write_archive_string(f, channel.active);
        write_archive_string(f, channel.reference);
    }


    static
    auto read_archive_header(FILE* f, event_library& lib) -> void {
        lib.version = 0;

        read(f, uint32_t{}); // ctime
        read(f, uint32_t{}); // mtime
        read(f, uint32_t{}); // atime
        lib.version = read(f, int32_t{});
        read(f, int32_t{}); // compression mode
        read(f, int32_t{}); // encryption mode
    }


    static
    auto write_archive_header(FILE* f, const event_library& lib) -> void {
        write(f, uint32_t{ 0 }); // ctime
        write(f, uint32_t{ 0 }); // mtime
        write(f, uint32_t{ 0 }); // atime
        write(f, lib.version);
        write(f, int32_t{ 0 }); // compression mode
        write(f, int32_t{ 0 }); // encryption mode
    }


    static
    auto read_empty_class(FILE* f) -> void {
        int32_t class_tag;
        std::string class_name;

        if (!read_class(f, class_tag, class_name) || class_tag != tags::null) {
            throw api::v1::ctk_data{ "read_empty_class: invalid input" };
        }
    }


    static
    auto write_empty_class(FILE* f) -> void {
        write_class(f, tags::null, "");
    }


    static
    auto read_abstract_unique_data_item(FILE* f, base_event& x) -> void {
        x.visible_id = read(f, int32_t{});
        x.unused = read_guid(f);
        read_empty_class(f);
    }


    static
    auto write_abstract_unique_data_item(FILE* f, const base_event& x) -> void {
        write(f, x.visible_id);
        write_guid(f, x.unused);
        write_empty_class(f);
    }


    static
    auto read_abstract_named_data_item(FILE* f, base_event& x, int32_t version) -> void {
        read_abstract_unique_data_item(f, x);
        x.name = read_archive_string(f);

        if (version >= 78) {
            x.user_visible_name = read_archive_string(f);
        }
    }


    static
    auto write_abstract_named_data_item(FILE* f, const base_event& x, int32_t version) -> void {
        write_abstract_unique_data_item(f, x);
        write_archive_string(f, x.name);

        if (version >= 78) {
            write_archive_string(f, x.user_visible_name);
        }
    }


    static
    auto read_descriptor(FILE* f) -> event_descriptor {
        event_descriptor x;
        x.name = read_archive_string(f);
        x.value = read_variant(f);
        x.unit = read_archive_string(f);
        return x;
    }


    static
    auto write_descriptor(FILE* f, const event_descriptor& descriptor) -> void {
        write_archive_string(f, descriptor.name);
        write_variant(f, descriptor.value);
        write_archive_string(f, descriptor.unit);
    }


    static
    auto read_descriptors(FILE* f) -> std::vector<event_descriptor> {
        const int32_t size{ read(f, int32_t{}) };
        if (size < 0) {
            throw api::v1::ctk_data{ "read_descriptors: negative array size" };
        }

        std::vector<event_descriptor> xs;
        xs.reserve(static_cast<size_t>(size));

        for (int32_t i{ 0 }; i < size; ++i) {
            xs.push_back(read_descriptor(f));
        }
        return xs;
    }


    static
    auto write_descriptors(FILE* f, const std::vector<event_descriptor>& xs) -> void {
        const int32_t size{ cast(xs.size(), int32_t{}, ok{}) };
        write(f, size);

        for (const event_descriptor& x : xs) {
            write_descriptor(f, x);
        }
    }


    static
    auto read_event(FILE* f, int32_t version) -> base_event {
        base_event x;
        read_abstract_named_data_item(f, x, version);

        x.type = read(f, int32_t{});
        x.state = read(f, int32_t{});
        x.original = read(f, int8_t{});
        x.duration = read(f, double{});
        x.duration_offset = read(f, double{});
        x.stamp = read_dcdate(f);

        if (version >= 11 && version < 19) {
            throw api::v1::ctk_limit{ "read_event: unsupported file version" };
        }

        if (version >= 19) {
            x.descriptors = read_descriptors(f);
        }

        return x;
    }


    static
    auto write_event(FILE* f, const base_event& x, int32_t version) -> void {
        write_abstract_named_data_item(f, x, version);

        write(f, x.type);
        write(f, x.state);
        write(f, x.original);
        write(f, x.duration);
        write(f, x.duration_offset);
        write_dcdate(f, x.stamp);

        if (version >= 11 && version < 19) {
            throw api::v1::ctk_bug{ "write_event: unsupported file version" };
        }

        if (version >= 19) {
            write_descriptors(f, x.descriptors);
        }
    }


    static
    auto store_event(FILE* f, const artefact_event& x, int32_t version) -> void {
        write_event(f, x.common, version);
        write_channel_info(f, x.channel);

        if (version >= 174) {
            write_archive_string(f, x.description);
        }
    }


    static
    auto load_event(FILE* f, int32_t version, artefact_event x) -> artefact_event {
        x.common = read_event(f, version);
        x.channel = read_channel_info(f);

        if (version >= 174) {
            x.description = read_archive_string(f);
        }

        return x;
    }


    static
    auto store_event(FILE* f, const epoch_event& x, int32_t version) -> void {
        write_event(f, x.common, version);

        if (version < 33) {
            write(f, int32_t{ 0 });
        }
    }


    static
    auto load_event(FILE* f, int32_t version, epoch_event x) -> epoch_event {
        x.common = read_event(f, version);

        if (version < 33) {
            read(f, int32_t{});
        }

        return x;
    }



    static
    auto store_event(FILE* f, const marker_event& x, int32_t version) -> void {
        write_event(f, x.common, version);
        write_channel_info(f, x.channel);
        write_archive_string(f, x.description);

        if (version >= 35) {
            if (version > 103) {
                write(f, x.show_amplitude);
            }
            else {
                const int8_t show_amplitude{ x.show_amplitude > 0 ? int8_t{ 1 } : int8_t{ 0 } };
                write(f, show_amplitude);
            }

            write(f, x.show_duration);
        }
    }


    static
    auto load_event(FILE *f, int32_t version, marker_event x) -> marker_event {
        x.common = read_event(f, version);
        x.channel = read_channel_info(f);
        x.description = read_archive_string(f);

        if (version >= 35) {
            if (version > 103) {
                x.show_amplitude = read(f, int32_t{});
            }
            else {
                x.show_amplitude = read(f, int8_t{});
            }

            x.show_duration = read(f, int8_t{});
        }

        return x;
    }



    static
    auto store_event(FILE* f, const rpeak_event& x, int32_t version) -> void {
        write_event(f, x.common, version);
        write_channel_info(f, x.channel);
        write(f, x.amplitude_peak);
    }


    static
    auto load_event(FILE* f, int32_t version, rpeak_event x) -> rpeak_event {
        x.common = read_event(f, version);
        x.channel = read_channel_info(f);
        x.amplitude_peak = read(f, float{});
        return x;
    }


    static
    auto store_event(FILE* f, const seizure_event& x, int32_t version) -> void {
        write_event(f, x.common, version);
        write_channel_info(f, x.channel);
    }


    static
    auto load_event(FILE* f, int32_t version, seizure_event x) -> seizure_event {
        x.common = read_event(f, version);
        x.channel = read_channel_info(f);
        return x;
    }


    static
    auto store_event(FILE* f, const sleep_event& x, int32_t version) -> void {
        write_event(f, x.common, version);
        write(f, x.base_level);
        write(f, x.threshold);
        write(f, x.min_duration);
        write(f, x.max_value);
        write(f, x.epoch_length);
        write(f, x.epoch_color);
    }


    static
    auto load_event(FILE* f, int32_t version, sleep_event x) -> sleep_event {
        x.common = read_event(f, version);
        x.base_level = read(f, int16_t{});
        x.threshold = read(f, int16_t{});
        x.min_duration = read(f, int16_t{});
        x.max_value = read(f, int16_t{});
        x.epoch_length = read(f, int16_t{});
        x.epoch_color = read(f, int32_t{});
        return x;
    }


    static
    auto store_event(FILE* f, const spike_event& x, int32_t version) -> void {
        write_event(f, x.common, version);
        write_channel_info(f, x.channel);
        write(f, x.amplitude_peak);
        write(f, x.sign);
        write(f, x.group);
        write_dcdate(f, x.top_date);
    }


    static
    auto load_event(FILE* f, int32_t version, spike_event x) -> spike_event {
        x.common = read_event(f, version);
        x.channel = read_channel_info(f);
        x.amplitude_peak = read(f, float{});
        x.sign = read(f, int16_t{});
        x.group = read(f, int16_t{});
        x.top_date = read_dcdate(f);
        return x;
    }



    static
    auto is_impedance(const marker_event& x) -> bool {
        const auto has_descriptor = [](const event_descriptor& d) -> bool { return d.name == descriptor_name::impedance; };

        const auto first{ begin(x.common.descriptors) };
        const auto last{ end(x.common.descriptors) };
        const auto found{ std::find_if(first, last, has_descriptor) };
        if (found == last) {
            return false;
        }

        return is_float_array(found->value);
    }


    static
    auto is_video(const marker_event& x) -> bool {
        const auto has_descriptor = [](const event_descriptor& d) -> bool { return d.name == descriptor_name::video_marker_type; };

        const auto first{ begin(x.common.descriptors) };
        const auto last{ end(x.common.descriptors) };
        const auto found{ std::find_if(first, last, has_descriptor) };
        return found != last;
    }


    static
    auto load_event(FILE* f, event_library& lib, const std::string& class_name) -> void {
        if (class_name == "class dcEpochEvent_c") {
            lib.epochs.push_back(load_event(f, lib.version, epoch_event{}));
        }
        else if (class_name == "class dcEventMarker_c") {
            const marker_event event{ load_event(f, lib.version, marker_event{}) };
            if (is_impedance(event)) {
                lib.impedances.push_back(event);
            }
            else if (is_video(event)) {
                lib.videos.push_back(event);
            }
            else {
                lib.markers.push_back(event);
            }
        }
        else if (class_name == "class dcArtefactEvent_c") {
            lib.artefacts.push_back(load_event(f, lib.version, artefact_event{}));
        }
        else if (class_name == "class dcSpikeEvent_c") {
            lib.spikes.push_back(load_event(f, lib.version, spike_event{}));
        }
        else if (class_name == "class dcSeizureEvent_c") {
            lib.seizures.push_back(load_event(f, lib.version, seizure_event{}));
        }
        else if (class_name == "class dcSleepEvent_c") {
            lib.sleeps.push_back(load_event(f, lib.version, sleep_event{}));
        }
        else if (class_name == "class dcRPeakEvent_c") {
            lib.rpeaks.push_back(load_event(f, lib.version, rpeak_event{}));
        }
        else {
            throw api::v1::ctk_data{ "load_event: invalid class name" };
        }
    }


    static
    auto load_vector_of_pointers(FILE* f, event_library& lib) -> void {
        int32_t class_tag;
        std::string class_name;

        const uint32_t size{ read(f, uint32_t{}) };

        for (uint32_t i{ 0 }; i < size; ++i) {
            if (!read_class(f, class_tag, class_name)) {
                throw api::v1::ctk_data{ "load_vector_of_pointers: invalid class" };
            }

            if (class_tag == tags::null) {
                continue;
            }

            load_event(f, lib, class_name);
        }
    }


    template<typename T>
    auto store_events(FILE* f, const std::vector<T>& xs, int32_t version, std::string class_name) -> void {
        constexpr const int32_t class_tag{ tags::name };

        for (const auto &x : xs) {
            write_class(f, class_tag, class_name);
            store_event(f, x, version);
        }
    }



    static
    auto store_vector_of_pointers(FILE* f, const event_library& lib) -> void {
        const uint32_t count{ event_count(lib, uint32_t{}) };
        write(f, count);

        store_events(f, lib.impedances, lib.version, "class dcEventMarker_c");
        store_events(f, lib.videos, lib.version, "class dcEventMarker_c");
        store_events(f, lib.markers, lib.version, "class dcEventMarker_c");
        store_events(f, lib.epochs, lib.version, "class dcEpochEvent_c");
        store_events(f, lib.artefacts, lib.version, "class dcArtefactEvent_c");
        store_events(f, lib.spikes, lib.version, "class dcSpikeEvent_c");
        store_events(f, lib.seizures, lib.version, "class dcSeizureEvent_c");
        store_events(f, lib.sleeps, lib.version, "class dcSleepEvent_c");
        store_events(f, lib.rpeaks, lib.version, "class dcRPeakEvent_c");
    }


    static
    auto read_abstract_data_item_library(FILE* f, event_library& lib) -> void {
        lib.name = read_archive_string(f);
    }


    static
    auto write_abstract_data_item_library(FILE* f, const event_library& lib) -> void {
        write_archive_string(f, lib.name);
    }


    static
    auto read_data_item_library(FILE* f, event_library& lib) -> void {
        read_abstract_data_item_library(f, lib);
        load_vector_of_pointers(f, lib);
    }



    static
    auto write_data_item_library(FILE* f, const event_library& lib) -> void {
        write_abstract_data_item_library(f, lib);
        store_vector_of_pointers(f, lib);
    }


    auto read_archive(FILE* f) -> event_library {
        event_library lib;
        read_archive_header(f, lib);

        int32_t class_tag;
        std::string class_name;
        read_class(f, class_tag, class_name);

        if (class_tag == tags::null) {
            return lib;
        }

        if (class_name != "class dcEventsLibrary_c") {
            throw api::v1::ctk_data{ "read_archive: not an events library" };
        }

        read_data_item_library(f, lib);
        return lib;
    }


    auto write_archive(FILE* f, const event_library& lib) -> void {
        write_archive_header(f,lib);
        write_class(f, tags::name, std::string("class dcEventsLibrary_c"));
        write_data_item_library(f, lib);
    }


    auto add_marker(marker_event marker, event_library& lib) -> void {
        const int32_t count{ event_count(lib, int32_t{}) };
        marker.common.visible_id = plus(count, 1, ok{});
        lib.markers.push_back(marker);
    }

    auto add_video(marker_event marker, event_library& lib) -> void {
        const int32_t count{ event_count(lib, int32_t{}) };
        marker.common.visible_id = plus(count, 1, ok{});
        lib.videos.push_back(marker);
    }

    auto add_impedance(marker_event marker, event_library& lib) -> void {
        const int32_t count{ event_count(lib, int32_t{}) };
        marker.common.visible_id = plus(count, 1, ok{});
        lib.impedances.push_back(marker);
    }

    auto add_epoch(epoch_event epoch, event_library& lib) -> void {
        const int32_t count{ event_count(lib, int32_t{}) };
        epoch.common.visible_id = plus(count, 1, ok{});
        lib.epochs.push_back(epoch);
    }

} /* namespace impl */ } /* namespace ctk */

