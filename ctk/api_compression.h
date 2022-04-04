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

// api_compression.h: generated from misc/compression.h.preface and misc/compression.h.content by cogapp (requires python)

#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>

namespace ctk { namespace api {
    namespace v1 {

        // NB: the encoded byte stream created by CompressReflib can be decoded ONLY by DecompressReflib
        struct CompressReflib
        {
            using value_type = int32_t;

            CompressReflib();
            CompressReflib(const CompressReflib&);
            CompressReflib(CompressReflib&&) = default;
            auto operator=(const CompressReflib&) -> CompressReflib&;
            auto operator=(CompressReflib&&) -> CompressReflib& = default;
            ~CompressReflib();

            auto Sensors(int64_t height) -> bool;
            auto Sensors(const std::vector<int16_t>& order) -> bool; // left for compatibility

            auto ColumnMajor(const std::vector<value_type>& matrix, int64_t length) -> std::vector<uint8_t>;
            auto RowMajor(const std::vector<value_type>& matrix, int64_t length) -> std::vector<uint8_t>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(CompressReflib&, CompressReflib&) -> void;
        };
        // returns NULL if an instance of CompressReflib can not be created instead of throwing an exception
        auto MakeCompressReflib() -> std::unique_ptr<CompressReflib>;

        struct DecompressReflib
        {
            using value_type = int32_t;

            DecompressReflib();
            DecompressReflib(const DecompressReflib&);
            DecompressReflib(DecompressReflib&&) = default;
            auto operator=(const DecompressReflib&) -> DecompressReflib&;
            auto operator=(DecompressReflib&&) -> DecompressReflib& = default;
            ~DecompressReflib();

            auto Sensors(int64_t height) -> bool;
            auto Sensors(const std::vector<int16_t>& order) -> bool; // left for compatibility

            auto ColumnMajor(const std::vector<uint8_t>& encoded, int64_t length) -> std::vector<value_type>;
            auto RowMajor(const std::vector<uint8_t>& encoded, int64_t length) -> std::vector<value_type>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(DecompressReflib&, DecompressReflib&) -> void;
        };
        // returns NULL if an instance of DecompressReflib can not be created instead of throwing an exception
        auto MakeDecompressReflib() -> std::unique_ptr<DecompressReflib>;



        // NB: the encoded byte stream created by CompressInt16 can be decoded ONLY by DecompressInt16
        struct CompressInt16
        {
            using value_type = int16_t;

            CompressInt16();
            CompressInt16(const CompressInt16&);
            CompressInt16(CompressInt16&&) = default;
            auto operator=(const CompressInt16&) -> CompressInt16&;
            auto operator=(CompressInt16&&) -> CompressInt16& = default;
            ~CompressInt16();

            auto Sensors(int64_t height) -> bool;
            auto Sensors(const std::vector<int16_t>& order) -> bool; // left for compatibility

            auto ColumnMajor(const std::vector<value_type>& matrix, int64_t length) -> std::vector<uint8_t>;
            auto RowMajor(const std::vector<value_type>& matrix, int64_t length) -> std::vector<uint8_t>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(CompressInt16&, CompressInt16&) -> void;
        };
        // returns NULL if an instance of CompressInt16 can not be created instead of throwing an exception
        auto MakeCompressInt16() -> std::unique_ptr<CompressInt16>;

        struct DecompressInt16
        {
            using value_type = int16_t;

            DecompressInt16();
            DecompressInt16(const DecompressInt16&);
            DecompressInt16(DecompressInt16&&) = default;
            auto operator=(const DecompressInt16&) -> DecompressInt16&;
            auto operator=(DecompressInt16&&) -> DecompressInt16& = default;
            ~DecompressInt16();

            auto Sensors(int64_t height) -> bool;
            auto Sensors(const std::vector<int16_t>& order) -> bool; // left for compatibility

            auto ColumnMajor(const std::vector<uint8_t>& encoded, int64_t length) -> std::vector<value_type>;
            auto RowMajor(const std::vector<uint8_t>& encoded, int64_t length) -> std::vector<value_type>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(DecompressInt16&, DecompressInt16&) -> void;
        };
        // returns NULL if an instance of DecompressInt16 can not be created instead of throwing an exception
        auto MakeDecompressInt16() -> std::unique_ptr<DecompressInt16>;



        // NB: the encoded byte stream created by CompressInt32 can be decoded ONLY by DecompressInt32
        struct CompressInt32
        {
            using value_type = int32_t;

            CompressInt32();
            CompressInt32(const CompressInt32&);
            CompressInt32(CompressInt32&&) = default;
            auto operator=(const CompressInt32&) -> CompressInt32&;
            auto operator=(CompressInt32&&) -> CompressInt32& = default;
            ~CompressInt32();

            auto Sensors(int64_t height) -> bool;
            auto Sensors(const std::vector<int16_t>& order) -> bool; // left for compatibility

