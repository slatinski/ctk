#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"
//#include <pybind11/numpy.h>
#include "ctk.h"
#include <sstream>

namespace py = pybind11;
namespace v1 = ctk::api::v1;

//PYBIND11_MAKE_OPAQUE(std::vector<int32_t>);
//PYBIND11_MAKE_OPAQUE(std::vector<uint8_t>);

template<typename T>
auto print(T x, const char* classname) -> std::string {
    std::ostringstream oss;
    oss << "<" << classname << ": " << x << ">";
    return oss.str();
}


using trigger_v4 = std::tuple<std::string, int64_t, int64_t, std::string, std::string, std::string>;
using channel_v4 = std::tuple<std::string, std::string, std::string>;

struct libeep_reader
{
    v1::CntReaderReflib reader;
    std::vector<v1::Trigger> triggers;
    v1::TimeSignal header;

    explicit
    libeep_reader(const std::string& fname)
    : reader{ fname }
    , triggers{ reader.triggers() }
    , header{ reader.description() } {
    }

    auto get_sample_count() const -> int64_t {
        return reader.sampleCount();
    }

    auto get_channel_count() const -> size_t {
      return header.electrodes.size();
    }

    auto get_channel(size_t i) const -> channel_v4 {
        const auto& e{ header.electrodes[i] };
        return { e.label, e.unit, e.reference };
    }

    auto get_sample_frequency() const -> double {
        return header.sampling_frequency;
    }

    auto get_samples(int64_t i, int64_t amount) -> std::vector<float> {
        return reader.rangeScaled(i, amount);
    }

    auto get_trigger_count() const -> size_t {
        return triggers.size();
    }

    auto get_trigger(size_t i) const -> trigger_v4 {
        if (triggers.size() <= i) {
            throw std::runtime_error("get_trigger: invalid index");
        }

        const auto code{ ctk::trigger_label(triggers[i].code) };
        const auto sample{ triggers[i].sample };
        const int64_t duration{ 0 };
        const std::string condition, descr, impedances;
        return { code, sample, duration, condition, descr, impedances };
    }
};


constexpr const double scaling_factor{ 128 };

struct channel2electrode
{
    auto operator()(const channel_v4& x) const -> v1::Electrode {
        const auto[label, unit, reference]{ x };
        v1::Electrode e;
        e.label = label;
        e.unit = unit;
        e.reference = reference;
        e.iscale = 1;
        e.rscale = 1 / scaling_factor;
        return e;
    }
};

auto channels2electrodes(const std::vector<channel_v4>& channels) -> std::vector<v1::Electrode> {
    std::vector<v1::Electrode> result(channels.size());
    std::transform(begin(channels), end(channels), begin(result), channel2electrode{});
    return result;
}


struct libeep_writer
{
    v1::CntWriterReflib writer;

    libeep_writer(const std::string& fname, int sample_rate, const std::vector<channel_v4>& channels, int cnt64)
    : writer{ fname, bool2riff(cnt64), "" } {
        v1::TimeSignal ts;
        ts.sampling_frequency = sample_rate;
        ts.electrodes = channels2electrodes(channels);
        writer.addTimeSignal(ts);
    }

    auto add_samples(const std::vector<float>& v) -> void {
        const auto float2int = [](float x) -> int32_t { return static_cast<int32_t>(std::round(x * scaling_factor)); };

        std::vector<int32_t> ints(v.size());
        std::transform(begin(v), end(v), begin(ints), float2int);
        writer.rangeColumnMajor(ints);
    }

    auto close() -> void {
        writer.close();
    }

    auto bool2riff(bool x) -> v1::RiffType {
        return x == 0 ? v1::RiffType::riff32 : v1::RiffType::riff64;
    }

};



