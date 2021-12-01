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

// api_compression.cc: generated from misc/compression.cc.preface and misc/compression.cc.content by cogapp (requires python)

#include "api_compression.h"
#include "compress/matrix.h"

#include <exception>
#include <algorithm>
#include <iostream>


namespace ctk { namespace api {

    namespace v1 {

        using namespace ctk::impl;

        auto acceptable_allocation_exception() -> void {
            try {
                throw;
            }
            // thrown by stl
            catch (const std::bad_alloc& e) {
                std::cerr << "exception: " << e.what() << "\n"; // internal library limitation. preventable by input size reduction.
                return;
            }
            catch (const std::length_error& e) {
                std::cerr << "exception: " << e.what() << "\n"; // internal library limitation. preventable by input size reduction.
                return;
            }
            // thrown by ctk
            catch (const ctk_limit& e) {
                std::cerr << "exception: " << e.what() << "\n"; // internal library limitation. preventable by input size reduction.
                return;
            }
            // not expected
            catch (const ctk_bug& e) {
                std::cerr << "unexpected exception: " << e.what() << ". aborting.\n"; // bug in this library
            }
            catch (std::exception& e) {
                std::cerr << "unexpected exception: " << e.what() << ". aborting.\n"; // bug in this library
            }

            abort();
        }

        auto acceptable_compression_exception() -> void {
            try {
                throw;
            }
            // thrown by stl
            catch (const std::bad_alloc& e) {
                std::cerr << "exception: "<< e.what() << "\n";
                return;
            }
            catch (const std::length_error& e) {
                std::cerr << "exception: "<< e.what() << "\n";
                return;
            }
            catch (const std::ios_base::failure& e) {
                std::cerr << "exception: "<< e.what() << "\n";
                return;
            }
            // thrown by ctk
            catch (const ctk_data& e) {
                std::cerr << "excepion " << e.what() << "\n"; // garbage data block
                return;
            }
            catch (const ctk_limit& e) {
                std::cerr << "exception: "<< e.what() << "\n"; // preventable user error
                return;
            }
            // not expected
            catch (const ctk_bug& e) {
                std::cerr << "unexpected exception: "<< e.what() << ". aborting.\n"; // bug in this library
            }
            catch (std::exception& e) {
                std::cerr << "unexpected exception: " << e.what() << ". aborting.\n"; // bug in this library
            }

            abort();
        }

        template<typename MatrixEncoder>
        // MatrixEncoder has an interface compatible with matrix_encoder_reflib or matrix_encoder<T> from compress/matrix.h
        struct compress_wrapper
        {
            using T = typename MatrixEncoder::value_type;

            MatrixEncoder encoder;
            std::vector<uint8_t> empty_bytes;

            compress_wrapper() = default;
            compress_wrapper(const compress_wrapper&) = default;
            compress_wrapper(compress_wrapper&&) = default;
            auto operator=(const compress_wrapper&) -> compress_wrapper& = default;
            auto operator=(compress_wrapper&&) -> compress_wrapper& = default;
            ~compress_wrapper() = default;

            auto sensors(int64_t height) -> bool {
                return encoder.row_count(sensor_count{ height });
            }

            auto sensors(int64_t height, std::nothrow_t) -> bool {
                try {
                    return sensors(height);
                }
                catch(const std::exception&) {
                    acceptable_allocation_exception(); 
                }

                return false;
            }

            auto sensors(const std::vector<int16_t>& order) -> bool {
                return encoder.row_order(order);
            }

            auto sensors(const std::vector<int16_t>& order, std::nothrow_t) -> bool {
                try {
                    return sensors(order);
                }
                catch(const std::exception&) {
                    acceptable_allocation_exception(); 
                }

                return false;
            }

            auto reserve(int64_t length) -> void {
                return encoder.reserve(measurement_count{ length });
            }

            auto reserve(int64_t length, std::nothrow_t) -> bool {
                try {
                    reserve(length);
                }
                catch(const std::exception&) {
                    acceptable_allocation_exception(); 
                    return false;
                }

                return true;
            }

            auto column_major(const std::vector<T>& input, int64_t length) -> std::vector<uint8_t> {
                constexpr const column_major2row_major transpose{};
                return encoder(input, measurement_count{ length }, transpose);
            }

            auto column_major(const std::vector<T>& input, int64_t length, std::nothrow_t) -> std::vector<uint8_t> {
                try {
                    return column_major(input, length);
                }
                catch (const std::exception&) {
                    acceptable_compression_exception();
                }

                return empty_bytes;
            }

            auto row_major(const std::vector<T>& input, int64_t length) -> std::vector<uint8_t> {
                constexpr const row_major2row_major copy{};
                return encoder(input, measurement_count{ length }, copy);
            }

            auto row_major(const std::vector<T>& input, int64_t length, std::nothrow_t) -> std::vector<uint8_t> {
                try {
                    return row_major(input, length);
                }
                catch (const std::exception&) {
                    acceptable_compression_exception();
                }

                return empty_bytes;
            }
        };


        template<typename MatrixDecoder>
        // MatrixDecoder has an interface compatible with matrix_decoder_reflib or matrix_decoder<T> from compress/matrix.h
        struct decompress_wrapper
        {
            using T = typename MatrixDecoder::value_type;

            MatrixDecoder decoder;
            std::vector<T> empty_words;

            decompress_wrapper() = default;
            decompress_wrapper(const decompress_wrapper&) = default;
            decompress_wrapper(decompress_wrapper&&) = default;
            auto operator=(const decompress_wrapper&) -> decompress_wrapper& = default;
            auto operator=(decompress_wrapper&&) -> decompress_wrapper& = default;
            ~decompress_wrapper() = default;

