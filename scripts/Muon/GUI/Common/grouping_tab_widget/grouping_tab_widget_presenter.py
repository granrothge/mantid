from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.observer_pattern import Observer, Observable

import Muon.GUI.Common.utilities.muon_file_utils as file_utils
import Muon.GUI.Common.utilities.xml_utils as xml_utils
import Muon.GUI.Common.utilities.algorithm_utils as algorithm_utils
from Muon.GUI.Common import thread_model
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper


class GroupingTabPresenter(object):
    """
    The grouping tab presenter is responsible for synchronizing the group and pair tables. It also maintains
    functionality which covers both groups/pairs ; e.g. loading/saving/updating data.
    """

    def __init__(self, view, model,
                 grouping_table_widget=None,
                 pairing_table_widget=None):
        self._view = view
        self._model = model

        self.grouping_table_widget = grouping_table_widget
        self.pairing_table_widget = pairing_table_widget

        # Synchronize the two tables
        self._view.on_grouping_table_changed(self.pairing_table_widget.update_view_from_model)
        self._view.on_pairing_table_changed(self.grouping_table_widget.update_view_from_model)

        self._view.set_description_text(self.text_for_description())
        self._view.on_add_pair_requested(self.add_pair_from_grouping_table)
        self._view.on_clear_grouping_button_clicked(self.on_clear_requested)
        self._view.on_update_button_clicked(self.handle_update_all_clicked)
        self._view.on_load_grouping_button_clicked(self.handle_load_grouping_from_file)
        self._view.on_save_grouping_button_clicked(self.handle_save_grouping_file)
        self._view.on_default_grouping_button_clicked(self.handle_default_grouping_button_clicked)

        # monitors for loaded data changing
        self.loadObserver = GroupingTabPresenter.LoadObserver(self)
        self.instrumentObserver = GroupingTabPresenter.InstrumentObserver(self)

        # notifiers
        self.groupingNotifier = GroupingTabPresenter.GroupingNotifier(self)
        self.grouping_table_widget.on_data_changed(self.group_table_changed)
        self.pairing_table_widget.on_data_changed(self.pair_table_changed)

        self.guessAlphaObserver = GroupingTabPresenter.GuessAlphaObserver(self)
        self.pairing_table_widget.guessAlphaNotifier.add_subscriber(self.guessAlphaObserver)

        self.update_thread = self.create_update_thread()

    def show(self):
        self._view.show()

    def text_for_description(self):
        """
        Generate the text for the description edit at the top of the widget.
        """
        instrument = self._model.instrument
        n_detectors = self._model.num_detectors
        main_field = self._model.main_field_direction
        text = "{} , {} detectors, main field : {} to muon polarization".format(
            instrument, n_detectors, main_field)
        return text

    def update_description_text(self, description_text=''):
        if not description_text:
            description_text = self.text_for_description()
        self._view.set_description_text(description_text)

    def add_pair_from_grouping_table(self, group_name1, group_name2):
        """
        If user requests to add a pair from the grouping table.
        """
        pair = self._model.construct_empty_pair_with_group_names(group_name1, group_name2)
        self._model.add_pair(pair)
        self.pairing_table_widget.update_view_from_model()

    def handle_guess_alpha(self, pair_name, group1_name, group2_name):
        """
        Calculate alpha for the pair for which "Guess Alpha" button was clicked.
        """
        ws1 = self._model.get_group_workspace(group1_name)
        ws2 = self._model.get_group_workspace(group2_name)

        ws = algorithm_utils.run_AppendSpectra(ws1, ws2)

        new_alpha = algorithm_utils.run_AlphaCalc({"InputWorkspace": ws,
                                                   "ForwardSpectra": [0],
                                                   "BackwardSpectra": [1]})
        self._model.update_pair_alpha(pair_name, new_alpha)
        self.pairing_table_widget.update_view_from_model()

        self.groupingNotifier.notify_subscribers()
        self.handle_update_all_clicked()

    def handle_load_grouping_from_file(self):
        # Only XML format
        file_filter = file_utils.filter_for_extensions(["xml"])
        filename = self._view.show_file_browser_and_return_selection(file_filter, [""])

        groups, pairs, description = xml_utils.load_grouping_from_XML(filename)

        self._model.clear()
        for group in groups:
            try:
                self._model.add_group(group)
            except ValueError as error:
                self._view.display_warning_box(str(error))

        for pair in pairs:
            if pair.forward_group in self._model.group_names and pair.backward_group in self._model.group_names:
                self._model.add_pair(pair)

        self.grouping_table_widget.update_view_from_model()
        self.pairing_table_widget.update_view_from_model()
        self.update_description_text(description)

        self.groupingNotifier.notify_subscribers()

    def disable_editing(self):
        self._view.set_buttons_enabled(False)
        self.grouping_table_widget.disable_editing()
        self.pairing_table_widget.disable_editing()

    def enable_editing(self, result=None):
        self._view.set_buttons_enabled(True)
        self.grouping_table_widget.enable_editing()
        self.pairing_table_widget.enable_editing()

    def calculate_all_data(self):
        self._model.show_all_groups_and_pairs()

    def handle_update_all_clicked(self):
        self.update_thread.threadWrapperSetUp(self.disable_editing,
                                              self.enable_editing,
                                              self._view.display_warning_box)
        self.update_thread.start()

    def handle_default_grouping_button_clicked(self):
        self._model.reset_groups_and_pairs_to_default()
        self.grouping_table_widget.update_view_from_model()
        self.pairing_table_widget.update_view_from_model()
        self.update_description_text()

        self.groupingNotifier.notify_subscribers()

    def on_clear_requested(self):
        self._model.clear()
        self.grouping_table_widget.update_view_from_model()
        self.pairing_table_widget.update_view_from_model()
        self.update_description_text()

        self.groupingNotifier.notify_subscribers()

    def handle_new_data_loaded(self):
        self._model._data.update_current_data()

        if self._model.is_data_loaded():
            self._model._data.show_raw_data()
            self._model._data.show_all_groups()
            self._model._data.show_all_pairs()
            self.grouping_table_widget.update_view_from_model()
            self.pairing_table_widget.update_view_from_model()
            self.update_description_text()
        else:
            self.on_clear_requested()

    def handle_save_grouping_file(self):
        filename = self._view.show_file_save_browser_and_return_selection()
        if filename != "":
            xml_utils.save_grouping_to_XML(self._model.groups, self._model.pairs, filename, description=self._view.get_description_text())

    def create_update_thread(self):
        self._update_model = ThreadModelWrapper(self.calculate_all_data)
        return thread_model.ThreadModel(self._update_model)
    # ------------------------------------------------------------------------------------------------------------------
    # Observer / Observable
    # ------------------------------------------------------------------------------------------------------------------

    def group_table_changed(self):
        self.groupingNotifier.notify_subscribers()

    def pair_table_changed(self):
        self.groupingNotifier.notify_subscribers()

    class LoadObserver(Observer):

        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, arg):
            self.outer.handle_new_data_loaded()

    class InstrumentObserver(Observer):

        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, arg):
            self.outer.on_clear_requested()

    class GuessAlphaObserver(Observer):

        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, arg):
            self.outer.handle_guess_alpha(arg[0], arg[1], arg[2])

    class GroupingNotifier(Observable):

        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer  # handle to containing class

        def notify_subscribers(self, *args, **kwargs):
            Observable.notify_subscribers(self, *args, **kwargs)
