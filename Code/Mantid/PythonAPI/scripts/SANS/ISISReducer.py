"""
    ISIS-specific implementation of the SANS Reducer. 
    
    WARNING: I'm still playing around with the ISIS reduction to try to 
    understand what's happening and how best to fit it in the Reducer design. 
     
"""
from SANSReducer import SANSReducer
from Reducer import ReductionStep
import SANSReductionSteps
import ISISReductionSteps
import SANSUtility
from mantidsimple import *

## Version number
__version__ = '0.0'

class ISISReducer(SANSReducer):
    """
        ISIS Reducer
        TODO: need documentation for all the data member
        TODO: need to see whether all those data members really belong here
    """
    CENT_FIND_RMIN = None
    CENT_FIND_RMAX = None
    
    #the rebin parameters used by Q1D
    Q_REBIN = None
    QXY2 = None
    DQY = None

    BACKMON_START = None
    BACKMON_END = None

    # Component positions
    PHIMIN=-90.0
    PHIMAX=90.0
    PHIMIRROR=True
    
    SPECMIN = None
    SPECMAX = None

    ## Path for user settings files
    _user_file_path = '.'
    
    def __init__(self):
        SANSReducer.__init__(self)

        self._init_steps()
        self.wksp_name = None
        self.full_trans_wav = True
        self._monitor_set = False


    def _to_steps(self):
        """
            Defines the steps that are run and their order
        """
