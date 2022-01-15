#pragma once

#include "compress/matrix.h"
#include "container/file_epoch.h"

namespace ctk { namespace impl {

template<typename I>
struct buf_win
{
    I first;
    I last;
    sint height;
    sint length;

    buf_win(I f, I l, sint h, sint e)
    : first{ f }
    , last{ l }
    , height{ h }
    , length{ e } {
        if (std::distance(first, last) != length * height) {
            throw api::v1::ctk_bug{ "buf_win: invalid size" };
        }
        if (length < 0 || height < 0) {
            throw api::v1::ctk_bug{ "buf_win: negative dimension" };
        }
    }

    buf_win(I f, I l, sensor_count h, measurement_count e)
    : buf_win{ f, l, static_cast<sint>(h), static_cast<sint>(e) } {
    }
};

template<typename I>
auto valid_i(const buf_win<I>& x, ptrdiff_t offset) -> bool {
    return 0 <= offset && offset < x.length;
}


template<typename I>
auto first_i(const buf_win<I>& x, ptrdiff_t offset) -> I {
    if (!valid_i(x, offset)) {
        throw api::v1::ctk_bug{ "first_i: invalid offset" };
    }

    return x.first + offset;
}

template<typename I>
auto last_i(const buf_win<I>& x, ptrdiff_t offset, ptrdiff_t length) -> I {
    if (!valid_i(x, offset + length - 1)) {
        throw api::v1::ctk_bug{ "last_i: invalid offset + length" };
    }

    return x.first + offset + length;
}


template<typename I, typename IOut>
auto submatrix(ptrdiff_t amount, const buf_win<I>& input, ptrdiff_t i_offset, const buf_win<IOut>& output, ptrdiff_t o_offset) -> IOut {
    if (amount == 0) {
        return output.first;
    }
    if (input.height != output.height) {
        throw api::v1::ctk_bug{ "submatrix: incompatible" };
    }

    auto fi{ first_i(input, i_offset) };
    auto li{ last_i(input, i_offset, amount) };

    auto fo{ first_i(output, o_offset) };
    auto lo{ last_i(output, o_offset, amount) };

    if (fi == li || std::distance(fi, li) != std::distance(fo, lo)) {
        throw api::v1::ctk_bug{ "submatrix: invalid distance" };
    }

    const IOut result{ std::copy(fi, li, fo) };

    for (ptrdiff_t row{ 1 }; row < input.height; ++row) {
        fi += input.length;
        fo += output.length;

        std::copy(fi, fi + amount, fo);
    }

    return result;
}


template<typename I, typename IOut>
auto submatrix(measurement_count amount, const buf_win<I>& input, measurement_count i_offset, const buf_win<IOut>& output, measurement_count o_offset) -> IOut {
    const sint a{ amount };
    const sint io{ i_offset };
    const sint oo{ o_offset };

    return submatrix(a, input, io, output, oo);
}


auto electrode_scaling(std::vector<api::v1::Electrode> electrodes) -> std::vector<double>;

template<typename EpochReader>
// EpochReader has interface identical to epoch_reader_riff and epoch_reader_flat
class reflib_reader_common
{
    EpochReader reader;
    matrix_decoder_reflib decode;

    epoch_count cached;
    measurement_count cached_epoch_length;
    measurement_count cache_index;
    std::vector<int32_t> cache;
    std::vector<int32_t> buffer;
    std::vector<double> scales;

public:

    // constructs epoch_reader_riff reader
    explicit
    reflib_reader_common(const std::filesystem::path& fname, bool is_broken)
    : reader{ fname, is_broken }
    , cached{ std::numeric_limits<measurement_count::value_type>::max() }
    , cached_epoch_length{ 0 }
    , cache_index{ measurement_count{ std::numeric_limits<measurement_count::value_type>::max() } }
    , scales{ electrode_scaling(reader.data().description().ts.electrodes)  } {
        decode.row_order(reader.data().order()); // TODO?
    }

    // constructs epoch_reader_flat reader
    reflib_reader_common(const std::filesystem::path& fname, const std::vector<tagged_file>& available)
    : reader{ fname, available }
    , cached{ std::numeric_limits<measurement_count::value_type>::max() }
    , cached_epoch_length{ 0 }
    , cache_index{ measurement_count{ std::numeric_limits<measurement_count::value_type>::max() } }
    , scales{ electrode_scaling(reader.data().description().ts.electrodes)  } {
        decode.row_order(reader.data().order());
    }

    auto sample_count() const -> measurement_count {
        return reader.data().sample_count();
    }

    /*
    range interface: returns variable amount of samples
    the output is in row major format.
    used internally by libeep and by this library.
    vector<int32_t> output {
        11, 12, 13, 14, // sample data at time points 1, 2, 3 and 4 for sensor 1
        21, 22, 23, 24, // sample data at time points 1, 2, 3 and 4 for sensor 2
        31, 32, 33, 34  // sample data at time points 1, 2, 3 and 4 for sensor 3
    } */
    auto range_row_major(measurement_count i, measurement_count amount) -> std::vector<int32_t> {
        constexpr const row_major2row_major copy;
        return get(i, amount, copy);
    }

    /*
    the output is in column major format.
    used by the libeep interface.
    vector<int32_t> output {
        11, 21, 31, // measurement at time point 1: sample data for sensors 1, 2 and 3
        12, 22, 32, // measurement at time point 2: sample data for sensors 1, 2 and 3
        13, 23, 33, // measurement at time point 3: sample data for sensors 1, 2 and 3
        14, 24, 34  // measurement at time point 4: sample data for sensors 1, 2 and 3
    } */
    auto range_column_major(measurement_count i, measurement_count amount) -> std::vector<int32_t> {
        constexpr const column_major2row_major transpose;
        return get(i, amount, transpose);
    }

    // libeep v4 interface
    auto range_scaled(measurement_count i, measurement_count amount) -> std::vector<float> {
        const auto scale = [](int32_t x, float y) -> float { return static_cast<float>(x) * y; };

        const auto unscaled{ range_column_major(i, amount) };
        std::vector<float> result(unscaled.size());
        std::transform(begin(unscaled), end(unscaled), begin(scales), begin(result), scale);
        return result;
    }


    /*
    epoch interface:
    omits the intermediate buffer used to collect samples into the variable sized output
    requested by the user (as range) out of "epoch length" sized epochs.
    this interface delivers data in the following fashion:
        - all output epochs but the last contain epoch length measurements
        - the last epoch might be shorter
    */
    auto epochs() const -> epoch_count {
        return reader.data().count();
    }

    auto epoch_row_major(epoch_count i) -> std::vector<int32_t> {
        if (!load_epoch(i)) {
            return {};
        }
        return cache;
    }

    auto epoch_column_major(epoch_count i) -> std::vector<int32_t> {
        if (!load_epoch(i)) {
            return {};
        }

        std::vector<int32_t> result(cache.size());
        constexpr const column_major2row_major transpose;
        transpose.to_client(begin(cache), begin(cache), reader.data().order(), cached_epoch_length);
        return result;
    }

    auto epoch_compressed(epoch_count i) -> std::vector<uint8_t> {
        const compressed_epoch ce{ reader.data().epoch(i) };
        return ce.data;
    }

    auto description() const -> time_signal {
        return reader.data().description();
    }

    auto cnt_type() const -> api::v1::RiffType {
        return reader.data().cnt_type();
    }

    auto epoch_length() const -> measurement_count {
        return reader.data().epoch_length();
    }

    auto sampling_frequency() const -> double {
        return reader.data().sampling_frequency();
    }

    auto channels() const -> std::vector<api::v1::Electrode> {
        return reader.data().channels();
    }

    auto triggers() const -> std::vector<api::v1::Trigger> {
        return reader.data().triggers();
    }