PYBIND11_MODULE(ctkpy, m) {

    //py::bind_vector<std::vector<int32_t>>(m, "VectorInt32", py::buffer_protocol());
    //py::bind_vector<std::vector<uint8_t>>(m, "VectorUInt8", py::buffer_protocol());

    py::enum_<v1::RiffType>(m, "cnt_type", py::module_local())
        .value("cnt32", v1::RiffType::riff32)
        .value("cnt64", v1::RiffType::riff64);

    py::enum_<v1::Sex>(m, "sex", py::module_local())
        .value("unknown", v1::Sex::unknown)
        .value("male", v1::Sex::male)
        .value("female", v1::Sex::female);

    py::enum_<v1::Handedness>(m, "handedness", py::module_local())
        .value("unknown", v1::Handedness::unknown)
        .value("left", v1::Handedness::left)
        .value("right", v1::Handedness::right)
        .value("mixed", v1::Handedness::mixed);

    py::class_<v1::FileVersion> fv(m, "file_version", py::module_local()/*, py::dynamic_attr()*/);
    fv.def_readwrite("major", &v1::FileVersion::major)
      .def_readwrite("minor", &v1::FileVersion::minor)
      .def("__repr__", [](const v1::FileVersion& x) { return print(x, "file_version"); });

    py::class_<v1::Trigger> t(m, "trigger", py::module_local()/*, py::dynamic_attr()*/);
    t.def_readwrite("sample", &v1::Trigger::sample)
     .def_readwrite("code", &v1::Trigger::code)
     .def("__repr__", [](const v1::Trigger& x) { return print(x, "trigger"); });

    py::class_<v1::Electrode> e(m, "electrode", py::module_local()/*, py::dynamic_attr()*/);
    e.def_readwrite("label", &v1::Electrode::label)
     .def_readwrite("reference", &v1::Electrode::reference)
     .def_readwrite("unit", &v1::Electrode::unit)
     .def_readwrite("status", &v1::Electrode::status)
     .def_readwrite("type", &v1::Electrode::type)
     .def_readwrite("iscale", &v1::Electrode::iscale)
     .def_readwrite("rscale", &v1::Electrode::rscale)
     .def("__repr__", [](const v1::Electrode& x) { return print(x, "electrode"); });

    py::class_<v1::TimeSignal> ts(m, "time_signal", py::module_local()/*, py::dynamic_attr()*/);
    ts.def_readwrite("epoch_length", &v1::TimeSignal::epoch_length)
      .def_readwrite("sampling_frequency", &v1::TimeSignal::sampling_frequency)
      // missing start time
      .def_readwrite("electrodes", &v1::TimeSignal::electrodes)
      .def("__repr__", [](const v1::TimeSignal& x) { return print(x, "time_signal"); });

    py::class_<v1::Info> i(m, "information", py::module_local()/*, py::dynamic_attr()*/);
    i.def_readwrite("hospital", &v1::Info::hospital)
     .def_readwrite("test_name", &v1::Info::test_name)
     .def_readwrite("test_serial", &v1::Info::test_serial)
     .def_readwrite("physician", &v1::Info::physician)
     .def_readwrite("technician", &v1::Info::technician)
     .def_readwrite("machine_make", &v1::Info::machine_make)
     .def_readwrite("machine_model", &v1::Info::machine_model)
     .def_readwrite("machine_sn", &v1::Info::machine_sn)
     .def_readwrite("subject_name", &v1::Info::subject_name)
     .def_readwrite("subject_id", &v1::Info::subject_id)
     .def_readwrite("subject_phone", &v1::Info::subject_phone)
     .def_readwrite("subject_address", &v1::Info::subject_address)
     .def_readwrite("subject_sex", &v1::Info::subject_sex)
     .def_readwrite("subject_handedness", &v1::Info::subject_handedness)
     .def_readwrite("subject_dob", &v1::Info::subject_dob)
     .def_readwrite("comment", &v1::Info::comment)
     .def("__repr__", [](const v1::Info& x) { return print(x, "information"); });

    py::class_<v1::UserFile> uf(m, "user_file", py::module_local()/*, py::dynamic_attr()*/);
    uf.def_readwrite("label", &v1::UserFile::label)
      .def_readwrite("file_name", &v1::UserFile::file_name)
      .def("__repr__", [](const v1::UserFile& x) { return print(x, "user_file"); });

    py::class_<v1::CntReaderReflib> rr(m, "reflib_reader", py::module_local()/*, py::dynamic_attr()*/);
    rr.def(py::init<const std::string&>())
      .def_property_readonly("sample_count", &v1::CntReaderReflib::sampleCount, "Amount of measurements. Each measurement consists of 'channel count' data points (integer values each representing a single measurement from a single sensor at a particular point in time).")
      .def("row_major", &v1::CntReaderReflib::rangeRowMajor, py::arg("i"), py::arg("amount"), "Returns 'amount' measurements starting at position 'i'. The samples in the output buffer are arranged in row major order.")
      .def("column_major", &v1::CntReaderReflib::rangeColumnMajor)
      .def_property_readonly("epoch_count", &v1::CntReaderReflib::epochs)
      .def("epoch_row_major", &v1::CntReaderReflib::epochRowMajor)
      .def("epoch_column_major", &v1::CntReaderReflib::epochColumnMajor)
      .def("epoch_compressed", &v1::CntReaderReflib::epochCompressed)
      .def("triggers", &v1::CntReaderReflib::triggers)
      .def_property_readonly("cnt_type", &v1::CntReaderReflib::cntType)
      .def_property_readonly("time_signal", &v1::CntReaderReflib::description)
      .def_property_readonly("history", &v1::CntReaderReflib::history)
      .def_property_readonly("recording_info", &v1::CntReaderReflib::information)
      .def_property_readonly("file_version", &v1::CntReaderReflib::fileVersion)
      .def_property_readonly("embedded", &v1::CntReaderReflib::embeddedFiles)
      .def_property_readonly("extract_embedded", &v1::CntReaderReflib::extractEmbeddedFile)
      .def("__repr__", [](const v1::CntReaderReflib& x) { return print(x.description(), "reflib_reader"); });

    py::class_<v1::CntWriterReflib> rw(m, "reflib_writer", py::module_local()/*, py::dynamic_attr()*/);
    rw.def(py::init<const std::string&, v1::RiffType, const std::string&>())
      .def("close", &v1::CntWriterReflib::close)
      .def_property("recording_info", nullptr, &v1::CntWriterReflib::recordingInfo)
      .def_property("time_signal", nullptr, &v1::CntWriterReflib::addTimeSignal)
      .def("row_major", py::overload_cast<const std::vector<int32_t>&>(&v1::CntWriterReflib::rangeRowMajor))
      .def("column_major", py::overload_cast<const std::vector<int32_t>&>(&v1::CntWriterReflib::rangeColumnMajor))
      .def("add_trigger", &v1::CntWriterReflib::trigger)
      .def("add_triggers", &v1::CntWriterReflib::triggers)
      //.def("flush", &v1::CntWriterReflib::flush)
      .def("embed", &v1::CntWriterReflib::embed);
      // reader functionality
      //.def_property_readonly("commited", &v1::CntWriterReflib::commited)
      //.def("read_row_major", py::overload_cast<int64_t, int64_t>(&v1::CntWriterReflib::rangeRowMajor))
      //.def("read_column_major", py::overload_cast<int64_t, int64_t>(&v1::CntWriterReflib::rangeColumnMajor));


    // pyeep interface
    py::class_<libeep_reader> lr(m, "read_cnt", py::module_local()/*, py::dynamic_attr()*/);
    lr.def(py::init<const std::string&>())
      .def("get_channel_count", &libeep_reader::get_channel_count)
      .def("get_channel", &libeep_reader::get_channel)
      .def("get_sample_frequency", &libeep_reader::get_sample_frequency)
      .def("get_sample_count", &libeep_reader::get_sample_count)
      .def("get_samples", &libeep_reader::get_samples)
      .def("get_trigger_count", &libeep_reader::get_trigger_count)
      .def("get_trigger", &libeep_reader::get_trigger);

    py::class_<libeep_writer> lw(m, "write_cnt", py::module_local()/*, py::dynamic_attr()*/);
    lw.def(py::init<const std::string&, int, const std::vector<channel_v4>&, int>())
      .def("add_samples", &libeep_writer::add_samples)
      .def("close", &libeep_writer::close);
}