#        self._reduction_steps.append(self.data_loader)
#        self._reduction_steps.append(self.user_settings)
        self._reduction_steps.append(self.place_det_sam)
        self._reduction_steps.append(self.geometry)
        self._reduction_steps.append(self._out_name)
        #---- the can special reducer uses the steps starting with the next one
        self._reduction_steps.append(self.flood_file)
        self._reduction_steps.append(self.crop_detector)
        self._reduction_steps.append(self.samp_trans_load)
        self._reduction_steps.append(self.mask)
        self._reduction_steps.append(self.to_wavelen)
        self._reduction_steps.append(self.norm_mon)
        self._reduction_steps.append(self.transmission_calculator)
        self._reduction_steps.append(self._corr_and_scale)
        self._reduction_steps.append(self._geo_corr)
        self._reduction_steps.append(self.to_Q)
        #---- the can special reducer ends on the previous step
        self._reduction_steps.append(self.background_subtracter)
        self._reduction_steps.append(self._zero_errors)
        self._reduction_steps.append(self._rem_zeros)
        
        #if set to an integer only that period will be extracted from the run file and processed 
        self._period_num = None

    def _init_steps(self):
        """
            Initialises the steps that are not initialised by (ISIS)CommandInterface.
        """
        #_to_steps() defines the order the steps are run in, any steps not in that list wont be run  
        
        self.data_loader =     None
        self.user_settings =   None
        self.place_det_sam =   ISISReductionSteps.MoveComponents()
        self.geometry =       SANSReductionSteps.GetSampleGeom()
        self._out_name =       ISISReductionSteps.GetOutputName()
        self.flood_file =      ISISReductionSteps.CorrectToFileISIS(
            '', 'SpectrumNumber','Divide', self._out_name.name_holder)
        self.crop_detector =   ISISReductionSteps.CropDetBank(
            self._out_name.name_holder)
        self.samp_trans_load = None
        self.can_trans_load =  None
        self.mask =self._mask= ISISReductionSteps.Mask_ISIS()
        self.to_wavelen =      ISISReductionSteps.UnitsConvert('Wavelength')
        self.norm_mon =        ISISReductionSteps.NormalizeToMonitor()
        self.transmission_calculator =\
                               ISISReductionSteps.TransmissionCalc(loader=None)
        self._corr_and_scale = ISISReductionSteps.ISISCorrections()
        self.to_Q =            ISISReductionSteps.ConvertToQ()
        self.background_subtracter = None
        self._geo_corr =       SANSReductionSteps.SampleGeomCor(self.geometry)
        self._zero_errors =    ISISReductionSteps.ReplaceErrors()
        self._rem_zeros =      SANSReductionSteps.StripEndZeros()
        
        #if set to an integer only that period will be extracted from the run file and processed 
        self._period_num = None

    def pre_process(self): 
        """
            Reduction steps that are meant to be executed only once per set
            of data files. After this is executed, all files will go through
            the list of reduction steps.
        """
        self._to_steps()

    def set_user_path(self, path):
        """
            Set the path for user files
            @param path: user file path
        """
        if os.path.isdir(path):
            self._user_file_path = path
        else:
            raise RuntimeError, "ISISReducer.set_user_path: provided path is not a directory (%s)" % path

    def get_user_path(self):
        return self._user_file_path
    
    user_file_path = property(get_user_path, set_user_path, None, None)

    def load_set_options(self, reload=True, period=-1):
        if not issubclass(self.data_loader.__class__, ISISReductionSteps.LoadSample):
            raise RuntimeError, "ISISReducer.load_set_options: method called with wrong loader class"
        self.data_loader.set_options(reload, period)
        if period > 0:
            self._period_num = period

    def append_data_file(self, data_file, workspace=None):
        """
            Append a file to be processed.
            @param data_file: name of the file to be processed
            @param workspace: optional name of the workspace for this data,
                default will be the name of the file 
        """
        wrkspc, _, _, filename = ISISReductionSteps.extract_workspace_name(
                            data_file, False, self.instrument.name(), self.instrument.run_number_width)
        if workspace is None:
            workspace = wrkspc
            
        self._full_file_path(filename)
        
        self._data_files.clear()
        self._data_files[workspace] = workspace

    def set_background(self, can_run=None, reload = True, period = -1):
        """
            Sets the can data to be subtracted from sample data files
            @param data_file: Name of the can run file
        """
        if can_run is None:
            self.background_subtracter = None
        else:
            self.background_subtracter = ISISReductionSteps.CanSubtraction(can_run, reload=reload, period=period)

    def set_trans_fit(self, lambda_min=None, lambda_max=None, fit_method="Log"):
        self.transmission_calculator.set_trans_fit(lambda_min, lambda_max, fit_method, override=True)
        self.transmission_calculator.enabled = True
        
    def set_trans_sample(self, sample, direct, reload=True, period=-1):
        if not issubclass(self.samp_trans_load.__class__, SANSReductionSteps.BaseTransmission):
            self.samp_trans_load = ISISReductionSteps.LoadTransmissions()
        self.samp_trans_load.set_run(sample, direct, reload, period)
        self.transmission_calculator.set_loader(self.samp_trans_load)

    def set_trans_can(self, can, direct, reload = True, period = -1):
        if not issubclass(self.can_trans_load.__class__, SANSReductionSteps.BaseTransmission):
            self.can_trans_load = ISISReductionSteps.LoadTransmissions(is_can=True)
        self.can_trans_load.set_run(can, direct, reload, period)
        
    def set_monitor_spectrum(self, specNum, interp=False, override=True):
        if override:
            self._monitor_set=True
        
        if not self._monitor_set or override:
            self.instrument.set_incident_mon(specNum)
        #if interpolate is stated once in the file, that is enough it wont be unset (until a file is loaded again)
        if override:
            self.instrument.set_interpolating_norm(interp)
        elif interp :
            self.instrument.set_interpolating_norm()
                        
    def suggest_monitor_spectrum(self, specNum, interp=False):
        if not self._monitor_set:
            self.instrument.suggest_incident_mntr(specNum)
        #if interpolate is stated once in the file, that is enough it wont be unset (until a file is loaded again)
        if interp :
            self.instrument.suggest_interpolating_norm()
                    
    def set_trans_spectrum(self, specNum, interp=False):
        self.instrument.incid_mon_4_trans_calc = int(specNum)
        #if interpolate is stated once in the file, that is enough it wont be unset (until a file is loaded again)
        if interp :
            self.instrument.use_interpol_trans_calc = True                    
                      
    def set_process_single_workspace(self, wk_name):
        """
            Clears the list of data files and inserts one entry
            to the workspace whose name is given
        """
        self._data_files = {'dummy' : wk_name}

    def _reduce(self):
        """
            Go through the list of reduction steps
        """
        # Check that an instrument was specified
        if self.instrument is None:
            raise RuntimeError, "Reducer: trying to run a reduction without an instrument specified"

        # Go through the list of steps that are common to all data files
        self.pre_process()

        #self._data_files[final_workspace] = self._data_files[file_ws]
        #del self._data_files[file_ws]
        #----> can_setup.setReducedWorkspace(tmp_can)
        
        #Correct(sample_setup, wav_start, wav_end, use_def_trans, finding_centre)
        self.run_steps(start_ind=0, stop_ind=len(self._reduction_steps))

        #any clean up, possibly removing workspaces 
        self.post_process()
        return self.wksp_name

    def run_steps(self, start_ind = None, stop_ind = None):
        """
            Run part of the chain, starting at the first specified step
            and ending at the last. If start or finish are set to None
            they will default to the first and last steps in the chain
            respectively. No pre- or post-processing is done. Assumes
            there are no duplicated steps
            @param start_ind the index number of the first step to run
            @param end_ind the index of the last step that will be run
        """
        if start_ind is None:
            start_ind = 0

        if stop_ind is None:
            stop_ind = len(self._reduction_steps)

        for file_ws in self._data_files:
            self.wksp_name = self._data_files.values()[0]
            for item in self._reduction_steps[start_ind:stop_ind+1]:
                if not item is None:
                    item.execute(self, self.wksp_name)


                #TODO: change the following
                finding_centre = False
        
                    
                if finding_centre:
                    self.final_workspace = file_ws.split('_')[0] + '_quadrants'
                 
                # Crop Workspace to remove leading and trailing zeroes
                #TODO: deal with this once we have the final workspace name sorted out
                if finding_centre:
                    quadrants = {1:'Left', 2:'Right', 3:'Up',4:'Down'}
                    for key, value in quadrants.iteritems():
                        old_name = self.final_workspace + '_' + str(key)
                        RenameWorkspace(old_name, value)


    def post_process(self):
        # Store the mask file within the final workspace so that it is saved to the CanSAS file
        if self.user_settings is None:
            user_file = 'None'
        else:
            user_file = self.user_settings.filename
        AddSampleLog(self.wksp_name, "UserFile", user_file)

    
    def get_instrument(self):
        """
            Convenience function used by the inst property to make
            calls shorter
        """
        return self.instrument
 
    #quicker to write than .instrument 
    inst = property(get_instrument, None, None, None)
 
            
    def ViewCurrentMask(self):
        self._mask.view(self.instrument)

    def reference(self):
        return self