    auto information() const -> api::v1::Info {
        return reader.data().information();
    }

    auto file_version() const -> api::v1::FileVersion {
        return reader.data().file_version();
    }

    auto segment_start_time() const -> api::v1::DcDate {
        return reader.data().segment_start_time();
    }

    auto history() const -> std::string {
        return reader.data().history();
    }

    auto embedded_files() const -> std::vector<std::string> {
        return reader.embedded_files();
    }

    auto extract_embedded_file(const std::string& label, const std::filesystem::path& fname) const -> bool {
        return reader.extract_embedded_file(label, fname);
    }

private:

    auto is_valid() const -> bool {
        return !cache.empty() && cache_index < cached_epoch_length;
    }

    auto load_epoch(epoch_count i) -> bool {
        if (i == cached) {
            return is_valid();
        }

        const compressed_epoch ce{ reader.data().epoch(i, std::nothrow) };
        if (ce.data.empty()) {
            cache.clear();
            cached_epoch_length = measurement_count{ 0 };
            return false;
        }

        // multiplexing is performed immediately before returning the data requested by the client
        constexpr const row_major2row_major copy;

        cache = decode(ce.data, ce.length, copy);
        cached = i;
        cached_epoch_length = ce.length;
        assert(cached_epoch_length <= epoch_length());

        return is_valid();
    }

    auto load_epoch(measurement_count n) -> bool {
        if (n < 0 || epoch_length() < 1 || sample_count() <= n) {
            return false;
        }

        const sint i{ n };
        const sint el{ epoch_length() };
        const auto[quot, rem]{ std::div(i, el) };

        cache_index = measurement_count{ cast(rem, sint{}, guarded{}) };
        return load_epoch(epoch_count{ cast(quot, sint{}, guarded{}) });
    }

    template<typename Multiplex>
    // Multiplex has an interface compatible with column_major2row_major and row_major2row_major (buffer_transfer.h)
    auto get(measurement_count i, measurement_count amount, Multiplex multiplex) -> std::vector<int32_t> {
        const sint si{ i };
        const sint size{ amount };
        const sint requested{ plus(si, size, ok{}) };
        const sint total{ sample_count() };
        if (i < 0 || sample_count() <= i || amount < 1 || total < requested) {
            return {};
        }

        const sensor_count height{ reader.data().channel_count() };
        measurement_count output_index{ 0 };
        measurement_count due{ amount };

        using IOut = decltype(begin(buffer));
        buffer.resize(as_sizet_unchecked(matrix_size(height, amount)));
        const buf_win<IOut> output{ begin(buffer), end(buffer), height, amount};

        while(0 < due && load_epoch(i)) {
            using I = decltype(begin(cache));
            const buf_win<I> input{ begin(cache), end(cache), height, cached_epoch_length };

            const auto stride{ std::min(cached_epoch_length - cache_index, due) };
            submatrix(stride, input, cache_index, output, output_index);

            due -= stride;
            i += stride;
            output_index += stride;
        }

        if (due != 0) {
            return {}; // can not load all requested data
        }

        std::vector<int32_t> result(buffer.size());
        multiplex.to_client(begin(buffer), begin(result), reader.data().order(), amount);

        return result;
    }
};


using cnt_reader_reflib_riff = reflib_reader_common<epoch_reader_riff>; 
using cnt_reader_reflib_flat = reflib_reader_common<epoch_reader_flat>; 


template<typename T>
auto signal_length(const std::vector<T>& v, sensor_count height) -> measurement_count {
    if (height < 1) {
        throw api::v1::ctk_bug{ "signal_length: not initialized" };
    }

    const sint area{ vsize(v) };
    const sint channels{ height };
    const auto[quot, rem]{ std::div(area, channels) };
    if (rem != 0) {
        throw api::v1::ctk_limit{ "signal_length: invalid input dimensions" };
    }

    return measurement_count{ cast(quot, sint{}, guarded{}) };
}



template<typename T, typename Format>
class cnt_writer_flat
{
    epoch_writer_flat epoch_writer;
    matrix_encoder_general<T, Format> encode;
    std::vector<T> cache;
    std::vector<T> buffer;
    measurement_count cache_index;
    sensor_count height;
    bool closed;

public:

    cnt_writer_flat(const std::filesystem::path& fname, const time_signal& description, api::v1::RiffType riff, const std::string& history)
    : epoch_writer{ fname, description, riff, history }
    , cache_index{ 0 }
    , height{ vsize(description.ts.electrodes) }
    , closed{ false } {
        const auto order{ natural_row_order(height) }; // TODO?
        encode.row_order(order);

        const measurement_count epoch_length{ description.ts.epoch_length };
        cache.resize(as_sizet_unchecked(matrix_size(height, epoch_length)));
    }

    ~cnt_writer_flat() = default;

    /*
    range interface: encodes variable amount of samples
    the input is in row major format.
    used internally by libeep and by this library.
    vector<T> client {
        11, 12, 13, 14, // sample data at time points 1, 2, 3 and 4 for sensor 1
        21, 22, 23, 24, // sample data at time points 1, 2, 3 and 4 for sensor 2
        31, 32, 33, 34  // sample data at time points 1, 2, 3 and 4 for sensor 3
    } */
    auto range_row_major(const std::vector<T>& client) -> void {
        constexpr const row_major2row_major copy;
        append_range(client, copy);
    }

    /*
    the input is in column major format.
    used by the libeep interface.
    vector<T> client {
        11, 21, 31, // measurement at time point 1: sample data for sensors 1, 2 and 3
        12, 22, 32, // measurement at time point 2: sample data for sensors 1, 2 and 3
        13, 23, 33, // measurement at time point 3: sample data for sensors 1, 2 and 3
        14, 24, 34  // measurement at time point 4: sample data for sensors 1, 2 and 3
    } */
    auto range_column_major(const std::vector<T>& client) -> void {
        constexpr const column_major2row_major transpose;
        append_range(client, transpose);
    }


    /*
    NB: do NOT interleave calls to range_xxx_major and epoch_xxx_major.
    use either only the range interface or the epoch interface functions for time signal writing.

    epoch interface:
    omits the intermediate buffer used to store the variable sized user input until epoch length measurements are collected.
    this interface needs to be called in the following fashion:
        - all input epochs but the last consist of epoch length measurements
        - the last epoch might be shorter
    */
    auto epoch_row_major(const std::vector<T>& client) -> void {
        commit(client, signal_length(client, height));
    }

    auto epoch_column_major(const std::vector<T>& client) -> void {
        const auto length{ signal_length(client, height) };

        buffer.resize(client.size());
        constexpr const column_major2row_major transpose;
        transpose.from_client(begin(client), begin(buffer), encode.row_order(), length); // demultiplex

        commit(buffer, length);
    }

    auto trigger(const api::v1::Trigger& x) -> void {
        epoch_writer.append(x);
    }

    auto triggers(const std::vector<api::v1::Trigger>& triggers) -> void {
        epoch_writer.append(triggers);
    }

    auto flush() -> void {
        epoch_writer.flush();
    }

    auto is_closed() const -> bool {
        return closed;
    }

    auto close() -> void {
        if (closed) {
            return;
        }

        if (cache_index < 1) {
            assert(cache_index == 0);
            closed = true;
            return;
        }

        buffer.resize(as_sizet_unchecked(matrix_size(height, cache_index)));
        const auto epoch_length{ epoch_writer.epoch_length() };
        const measurement_count zero{ 0 };

        using I = decltype(begin(cache));
        const buf_win<I> input{ begin(cache), end(cache), height, epoch_length };

        using IOut = decltype(begin(buffer));
        const buf_win<IOut> output{ begin(buffer), end(buffer), height, cache_index};

        submatrix(cache_index, input, zero, output, zero);
        commit(buffer, cache_index);
        epoch_writer.close();
        closed = true;
    }

