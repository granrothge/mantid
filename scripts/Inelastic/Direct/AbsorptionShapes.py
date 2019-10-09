# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

#pylind: disable=attribute-defined-outside-init
from __future__ import (absolute_import, division, print_function)
from mantid.kernel import funcinspect
from mantid.simpleapi import *
from mantid.api import (AlgorithmManager, Algorithm)
import random




class anAbsorptionShape(object):
    """ The parent class for all shapes, used to perform various absorption corrections
    in direct inelastic analysis.

    Contains material and environment properties necessary for absorption corrections calculations,
    as recognized by SetSampleMaterial algorithm. 

    See SetSampleMaterial for full list of properties available to set. 

    Usage:
    Instantiate as:
    anShape = anAbsorptionShape([ChemicalFormula,NumberDensity])
        where:
        -- ChemicalFormula is the string, describing the sample formula
        -- NumberDensity   is the number density of the sample in number
           of atoms or formula units per cubic Angstrom. If provided, will be used
           instead of the one, calculated from the Chemical formula
        e.g:
        anShape = anAbsorptionShape(['Li',0.1])
    or 
    anShape = anAbsorptionShape({'ChemicalFormula':'Formula','NumberDensity':Number})
        e.g:
        anShape = anAbsorptionShape({'ChemicalFormula':'(Li7)2-C-H4-N-Cl6','NumberDensity':0.8}) 
    or 
    anShape = anAdsrbShape(Dictionary with any number of key-value pairs recognized by SetSampleMaterial algorithm)
    e.g:
    anShape = anAbsorptionShape({'AtomicNumber':12,'AttenuationXSection':0.5,'SampleMassDensity':120})

    The Material definition should unambiguously define sample material for absorption calculations.

    The set material value may be accessed or changes through the property material: e.g.
    mat = anShape.material
    or 
    anShape.material = {'ChemicalFormula':'Br'} (change existing ChemicalFormula)

    Note: 
    Adding dictionary appends or modifies existing properties, but adding a list, clears up all properties, previously 
    set-up on class. 
    The list can contain up two members where first corresponds to the ChemicalFormula and the second one -- to the
    material's number density
    """

    def __init__(self,MaterialValue=None):
        # environment according to existing definitions of the environment
        self._Environment = {}
        # default sample material (formula)
        self._Material = {}
        # the shape holder.  Not defined in this class but the holder for all
        # other classes
        self._ShapeDescription = {}

        # the workspace used for testing correct properties settings
        rhash = random.randint(1,100000)
        self._testWorkspace = CreateSampleWorkspace(OutputWorkspace='_adsShape_' + str(rhash))
        if MaterialValue is not None:
            self.material = MaterialValue
    #
    def __del__(self):
        DeleteWorkspace(self._testWorkspace)
    #
    @property
    def material(self):
        """ Contains the material, used in adsorbtion correction calculations"""
        return self._Material
    @material.setter
    def material(self,value):
        if value is None:
            self._Material = {}
            return
        if isinstance(value,str):
            value = [value]
        if isinstance(value,(list,tuple)):
            if len(value) == 2:
                self._Material = {'ChemicalFormula':value[0],'SampleNumberDensity':float(value[1])}
            elif len(value) == 1:
                self._Material = {'ChemicalFormula':value[0]}
            else:
                raise TypeError('*** If material is defined by list or tuple, it may consist of 1 or 2 members, '\
                    'defining the material formula and material number density')
        elif isinstance(value,dict):
            for key,val in value.iteritems():
                self._Material[key] = val
        else:
            raise TypeError('*** Material accepts only list or tuple containing up to 2 values'\
                ' corresponding to material formula and material number density or dictionary with keys,'\
                ' recognized by SetSampleMaterial algorithm')
        #
        Mater_properties = self._Material
        if 'ChemicalFormula' in Mater_properties:
            if not isinstance(Mater_properties['ChemicalFormula'],str):
                raise TypeError('*** The chemical formula for the material must be described by a string')
        # Test if the material property are recognized by SetSampleMaterial
        # algorithm.
        SetSampleMaterial(self._testWorkspace,**Mater_properties)
    #
    def correct_absorption(self,ws,*args,**kwargs):
        """ The generic method, which switches between fast and Monte-Carlo absorption corrections
        depending on the type and properties variable provided as second input

        kwargs is a dictionary, with at least one key, describing type of
               corrections (fast or Monte-Carlo) and other keys (if any)
               containing additional properties of the correspondent correction
               algorithm.

        Returns:
        1) absorption-corrected workspace
        2) if two output arguments are provided, second would be workspace with adsorbtion corrections coefficients
        """
        n_outputs,var_names = funcinspect.lhs_info('both')

        if args is None or len(args) == 0:
            if kwargs is None:
                corr_properties = {}
            else:
                corr_properties = kwargs
        else:
            corr_properties = args[0]
            if corr_properties is None:
                corr_properties = {}
            else:
                if not isinstance(corr_properties,dict):
                    raise TypeError('*** Second non-keyword argument of the correct_absorption routine'\
                                   ' (if present) should be a dictionary containing '\
                                   ' additional parameters for selected AbsorptionCorrections algorithm')

        correction_base_ws = ConvertUnits(ws,'Wavelength',EMode='Direct')
        Mater_properties = self._Material
        SetSampleMaterial(correction_base_ws,**Mater_properties)
        shape_description = self._ShapeDescription
        SetSample(correction_base_ws,Geometry=shape_description)

        mc_corrections = corr_properties.pop('is_mc', False)
        fast_corrections = corr_properties.pop('is_fast',False)
        if  not (mc_corrections or fast_corrections):
            fast_corrections = True
        if fast_corrections: 
            corrections = self.fast_abs_corrections(correction_base_ws,corr_properties)
        else:
            corrections = self.mc_abs_corrections(correction_base_ws,corr_properties)

        corrections = ConvertUnits(corrections,'DeltaE',EMode='Direct')
        ws = ws / corrections

        if ws.name() != var_names[0]:
            RenameWorkspace(ws,var_names[0])
        if n_outputs == 1:
            DeleteWorkspace(corrections)
            return ws
        elif n_outputs == 2:
            if corrections.name() != var_names[1]:
                RenameWorkspace(corrections,var_names[1])
            return (ws,corrections)

    def fast_abs_corrections(self,correction_base_ws,kwarg={}):
        """ Method to correct adsorption on a shape using fast (analytical) method if such method is available
        
        Not available on arbitrary shapes
        Inputs:
         ws     -- workspace to correct. Should be in the units of wavelength
        **kwarg -- dictionary of the additional keyword arguments to provide as input for
                the absorption corrections algorithm
                These arguments should not be related to the sample as the sample should already be defined.
        Returns: 
            workspace with absorption corrections.
        """
        adsrbtn_correctios = AbsorptionCorrection(correction_base_ws,**kwarg)
        return adsrbtn_correctios




    def mc_abs_corrections(self,correction_base_ws,kwarg={}):
        """ Method to correct adsorption on a shape using Mont-Carlo integration
        Inputs:
         ws     -- workspace to correct. Should be in the units of wavelength
        **kwarg -- dictionary of the additional keyword arguments to provide as input for
                the absorption corrections algorithm
                These arguments should not be related to the sample as the sample should already be defined.
        Returns: 
            workspace with absorption corrections.
        """
        adsrbtn_correctios = MonteCarloAbsorption(correction_base_ws,**kwarg)
        return adsrbtn_correctios
    def str(self):
        raise RuntimeError('This option is not yet implemented')
    #
    def _set_shape_property(self,value,shape_name,mandatory_prop_list,opt_prop_list,opt_val_list):
        """ General function to build shape property"""
        if value is None:
            return {}
        n_elements = len(value)
        if n_elements < len(mandatory_prop_list):
            raise TypeError('*** {0} shape parameter needes at least {1} imput parameters namely: {2}'\
                            .format(shape_name,len(mandatory_prop_list),mandatory_prop_list))

        shape_dict = {'Shape':shape_name}
        all_prop = mandatory_prop_list+opt_prop_list
        if isinstance(value,(list,tuple)):
            for i in range(0,n_elements):
                val = value[i]
                if isinstance(val,(list,tuple)):
                    val = map(lambda x: float(x),val)
                else:
                    val = float(val)
                shape_dict[all_prop[i]] = val
        elif isinstance(value,dict):
            for key,val in value.iteritems():
                if not isinstance(val,(list,tuple)):
                    val = float(val)
                shape_dict[key] = val
        else:
            raise TypeError('*** {0} shape parameter accepts only list or tuple containing from {1} to {2} values'\
                ' corresponding to the {0} parameters: {3} or the dictionary with these,'\
                ' as recognized by SetSample algorithm Geometry property'.format(\
                shape_name,len(mandatory_prop_list),len(all_prop),all_prop))
        #
        for ik in range(0,len(opt_prop_list)):
            if not opt_prop_list[ik] in shape_dict:
                shape_dict[opt_prop_list[ik]] = opt_val_list[ik]

        # Test if the material property are recognized by SetSampleMaterial
        # algorithm.
        SetSample(self._testWorkspace,Geometry=shape_dict)

        return shape_dict



class Cylinder(anAbsorptionShape):
    """Define the absorbing cylinder and calculate absorption corrections for this cylinder
    
    Usage:
    abs = Cylinder(SampleMaterial,CylinderParameters)
    The SampleMaterial is the list or dictionary as described on anAbsorptionShape class 

    and

    CylinderParameters can have the form:

    a) The list consisting of 2 to 4 members.:
    CylinderParameters = [Height,Radus,[[Axis],[Center]] 
    where Height, Radus are the cylinder height and radius in cm and 
    Axis, if present is the direction of the cylinder wrt to the beam direction [0,0,1]
    e.g.:
    abs = Cylinder(['Al',0.1],[10,2,[1,0,0],[0,0,0]])
    b) The diary: 
    CylinderParameters = {Height:Height_value,Radus:Radius_value,[Axis:axisValue],[Center:TheSampleCentre]} 
    e.g:
    abs = Cylinder(['Al',0.1],[Height:10,Radius:2,Axis:[1,0,0]])

    Usage of the defined Cylinder class instance:
    Correct absorption on the cylinder using CylinderAbsorption algorithm:
    ws = abs.correct_adsorption(ws) % it is default

    Correct absorption on the defined cylinder using Monte-Carlo Absorption algorithm:
    ws = ads.correct_adsorption(ws,{is_mc:True,AdditionalMonte-Carlo Absorption parameters});
    """
    def __init__(self,Material=None,CylinderParams=None):

        anAbsorptionShape.__init__(self,Material)
        self.shape = CylinderParams

    @property
    def shape(self):
        return self._ShapeDescription
    @shape.setter
    def shape(self,value):
        shape_dict = self._set_shape_property(value,'Cylinder',['Height','Radius'],\
            ['Axis','Center'],[[0,1,0],[0,0,0]])

        self._ShapeDescription = shape_dict
    #
    def fast_abs_corrections(self,correction_base_ws,kwarg={}):
        """ Method to calculate absorption corrections quickly using fast (integration) method
        """
        adsrbtn_correctios = CylinderAbsorption(correction_base_ws,**kwarg)
        return adsrbtn_correctios

