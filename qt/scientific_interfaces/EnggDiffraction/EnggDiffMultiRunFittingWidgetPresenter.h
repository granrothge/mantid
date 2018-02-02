#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTER_H_

#include "DllConfig.h"
#include "IEnggDiffMultiRunFittingWidgetModel.h"
#include "IEnggDiffMultiRunFittingWidgetPresenter.h"
#include "IEnggDiffMultiRunFittingWidgetView.h"

#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffMultiRunFittingWidgetPresenter
    : public IEnggDiffMultiRunFittingWidgetPresenter {

public:
  EnggDiffMultiRunFittingWidgetPresenter(
      std::unique_ptr<IEnggDiffMultiRunFittingWidgetModel> model,
      boost::shared_ptr<IEnggDiffMultiRunFittingWidgetView> view);

  void addFittedPeaks(const RunLabel &runLabel,
                      const Mantid::API::MatrixWorkspace_sptr ws) override;

  void addFocusedRun(const Mantid::API::MatrixWorkspace_sptr ws) override;

  boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFittedPeaks(const RunLabel &runLabel) const override;

  boost::optional<Mantid::API::MatrixWorkspace_sptr>
  getFocusedRun(const RunLabel &runLabel) const override;

  RunLabel getSelectedRunLabel() const override;

  std::unique_ptr<IEnggDiffMultiRunFittingWidgetAdder>
  getWidgetAdder() const override;

  void
  notify(IEnggDiffMultiRunFittingWidgetPresenter::Notification notif) override;

  bool showFitResultsSelected() const override;

private:
  void processSelectRun();
  void processPlotPeaksStateChanged();

  /// Display fitted peaks and any other fit information for a certain run
  void displayFitResults(const RunLabel &runLabel);

  /// Update the plot area with a focused run, and its fitted peaks if available
  /// and requested
  void updatePlot(const RunLabel &runLabel);

  std::unique_ptr<IEnggDiffMultiRunFittingWidgetModel> m_model;

  boost::shared_ptr<IEnggDiffMultiRunFittingWidgetView> m_view;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETPRESENTER_H_
