#ifndef MANTID_CRYSTAL_PREDICTPEAKS_H_
#define MANTID_CRYSTAL_PREDICTPEAKS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidKernel/System.h"
#include <MantidGeometry/Crystal/OrientedLattice.h>
#include "MantidKernel/Matrix.h"

namespace Mantid {
namespace Crystal {

/** Using a known crystal lattice and UB matrix, predict where single crystal
 *peaks
 * should be found in detector/TOF space. Creates a PeaksWorkspace containing
 * the peaks at the expected positions.
 *
 * @author Janik Zikovsky
 * @date 2011-04-29 16:30:52.986094
 */
class DLLExport PredictPeaks : public API::Algorithm {
public:
  PredictPeaks();
  ~PredictPeaks();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "PredictPeaks"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Using a known crystal lattice and UB matrix, predict where single "
           "crystal peaks should be found in detector/TOF space. Creates a "
           "PeaksWorkspace containing the peaks at the expected positions.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Crystal"; }

  void checkBeamDirection() const;
  void setInstrumentFromInputWorkspace(const API::ExperimentInfo_sptr &inWS);
  void setRunNumberFromInputWorkspace(const API::ExperimentInfo_sptr &inWS);
private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  void fillPossibleHKLsUsingGenerator(
      const Kernel::DblMatrix &ub,
      const Geometry::OrientedLattice &orientedLattice,
      std::vector<Kernel::V3D> &possibleHKLs);

  void fillPossibleHKLsUsingPeaksWorkspace(
      const DataObjects::PeaksWorkspace_sptr &possibleHKLWorkspace,
      std::vector<Kernel::V3D> &possibleHKLs) const;

  void calculateQAndAddToOutput(const Kernel::V3D &hkl,
                                const Kernel::DblMatrix &orientedUB,
                                const Kernel::DblMatrix &goniometerMatrix);

private:
  /// Reflection conditions possible
  std::vector<Mantid::Geometry::ReflectionCondition_sptr> m_refConds;

  /// Run number of input workspace
  int m_runNumber;
  /// Instrument reference
  Geometry::Instrument_const_sptr m_inst;
  /// Output peaks workspace
  Mantid::DataObjects::PeaksWorkspace_sptr m_pw;
};

} // namespace Mantid
} // namespace Crystal

#endif /* MANTID_CRYSTAL_PREDICTPEAKS_H_ */
