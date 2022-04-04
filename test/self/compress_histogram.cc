#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "../catch.hpp"

#include "compress/matrix.h"
#include "test/util.h"

namespace ctk { namespace impl { namespace test {


struct encoded_pattern_size
{
    const bit_count fixed_size;
    const bit_count variable_size;

    encoded_pattern_size(bit_count n, bit_count nexc)
        : fixed_size{ n }
        , variable_size{ n + nexc } {
    }

    // appends the encoded size to a running sum.
    constexpr
    auto operator()(bit_count acc, bool is_exceptional) const -> bit_count {
        return acc + (is_exceptional ? variable_size : fixed_size);
    }
};


auto calculate_compressed_block_size(std::vector<bool>::const_iterator first, std::vector<bool>::const_iterator last, bit_count n, bit_count nexc, bit_count header) -> bit_count {

    const bit_count acc{0};
    const bit_count data{ std::accumulate(first, last, acc, encoded_pattern_size{ n, nexc }) };

    return header + data;
}




template<typename T, typename Format>
auto histogram_n(reduction<T>& dut, estimation<T>& e, Format) -> bit_count {
    dut.method = encoding_method::time; // any compressed method
    compressed_parameters(dut, e, Format{});
    return dut.n;
}


auto bit_count_max() -> bit_count {
    return bit_count{ std::numeric_limits<sint>::max() };
}


template<typename T, typename Format>
auto exhaustive_n(const std::vector<T>& residuals, Format) -> bit_count {
    std::vector<bit_count> sizes(residuals.size());
    std::transform(begin(residuals), end(residuals), begin(sizes), count_raw3{});
    const bit_count nexc{ *std::max_element(std::next(begin(sizes)), end(sizes)) };

    const auto encoding_size{ min_data_size(nexc, sizes[0], Format{}) };
    bit_count header_size{ compressed_header_width(encoding_size, Format{}) };

    std::vector<bool> encoding_map(residuals.size());

    bit_count best_size{ bit_count_max() };
    bit_count best{ 0 };

    for (bit_count n{ 2 }; n <= nexc; ++n) {
        if (n == nexc) {
            std::fill(begin(encoding_map), end(encoding_map), false);
        }
        else {
            std::transform(begin(residuals), end(residuals), begin(sizes), begin(encoding_map), is_exception{ n });
        }

        const auto second{ std::next(begin(encoding_map)) }; // master value, included in the header size
        const auto last{ end(encoding_map) };
        const auto current_size{ calculate_compressed_block_size(second, last, n, nexc, header_size) };
        if (current_size < best_size) {
            best_size = current_size;
            best = n;
        }
    }

    return best;
}


TEST_CASE("histogram", "[correct]") {
    using S = sint;
    using T = typename std::make_unsigned<S>::type;
    using Format = extended;

	random_values random;
    const Format format;

    for (S length{ 2 }; length < 512; length += 1) {
        reduction<T> dut;
        estimation<T> e;
        dut.resize(measurement_count{ length });
        e.resize(measurement_count{ length }, format);

        for(T i{ 0 }; i < 10; ++i) {
            std::vector<T> residuals(static_cast<size_t>(length));
            random.fill(T{ 0 }, T{ static_cast<T>(i + 10) }, residuals);

            std::copy(begin(residuals), end(residuals), begin(dut.residuals));
            bit_count x{ histogram_n(dut, e, format) };
            bit_count y{ exhaustive_n(residuals, format) };
            REQUIRE(x == y);

            std::adjacent_difference(begin(residuals), end(residuals), begin(residuals));
            std::copy(begin(residuals), end(residuals), begin(dut.residuals));
            x = histogram_n(dut, e, format);
            y = exhaustive_n(residuals, format);
            REQUIRE(x == y);

            std::adjacent_difference(std::next(begin(residuals)), end(residuals), std::next(begin(residuals)));
            std::copy(begin(residuals), end(residuals), begin(dut.residuals));
            x = histogram_n(dut, e, format);
            y = exhaustive_n(residuals, format);
            REQUIRE(x == y);
        }
    }
}

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */
