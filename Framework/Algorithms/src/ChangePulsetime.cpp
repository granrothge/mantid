#include "MantidAlgorithms/ChangePulsetime.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspacePropertyWithIndexHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ChangePulsetime)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using std::size_t;

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ChangePulsetime::init() {
  declareProperty(createWithWorkspaceIndices<EventWorkspace>("InputWorkspace"),
                  "An input event workspace.");
  declareProperty(
      make_unique<PropertyWithValue<double>>("TimeOffset", Direction::Input),
      "Number of seconds (a float) to add to each event's pulse "
      "time. Required.");

  declareProperty(make_unique<WorkspaceProperty<EventWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output event workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ChangePulsetime::exec() {
  EventWorkspace_sptr in_ws;
  SpectrumIndexSet indexSet(0);

  std::tie(in_ws, indexSet) = std::tuple<EventWorkspace_sptr, SpectrumIndexSet>(
      getProperty("InputWorkspace"));

  EventWorkspace_sptr out_ws = getProperty("OutputWorkspace");
  if (!out_ws) {
    out_ws = in_ws->clone();
  }

  double timeOffset = getProperty("TimeOffset");
  Progress prog(this, 0.0, 1.0, indexSet.size());

  for (auto i : indexSet) {
    out_ws->getSpectrum(i).addPulsetime(timeOffset);
    prog.report(name());
  }

  setProperty("OutputWorkspace", out_ws);
}

} // namespace Algorithms
} // namespace Mantid