            auto ColumnMajor(const std::vector<value_type>& matrix, int64_t length) -> std::vector<uint8_t>;
            auto RowMajor(const std::vector<value_type>& matrix, int64_t length) -> std::vector<uint8_t>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(CompressInt32&, CompressInt32&) -> void;
        };
        // returns NULL if an instance of CompressInt32 can not be created instead of throwing an exception
        auto MakeCompressInt32() -> std::unique_ptr<CompressInt32>;

        struct DecompressInt32
        {
            using value_type = int32_t;

            DecompressInt32();
            DecompressInt32(const DecompressInt32&);
            DecompressInt32(DecompressInt32&&) = default;
            auto operator=(const DecompressInt32&) -> DecompressInt32&;
            auto operator=(DecompressInt32&&) -> DecompressInt32& = default;
            ~DecompressInt32();

            auto Sensors(int64_t height) -> bool;
            auto Sensors(const std::vector<int16_t>& order) -> bool; // left for compatibility

            auto ColumnMajor(const std::vector<uint8_t>& encoded, int64_t length) -> std::vector<value_type>;
            auto RowMajor(const std::vector<uint8_t>& encoded, int64_t length) -> std::vector<value_type>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(DecompressInt32&, DecompressInt32&) -> void;
        };
        // returns NULL if an instance of DecompressInt32 can not be created instead of throwing an exception
        auto MakeDecompressInt32() -> std::unique_ptr<DecompressInt32>;



        // NB: the encoded byte stream created by CompressInt64 can be decoded ONLY by DecompressInt64
        struct CompressInt64
        {
            using value_type = int64_t;

            CompressInt64();
            CompressInt64(const CompressInt64&);
            CompressInt64(CompressInt64&&) = default;
            auto operator=(const CompressInt64&) -> CompressInt64&;
            auto operator=(CompressInt64&&) -> CompressInt64& = default;
            ~CompressInt64();

            auto Sensors(int64_t height) -> bool;
            auto Sensors(const std::vector<int16_t>& order) -> bool; // left for compatibility

            auto ColumnMajor(const std::vector<value_type>& matrix, int64_t length) -> std::vector<uint8_t>;
            auto RowMajor(const std::vector<value_type>& matrix, int64_t length) -> std::vector<uint8_t>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(CompressInt64&, CompressInt64&) -> void;
        };
        // returns NULL if an instance of CompressInt64 can not be created instead of throwing an exception
        auto MakeCompressInt64() -> std::unique_ptr<CompressInt64>;

        struct DecompressInt64
        {
            using value_type = int64_t;

            DecompressInt64();
            DecompressInt64(const DecompressInt64&);
            DecompressInt64(DecompressInt64&&) = default;
            auto operator=(const DecompressInt64&) -> DecompressInt64&;
            auto operator=(DecompressInt64&&) -> DecompressInt64& = default;
            ~DecompressInt64();

            auto Sensors(int64_t height) -> bool;
            auto Sensors(const std::vector<int16_t>& order) -> bool; // left for compatibility

            auto ColumnMajor(const std::vector<uint8_t>& encoded, int64_t length) -> std::vector<value_type>;
            auto RowMajor(const std::vector<uint8_t>& encoded, int64_t length) -> std::vector<value_type>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(DecompressInt64&, DecompressInt64&) -> void;
        };
        // returns NULL if an instance of DecompressInt64 can not be created instead of throwing an exception
        auto MakeDecompressInt64() -> std::unique_ptr<DecompressInt64>;



        // NB: the encoded byte stream created by CompressUInt16 can be decoded ONLY by DecompressUInt16
        struct CompressUInt16
        {
            using value_type = uint16_t;

            CompressUInt16();
            CompressUInt16(const CompressUInt16&);
            CompressUInt16(CompressUInt16&&) = default;
            auto operator=(const CompressUInt16&) -> CompressUInt16&;
            auto operator=(CompressUInt16&&) -> CompressUInt16& = default;
            ~CompressUInt16();

            auto Sensors(int64_t height) -> bool;
            auto Sensors(const std::vector<int16_t>& order) -> bool; // left for compatibility

            auto ColumnMajor(const std::vector<value_type>& matrix, int64_t length) -> std::vector<uint8_t>;
            auto RowMajor(const std::vector<value_type>& matrix, int64_t length) -> std::vector<uint8_t>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(CompressUInt16&, CompressUInt16&) -> void;
        };
        // returns NULL if an instance of CompressUInt16 can not be created instead of throwing an exception
        auto MakeCompressUInt16() -> std::unique_ptr<CompressUInt16>;

        struct DecompressUInt16
        {
            using value_type = uint16_t;

            DecompressUInt16();
            DecompressUInt16(const DecompressUInt16&);
            DecompressUInt16(DecompressUInt16&&) = default;
            auto operator=(const DecompressUInt16&) -> DecompressUInt16&;
            auto operator=(DecompressUInt16&&) -> DecompressUInt16& = default;
            ~DecompressUInt16();

            auto Sensors(int64_t height) -> bool;
            auto Sensors(const std::vector<int16_t>& order) -> bool; // left for compatibility

