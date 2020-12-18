// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <iterator>
#include <stdexcept>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("FitScriptGenerator");

IFunction_sptr createIFunction(std::string const &functionString) {
  return FunctionFactory::Instance().createInitialized(functionString);
}

std::vector<std::string> splitStringBy(std::string const &str,
                                       std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  subStrings.erase(std::remove_if(subStrings.begin(), subStrings.end(),
                                  [](std::string const &subString) {
                                    return subString.empty();
                                  }),
                   subStrings.cend());
  return subStrings;
}

std::size_t getPrefixIndexAt(std::string const &functionPrefix,
                             std::size_t const &index) {
  auto const subStrings = splitStringBy(functionPrefix, "f.");
  return std::stoull(subStrings[index]);
}

std::string removeTopFunctionIndex(std::string const &functionPrefix) {
  auto resultPrefix = functionPrefix;
  auto const firstDotIndex = resultPrefix.find(".");
  if (firstDotIndex != std::string::npos)
    resultPrefix.erase(0, firstDotIndex + 1);
  return resultPrefix;
}

bool isTieNumber(std::string const &tie) {
  return !tie.empty() &&
         tie.find_first_not_of("0123456789.-") == std::string::npos;
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {

FitScriptGeneratorModel::FitScriptGeneratorModel() : m_fitDomains() {}

FitScriptGeneratorModel::~FitScriptGeneratorModel() {}

void FitScriptGeneratorModel::removeWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex) {
  auto const removeIter = findWorkspaceDomain(workspaceName, workspaceIndex);
  if (removeIter != m_fitDomains.cend()) {
    auto const removeIndex = std::distance(m_fitDomains.cbegin(), removeIter);

    if (removeIter != m_fitDomains.cend())
      m_fitDomains.erase(removeIter);
  }
}

void FitScriptGeneratorModel::addWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    double startX, double endX) {
  if (hasWorkspaceDomain(workspaceName, workspaceIndex))
    throw std::invalid_argument("The '" + workspaceName + " (" +
                                std::to_string(workspaceIndex.value) +
                                ")' domain already exists.");

  m_fitDomains.emplace_back(
      FitDomain(workspaceName, workspaceIndex, startX, endX));
}

std::size_t
FitScriptGeneratorModel::findDomainIndex(std::string const &workspaceName,
                                         WorkspaceIndex workspaceIndex) const {
  auto const iter = findWorkspaceDomain(workspaceName, workspaceIndex);
  if (iter != m_fitDomains.cend())
    return std::distance(m_fitDomains.cbegin(), iter);

  throw std::invalid_argument("The domain '" + workspaceName + " (" +
                              std::to_string(workspaceIndex.value) +
                              ")' could not be found.");
}

std::vector<FitDomain>::const_iterator
FitScriptGeneratorModel::findWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex) const {
  auto const isMatch = [&](FitDomain const &fitDomain) {
    return fitDomain.workspaceName() == workspaceName &&
           fitDomain.workspaceIndex() == workspaceIndex;
  };

  return std::find_if(m_fitDomains.cbegin(), m_fitDomains.cend(), isMatch);
}

bool FitScriptGeneratorModel::hasWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex) const {
  return findWorkspaceDomain(workspaceName, workspaceIndex) !=
         m_fitDomains.end();
}

bool FitScriptGeneratorModel::updateStartX(std::string const &workspaceName,
                                           WorkspaceIndex workspaceIndex,
                                           double startX) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return m_fitDomains[domainIndex].setStartX(startX);
}

bool FitScriptGeneratorModel::updateEndX(std::string const &workspaceName,
                                         WorkspaceIndex workspaceIndex,
                                         double endX) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return m_fitDomains[domainIndex].setEndX(endX);
}

void FitScriptGeneratorModel::removeFunction(std::string const &workspaceName,
                                             WorkspaceIndex workspaceIndex,
                                             std::string const &function) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex].removeFunction(function);
}

void FitScriptGeneratorModel::addFunction(std::string const &workspaceName,
                                          WorkspaceIndex workspaceIndex,
                                          std::string const &function) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex].addFunction(createIFunction(function));
}

void FitScriptGeneratorModel::setFunction(std::string const &workspaceName,
                                          WorkspaceIndex workspaceIndex,
                                          std::string const &function) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex].setFunction(createIFunction(function));
}

IFunction_sptr
FitScriptGeneratorModel::getFunction(std::string const &workspaceName,
                                     WorkspaceIndex workspaceIndex) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return m_fitDomains[domainIndex].getFunction();
}

void FitScriptGeneratorModel::updateParameterValue(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &parameter, double newValue) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex].setParameterValue(removeTopFunctionIndex(parameter),
                                              newValue);
}

void FitScriptGeneratorModel::updateAttributeValue(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &attribute, IFunction::Attribute const &newValue) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex].setAttributeValue(removeTopFunctionIndex(attribute),
                                              newValue);
}

void FitScriptGeneratorModel::updateParameterTie(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &parameter, std::string const &tie) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);

  auto const tieSplit = splitStringBy(tie, "=");
  auto const tieValue = tieSplit.size() > 1 ? tieSplit[1] : tieSplit[0];

  auto const tieExpression =
      isTieNumber(tieValue) ? tieValue : removeTopFunctionIndex(tieValue);

  m_fitDomains[domainIndex].updateParameterTie(
      removeTopFunctionIndex(parameter), tieExpression);

  // if (auto composite = toComposite(m_function->getFunction(domainIndex))) {

  //  if (composite->nFunctions() == 1) {
  //    auto function = composite->getFunction(0);
  //    if (function->hasParameter(parameter))
  //      updateParameterTie(function, parameter, tie);
  //  } else {
  //    if (composite->hasParameter(parameter))
  //      updateParameterTie(composite, parameter, tie);
  //  }
  //}
}

void FitScriptGeneratorModel::updateParameterTie(IFunction_sptr const &function,
                                                 std::string const &parameter,
                                                 std::string const &tie) {
  // if (tie.empty()) {
  //  function->removeTie(function->parameterIndex(parameter));
  //} else {
  //  auto const tieSplit = splitStringBy(tie, "=");
  //  auto const tieValue = tieSplit.size() > 1 ? tieSplit[1] : tieSplit[0];

  //  try {
  //    function->tie(parameter, tieValue);
  //  } catch (std::invalid_argument const &ex) {
  //    g_log.error(ex.what());
  //  } catch (std::runtime_error const &ex) {
  //    g_log.error(ex.what());
  //  }
  //}
}

} // namespace MantidWidgets
} // namespace MantidQt
