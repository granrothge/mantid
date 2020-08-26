// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

#include "DllConfig.h"
#include "IALCDataLoadingView.h"

#include "ui_ALCDataLoadingView.h"

namespace MantidQt {
namespace MantidWidgets {
class LogValueSelector;
} // namespace MantidWidgets
} // namespace MantidQt

namespace MantidQt {
namespace CustomInterfaces {

/** ALCDataLoadingView : ALC Data Loading view interface implementation using Qt
  widgets
*/

class MANTIDQT_MUONINTERFACE_DLL ALCDataLoadingView
    : public IALCDataLoadingView {
public:
  ALCDataLoadingView(QWidget *widget);
  ~ALCDataLoadingView();

  // -- IALCDataLoadingView interface
  // ------------------------------------------------------------

  void initialize() override;

  std::string firstRun() const override;
  std::string lastRun() const override;
  std::vector<std::string> getRuns() const override;
  std::string log() const override;
  std::string function() const override;
  std::string deadTimeType() const override;
  std::string deadTimeFile() const override;
  std::string detectorGroupingType() const override;
  std::string getForwardGrouping() const override;
  std::string getBackwardGrouping() const override;
  std::string redPeriod() const override;
  std::string greenPeriod() const override;
  bool subtractIsChecked() const override;
  std::string calculationType() const override;
  boost::optional<std::pair<double, double>> timeRange() const override;

  void setDataCurve(Mantid::API::MatrixWorkspace_sptr workspace,
                    std::size_t const &workspaceIndex = 0) override;
  void displayError(const std::string &error) override;
  void setAvailableLogs(const std::vector<std::string> &logs) override;
  void setAvailablePeriods(const std::vector<std::string> &periods) override;
  void setTimeLimits(double tMin, double tMax) override;
  void setTimeRange(double tMin, double tMax) override;
  void help() override;
  void disableAll() override;
  void enableAll() override;
  void checkBoxAutoChanged(int state) override;
  //void updateRunsTextFromAuto() override;
  std::string getCurrentRunsText() const override;
  void setRunsTextWithSearch(const QString &text) override;
  std::string getRunsOldInput() const override;
  void setRunsOldInput(const std::string &oldInput) override;

  void setCurrentAutoRun(const int run) override { 
    m_currentAutoRun = run;
  }

  int extractRunNumber(const std::string &file) override;

  // -- End of IALCDataLoadingView interface
  // -----------------------------------------------------

private:
  /// Common function to set available items in a combo box
  void setAvailableItems(QComboBox *comboBox,
                         const std::vector<std::string> &items);

  /// UI form
  Ui::ALCDataLoadingView m_ui;

  /// The widget used
  QWidget *const m_widget;

  /// The currently found last run when auto checked, -1 if not found
  int m_currentAutoRun;

  /// The input saved when auto checkbox checked
  std::string m_oldInput;
};

} // namespace CustomInterfaces
} // namespace MantidQt
