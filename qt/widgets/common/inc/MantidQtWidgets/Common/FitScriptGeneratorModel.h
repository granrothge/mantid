// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorModel.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <string>
#include <utility>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

/**
 * This struct is used to store all data relating to a single domain to be
 * fitted. This includes the location of the domain (workspace Name & index),
 * and the fit range (start and end X).
 */
struct FitDomain {

  FitDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
            double startX, double endX);

  std::string m_workspaceName;
  WorkspaceIndex m_workspaceIndex;
  double m_startX;
  double m_endX;
};

/**
 * This class stores the domain data to be fitted to, and the multi domain
 * function relating to this domain data. This data is used to generate a python
 * script for complex Mantid fitting.
 */
class EXPORT_OPT_MANTIDQT_COMMON FitScriptGeneratorModel
    : public IFitScriptGeneratorModel {
public:
  FitScriptGeneratorModel();
  ~FitScriptGeneratorModel();

  void removeWorkspaceDomain(std::string const &workspaceName,
                             WorkspaceIndex workspaceIndex) override;
  void addWorkspaceDomain(std::string const &workspaceName,
                          WorkspaceIndex workspaceIndex, double startX,
                          double endX) override;

  [[nodiscard]] bool isStartXValid(std::string const &workspaceName,
                                   WorkspaceIndex workspaceIndex,
                                   double startX) const override;
  [[nodiscard]] bool isEndXValid(std::string const &workspaceName,
                                 WorkspaceIndex workspaceIndex,
                                 double endX) const override;

  void updateStartX(std::string const &workspaceName,
                    WorkspaceIndex workspaceIndex, double startX) override;
  void updateEndX(std::string const &workspaceName,
                  WorkspaceIndex workspaceIndex, double endX) override;

  void removeFunction(std::string const &workspaceName,
                      WorkspaceIndex workspaceIndex,
                      std::string const &function) override;
  void addFunction(std::string const &workspaceName,
                   WorkspaceIndex workspaceIndex,
                   std::string const &function) override;
  void setFunction(std::string const &workspaceName,
                   WorkspaceIndex workspaceIndex,
                   std::string const &function) override;
  Mantid::API::CompositeFunction_sptr
  getFunction(std::string const &workspaceName,
              WorkspaceIndex workspaceIndex) override;

  void updateParameterValue(std::string const &workspaceName,
                            WorkspaceIndex workspaceIndex,
                            std::string const &parameter,
                            double newValue) override;
  void updateAttributeValue(
      std::string const &workspaceName, WorkspaceIndex workspaceIndex,
      std::string const &attribute,
      Mantid::API::IFunction::Attribute const &newValue) override;
  void updateParameterTie(std::string const &workspaceName,
                          WorkspaceIndex workspaceIndex,
                          std::string const &parameter,
                          std::string const &tie) override;

private:
  [[nodiscard]] std::pair<double, double>
  xLimits(std::string const &workspaceName,
          WorkspaceIndex workspaceIndex) const;
  [[nodiscard]] std::pair<double, double>
  xLimits(Mantid::API::MatrixWorkspace_const_sptr const &workspace,
          WorkspaceIndex workspaceIndex) const;

  void addFunction(std::string const &workspaceName,
                   WorkspaceIndex workspaceIndex,
                   Mantid::API::IFunction_sptr const &function);

  [[nodiscard]] std::size_t
  findDomainIndex(std::string const &workspaceName,
                  WorkspaceIndex workspaceIndex) const;
  [[nodiscard]] std::vector<FitDomain>::const_iterator
  findWorkspaceDomain(std::string const &workspaceName,
                      WorkspaceIndex workspaceIndex) const;
  [[nodiscard]] bool hasWorkspaceDomain(std::string const &workspaceName,
                                        WorkspaceIndex workspaceIndex) const;

  void updateParameterTie(Mantid::API::IFunction_sptr const &function,
                          std::string const &parameter, std::string const &tie);

  void removeCompositeAtDomainIndex(std::size_t const &domainIndex);
  void clearCompositeAtDomainIndex(std::size_t const &domainIndex);

  void addEmptyCompositeAtDomainIndex(std::size_t const &domainIndex);

  [[nodiscard]] std::string nextAvailableDomainPrefix() const;

  [[nodiscard]] inline std::size_t numberOfDomains() const noexcept {
    return m_fitDomains.size();
  }

  std::vector<FitDomain> m_fitDomains;
  Mantid::API::MultiDomainFunction_sptr m_function;
};

} // namespace MantidWidgets
} // namespace MantidQt