    auto set_info(const api::v1::Info& x) -> void {
        epoch_writer.set_info(x);
    }

    auto file_tokens() const -> std::vector<tagged_file> {
        return epoch_writer.file_tokens();
    }

    auto sample_count() const -> measurement_count {
        return epoch_writer.sample_count();
    }


    // reader functionality (completely untested, especially for unsynchronized reads during writing - the intended use case)
    auto commited() const -> measurement_count {
        return sample_count() - cache_index;
    }

    auto range_row_major(measurement_count i, measurement_count amount) -> std::vector<T> {
        cnt_reader_reflib_flat reader{ epoch_writer.file_name(), epoch_writer.file_tokens() };
        return reader.range_row_major(i, amount);
    }

    auto range_column_major(measurement_count i, measurement_count amount) -> std::vector<T> {
        cnt_reader_reflib_flat reader{ epoch_writer.file_name(), epoch_writer.file_tokens() };
        return reader.range_column_major(i, amount);
    }

private:

    template<typename Multiplex>
    // Multiplex has an interface compatible with column_major2row_major and row_major2row_major (buffer_transfer.h)
    auto append_range(const std::vector<T>& client, Multiplex multiplex) -> void {
        if (closed) {
            throw api::v1::ctk_bug{ "cnt_writer_flat::append_range: already closed" };
        }

        const auto length{ signal_length(client, height) };

        buffer.resize(client.size());
        multiplex.from_client(begin(client), begin(buffer), encode.row_order(), length); // demultiplex

        const auto epoch_length{ epoch_writer.epoch_length() };

        using I = decltype(begin(buffer));
        const buf_win<I> input{ begin(buffer), end(buffer), height, length };

        using IOut = decltype(begin(cache));
        const buf_win<IOut> output{ begin(cache), end(cache), height, epoch_length};

        auto input_index{ measurement_count{ 0 } };
        while(input_index < length) {
            const auto stride{ std::min(epoch_length - cache_index, length - input_index) };
            submatrix(stride, input, input_index, output, cache_index);

            cache_index += stride;
            input_index += stride;

            if (cache_index == epoch_length) {
                commit(cache, epoch_length);
                cache_index = measurement_count{ 0 };
            }
        }
    }

    auto commit(const std::vector<T>& input, measurement_count length) -> void {
        if (closed) {
            throw api::v1::ctk_bug{ "cnt_writer_flat::commit: already closed" };
        }
        const auto epoch_length{ epoch_writer.epoch_length() };
        closed = length < epoch_length;

        if (cache.size() < input.size() || length < 1 || epoch_length < length) {
            throw api::v1::ctk_bug{ "cnt_writer_flat::commit: invalid input" };
        }

        constexpr const row_major2row_major copy;
        const auto bytes{ encode(input, length, copy) };

        epoch_writer.append({ length, bytes });
    }
};

using cnt_writer_reflib_flat = cnt_writer_flat<int32_t, reflib>;


struct external_file
{
    std::string label;
    std::filesystem::path file_name;

    external_file(const std::string& label, const std::filesystem::path& file_name)
    : label{ label }
    , file_name{ file_name } {
    }
};
auto operator<<(std::ostream&, const external_file&) -> std::ostream&;


class cnt_writer_reflib_riff
{
    api::v1::RiffType riff;
    std::filesystem::path file_name;
    std::unique_ptr<cnt_writer_reflib_flat> flat_writer;
    api::v1::Info information;
    std::string history;
    std::vector<external_file> user;

public:

    cnt_writer_reflib_riff(const std::filesystem::path& name, api::v1::RiffType riff, const std::string& history);
    ~cnt_writer_reflib_riff() = default;
    cnt_writer_reflib_riff(const cnt_writer_reflib_riff&) = delete;
    cnt_writer_reflib_riff& operator=(const cnt_writer_reflib_riff&) = delete;

