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

namespace ctk { namespace api {

    namespace v1 {

        using namespace ctk::impl;


        struct CompressReflib::impl
        {
            ctk::matrix_encoder_reflib encode;

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

        auto CompressReflib::Sensors(int64_t x) -> bool {
            assert(p);
            return p->encode.row_count(sensor_count{ x });
        }

        auto CompressReflib::Sensors(const std::vector<int16_t>& xs) -> bool {
            assert(p);
            return p->encode.row_order(xs);
        }

        auto CompressReflib::ColumnMajor(const std::vector<int32_t>& matrix, int64_t n) -> std::vector<uint8_t> {
            assert(p);
            constexpr const column_major2row_major transpose{};
            return p->encode(matrix, measurement_count{ n }, transpose);
        }

        auto CompressReflib::RowMajor(const std::vector<int32_t>& matrix, int64_t n) -> std::vector<uint8_t> {
            assert(p);
            constexpr const row_major2row_major copy{};
            return p->encode(matrix, measurement_count{ n }, copy);
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
            ctk::matrix_decoder_reflib decode;

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

        auto DecompressReflib::Sensors(int64_t x) -> bool {
            assert(p);
            return p->decode.row_count(sensor_count{ x });
        }

        auto DecompressReflib::Sensors(const std::vector<int16_t>& xs) -> bool {
            assert(p);
            return p->decode.row_order(xs);
        }

        auto DecompressReflib::ColumnMajor(const std::vector<uint8_t>& xs, int64_t n) -> std::vector<int32_t> {
            assert(p);
            constexpr const column_major2row_major transpose{};
            return p->decode(xs, measurement_count{ n }, transpose);
        }

        auto DecompressReflib::RowMajor(const std::vector<uint8_t>& xs, int64_t n) -> std::vector<int32_t> {
            assert(p);
            constexpr const row_major2row_major copy{};
            return p->decode(xs, measurement_count{ n }, copy);
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
            ctk::matrix_encoder<int16_t> encode;

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

        auto CompressInt16::Sensors(int64_t x) -> bool {
            assert(p);
            return p->encode.row_count(sensor_count{ x });
        }

        auto CompressInt16::Sensors(const std::vector<int16_t>& xs) -> bool {
            assert(p);
            return p->encode.row_order(xs);
        }

        auto CompressInt16::ColumnMajor(const std::vector<int16_t>& matrix, int64_t n) -> std::vector<uint8_t> {
            assert(p);
            constexpr const column_major2row_major transpose{};
            return p->encode(matrix, measurement_count{ n }, transpose);
        }

        auto CompressInt16::RowMajor(const std::vector<int16_t>& matrix, int64_t n) -> std::vector<uint8_t> {
            assert(p);
            constexpr const row_major2row_major copy{};
            return p->encode(matrix, measurement_count{ n }, copy);
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
            ctk::matrix_decoder<int16_t> decode;

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

        auto DecompressInt16::Sensors(int64_t x) -> bool {
            assert(p);
            return p->decode.row_count(sensor_count{ x });
        }

        auto DecompressInt16::Sensors(const std::vector<int16_t>& xs) -> bool {
            assert(p);
            return p->decode.row_order(xs);
        }

        auto DecompressInt16::ColumnMajor(const std::vector<uint8_t>& xs, int64_t n) -> std::vector<int16_t> {
            assert(p);
            constexpr const column_major2row_major transpose{};
            return p->decode(xs, measurement_count{ n }, transpose);
        }

        auto DecompressInt16::RowMajor(const std::vector<uint8_t>& xs, int64_t n) -> std::vector<int16_t> {
            assert(p);
            constexpr const row_major2row_major copy{};
            return p->decode(xs, measurement_count{ n }, copy);
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
            ctk::matrix_encoder<int32_t> encode;

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

        auto CompressInt32::Sensors(int64_t x) -> bool {
            assert(p);
            return p->encode.row_count(sensor_count{ x });
        }

        auto CompressInt32::Sensors(const std::vector<int16_t>& xs) -> bool {
            assert(p);
            return p->encode.row_order(xs);
        }

        auto CompressInt32::ColumnMajor(const std::vector<int32_t>& matrix, int64_t n) -> std::vector<uint8_t> {
            assert(p);
            constexpr const column_major2row_major transpose{};
            return p->encode(matrix, measurement_count{ n }, transpose);
        }

        auto CompressInt32::RowMajor(const std::vector<int32_t>& matrix, int64_t n) -> std::vector<uint8_t> {
            assert(p);
            constexpr const row_major2row_major copy{};
            return p->encode(matrix, measurement_count{ n }, copy);
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
            ctk::matrix_decoder<int32_t> decode;

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

        auto DecompressInt32::Sensors(int64_t x) -> bool {
            assert(p);
            return p->decode.row_count(sensor_count{ x });
        }

        auto DecompressInt32::Sensors(const std::vector<int16_t>& xs) -> bool {
            assert(p);
            return p->decode.row_order(xs);
        }

        auto DecompressInt32::ColumnMajor(const std::vector<uint8_t>& xs, int64_t n) -> std::vector<int32_t> {
            assert(p);
            constexpr const column_major2row_major transpose{};
            return p->decode(xs, measurement_count{ n }, transpose);
        }

        auto DecompressInt32::RowMajor(const std::vector<uint8_t>& xs, int64_t n) -> std::vector<int32_t> {
            assert(p);
            constexpr const row_major2row_major copy{};
            return p->decode(xs, measurement_count{ n }, copy);
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
            ctk::matrix_encoder<int64_t> encode;

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

        auto CompressInt64::Sensors(int64_t x) -> bool {
            assert(p);
            return p->encode.row_count(sensor_count{ x });
        }

        auto CompressInt64::Sensors(const std::vector<int16_t>& xs) -> bool {
            assert(p);
            return p->encode.row_order(xs);
        }

        auto CompressInt64::ColumnMajor(const std::vector<int64_t>& matrix, int64_t n) -> std::vector<uint8_t> {
            assert(p);
            constexpr const column_major2row_major transpose{};
            return p->encode(matrix, measurement_count{ n }, transpose);
        }

        auto CompressInt64::RowMajor(const std::vector<int64_t>& matrix, int64_t n) -> std::vector<uint8_t> {
            assert(p);
            constexpr const row_major2row_major copy{};
            return p->encode(matrix, measurement_count{ n }, copy);
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
            ctk::matrix_decoder<int64_t> decode;

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

        auto DecompressInt64::Sensors(int64_t x) -> bool {
            assert(p);
            return p->decode.row_count(sensor_count{ x });
        }

        auto DecompressInt64::Sensors(const std::vector<int16_t>& xs) -> bool {
            assert(p);
            return p->decode.row_order(xs);
        }

        auto DecompressInt64::ColumnMajor(const std::vector<uint8_t>& xs, int64_t n) -> std::vector<int64_t> {
            assert(p);
            constexpr const column_major2row_major transpose{};
            return p->decode(xs, measurement_count{ n }, transpose);
        }

        auto DecompressInt64::RowMajor(const std::vector<uint8_t>& xs, int64_t n) -> std::vector<int64_t> {
            assert(p);
            constexpr const row_major2row_major copy{};
            return p->decode(xs, measurement_count{ n }, copy);
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
            ctk::matrix_encoder<uint16_t> encode;

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

        auto CompressUInt16::Sensors(int64_t x) -> bool {
            assert(p);
            return p->encode.row_count(sensor_count{ x });
        }

        auto CompressUInt16::Sensors(const std::vector<int16_t>& xs) -> bool {
            assert(p);
            return p->encode.row_order(xs);
        }

        auto CompressUInt16::ColumnMajor(const std::vector<uint16_t>& matrix, int64_t n) -> std::vector<uint8_t> {
            assert(p);
            constexpr const column_major2row_major transpose{};
            return p->encode(matrix, measurement_count{ n }, transpose);
        }

        auto CompressUInt16::RowMajor(const std::vector<uint16_t>& matrix, int64_t n) -> std::vector<uint8_t> {
            assert(p);
            constexpr const row_major2row_major copy{};
            return p->encode(matrix, measurement_count{ n }, copy);
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
            ctk::matrix_decoder<uint16_t> decode;

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

        auto DecompressUInt16::Sensors(int64_t x) -> bool {
            assert(p);
            return p->decode.row_count(sensor_count{ x });
        }

        auto DecompressUInt16::Sensors(const std::vector<int16_t>& xs) -> bool {
            assert(p);
            return p->decode.row_order(xs);
        }

        auto DecompressUInt16::ColumnMajor(const std::vector<uint8_t>& xs, int64_t n) -> std::vector<uint16_t> {
            assert(p);
            constexpr const column_major2row_major transpose{};
            return p->decode(xs, measurement_count{ n }, transpose);
        }

        auto DecompressUInt16::RowMajor(const std::vector<uint8_t>& xs, int64_t n) -> std::vector<uint16_t> {
            assert(p);
            constexpr const row_major2row_major copy{};
            return p->decode(xs, measurement_count{ n }, copy);
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
            ctk::matrix_encoder<uint32_t> encode;

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

        auto CompressUInt32::Sensors(int64_t x) -> bool {
            assert(p);
            return p->encode.row_count(sensor_count{ x });
        }

        auto CompressUInt32::Sensors(const std::vector<int16_t>& xs) -> bool {
            assert(p);
            return p->encode.row_order(xs);
        }

        auto CompressUInt32::ColumnMajor(const std::vector<uint32_t>& matrix, int64_t n) -> std::vector<uint8_t> {
            assert(p);
            constexpr const column_major2row_major transpose{};
            return p->encode(matrix, measurement_count{ n }, transpose);
        }

        auto CompressUInt32::RowMajor(const std::vector<uint32_t>& matrix, int64_t n) -> std::vector<uint8_t> {
            assert(p);
            constexpr const row_major2row_major copy{};
            return p->encode(matrix, measurement_count{ n }, copy);
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
            ctk::matrix_decoder<uint32_t> decode;

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

        auto DecompressUInt32::Sensors(int64_t x) -> bool {
            assert(p);
            return p->decode.row_count(sensor_count{ x });
        }

        auto DecompressUInt32::Sensors(const std::vector<int16_t>& xs) -> bool {
            assert(p);
            return p->decode.row_order(xs);
        }

        auto DecompressUInt32::ColumnMajor(const std::vector<uint8_t>& xs, int64_t n) -> std::vector<uint32_t> {
            assert(p);
            constexpr const column_major2row_major transpose{};
            return p->decode(xs, measurement_count{ n }, transpose);
        }

        auto DecompressUInt32::RowMajor(const std::vector<uint8_t>& xs, int64_t n) -> std::vector<uint32_t> {
            assert(p);
            constexpr const row_major2row_major copy{};
            return p->decode(xs, measurement_count{ n }, copy);
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
            ctk::matrix_encoder<uint64_t> encode;

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

        auto CompressUInt64::Sensors(int64_t x) -> bool {
            assert(p);
            return p->encode.row_count(sensor_count{ x });
        }

        auto CompressUInt64::Sensors(const std::vector<int16_t>& xs) -> bool {
            assert(p);
            return p->encode.row_order(xs);
        }

        auto CompressUInt64::ColumnMajor(const std::vector<uint64_t>& matrix, int64_t n) -> std::vector<uint8_t> {
            assert(p);
            constexpr const column_major2row_major transpose{};
            return p->encode(matrix, measurement_count{ n }, transpose);
        }

        auto CompressUInt64::RowMajor(const std::vector<uint64_t>& matrix, int64_t n) -> std::vector<uint8_t> {
            assert(p);
            constexpr const row_major2row_major copy{};
            return p->encode(matrix, measurement_count{ n }, copy);
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
            ctk::matrix_decoder<uint64_t> decode;

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

        auto DecompressUInt64::Sensors(int64_t x) -> bool {
            assert(p);
            return p->decode.row_count(sensor_count{ x });
        }

        auto DecompressUInt64::Sensors(const std::vector<int16_t>& xs) -> bool {
            assert(p);
            return p->decode.row_order(xs);
        }

        auto DecompressUInt64::ColumnMajor(const std::vector<uint8_t>& xs, int64_t n) -> std::vector<uint64_t> {
            assert(p);
            constexpr const column_major2row_major transpose{};
            return p->decode(xs, measurement_count{ n }, transpose);
        }

        auto DecompressUInt64::RowMajor(const std::vector<uint8_t>& xs, int64_t n) -> std::vector<uint64_t> {
            assert(p);
            constexpr const row_major2row_major copy{};
            return p->decode(xs, measurement_count{ n }, copy);
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


