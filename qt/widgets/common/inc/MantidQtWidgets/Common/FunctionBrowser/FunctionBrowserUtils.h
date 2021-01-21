// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/IFunction_fwd.h"
#include "MantidQtWidgets/Common/DllOption.h"

#include <QString>
#include <boost/optional.hpp>

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

/// Split a qualified parameter name into function index and local parameter
/// name.
/// @param paramName :: Fully qualified parameter name (includes function
/// prefix)
///   for example: f0.f1.A0
/// @return :: A pair with the first item is the function index and the
/// second item is the param local name.
EXPORT_OPT_MANTIDQT_COMMON std::pair<QString, QString> splitParameterName(const QString &paramName);

/// Get a child function of a parent function whose parameters start with a
/// given prefix.
/// @param prefix :: A prefix of the form f0.f1. If en empty string is given
/// then the parent function is returned.
/// @param fun :: The parent function.
EXPORT_OPT_MANTIDQT_COMMON IFunction_sptr getFunctionWithPrefix(const QString &prefix, const IFunction_sptr &fun);

/// Split a function (eg f0.f3.f1.) into the parent prefix (f0.f3.) and the
/// index of the child function (1).
EXPORT_OPT_MANTIDQT_COMMON std::pair<QString, int> splitFunctionPrefix(const QString &prefix);

/// Split a constraint definition into a parameter name and a pair of bounds,
/// for example -1 < f0.A1 < 2 ==> (f0.A1, (-1, 2))
EXPORT_OPT_MANTIDQT_COMMON std::pair<QString, std::pair<QString, QString>>
splitConstraintString(const QString &constraint);

class ScopedFalse {
  bool &m_ref;
  bool m_oldValue;

public:
  ScopedFalse(bool &variable) : m_ref(variable), m_oldValue(variable) { m_ref = false; }
  ~ScopedFalse() { m_ref = m_oldValue; }
};

} // namespace MantidWidgets
} // namespace MantidQt
