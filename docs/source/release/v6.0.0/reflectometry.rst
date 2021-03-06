=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

ISIS Reflectometry Interface
############################

- You can now exclude or annotate runs in the search results list, e.g. to
  exclude them from autoprocessing. See the
  :ref:`ISIS Reflectometry Interface <interface-isis-refl>` documentation for details.
- Fixed a bug where the flood workspace drop-down box was being accidentally populated with the first workspace created after clearing the workspaces list.

Algorithms
##########

- Fixed an issue where `import CaChannel` on Linux would cause a hard crash.
- Fixed spurious error messages about transmission workspaces when running :ref:`algm-ReflectometryReductionOneAuto` on workspace groups

:ref:`Release 6.0.0 <v6.0.0>`
