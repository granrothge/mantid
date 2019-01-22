# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#


class ObservingView:
    """
    This class provides some common functions needed across views observing the ADS.
    It runs the close_signal so that the view is closed from the GUI thread,
    and ensures that the closeEvent will clear the observer to prevent a memory leak.

    This class shouldn't inherit from a QObject/QWidget, otherwise the closeEvent
    doesn't replace the closeEvent of the view that is inheriting this.
    """

    def __init__(self, _):
        pass

    def close_later(self):
        """
        Emits a close signal to the main GUI thread that triggers the built-in close method.

        This function can be called from outside the main GUI thread. It is currently
        used to close the window when the relevant workspace is deleted.

        When the signal is emitted, the execution returns to the GUI thread, and thus
        GUI code can be executed.
        """
        self.close_signal.emit()

    def closeEvent(self, event):
        # This close prevents a leak when the window is closed from X by the user
        self.presenter.clear_observer()
        event.accept()
