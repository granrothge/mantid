// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/SliceViewer/PeaksPresenter.h"
#include <QDialog>

class QAbstractButton;
namespace Ui {
class PeaksViewerOverlayDialog;
}

namespace MantidQt {
namespace SliceViewer {
class PeaksViewerOverlayDialog : public QDialog {
  Q_OBJECT

public:
  explicit PeaksViewerOverlayDialog(const PeaksPresenter_sptr &peaksPresenter, QWidget *parent = nullptr);
  ~PeaksViewerOverlayDialog() override;

  void closeEvent(QCloseEvent * /*unused*/) override;
  void reject() override;

private slots:

  void onSliderIntoProjectionMoved(int value);
  void onSliderOnProjectionMoved(int value);
  void onReset();
  void onCompleteClicked(QAbstractButton *button);
  void onHelp();

private:
  Ui::PeaksViewerOverlayDialog *ui;
  PeaksPresenter_sptr m_peaksPresenter;

  double m_originalOnProjectionFraction;
  double m_originalIntoProjectionFraction;
};
} // namespace SliceViewer
} // namespace MantidQt
