#include "MantidKernel/ConfigObserver.h"
#include "MantidPythonInterface/kernel/Environment/GlobalInterpreterLock.h"
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/pure_virtual.hpp>

using namespace boost::python;
using Mantid::Kernel::ConfigObserver;
using Mantid::PythonInterface::Environment::GlobalInterpreterLock;

class ConfigObserverWrapper : public ConfigObserver,
                              public wrapper<ConfigObserver> {
public:
  using ConfigObserver::ConfigObserver;
  using ConfigObserver::notifyValueChanged;

  void onValueChanged(const std::string &name, const std::string &newValue,
                      const std::string &prevValue) override {
    GlobalInterpreterLock lock;
    auto onValueChangedOverride = this->get_override("onValueChanged");
    onValueChangedOverride(name, newValue, prevValue);
  }
};

void export_ConfigObserver() {
  class_<ConfigObserverWrapper, boost::noncopyable>("ConfigObserver")
      .def("onValueChanged",
           pure_virtual(&ConfigObserverWrapper::onValueChanged));
}
