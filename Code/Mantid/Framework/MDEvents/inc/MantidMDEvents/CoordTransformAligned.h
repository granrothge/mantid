#ifndef MANTID_MDEVENTS_COORDTRANSFORMALIGNED_H_
#define MANTID_MDEVENTS_COORDTRANSFORMALIGNED_H_
    
#include "MantidKernel/System.h"
#include "MantidMDEvents/CoordTransform.h"
#include "MantidAPI/VectorParameter.h"


namespace Mantid
{
namespace MDEvents
{

  /// Unique type declaration for which dimensions are used in the input workspace
  DECLARE_VECTOR_PARAMETER(DimensionsToBinFromParam, size_t);

  /** A restricted version of CoordTransform which transforms
    from one set of dimensions to another, allowing:

     - An offset
     - A reduction in the number of dimensions
     - A scaling

    While a normal Affine matrix would handle this case, this special
    case is used in order to reduce the number of calculation.
    
    @author Janik Zikovsky
    @date 2011-08-29

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport CoordTransformAligned : public CoordTransform
  {
  public:
    CoordTransformAligned(const size_t inD, const size_t outD, const size_t * dimensionToBinFrom,
        const coord_t * min, const coord_t * step);
    virtual ~CoordTransformAligned();
    
  protected:


  };


} // namespace MDEvents
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_COORDTRANSFORMALIGNED_H_ */
