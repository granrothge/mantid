from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import AppendSpectra, CloneWorkspace, ElasticWindow, LoadLog, Logarithm, SortXAxis, Transpose
from mantid.kernel import *
from mantid.api import *

import numpy as np


def _normalize_by_index(workspace, index):
    """
    Normalize each spectra of the specified workspace by the
    y-value at the specified index in that spectra.

    @param workspace    The workspace to normalize.
    @param index        The index of the y-value to normalize by.
    """

    num_hist = workspace.getNumberHistograms()

    # Normalize each spectrum in the workspace
    for idx in range(0, num_hist):
        y_vals = workspace.readY(idx)
        scale = 1.0 / y_vals[index]
        y_vals_scaled = scale * y_vals
        workspace.setY(idx, y_vals_scaled)


class ElasticWindowMultiple(DataProcessorAlgorithm):
    _sample_log_name = None
    _sample_log_value = None
    _input_workspaces = None
    _q_workspace = None
    _q2_workspace = None
    _elf_workspace = None
    _elt_workspace = None
    _integration_range_start = None
    _integration_range_end = None
    _background_range_start = None
    _background_range_end = None

    def category(self):
        return 'Workflow\\Inelastic;Inelastic\\Indirect'

    def summary(self):
        return 'Performs the ElasticWindow algorithm over multiple input workspaces'

    def PyInit(self):
        self.declareProperty(WorkspaceGroupProperty('InputWorkspaces', '', Direction.Input),
                             doc='Grouped input workspaces')

        self.declareProperty(name='IntegrationRangeStart', defaultValue=0.0,
                             doc='Start of integration range in time of flight')
        self.declareProperty(name='IntegrationRangeEnd', defaultValue=0.0,
                             doc='End of integration range in time of flight')

        self.declareProperty(name='BackgroundRangeStart', defaultValue=Property.EMPTY_DBL,
                             doc='Start of background range in time of flight')
        self.declareProperty(name='BackgroundRangeEnd', defaultValue=Property.EMPTY_DBL,
                             doc='End of background range in time of flight')

        self.declareProperty(name='SampleEnvironmentLogName', defaultValue='sample',
                             doc='Name of the sample environment log entry')

        sample_environment_log_values = ['last_value', 'average']
        self.declareProperty('SampleEnvironmentLogValue', 'last_value',
                             StringListValidator(sample_environment_log_values),
                             doc='Value selection of the sample environment log entry')

        self.declareProperty(WorkspaceProperty('OutputInQ', '', Direction.Output),
                             doc='Output workspace in Q')

        self.declareProperty(WorkspaceProperty('OutputInQSquared', '', Direction.Output),
                             doc='Output workspace in Q Squared')

        self.declareProperty(WorkspaceProperty('OutputELF', '', Direction.Output,
                                               PropertyMode.Optional),
                             doc='Output workspace ELF')

        self.declareProperty(WorkspaceProperty('OutputELT', '', Direction.Output,
                                               PropertyMode.Optional),
                             doc='Output workspace ELT')

    def validateInputs(self):
        issues = dict()

        background_range_start = self.getProperty('BackgroundRangeStart').value
        background_range_end = self.getProperty('BackgroundRangeEnd').value

        if background_range_start != Property.EMPTY_DBL and background_range_end == Property.EMPTY_DBL:
            issues['BackgroundRangeEnd'] = 'If background range start was given and ' \
                                           'background range end must also be provided.'

        if background_range_start == Property.EMPTY_DBL and background_range_end != Property.EMPTY_DBL:
            issues['BackgroundRangeStart'] = 'If background range end was given and background ' \
                                             'range start must also be provided.'

        return issues

    def _setup(self):
        """
        Gets algorithm properties.
        """
        self._sample_log_name = self.getPropertyValue('SampleEnvironmentLogName')
        self._sample_log_value = self.getPropertyValue('SampleEnvironmentLogValue')

        self._input_workspaces = self.getProperty('InputWorkspaces').value
        self._input_size = len(self._input_workspaces)
        self._elf_ws_name = self.getPropertyValue('OutputELF')
        self._elt_ws_name = self.getPropertyValue('OutputELT')

        self._integration_range_start = self.getProperty('IntegrationRangeStart').value
        self._integration_range_end = self.getProperty('IntegrationRangeEnd').value

        self._background_range_start = self.getProperty('BackgroundRangeStart').value
        self._background_range_end = self.getProperty('BackgroundRangeEnd').value

    def PyExec(self):
        from IndirectCommon import getInstrRun

        # Do setup
        self._setup()

        # Lists of input and output workspaces
        q_workspaces = list()
        q2_workspaces = list()
        run_numbers = list()
        sample_param = list()

        progress = Progress(self, 0.0, 0.05, 3)

        # Perform the ElasticWindow algorithms
        for input_ws in self._input_workspaces:
            logger.information('Running ElasticWindow for workspace: %s' % input_ws.getName())
            progress.report('ElasticWindow for workspace: %s' % input_ws.getName())

            q_workspace, q2_workspace = ElasticWindow(InputWorkspace=input_ws,
                                                      IntegrationRangeStart=self._integration_range_start,
                                                      IntegrationRangeEnd=self._integration_range_end,
                                                      BackgroundRangeStart=self._background_range_start,
                                                      BackgroundRangeEnd=self._background_range_end,
                                                      OutputInQ="__q", OutputInQSquared="__q2",
                                                      StoreInADS=False, EnableLogging=False)

            q2_workspace = Logarithm(InputWorkspace=q2_workspace, OutputWorkspace="__q2",
                                     StoreInADS=False, EnableLogging=False)

            q_workspaces.append(q_workspace)
            q2_workspaces.append(q2_workspace)

            # Get the run number
            run_no = getInstrRun(input_ws.getName())[1]
            run_numbers.append(run_no)

            # Get the sample environment unit
            sample, unit = self._get_sample_units(input_ws)
            if sample is not None:
                sample_param.append(sample)
            else:
                # No need to output a temperature workspace if there are no temperatures
                self._elt_ws_name = ''

        logger.information('Creating Q and Q^2 workspaces')
        progress.report('Creating Q workspaces')

        if self._input_size == 1:
            q_workspace = q_workspaces[0]
            q2_workspace = q2_workspaces[0]
        else:
            q_workspace = _append_all(q_workspaces)
            q2_workspace = _append_all(q2_workspaces)

        # Set the vertical axis units
        v_axis_is_sample = self._input_size == len(sample_param)

        if v_axis_is_sample:
            logger.notice('Vertical axis is in units of %s' % unit)
            unit = (self._sample_log_name, unit)

            def axis_value(index):
                return float(sample_param[index])
        else:
            logger.notice('Vertical axis is in run number')
            unit = ('Run No', 'last 3 digits')

            def axis_value(index):
                return float(run_numbers[index][-3:])

        # Create and set new vertical axis for the Q and Q**2 workspaces
        _set_numeric_y_axis(q_workspace, self._input_size, unit, axis_value)
        _set_numeric_y_axis(q2_workspace, self._input_size, unit, axis_value)

        progress.report('Creating ELF workspaces')

        # Process the ELF workspace
        if self._elf_ws_name != '':
            logger.information('Creating ELF workspace')
            elf_workspace = sort_x_axis(self._transpose(q_workspace))
            self.setProperty('OutputELF', elf_workspace)

        # Do temperature normalisation
        if self._elt_ws_name != '':
            logger.information('Creating ELT workspace')

            # If the ELF workspace was not created, create the ELT workspace
            # from the Q workspace. Else, clone the ELF workspace.
            if self._elf_ws_name == '':
                elt_workspace = sort_x_axis(self._transpose(q_workspace))
            else:
                elt_workspace = CloneWorkspace(InputWorkspace=elf_workspace, OutputWorkspace="__cloned",
                                               StoreInADS=False, EnableLogging=False)

            _normalize_by_index(elt_workspace, np.argmin(sample_param))

            self.setProperty('OutputELT', elt_workspace)

        # Set the output workspace
        self.setProperty('OutputInQ', q_workspace)
        self.setProperty('OutputInQSquared', q2_workspace)

    def _get_sample_units(self, workspace):
        """
        Gets the sample environment units for a given workspace.

        @param workspace The workspace
        @returns sample in given units or None if not found
        """
        from IndirectCommon import getInstrRun

        instr, run_number = getInstrRun(workspace.getName())

        instrument = config.getFacility().instrument(instr)
        pad_num = instrument.zeroPadding(int(run_number))
        zero_padding = '0' * (pad_num - len(run_number))

        run_name = instr + zero_padding + run_number
        log_filename = run_name.upper() + '.log'

        run = workspace.getRun()

        if self._sample_log_name == 'Position':
            self._sample_log_name = _extract_sensor_name(self._sample_log_name, run, instrument)

        if self._sample_log_name in run:
            # Look for sample unit in logs in workspace
            if self._sample_log_value == 'last_value':
                sample = run[self._sample_log_name].value[-1]
            else:
                sample = run[self._sample_log_name].value.mean()

            unit = run[self._sample_log_name].units
        else:
            # Logs not in workspace, try loading from file
            logger.information('Log parameter not found in workspace. Searching for log file.')
            sample, unit = _extract_temperature_from_log(workspace, self._sample_log_name, log_filename, run_name)

        if sample is not None and unit is not None:
            logger.debug('%d %s found for run: %s' % (sample, unit, run_name))
        else:
            logger.warning('No sample units found for run: %s' % run_name)
        return sample, unit