class FlatPlate(anAbsorptionShape):
    """Define the absorbing plate and calculate absorption corrections from this cylinder
    
    Usage:
    abs = Plate(SampleMaterial,PlateParameters)
    The SampleMaterial is the list or dictionary as described on anAbsorptionShape class

    and

    PlateParameters can have the form:

    a) The list consisting of 3 to 5 members e.g.:
    PlateParameters = [Height,Width,Thickness,[Center,[Angle]]]
    where Height,Width,Thickness are the plate sizes in cm,
    the Center, if present, a list of three values indicating the [X,Y,Z] position of the center. T
    he reference frame of the defined instrument is used to set the coordinate system for the shape

    The Angle argument for a flat plate shape is expected to be in degrees and is defined as the
    angle between the positive beam axis and the normal to the face perpendicular to the beam axis
    when it is not rotated, increasing in an anti-clockwise sense. The rotation is performed about
    the vertical axis of the instrument's reference frame.

    e.g.:
    abs = Plate(['Al',0.1],[10,2,0.2,[1,0,0],5])
    b) The diary: 
    PlateParameters = {Height:Height_value,Width:Width_value,Thickness:Thickness_value etc}
    e.g:
    abs = Plate(['Al',0.1],['Height':10,'Width':4,'Thickness':2,'Angle':30)

    Usage of the defined Plate class instance:
    Correct absorption on the defined plate using AbsorptionCorrections algorithm:
    ws = abs.correct_adsorption(ws)

    Correct absorption on the defined Plate using Monte-Carlo Absorption algorithm:
    ws = ads.correct_adsorption(ws,{'is_mc':True,AdditionalMonte-Carlo Absorption parameters});
    """
    def __init__(self,Material=None,PlateParams=None):

        anAbsorptionShape.__init__(self,Material)
        self.shape = PlateParams

    @property
    def shape(self):
        return self._ShapeDescription
    @shape.setter
    def shape(self,value):
        shape_dict = self._set_shape_property(value,'FlatPlate',['Height','Width','Thick'],\
            ['Center','Angle'],[[0,0,0],0])
        self._ShapeDescription = shape_dict

    #
    def fast_abs_corrections(self,correction_base_ws,kwarg={}):
        """ Method to calculate absorption corrections quickly using fast (integration) method
        """
        adsrbtn_correctios = FlatPlateAbsorption(correction_base_ws,**kwarg)
        return adsrbtn_correctios


#-----------------------------------------------------------------
if __name__ == "__main__":
    pass
