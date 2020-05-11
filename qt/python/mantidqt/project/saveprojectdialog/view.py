# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench

from qtpy.QtCore import Qt
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QDialog

from mantidqt.utils.qt import load_ui


class ProjectSaveDialogView(QDialog):

    def __init__(self):
        super(ProjectSaveDialogView, self).__init__()

        self.ui = load_ui(__file__, 'saveprojectdialog.ui', baseinstance=self)
        self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
        self.setModal(True)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

    def set_location(self, path):
        self.location_line_edit.setText(path)

    def get_location(self):
        return self.location_line_edit.text()

    def get_save_all_workspaces(self):
        return self.save_all_workspaces_radio_button.isChecked()

    def get_remember_selection(self):
        return self.remember_selection_checkbox.isChecked()