            auto ColumnMajor(const std::vector<uint8_t>& encoded, int64_t length) -> std::vector<value_type>;
            auto RowMajor(const std::vector<uint8_t>& encoded, int64_t length) -> std::vector<value_type>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(DecompressUInt16&, DecompressUInt16&) -> void;
        };
        // returns NULL if an instance of DecompressUInt16 can not be created instead of throwing an exception
        auto MakeDecompressUInt16() -> std::unique_ptr<DecompressUInt16>;



        // NB: the encoded byte stream created by CompressUInt32 can be decoded ONLY by DecompressUInt32
        struct CompressUInt32
        {
            using value_type = uint32_t;

            CompressUInt32();
            CompressUInt32(const CompressUInt32&);
            CompressUInt32(CompressUInt32&&) = default;
            auto operator=(const CompressUInt32&) -> CompressUInt32&;
            auto operator=(CompressUInt32&&) -> CompressUInt32& = default;
            ~CompressUInt32();

            auto Sensors(int64_t height) -> bool;
            auto Sensors(const std::vector<int16_t>& order) -> bool; // left for compatibility

            auto ColumnMajor(const std::vector<value_type>& matrix, int64_t length) -> std::vector<uint8_t>;
            auto RowMajor(const std::vector<value_type>& matrix, int64_t length) -> std::vector<uint8_t>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(CompressUInt32&, CompressUInt32&) -> void;
        };
        // returns NULL if an instance of CompressUInt32 can not be created instead of throwing an exception
        auto MakeCompressUInt32() -> std::unique_ptr<CompressUInt32>;

        struct DecompressUInt32
        {
            using value_type = uint32_t;

            DecompressUInt32();
            DecompressUInt32(const DecompressUInt32&);
            DecompressUInt32(DecompressUInt32&&) = default;
            auto operator=(const DecompressUInt32&) -> DecompressUInt32&;
            auto operator=(DecompressUInt32&&) -> DecompressUInt32& = default;
            ~DecompressUInt32();

            auto Sensors(int64_t height) -> bool;
            auto Sensors(const std::vector<int16_t>& order) -> bool; // left for compatibility

            auto ColumnMajor(const std::vector<uint8_t>& encoded, int64_t length) -> std::vector<value_type>;
            auto RowMajor(const std::vector<uint8_t>& encoded, int64_t length) -> std::vector<value_type>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(DecompressUInt32&, DecompressUInt32&) -> void;
        };
        // returns NULL if an instance of DecompressUInt32 can not be created instead of throwing an exception
        auto MakeDecompressUInt32() -> std::unique_ptr<DecompressUInt32>;



        // NB: the encoded byte stream created by CompressUInt64 can be decoded ONLY by DecompressUInt64
        struct CompressUInt64
        {
            using value_type = uint64_t;

            CompressUInt64();
            CompressUInt64(const CompressUInt64&);
            CompressUInt64(CompressUInt64&&) = default;
            auto operator=(const CompressUInt64&) -> CompressUInt64&;
            auto operator=(CompressUInt64&&) -> CompressUInt64& = default;
            ~CompressUInt64();

            auto Sensors(int64_t height) -> bool;
            auto Sensors(const std::vector<int16_t>& order) -> bool; // left for compatibility

            auto ColumnMajor(const std::vector<value_type>& matrix, int64_t length) -> std::vector<uint8_t>;
            auto RowMajor(const std::vector<value_type>& matrix, int64_t length) -> std::vector<uint8_t>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(CompressUInt64&, CompressUInt64&) -> void;
        };
        // returns NULL if an instance of CompressUInt64 can not be created instead of throwing an exception
        auto MakeCompressUInt64() -> std::unique_ptr<CompressUInt64>;

        struct DecompressUInt64
        {
            using value_type = uint64_t;

            DecompressUInt64();
            DecompressUInt64(const DecompressUInt64&);
            DecompressUInt64(DecompressUInt64&&) = default;
            auto operator=(const DecompressUInt64&) -> DecompressUInt64&;
            auto operator=(DecompressUInt64&&) -> DecompressUInt64& = default;
            ~DecompressUInt64();

            auto Sensors(int64_t height) -> bool;
            auto Sensors(const std::vector<int16_t>& order) -> bool; // left for compatibility

            auto ColumnMajor(const std::vector<uint8_t>& encoded, int64_t length) -> std::vector<value_type>;
            auto RowMajor(const std::vector<uint8_t>& encoded, int64_t length) -> std::vector<value_type>;

        private:
            struct impl;
            std::unique_ptr<impl> p;
            friend auto swap(DecompressUInt64&, DecompressUInt64&) -> void;
        };
        // returns NULL if an instance of DecompressUInt64 can not be created instead of throwing an exception
        auto MakeDecompressUInt64() -> std::unique_ptr<DecompressUInt64>;




    } /* namespace v1 */
} /* namespace api */ } /* namespace ctk */