def _extract_temperature_from_log(workspace, sample_log_name, log_filename, run_name):
    log_path = FileFinder.getFullPath(log_filename)

    if not log_path:
        logger.warning('Log file for run %s not found' % run_name)
        return None, None

    LoadLog(Workspace=workspace, Filename=log_filename, EnableLogging=False)
    run = workspace.getRun()

    if sample_log_name in run:
        temperature = run[sample_log_name].value[-1]
        unit = run[sample_log_name].units
        return temperature, unit

    logger.warning('Log entry %s for run %s not found' % (sample_log_name, run_name))
    return None, None


def _extract_sensor_name(sample_log_name, run, instrument):
    sensor_names = instrument.getStringParameter("Workflow.TemperatureSensorNames")

    if sample_log_name in run:
        position = run[sample_log_name].value[-1]

        if position < len(sensor_names):
            return sensor_names[position]
        else:
            logger.warning('Invalid position (' + str(position) + ') found in workspace.')
    else:
        logger.information('Position not found in workspace.')


def _set_numeric_y_axis(workspace, length, unit, get_axis_value):
    workspace_axis = NumericAxis.create(length)
    workspace_axis.setUnit("Label").setLabel(unit[0], unit[1])

    for index in range(length):
        workspace_axis.setValue(index, get_axis_value(index))
    workspace.replaceAxis(1, workspace_axis)


def _append_all(workspaces):
    initial_workspace = workspaces[0]

    for workspace in workspaces[1:]:
        initial_workspace = _append_spectra(initial_workspace, workspace)
    return initial_workspace


def _append_spectra(workspace1, workspace2):
    return AppendSpectra(InputWorkspace1=workspace1, InputWorkspace2=workspace2,
                         OutputWorkspace="__appended", StoreInADS=False, EnableLogging=False)


def _transpose(workspace):
    return Transpose(InputWorkspace=workspace, OutputWorkspace="__transposed",
                     StoreInADS=False, EnableLogging=False)


def _sort_x_axis(workspace):
    return SortXAxis(InputWorkspace=workspace, OutputWorkspace="__sorted",
                     StoreInADS=False, EnableLogging=False)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ElasticWindowMultiple)