            auto sensors(int64_t height) -> bool {
                return decoder.row_count(sensor_count{ height });
            }

            auto sensors(int64_t height, std::nothrow_t) -> bool {
                try {
                    return sensors(height);
                }
                catch(const std::exception&) {
                    acceptable_allocation_exception(); 
                }

                return false;
            }

            auto sensors(const std::vector<int16_t>& order) -> bool {
                return decoder.row_order(order);
            }

            auto sensors(const std::vector<int16_t>& order, std::nothrow_t) -> bool {
                try {
                    return sensors(order);
                }
                catch(const std::exception&) {
                    acceptable_allocation_exception(); 
                }

                return false;
            }

            auto reserve(int64_t length) -> void {
                return decoder.reserve(measurement_count{ length });
            }

            auto reserve(int64_t length, std::nothrow_t) -> bool {
                try {
                    reserve(length);
                }
                catch(const std::exception&) {
                    acceptable_allocation_exception(); 
                    return false;
                }

                return true;
            }

            auto column_major(const std::vector<uint8_t>& input, int64_t length) -> std::vector<T> {
                constexpr const column_major2row_major transpose{};
                return decoder(input, measurement_count{ length }, transpose);
            }

            auto column_major(const std::vector<uint8_t>& input, int64_t length, std::nothrow_t) -> std::vector<T> {
                try {
                    return column_major(input, length);
                }
                catch (const std::exception&) {
                    acceptable_compression_exception();
                }

                return empty_words;
            }

            auto row_major(const std::vector<uint8_t>& input, int64_t length) -> std::vector<T> {
                constexpr const row_major2row_major copy{};
                return decoder(input, measurement_count{ length }, copy);
            }

            auto row_major(const std::vector<uint8_t>& input, int64_t length, std::nothrow_t) -> std::vector<T> {
                try {
                    return row_major(input, length);
                }
                catch (const std::exception&) {
                    acceptable_compression_exception();
                }

                return empty_words;
            }
        };



        struct CompressReflib::impl
        {
            compress_wrapper<ctk::matrix_encoder_reflib> encode;

            impl() = default;
            impl(const impl&) = default;
            impl(impl&&) = default;
            auto operator=(const impl&) -> impl& = default;
            auto operator=(impl&&) -> impl& = default;
            ~impl() = default;
        };

        CompressReflib::CompressReflib()
            : p{ new impl{} } {
            assert(p);
        }

        CompressReflib::~CompressReflib() {
        }

        CompressReflib::CompressReflib(const CompressReflib& x)
            : p{ new impl{ *x.p } } {
            assert(x.p);
            assert(p);
        }

        auto swap(CompressReflib& x, CompressReflib& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        auto CompressReflib::operator=(const CompressReflib& x) -> CompressReflib& {
            CompressReflib y{ x };
            swap(*this, y);
            return *this;
        }

        auto CompressReflib::sensors(int64_t height) -> bool {
            assert(p);
            return p->encode.sensors(height);
        }

        auto CompressReflib::sensors(int64_t height, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.sensors(height, nothrow);
        }

        auto CompressReflib::sensors(const std::vector<int16_t>& order) -> bool {
            assert(p);
            return p->encode.sensors(order);
        }

        auto CompressReflib::sensors(const std::vector<int16_t>& order, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.sensors(order, nothrow);
        }

        auto CompressReflib::reserve(int64_t length) -> void {
            assert(p);
            p->encode.reserve(length);
        }

        auto CompressReflib::reserve(int64_t length, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.reserve(length, nothrow);
        }

        auto CompressReflib::column_major(const std::vector<int32_t>& matrix, int64_t length) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.column_major(matrix, length);
        }

        auto CompressReflib::column_major(const std::vector<int32_t>& matrix, int64_t length, std::nothrow_t nothrow) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.column_major(matrix, length, nothrow);
        }

        auto CompressReflib::row_major(const std::vector<int32_t>& matrix, int64_t length) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.row_major(matrix, length);
        }

        auto CompressReflib::row_major(const std::vector<int32_t>& matrix, int64_t length, std::nothrow_t nothrow) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.row_major(matrix, length, nothrow);
        }

        auto MakeCompressReflib() -> std::unique_ptr<CompressReflib> {
            try {
                return std::make_unique<CompressReflib>();
            }
            catch(const std::exception&) {
                return nullptr;
            }
        }


        struct DecompressReflib::impl
        {
            decompress_wrapper<ctk::matrix_decoder_reflib> decode;

            impl() = default;
            impl(const impl&) = default;
            impl(impl&&) = default;
            auto operator=(const impl&) -> impl& = default;
            auto operator=(impl&&) -> impl& = default;
            ~impl() = default;
        };

        DecompressReflib::DecompressReflib()
            : p{ new impl{} } {
            assert(p);
        }

        DecompressReflib::~DecompressReflib() {
        }

        DecompressReflib::DecompressReflib(const DecompressReflib& x)
            : p{ new impl{ *x.p } } {
            assert(x.p);
            assert(p);
        }

        auto swap(DecompressReflib& x, DecompressReflib& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        auto DecompressReflib::operator=(const DecompressReflib& x) -> DecompressReflib& {
            DecompressReflib y{ x };
            swap(*this, y);
            return *this;
        }

        auto DecompressReflib::sensors(int64_t height) -> bool {
            assert(p);
            return p->decode.sensors(height);
        }

        auto DecompressReflib::sensors(int64_t height, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.sensors(height, nothrow);
        }

        auto DecompressReflib::sensors(const std::vector<int16_t>& order) -> bool {
            assert(p);
            return p->decode.sensors(order);
        }

        auto DecompressReflib::sensors(const std::vector<int16_t>& order, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.sensors(order, nothrow);
        }

        auto DecompressReflib::reserve(int64_t length) -> void {
            assert(p);
            p->decode.reserve(length);
        }

        auto DecompressReflib::reserve(int64_t length, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.reserve(length, nothrow);
        }

        auto DecompressReflib::column_major(const std::vector<uint8_t>& compressed, int64_t length) -> std::vector<int32_t> {
            assert(p);
            return p->decode.column_major(compressed, length);
        }

        auto DecompressReflib::column_major(const std::vector<uint8_t>& compressed, int64_t length, std::nothrow_t nothrow) -> std::vector<int32_t> {
            assert(p);
            return p->decode.column_major(compressed, length, nothrow);
        }

        auto DecompressReflib::row_major(const std::vector<uint8_t>& compressed, int64_t length) -> std::vector<int32_t> {
            assert(p);
            return p->decode.row_major(compressed, length);
        }

        auto DecompressReflib::row_major(const std::vector<uint8_t>& compressed, int64_t length, std::nothrow_t nothrow) -> std::vector<int32_t> {
            assert(p);
            return p->decode.row_major(compressed, length, nothrow);
        }

        auto MakeDecompressReflib() -> std::unique_ptr<DecompressReflib> {
            try {
                return std::make_unique<DecompressReflib>();
            }
            catch(const std::exception&) {
                return nullptr;
            }
        }

        // ---




        struct CompressInt16::impl
        {
            compress_wrapper<ctk::matrix_encoder<int16_t>> encode;

            impl() = default;
            impl(const impl&) = default;
            impl(impl&&) = default;
            auto operator=(const impl&) -> impl& = default;
            auto operator=(impl&&) -> impl& = default;
            ~impl() = default;
        };

        CompressInt16::CompressInt16()
            : p{ new impl{} } {
            assert(p);
        }

        CompressInt16::~CompressInt16() {
        }

        CompressInt16::CompressInt16(const CompressInt16& x)
            : p{ new impl{ *x.p } } {
            assert(x.p);
            assert(p);
        }

        auto swap(CompressInt16& x, CompressInt16& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        auto CompressInt16::operator=(const CompressInt16& x) -> CompressInt16& {
            CompressInt16 y{ x };
            swap(*this, y);
            return *this;
        }

        auto CompressInt16::sensors(int64_t height) -> bool {
            assert(p);
            return p->encode.sensors(height);
        }

        auto CompressInt16::sensors(int64_t height, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.sensors(height, nothrow);
        }

        auto CompressInt16::sensors(const std::vector<int16_t>& order) -> bool {
            assert(p);
            return p->encode.sensors(order);
        }

        auto CompressInt16::sensors(const std::vector<int16_t>& order, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.sensors(order, nothrow);
        }

        auto CompressInt16::reserve(int64_t length) -> void {
            assert(p);
            p->encode.reserve(length);
        }

        auto CompressInt16::reserve(int64_t length, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.reserve(length, nothrow);
        }

        auto CompressInt16::column_major(const std::vector<int16_t>& matrix, int64_t length) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.column_major(matrix, length);
        }

        auto CompressInt16::column_major(const std::vector<int16_t>& matrix, int64_t length, std::nothrow_t nothrow) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.column_major(matrix, length, nothrow);
        }

        auto CompressInt16::row_major(const std::vector<int16_t>& matrix, int64_t length) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.row_major(matrix, length);
        }

        auto CompressInt16::row_major(const std::vector<int16_t>& matrix, int64_t length, std::nothrow_t nothrow) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.row_major(matrix, length, nothrow);
        }

        auto MakeCompressInt16() -> std::unique_ptr<CompressInt16> {
            try {
                return std::make_unique<CompressInt16>();
            }
            catch(const std::exception&) {
                return nullptr;
            }
        }


        struct DecompressInt16::impl
        {
            decompress_wrapper<ctk::matrix_decoder<int16_t>> decode;

            impl() = default;
            impl(const impl&) = default;
            impl(impl&&) = default;
            auto operator=(const impl&) -> impl& = default;
            auto operator=(impl&&) -> impl& = default;
            ~impl() = default;
        };

        DecompressInt16::DecompressInt16()
            : p{ new impl{} } {
            assert(p);
        }

        DecompressInt16::~DecompressInt16() {
        }

        DecompressInt16::DecompressInt16(const DecompressInt16& x)
            : p{ new impl{ *x.p } } {
            assert(x.p);
            assert(p);
        }

        auto swap(DecompressInt16& x, DecompressInt16& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        auto DecompressInt16::operator=(const DecompressInt16& x) -> DecompressInt16& {
            DecompressInt16 y{ x };
            swap(*this, y);
            return *this;
        }

        auto DecompressInt16::sensors(int64_t height) -> bool {
            assert(p);
            return p->decode.sensors(height);
        }

        auto DecompressInt16::sensors(int64_t height, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.sensors(height, nothrow);
        }

        auto DecompressInt16::sensors(const std::vector<int16_t>& order) -> bool {
            assert(p);
            return p->decode.sensors(order);
        }

        auto DecompressInt16::sensors(const std::vector<int16_t>& order, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.sensors(order, nothrow);
        }

        auto DecompressInt16::reserve(int64_t length) -> void {
            assert(p);
            p->decode.reserve(length);
        }

        auto DecompressInt16::reserve(int64_t length, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.reserve(length, nothrow);
        }

        auto DecompressInt16::column_major(const std::vector<uint8_t>& compressed, int64_t length) -> std::vector<int16_t> {
            assert(p);
            return p->decode.column_major(compressed, length);
        }

        auto DecompressInt16::column_major(const std::vector<uint8_t>& compressed, int64_t length, std::nothrow_t nothrow) -> std::vector<int16_t> {
            assert(p);
            return p->decode.column_major(compressed, length, nothrow);
        }

        auto DecompressInt16::row_major(const std::vector<uint8_t>& compressed, int64_t length) -> std::vector<int16_t> {
            assert(p);
            return p->decode.row_major(compressed, length);
        }

        auto DecompressInt16::row_major(const std::vector<uint8_t>& compressed, int64_t length, std::nothrow_t nothrow) -> std::vector<int16_t> {
            assert(p);
            return p->decode.row_major(compressed, length, nothrow);
        }

        auto MakeDecompressInt16() -> std::unique_ptr<DecompressInt16> {
            try {
                return std::make_unique<DecompressInt16>();
            }
            catch(const std::exception&) {
                return nullptr;
            }
        }

        // ---




        struct CompressInt32::impl
        {
            compress_wrapper<ctk::matrix_encoder<int32_t>> encode;

            impl() = default;
            impl(const impl&) = default;
            impl(impl&&) = default;
            auto operator=(const impl&) -> impl& = default;
            auto operator=(impl&&) -> impl& = default;
            ~impl() = default;
        };

        CompressInt32::CompressInt32()
            : p{ new impl{} } {
            assert(p);
        }

        CompressInt32::~CompressInt32() {
        }

        CompressInt32::CompressInt32(const CompressInt32& x)
            : p{ new impl{ *x.p } } {
            assert(x.p);
            assert(p);
        }

        auto swap(CompressInt32& x, CompressInt32& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        auto CompressInt32::operator=(const CompressInt32& x) -> CompressInt32& {
            CompressInt32 y{ x };
            swap(*this, y);
            return *this;
        }

        auto CompressInt32::sensors(int64_t height) -> bool {
            assert(p);
            return p->encode.sensors(height);
        }

        auto CompressInt32::sensors(int64_t height, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.sensors(height, nothrow);
        }

        auto CompressInt32::sensors(const std::vector<int16_t>& order) -> bool {
            assert(p);
            return p->encode.sensors(order);
        }

        auto CompressInt32::sensors(const std::vector<int16_t>& order, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.sensors(order, nothrow);
        }

        auto CompressInt32::reserve(int64_t length) -> void {
            assert(p);
            p->encode.reserve(length);
        }

        auto CompressInt32::reserve(int64_t length, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.reserve(length, nothrow);
        }

        auto CompressInt32::column_major(const std::vector<int32_t>& matrix, int64_t length) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.column_major(matrix, length);
        }

        auto CompressInt32::column_major(const std::vector<int32_t>& matrix, int64_t length, std::nothrow_t nothrow) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.column_major(matrix, length, nothrow);
        }

        auto CompressInt32::row_major(const std::vector<int32_t>& matrix, int64_t length) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.row_major(matrix, length);
        }

        auto CompressInt32::row_major(const std::vector<int32_t>& matrix, int64_t length, std::nothrow_t nothrow) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.row_major(matrix, length, nothrow);
        }

        auto MakeCompressInt32() -> std::unique_ptr<CompressInt32> {
            try {
                return std::make_unique<CompressInt32>();
            }
            catch(const std::exception&) {
                return nullptr;
            }
        }


        struct DecompressInt32::impl
        {
            decompress_wrapper<ctk::matrix_decoder<int32_t>> decode;

            impl() = default;
            impl(const impl&) = default;
            impl(impl&&) = default;
            auto operator=(const impl&) -> impl& = default;
            auto operator=(impl&&) -> impl& = default;
            ~impl() = default;
        };

        DecompressInt32::DecompressInt32()
            : p{ new impl{} } {
            assert(p);
        }

        DecompressInt32::~DecompressInt32() {
        }

        DecompressInt32::DecompressInt32(const DecompressInt32& x)
            : p{ new impl{ *x.p } } {
            assert(x.p);
            assert(p);
        }

        auto swap(DecompressInt32& x, DecompressInt32& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        auto DecompressInt32::operator=(const DecompressInt32& x) -> DecompressInt32& {
            DecompressInt32 y{ x };
            swap(*this, y);
            return *this;
        }

        auto DecompressInt32::sensors(int64_t height) -> bool {
            assert(p);
            return p->decode.sensors(height);
        }

        auto DecompressInt32::sensors(int64_t height, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.sensors(height, nothrow);
        }

        auto DecompressInt32::sensors(const std::vector<int16_t>& order) -> bool {
            assert(p);
            return p->decode.sensors(order);
        }

        auto DecompressInt32::sensors(const std::vector<int16_t>& order, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.sensors(order, nothrow);
        }

        auto DecompressInt32::reserve(int64_t length) -> void {
            assert(p);
            p->decode.reserve(length);
        }

        auto DecompressInt32::reserve(int64_t length, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.reserve(length, nothrow);
        }

        auto DecompressInt32::column_major(const std::vector<uint8_t>& compressed, int64_t length) -> std::vector<int32_t> {
            assert(p);
            return p->decode.column_major(compressed, length);
        }

        auto DecompressInt32::column_major(const std::vector<uint8_t>& compressed, int64_t length, std::nothrow_t nothrow) -> std::vector<int32_t> {
            assert(p);
            return p->decode.column_major(compressed, length, nothrow);
        }

        auto DecompressInt32::row_major(const std::vector<uint8_t>& compressed, int64_t length) -> std::vector<int32_t> {
            assert(p);
            return p->decode.row_major(compressed, length);
        }

        auto DecompressInt32::row_major(const std::vector<uint8_t>& compressed, int64_t length, std::nothrow_t nothrow) -> std::vector<int32_t> {
            assert(p);
            return p->decode.row_major(compressed, length, nothrow);
        }

        auto MakeDecompressInt32() -> std::unique_ptr<DecompressInt32> {
            try {
                return std::make_unique<DecompressInt32>();
            }
            catch(const std::exception&) {
                return nullptr;
            }
        }

        // ---




        struct CompressInt64::impl
        {
            compress_wrapper<ctk::matrix_encoder<int64_t>> encode;

            impl() = default;
            impl(const impl&) = default;
            impl(impl&&) = default;
            auto operator=(const impl&) -> impl& = default;
            auto operator=(impl&&) -> impl& = default;
            ~impl() = default;
        };

        CompressInt64::CompressInt64()
            : p{ new impl{} } {
            assert(p);
        }

        CompressInt64::~CompressInt64() {
        }

        CompressInt64::CompressInt64(const CompressInt64& x)
            : p{ new impl{ *x.p } } {
            assert(x.p);
            assert(p);
        }

        auto swap(CompressInt64& x, CompressInt64& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        auto CompressInt64::operator=(const CompressInt64& x) -> CompressInt64& {
            CompressInt64 y{ x };
            swap(*this, y);
            return *this;
        }

        auto CompressInt64::sensors(int64_t height) -> bool {
            assert(p);
            return p->encode.sensors(height);
        }

        auto CompressInt64::sensors(int64_t height, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.sensors(height, nothrow);
        }

        auto CompressInt64::sensors(const std::vector<int16_t>& order) -> bool {
            assert(p);
            return p->encode.sensors(order);
        }

        auto CompressInt64::sensors(const std::vector<int16_t>& order, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.sensors(order, nothrow);
        }

        auto CompressInt64::reserve(int64_t length) -> void {
            assert(p);
            p->encode.reserve(length);
        }

        auto CompressInt64::reserve(int64_t length, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.reserve(length, nothrow);
        }

        auto CompressInt64::column_major(const std::vector<int64_t>& matrix, int64_t length) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.column_major(matrix, length);
        }

        auto CompressInt64::column_major(const std::vector<int64_t>& matrix, int64_t length, std::nothrow_t nothrow) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.column_major(matrix, length, nothrow);
        }

        auto CompressInt64::row_major(const std::vector<int64_t>& matrix, int64_t length) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.row_major(matrix, length);
        }

        auto CompressInt64::row_major(const std::vector<int64_t>& matrix, int64_t length, std::nothrow_t nothrow) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.row_major(matrix, length, nothrow);
        }

        auto MakeCompressInt64() -> std::unique_ptr<CompressInt64> {
            try {
                return std::make_unique<CompressInt64>();
            }
            catch(const std::exception&) {
                return nullptr;
            }
        }


        struct DecompressInt64::impl
        {
            decompress_wrapper<ctk::matrix_decoder<int64_t>> decode;

            impl() = default;
            impl(const impl&) = default;
            impl(impl&&) = default;
            auto operator=(const impl&) -> impl& = default;
            auto operator=(impl&&) -> impl& = default;
            ~impl() = default;
        };

        DecompressInt64::DecompressInt64()
            : p{ new impl{} } {
            assert(p);
        }

        DecompressInt64::~DecompressInt64() {
        }

        DecompressInt64::DecompressInt64(const DecompressInt64& x)
            : p{ new impl{ *x.p } } {
            assert(x.p);
            assert(p);
        }

        auto swap(DecompressInt64& x, DecompressInt64& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        auto DecompressInt64::operator=(const DecompressInt64& x) -> DecompressInt64& {
            DecompressInt64 y{ x };
            swap(*this, y);
            return *this;
        }

        auto DecompressInt64::sensors(int64_t height) -> bool {
            assert(p);
            return p->decode.sensors(height);
        }

        auto DecompressInt64::sensors(int64_t height, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.sensors(height, nothrow);
        }

        auto DecompressInt64::sensors(const std::vector<int16_t>& order) -> bool {
            assert(p);
            return p->decode.sensors(order);
        }

        auto DecompressInt64::sensors(const std::vector<int16_t>& order, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.sensors(order, nothrow);
        }

        auto DecompressInt64::reserve(int64_t length) -> void {
            assert(p);
            p->decode.reserve(length);
        }

        auto DecompressInt64::reserve(int64_t length, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.reserve(length, nothrow);
        }

        auto DecompressInt64::column_major(const std::vector<uint8_t>& compressed, int64_t length) -> std::vector<int64_t> {
            assert(p);
            return p->decode.column_major(compressed, length);
        }

        auto DecompressInt64::column_major(const std::vector<uint8_t>& compressed, int64_t length, std::nothrow_t nothrow) -> std::vector<int64_t> {
            assert(p);
            return p->decode.column_major(compressed, length, nothrow);
        }

        auto DecompressInt64::row_major(const std::vector<uint8_t>& compressed, int64_t length) -> std::vector<int64_t> {
            assert(p);
            return p->decode.row_major(compressed, length);
        }

        auto DecompressInt64::row_major(const std::vector<uint8_t>& compressed, int64_t length, std::nothrow_t nothrow) -> std::vector<int64_t> {
            assert(p);
            return p->decode.row_major(compressed, length, nothrow);
        }

        auto MakeDecompressInt64() -> std::unique_ptr<DecompressInt64> {
            try {
                return std::make_unique<DecompressInt64>();
            }
            catch(const std::exception&) {
                return nullptr;
            }
        }

        // ---




        struct CompressUInt16::impl
        {
            compress_wrapper<ctk::matrix_encoder<uint16_t>> encode;

            impl() = default;
            impl(const impl&) = default;
            impl(impl&&) = default;
            auto operator=(const impl&) -> impl& = default;
            auto operator=(impl&&) -> impl& = default;
            ~impl() = default;
        };

        CompressUInt16::CompressUInt16()
            : p{ new impl{} } {
            assert(p);
        }

        CompressUInt16::~CompressUInt16() {
        }

        CompressUInt16::CompressUInt16(const CompressUInt16& x)
            : p{ new impl{ *x.p } } {
            assert(x.p);
            assert(p);
        }

        auto swap(CompressUInt16& x, CompressUInt16& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        auto CompressUInt16::operator=(const CompressUInt16& x) -> CompressUInt16& {
            CompressUInt16 y{ x };
            swap(*this, y);
            return *this;
        }

        auto CompressUInt16::sensors(int64_t height) -> bool {
            assert(p);
            return p->encode.sensors(height);
        }

        auto CompressUInt16::sensors(int64_t height, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.sensors(height, nothrow);
        }

        auto CompressUInt16::sensors(const std::vector<int16_t>& order) -> bool {
            assert(p);
            return p->encode.sensors(order);
        }

        auto CompressUInt16::sensors(const std::vector<int16_t>& order, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.sensors(order, nothrow);
        }

        auto CompressUInt16::reserve(int64_t length) -> void {
            assert(p);
            p->encode.reserve(length);
        }

        auto CompressUInt16::reserve(int64_t length, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.reserve(length, nothrow);
        }

        auto CompressUInt16::column_major(const std::vector<uint16_t>& matrix, int64_t length) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.column_major(matrix, length);
        }

        auto CompressUInt16::column_major(const std::vector<uint16_t>& matrix, int64_t length, std::nothrow_t nothrow) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.column_major(matrix, length, nothrow);
        }

        auto CompressUInt16::row_major(const std::vector<uint16_t>& matrix, int64_t length) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.row_major(matrix, length);
        }

        auto CompressUInt16::row_major(const std::vector<uint16_t>& matrix, int64_t length, std::nothrow_t nothrow) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.row_major(matrix, length, nothrow);
        }

        auto MakeCompressUInt16() -> std::unique_ptr<CompressUInt16> {
            try {
                return std::make_unique<CompressUInt16>();
            }
            catch(const std::exception&) {
                return nullptr;
            }
        }


        struct DecompressUInt16::impl
        {
            decompress_wrapper<ctk::matrix_decoder<uint16_t>> decode;

            impl() = default;
            impl(const impl&) = default;
            impl(impl&&) = default;
            auto operator=(const impl&) -> impl& = default;
            auto operator=(impl&&) -> impl& = default;
            ~impl() = default;
        };

        DecompressUInt16::DecompressUInt16()
            : p{ new impl{} } {
            assert(p);
        }

        DecompressUInt16::~DecompressUInt16() {
        }

        DecompressUInt16::DecompressUInt16(const DecompressUInt16& x)
            : p{ new impl{ *x.p } } {
            assert(x.p);
            assert(p);
        }

        auto swap(DecompressUInt16& x, DecompressUInt16& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        auto DecompressUInt16::operator=(const DecompressUInt16& x) -> DecompressUInt16& {
            DecompressUInt16 y{ x };
            swap(*this, y);
            return *this;
        }

        auto DecompressUInt16::sensors(int64_t height) -> bool {
            assert(p);
            return p->decode.sensors(height);
        }

        auto DecompressUInt16::sensors(int64_t height, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.sensors(height, nothrow);
        }

        auto DecompressUInt16::sensors(const std::vector<int16_t>& order) -> bool {
            assert(p);
            return p->decode.sensors(order);
        }

        auto DecompressUInt16::sensors(const std::vector<int16_t>& order, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.sensors(order, nothrow);
        }

        auto DecompressUInt16::reserve(int64_t length) -> void {
            assert(p);
            p->decode.reserve(length);
        }

        auto DecompressUInt16::reserve(int64_t length, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.reserve(length, nothrow);
        }

        auto DecompressUInt16::column_major(const std::vector<uint8_t>& compressed, int64_t length) -> std::vector<uint16_t> {
            assert(p);
            return p->decode.column_major(compressed, length);
        }

        auto DecompressUInt16::column_major(const std::vector<uint8_t>& compressed, int64_t length, std::nothrow_t nothrow) -> std::vector<uint16_t> {
            assert(p);
            return p->decode.column_major(compressed, length, nothrow);
        }

        auto DecompressUInt16::row_major(const std::vector<uint8_t>& compressed, int64_t length) -> std::vector<uint16_t> {
            assert(p);
            return p->decode.row_major(compressed, length);
        }

        auto DecompressUInt16::row_major(const std::vector<uint8_t>& compressed, int64_t length, std::nothrow_t nothrow) -> std::vector<uint16_t> {
            assert(p);
            return p->decode.row_major(compressed, length, nothrow);
        }

        auto MakeDecompressUInt16() -> std::unique_ptr<DecompressUInt16> {
            try {
                return std::make_unique<DecompressUInt16>();
            }
            catch(const std::exception&) {
                return nullptr;
            }
        }

        // ---




        struct CompressUInt32::impl
        {
            compress_wrapper<ctk::matrix_encoder<uint32_t>> encode;

            impl() = default;
            impl(const impl&) = default;
            impl(impl&&) = default;
            auto operator=(const impl&) -> impl& = default;
            auto operator=(impl&&) -> impl& = default;
            ~impl() = default;
        };

        CompressUInt32::CompressUInt32()
            : p{ new impl{} } {
            assert(p);
        }

        CompressUInt32::~CompressUInt32() {
        }

        CompressUInt32::CompressUInt32(const CompressUInt32& x)
            : p{ new impl{ *x.p } } {
            assert(x.p);
            assert(p);
        }

        auto swap(CompressUInt32& x, CompressUInt32& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        auto CompressUInt32::operator=(const CompressUInt32& x) -> CompressUInt32& {
            CompressUInt32 y{ x };
            swap(*this, y);
            return *this;
        }

        auto CompressUInt32::sensors(int64_t height) -> bool {
            assert(p);
            return p->encode.sensors(height);
        }

        auto CompressUInt32::sensors(int64_t height, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.sensors(height, nothrow);
        }

        auto CompressUInt32::sensors(const std::vector<int16_t>& order) -> bool {
            assert(p);
            return p->encode.sensors(order);
        }

        auto CompressUInt32::sensors(const std::vector<int16_t>& order, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.sensors(order, nothrow);
        }

        auto CompressUInt32::reserve(int64_t length) -> void {
            assert(p);
            p->encode.reserve(length);
        }

        auto CompressUInt32::reserve(int64_t length, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.reserve(length, nothrow);
        }

        auto CompressUInt32::column_major(const std::vector<uint32_t>& matrix, int64_t length) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.column_major(matrix, length);
        }

        auto CompressUInt32::column_major(const std::vector<uint32_t>& matrix, int64_t length, std::nothrow_t nothrow) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.column_major(matrix, length, nothrow);
        }

        auto CompressUInt32::row_major(const std::vector<uint32_t>& matrix, int64_t length) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.row_major(matrix, length);
        }

        auto CompressUInt32::row_major(const std::vector<uint32_t>& matrix, int64_t length, std::nothrow_t nothrow) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.row_major(matrix, length, nothrow);
        }

        auto MakeCompressUInt32() -> std::unique_ptr<CompressUInt32> {
            try {
                return std::make_unique<CompressUInt32>();
            }
            catch(const std::exception&) {
                return nullptr;
            }
        }


        struct DecompressUInt32::impl
        {
            decompress_wrapper<ctk::matrix_decoder<uint32_t>> decode;

            impl() = default;
            impl(const impl&) = default;
            impl(impl&&) = default;
            auto operator=(const impl&) -> impl& = default;
            auto operator=(impl&&) -> impl& = default;
            ~impl() = default;
        };

        DecompressUInt32::DecompressUInt32()
            : p{ new impl{} } {
            assert(p);
        }

        DecompressUInt32::~DecompressUInt32() {
        }

        DecompressUInt32::DecompressUInt32(const DecompressUInt32& x)
            : p{ new impl{ *x.p } } {
            assert(x.p);
            assert(p);
        }

        auto swap(DecompressUInt32& x, DecompressUInt32& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        auto DecompressUInt32::operator=(const DecompressUInt32& x) -> DecompressUInt32& {
            DecompressUInt32 y{ x };
            swap(*this, y);
            return *this;
        }

        auto DecompressUInt32::sensors(int64_t height) -> bool {
            assert(p);
            return p->decode.sensors(height);
        }

        auto DecompressUInt32::sensors(int64_t height, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.sensors(height, nothrow);
        }

        auto DecompressUInt32::sensors(const std::vector<int16_t>& order) -> bool {
            assert(p);
            return p->decode.sensors(order);
        }

        auto DecompressUInt32::sensors(const std::vector<int16_t>& order, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.sensors(order, nothrow);
        }

        auto DecompressUInt32::reserve(int64_t length) -> void {
            assert(p);
            p->decode.reserve(length);
        }

        auto DecompressUInt32::reserve(int64_t length, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.reserve(length, nothrow);
        }

        auto DecompressUInt32::column_major(const std::vector<uint8_t>& compressed, int64_t length) -> std::vector<uint32_t> {
            assert(p);
            return p->decode.column_major(compressed, length);
        }

        auto DecompressUInt32::column_major(const std::vector<uint8_t>& compressed, int64_t length, std::nothrow_t nothrow) -> std::vector<uint32_t> {
            assert(p);
            return p->decode.column_major(compressed, length, nothrow);
        }

        auto DecompressUInt32::row_major(const std::vector<uint8_t>& compressed, int64_t length) -> std::vector<uint32_t> {
            assert(p);
            return p->decode.row_major(compressed, length);
        }

        auto DecompressUInt32::row_major(const std::vector<uint8_t>& compressed, int64_t length, std::nothrow_t nothrow) -> std::vector<uint32_t> {
            assert(p);
            return p->decode.row_major(compressed, length, nothrow);
        }

        auto MakeDecompressUInt32() -> std::unique_ptr<DecompressUInt32> {
            try {
                return std::make_unique<DecompressUInt32>();
            }
            catch(const std::exception&) {
                return nullptr;
            }
        }

        // ---




        struct CompressUInt64::impl
        {
            compress_wrapper<ctk::matrix_encoder<uint64_t>> encode;

            impl() = default;
            impl(const impl&) = default;
            impl(impl&&) = default;
            auto operator=(const impl&) -> impl& = default;
            auto operator=(impl&&) -> impl& = default;
            ~impl() = default;
        };

        CompressUInt64::CompressUInt64()
            : p{ new impl{} } {
            assert(p);
        }

        CompressUInt64::~CompressUInt64() {
        }

        CompressUInt64::CompressUInt64(const CompressUInt64& x)
            : p{ new impl{ *x.p } } {
            assert(x.p);
            assert(p);
        }

        auto swap(CompressUInt64& x, CompressUInt64& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        auto CompressUInt64::operator=(const CompressUInt64& x) -> CompressUInt64& {
            CompressUInt64 y{ x };
            swap(*this, y);
            return *this;
        }

        auto CompressUInt64::sensors(int64_t height) -> bool {
            assert(p);
            return p->encode.sensors(height);
        }

        auto CompressUInt64::sensors(int64_t height, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.sensors(height, nothrow);
        }

        auto CompressUInt64::sensors(const std::vector<int16_t>& order) -> bool {
            assert(p);
            return p->encode.sensors(order);
        }

        auto CompressUInt64::sensors(const std::vector<int16_t>& order, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.sensors(order, nothrow);
        }

        auto CompressUInt64::reserve(int64_t length) -> void {
            assert(p);
            p->encode.reserve(length);
        }

        auto CompressUInt64::reserve(int64_t length, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->encode.reserve(length, nothrow);
        }

        auto CompressUInt64::column_major(const std::vector<uint64_t>& matrix, int64_t length) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.column_major(matrix, length);
        }

        auto CompressUInt64::column_major(const std::vector<uint64_t>& matrix, int64_t length, std::nothrow_t nothrow) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.column_major(matrix, length, nothrow);
        }

        auto CompressUInt64::row_major(const std::vector<uint64_t>& matrix, int64_t length) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.row_major(matrix, length);
        }

        auto CompressUInt64::row_major(const std::vector<uint64_t>& matrix, int64_t length, std::nothrow_t nothrow) -> std::vector<uint8_t> {
            assert(p);
            return p->encode.row_major(matrix, length, nothrow);
        }

        auto MakeCompressUInt64() -> std::unique_ptr<CompressUInt64> {
            try {
                return std::make_unique<CompressUInt64>();
            }
            catch(const std::exception&) {
                return nullptr;
            }
        }


        struct DecompressUInt64::impl
        {
            decompress_wrapper<ctk::matrix_decoder<uint64_t>> decode;

            impl() = default;
            impl(const impl&) = default;
            impl(impl&&) = default;
            auto operator=(const impl&) -> impl& = default;
            auto operator=(impl&&) -> impl& = default;
            ~impl() = default;
        };

        DecompressUInt64::DecompressUInt64()
            : p{ new impl{} } {
            assert(p);
        }

        DecompressUInt64::~DecompressUInt64() {
        }

        DecompressUInt64::DecompressUInt64(const DecompressUInt64& x)
            : p{ new impl{ *x.p } } {
            assert(x.p);
            assert(p);
        }

        auto swap(DecompressUInt64& x, DecompressUInt64& y) -> void {
            using namespace std;
            swap(x.p, y.p);
        }

        auto DecompressUInt64::operator=(const DecompressUInt64& x) -> DecompressUInt64& {
            DecompressUInt64 y{ x };
            swap(*this, y);
            return *this;
        }

        auto DecompressUInt64::sensors(int64_t height) -> bool {
            assert(p);
            return p->decode.sensors(height);
        }

        auto DecompressUInt64::sensors(int64_t height, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.sensors(height, nothrow);
        }

        auto DecompressUInt64::sensors(const std::vector<int16_t>& order) -> bool {
            assert(p);
            return p->decode.sensors(order);
        }

        auto DecompressUInt64::sensors(const std::vector<int16_t>& order, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.sensors(order, nothrow);
        }

        auto DecompressUInt64::reserve(int64_t length) -> void {
            assert(p);
            p->decode.reserve(length);
        }

        auto DecompressUInt64::reserve(int64_t length, std::nothrow_t nothrow) -> bool {
            assert(p);
            return p->decode.reserve(length, nothrow);
        }

        auto DecompressUInt64::column_major(const std::vector<uint8_t>& compressed, int64_t length) -> std::vector<uint64_t> {
            assert(p);
            return p->decode.column_major(compressed, length);
        }

        auto DecompressUInt64::column_major(const std::vector<uint8_t>& compressed, int64_t length, std::nothrow_t nothrow) -> std::vector<uint64_t> {
            assert(p);
            return p->decode.column_major(compressed, length, nothrow);
        }

        auto DecompressUInt64::row_major(const std::vector<uint8_t>& compressed, int64_t length) -> std::vector<uint64_t> {
            assert(p);
            return p->decode.row_major(compressed, length);
        }

        auto DecompressUInt64::row_major(const std::vector<uint8_t>& compressed, int64_t length, std::nothrow_t nothrow) -> std::vector<uint64_t> {
            assert(p);
            return p->decode.row_major(compressed, length, nothrow);
        }

        auto MakeDecompressUInt64() -> std::unique_ptr<DecompressUInt64> {
            try {
                return std::make_unique<DecompressUInt64>();
            }
            catch(const std::exception&) {
                return nullptr;
            }
        }

        // ---




    } /* namespace v1 */
} /* namespace api */ } /* namespace ctk */


