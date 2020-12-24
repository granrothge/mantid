============
SANS Changes
============

.. contents:: Table of Contents
   :local:

Improvements
############

- Add support for D11, D16, D22 and D33 in the :ref:`MaskBTP <algm-MaskBTP>` algorithm.

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Algorithms and instruments
--------------------------

Improvements
############

 - The core algorithm for ISIS SANS reductions underwent a significant rewrite to improve processing wavelength slices.
   For 15 slices the reduction time dropped by 55% compared to the previous release.
 - In :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>`, the beam radius can be different for each distance.
   A new parameter, TransmissionBeamRadius, has been added to set the beam radius for transmission experiments.
   The default value of all beam radii is now 0.1m.
 - With :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>`, if sample thickness is set to -1, the algorithm will try to get it
   from the nexus file.
 - With :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>`, the output workspace will get its title from the nexus file.

Bugfixes
########

- Fixed a bug in ISIS SANS GUI where all changes to settings on the adjustment page would be ignored, so that
  it only used parameters from the user file instead.
- Fixed "Falsey" values such as 0.0 or False getting replaced with a default value in the ISIS SANS settings.
  For example, a Phi limit of 0.0 remains at 0.0 rather than defaulting back to -90
- Detector IDs are no longer copied during a 2D reduction. This also resolves
  a bug where the first two spectra were marked as monitors and would not appear
  in a colour fill plot on Workbench.
- Wavelength limits entered with comma ranges larger than 10, e.g. `1,5,10,15` no longer
  throw a Runtime Error.
- ISIS SANS will print the name of any missing maskfiles instead of an empty name.

Changes
#######

- Workspace names for ISIS SANS reductions no longer append the wavelength to the name. The prepended
  wavelength is still present. For example `12345_rear_1d_1.0_10.0_...p0_t4_1.0_10.0` will now be called
  `12345_rear_1d_1.0_10.0_...p0_t4`, where `1.0_10.0` is the wavelength of that workspace.

:ref:`Release 6.0.0 <v6.0.0>`