    // assemblies the generated files into a single RIFF file.
    // this is the last function to call.
    // NB: calling any other function of this class (or the class returned by add_time_signal) after close will access uninitialized memory.
    auto close() -> void;

    auto flush() -> void;

    auto recording_info(const api::v1::Info&) -> void;
    auto add_time_signal(const time_signal&) -> cnt_writer_reflib_flat*;
    //auto add_avg_signal(const mean_signal& description) -> output_segment<float>&;
    //auto add_stddev_signal(const mean_signal& description) -> output_segment<float>&;
    //auto add_wav_signal(const wav_signal& description) -> output_segment<float>&;

    // compatible extension: user supplied content placed into a top-level chunk.
    // fname should exist and be accessible when close is called.
    // label is a 4 byte label.
    // label can not be any of: "eeph", "info", "evt ", "raw3", "rawf", "stdd", "tfh " or "tfd " (maybe not "refh", "imp ", "nsh ", "vish", "egih", "egig", "egiz" and "binh" as well).
    // at most one user supplied chunk can have this label.
    auto embed(std::string label, const std::filesystem::path& fname) -> void;

    // reader functionality (completely untested, especially for unsynchronized reads during writing - the intended use case)
    auto commited() const -> measurement_count;
    auto range_row_major(measurement_count i, measurement_count samples) -> std::vector<int32_t>;
    auto range_column_major(measurement_count i, measurement_count samples) -> std::vector<int32_t>;
};


class cnt_writer_riff
{
    api::v1::RiffType riff;
    std::filesystem::path file_name;
    //std::vector< std::unique_ptr<cnt_writer_reflib_flat> > flat_writers;
    //std::unique_ptr<cnt_writer_reflib_flat> flat_writer;
    std::vector<std::unique_ptr<cnt_writer_flat<int32_t, extended>>> ts_segments;
    api::v1::Info information;
    std::string history;
    std::vector<external_file> user;

public:

    cnt_writer_riff(const std::filesystem::path& name, api::v1::RiffType riff, const std::string& history);
    ~cnt_writer_riff() = default;
    cnt_writer_riff(const cnt_writer_riff&) = delete;
    cnt_writer_riff& operator=(const cnt_writer_riff&) = delete;

    // assemblies the generated files into a single RIFF file.
    // this is the last function to call.
    // NB: calling any other function of this class (or the class returned by add_time_signal) after close will access uninitialized memory.
    auto close() -> void;

    auto flush() -> void;

    auto recording_info(const api::v1::Info&) -> void;
    auto add_time_signal(const time_signal&) -> cnt_writer_reflib_flat/*time_series_writer*/*;
    //auto add_avg_series(const mean_series& description) -> output_segment<float>&;
    //auto add_stddev_series(const mean_series& description) -> output_segment<float>&;
    //auto add_wav_series(const wav_series& description) -> output_segment<float>&;

    // compatible extension: user supplied content placed into a top-level chunk.
    // fname should exist and be accessible when close is called.
    // label is a 4 byte label.
    // label can not be any of: "eeph", "info", "evt ", "raw3", "rawf", "stdd", "tfh " or "tfd " (maybe not "refh", "imp ", "nsh ", "vish", "egih", "egig", "egiz" and "binh" as well).
    // all labels in a file should be unique.
    auto embed(std::string label, const std::filesystem::path& fname) -> void;

    // reader functionality (completely untested, especially for unsynchronized reads during writing - the intended use case)
    auto commited() const -> measurement_count;
    auto range_row_major(cnt_writer_reflib_flat* ts, measurement_count i, measurement_count samples) -> std::vector<int32_t>;
    auto range_column_major(cnt_writer_reflib_flat* ts, measurement_count i, measurement_count samples) -> std::vector<int32_t>;
};


} /* namespace impl */ } /* namespace ctk */


