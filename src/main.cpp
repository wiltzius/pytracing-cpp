#include <pybind11/pybind11.h>
#include <fstream>
#include <string>
#include <iostream>
#include <python/frameobject.h>
#include <time.h>
#include <stdio.h>
#include <chrono>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

namespace py = pybind11;

// barebones, very un-safe JSON serialization tools

const std::string begin_collection = "[";
const std::string end_collection = "{}]";
const std::string collection_delimiter = ",";
const std::string begin_dict = "{";
const std::string end_dict = "}";
const std::string begin_dict_item = "\"";
const std::string end_dict_item = "\"" + collection_delimiter;

std::string quotes(std::string string_)
{
  return "\"" + string_ + "\"";
}

std::string dict_item_final(std::string key, std::string val)
{
  return quotes(key) + ":" + quotes(val);
}

std::string dict_item(std::string key, std::string val)
{
  return dict_item_final(key, val) + ",";
}

std::string dict_item(std::string key, int val)
{
  return quotes(key) + ":" + std::to_string(val) + ",";
}

std::string dict_item(std::string key, long long val)
{
  return quotes(key) + ":" + std::to_string(val) + ",";
}

// internal representation of a Python frame; only the parts we care about

typedef struct FrameInfoSubset
{
  std::string event_type;
  std::string func_name;
  std::string func_filename;
  int func_line_no;
  std::string caller_filename;
  int caller_line_no;
  long long timestamp;
  pid_t pid;
  pid_t tid;
} FrameInfoSubset;

// tracer function itself
auto myDict = std::unordered_map<std::string, std::string>{{"call", "B"}, {"return", "E"}};

class Profiler
{
public:
  Profiler(py::str filename)
  {
    fd.open(filename);
    fd << begin_collection;
  }

  ~Profiler()
  {
    fd << end_collection << std::endl;
  }

  void tracer(py::object frame, py::str event_type, py::object arg)
  {
    if (std::string(event_type) == "call" || std::string(event_type) == "return")
    {
      if (py::object(frame.attr("f_back")) != (Py_None))
      {
        FrameInfoSubset *frame_info = new FrameInfoSubset{
            event_type,
            py::str(frame.attr("f_code").attr("co_name")),
            py::str(frame.attr("f_code").attr("co_filename")),
            py::int_(frame.attr("f_lineno")),
            py::str(frame.attr("f_back").attr("f_code").attr("co_filename")),
            py::int_(frame.attr("f_back").attr("f_lineno")),
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count()};
        ::getpid(); // todo this returns 0, maybe due to process namespace? Could bind the information from "outside" (ie from Python)
        syscall(SYS_gettid);
        serialize(frame_info);
      }
    }
  }

private:
  std::ofstream fd;

  std::string serialize_item(FrameInfoSubset *frameinfo)
  {
    // trace event format:
    // https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview#
    return begin_dict +
           dict_item("name", frameinfo->func_name) +
           dict_item("cat", frameinfo->func_filename) +
           dict_item("tid", frameinfo->tid) +
           dict_item("ph", myDict[frameinfo->event_type]) +
           dict_item("pid", frameinfo->pid) +
           dict_item("ts", frameinfo->timestamp) +
           "\"args\":" +
           begin_dict +
           dict_item("function", frameinfo->func_filename +
                                     ":" + std::to_string(frameinfo->func_line_no) +
                                     ":" + frameinfo->func_name) +
           dict_item_final("caller", frameinfo->caller_filename +
                                         ":" +
                                         std::to_string(frameinfo->caller_line_no)) +
           end_dict +
           end_dict;
  }

  void serialize(FrameInfoSubset *frameinfo)
  {
    fd << serialize_item(frameinfo) + ",";
    free(frameinfo);
  }
};

PYBIND11_MODULE(python_example, m)
{
  m.doc() = R"pbdoc(
        Pybind11 example plugin
        -----------------------

        .. currentmodule:: python_example

        .. autosummary::
           :toctree: _generate

           add
           subtract
    )pbdoc";

  py::class_<Profiler>(m, "Profiler")
      .def(py::init<py::str>())
      .def("tracer", &Profiler::tracer);

#ifdef VERSION_INFO
  m.attr("__version__") = VERSION_INFO;
#else
  m.attr("__version__") = "dev";
#endif
}
