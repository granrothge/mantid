/*WIKI*
 TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
 *WIKI*/

#include "MantidDataHandling/LoadILLReflectometry.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"

#include <boost/algorithm/string.hpp>

#include <nexus/napi.h>
#include <iostream>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM (LoadILLReflectometry);

// PI again !
const double PI = 3.14159265358979323846264338327950288419716939937510582;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadILLReflectometry::LoadILLReflectometry() {
	m_numberOfTubes = 0; // number of tubes - X
	m_numberOfPixelsPerTube = 0; //number of pixels per tube - Y
	m_numberOfChannels = 0; // time channels - Z
	m_numberOfHistograms = 0;
	m_supportedInstruments.push_back("D17");
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadILLReflectometry::~LoadILLReflectometry() {
}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadILLReflectometry::name() const {
	return "LoadILLReflectometry";
}
;

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLReflectometry::version() const {
	return 1;
}
;

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLReflectometry::category() const {
	return "DataHandling";
}

//----------------------------------------------------------------------------------------------
/// Sets documentation strings for this algorithm
void LoadILLReflectometry::initDocs() {
	this->setWikiSummary("Loads a ILL/D17 nexus file. ");
	this->setOptionalMessage("Loads a ILL/D17 nexus file. ");
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not be used
 */
int LoadILLReflectometry::confidence(
		Kernel::NexusDescriptor & descriptor) const {

	// fields existent only at the ILL
	if (descriptor.pathExists("/entry0/wavelength")  // ILL
	&& descriptor.pathExists("/entry0/experiment_identifier")  // ILL
			&& descriptor.pathExists("/entry0/mode")  // ILL
			&& descriptor.pathExists("/entry0/instrument/Chopper1") // TO BE DONE
			&& descriptor.pathExists("/entry0/instrument/Chopper2")  // ???
					) {
		return 80;
	} else {
		return 0;
	}
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadILLReflectometry::init() {
	declareProperty(
			new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
			"File path of the Data file to load");

	declareProperty(
			new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
			"The name to use for the output workspace");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLReflectometry::exec() {
	// Retrieve filename
	std::string filenameData = getPropertyValue("Filename");

	// open the root node
	NeXus::NXRoot dataRoot(filenameData);
	NXEntry firstEntry = dataRoot.openFirstEntry();

	// Load Monitor details: n. monitors x monitor contents
	std::vector<std::vector<int> > monitorsData = loadMonitors(firstEntry);


	// Load Data details (number of tubes, channels, etc)
	loadDataDetails(firstEntry);

	std::string instrumentPath = m_loader.findInstrumentNexusPath(firstEntry);
	setInstrumentName(firstEntry, instrumentPath);

	initWorkSpace(firstEntry, monitorsData);

	g_log.debug("Building properties...");
	loadNexusEntriesIntoProperties(filenameData);

//	g_log.debug("Loading data...");
	loadDataIntoTheWorkSpace(firstEntry, monitorsData);

	// load the instrument from the IDF if it exists
	g_log.debug("Loading instrument definition...");
	runLoadInstrument();




	// 1) Move

	// Get distance and tilt angle stored in nexus file
	// Mantid way
	//	auto angleProp = dynamic_cast<PropertyWithValue<double>*>(m_localWorkspace->run().getProperty("dan.value"));
	// Nexus way
	double angle = firstEntry.getFloat("instrument/dan/value");// detector angle in degrees
	double distance = firstEntry.getFloat("instrument/det/value");// detector distance in millimeter
	distance /= 1000;// convert to meter
	placeDetector(distance, angle);

	// 2) Center, (must be done after move)
	int par1_101 = firstEntry.getInt("instrument/PSD/ny");
	g_log.debug("Note: using PSD/ny instead of PSD/nx. Should be corrected in next D17 nexus file.");
	double xCenter = 0.1325 / par1_101;// As in lamp, but in meter
	centerDetector(xCenter);


	// Set the channel width property
	auto channel_width = dynamic_cast<PropertyWithValue<double>*>(m_localWorkspace->run().getProperty("monitor1.time_of_flight_0"));
	m_localWorkspace->mutableRun().addProperty<double>("channel_width", *channel_width, true); //overwrite

	// Set the output workspace property
	setProperty("OutputWorkspace", m_localWorkspace);
}


/**
* Set member variable with the instrument name
*/
void LoadILLReflectometry::setInstrumentName(const NeXus::NXEntry &firstEntry,
		const std::string &instrumentNamePath) {

	if (instrumentNamePath == "") {
		std::string message(
				"Cannot set the instrument name from the Nexus file!");
		g_log.error(message);
		throw std::runtime_error(message);
	}
	m_instrumentName = m_loader.getStringFromNexusPath(firstEntry,
			instrumentNamePath + "/name");
	boost::to_upper(m_instrumentName);// "D17" in file, keep it upper case.
	g_log.debug() << "Instrument name set to: " + m_instrumentName << std::endl;

}


/**
   * Creates the workspace and initialises member variables with
   * the corresponding values
   *
   * @param entry :: The Nexus entry
   * @param monitorsData :: Monitors data already loaded
   *
   */
void LoadILLReflectometry::initWorkSpace(NeXus::NXEntry& /*entry*/, std::vector< std::vector<int> > monitorsData)
{

	// dim0 * m_numberOfPixelsPerTube is the total number of detectors
	m_numberOfHistograms = m_numberOfTubes * m_numberOfPixelsPerTube;

	g_log.debug() << "NumberOfTubes: " << m_numberOfTubes << std::endl;
	g_log.debug() << "NumberOfPixelsPerTube: " << m_numberOfPixelsPerTube << std::endl;
	g_log.debug() << "NumberOfChannels: " << m_numberOfChannels << std::endl;
	g_log.debug() << "Monitors: " << monitorsData.size() << std::endl;
	g_log.debug() << "Monitors[0]: " << monitorsData[0].size() << std::endl;
	g_log.debug() << "Monitors[1]: " << monitorsData[1].size() << std::endl;

	// Now create the output workspace

	m_localWorkspace = WorkspaceFactory::Instance().create(
			"Workspace2D",
			m_numberOfHistograms+monitorsData.size(),
			m_numberOfChannels + 1,
			m_numberOfChannels);

	m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");


	m_localWorkspace->setYUnitLabel("Counts");

}


/**
* Load Data details (number of tubes, channels, etc)
* @param entry First entry of nexus file
*/
void LoadILLReflectometry::loadDataDetails(NeXus::NXEntry& entry)
{
	// read in the data
	NXData dataGroup = entry.openNXData("data");
	NXInt data = dataGroup.openIntData();

	m_numberOfTubes = static_cast<size_t>(data.dim0());
	m_numberOfPixelsPerTube = static_cast<size_t>(data.dim1());
	m_numberOfChannels = static_cast<size_t>(data.dim2());


}




/**
   * Load monitors data found in nexus file
   *
   * @param entry :: The Nexus entry
   *
   */
std::vector< std::vector<int> > LoadILLReflectometry::loadMonitors(NeXus::NXEntry& entry){
	// read in the data
	g_log.debug("Fetching monitor data...");

	NXData dataGroup = entry.openNXData("monitor1/data");
	NXInt data = dataGroup.openIntData();
	// load the counts from the file into memory
	data.load();

	std::vector< std::vector<int> > monitors(1);// vector of monitors with one entry
	std::vector<int> monitor1(data(), data()+data.size());
	monitors[0].swap(monitor1);

	// There is two monitors in data file, but the second one seems to be always 0
	dataGroup = entry.openNXData("monitor2/data");
	data = dataGroup.openIntData();
	data.load();

	std::vector<int> monitor2(data(), data()+data.size());
	monitors.push_back(monitor2);

	return monitors;
}


/**
   * Load data found in nexus file
   *
   * @param entry :: The Nexus entry
   * @param monitorsData :: Monitors data already loaded
   *
   */
void LoadILLReflectometry::loadDataIntoTheWorkSpace(NeXus::NXEntry& entry, std::vector< std::vector<int> > monitorsData)
{

  // read in the data
  NXData dataGroup = entry.openNXData("data");
  NXInt data = dataGroup.openIntData();
  // load the counts from the file into memory
  data.load();



  // Assign calculated bins to first X axis
////  m_localWorkspace->dataX(0).assign(detectorTofBins.begin(), detectorTofBins.end());

  size_t spec = 0;
  size_t nb_monitors = monitorsData.size();

  Progress progress(this, 0, 1, m_numberOfTubes * m_numberOfPixelsPerTube + nb_monitors);

  // Assign fake values to first X axis <<to be completed>>

  // Get some parameters from nexus file and properties
  auto tof_channel_width_prop = dynamic_cast<PropertyWithValue<double>*>(m_localWorkspace->run().getProperty("monitor1.time_of_flight_0"));
  double tof_channel_width = *tof_channel_width_prop; /* PAR1[86] */

  auto tof_delay_prop = dynamic_cast<PropertyWithValue<double>*>(m_localWorkspace->run().getProperty("monitor1.time_of_flight_2"));
  double tof_delay = *tof_delay_prop; /* PAR1[85] */

  double POFF = entry.getFloat("instrument/VirtualChopper/poff"); /* par1[54] */;
  double mean_chop_2_phase = entry.getFloat("instrument/VirtualChopper/chopper2_phase_average"); /* PAR2[114] */;
  double mean_chop_1_phase = entry.getFloat("instrument/VirtualChopper/chopper1_phase_average"); /* PAR2[110] */;
  double openoff = entry.getFloat("instrument/VirtualChopper/open_offset"); /* par1[56] */
  double variance_chop1_speed = entry.getFloat("instrument/VirtualChopper/chopper1_speed_sigma"); /* PAR2[111] */;
  double deg360 = PI ; /* ? */

  for (size_t timechannelnumber = 0; timechannelnumber <= m_numberOfChannels; ++timechannelnumber) {

    double t_TOF =
       (static_cast<int>(timechannelnumber)+0.5) / tof_channel_width + tof_delay
        - 60.0 * (POFF - 45.0 + mean_chop_2_phase - mean_chop_1_phase + openoff)
                 /
                 (2.0 * 360.0 * variance_chop1_speed);

	  m_localWorkspace->dataX(0)[timechannelnumber] = t_TOF;
  }

  // First, Monitor
  for (size_t im = 0; im<nb_monitors; im++){

      if (im > 0)
      {
        m_localWorkspace->dataX(im) = m_localWorkspace->readX(0);
      }

      // Assign Y
      int* monitor_p = monitorsData[im].data();
      m_localWorkspace->dataY(im).assign(monitor_p, monitor_p + m_numberOfChannels);

	  progress.report();
  }

  // TODO
  // copy data if m_numberOfTubes = 1 or m_numberOfPixelsPerTube = 1

  // Then Tubes
  for (size_t i = 0; i < m_numberOfTubes; ++i)
  {
    for (size_t j = 0; j < m_numberOfPixelsPerTube; ++j)
    {

      // just copy the time binning axis to every spectra
      m_localWorkspace->dataX(spec+nb_monitors) = m_localWorkspace->readX(0);

      // Assign Y
      int* data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
      m_localWorkspace->dataY(spec+nb_monitors).assign(data_p, data_p + m_numberOfChannels);

      // Assign Error
      MantidVec& E = m_localWorkspace->dataE(spec+nb_monitors);
      std::transform(data_p, data_p + m_numberOfChannels, E.begin(),
    		LoadHelper::calculateStandardError);

      ++spec;
      progress.report();
    }
  }// for m_numberOfTubes


}// LoadILLIndirect::loadDataIntoTheWorkSpace




void LoadILLReflectometry::loadNexusEntriesIntoProperties(std::string nexusfilename) {

    API::Run & runDetails = m_localWorkspace->mutableRun();

    // Open NeXus file
    NXhandle nxfileID;
    NXstatus stat=NXopen(nexusfilename.c_str(), NXACC_READ, &nxfileID);
    if(stat==NX_ERROR)
    {
    	g_log.debug() << "convertNexusToProperties: Error loading " << nexusfilename;
        throw Kernel::Exception::FileError("Unable to open File:" , nexusfilename);
    }
    m_loader.addNexusFieldsToWsRun(nxfileID, runDetails, nexusfilename, nexusfilename, 0);

    // Add also "Facility", as asked
    runDetails.addProperty("Facility", std::string("ILL"));

    stat=NXclose(&nxfileID);
}




/**
   * Run the Child Algorithm LoadInstrument.
   */
void LoadILLReflectometry::runLoadInstrument() {

	IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

	// Now execute the Child Algorithm. Catch and log any error, but don't stop.
	try {

		loadInst->setPropertyValue("InstrumentName", m_instrumentName);
		loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
		loadInst->execute();

	} catch (...) {
		g_log.information("Cannot load the instrument definition.");
	}
}

void LoadILLReflectometry::centerDetector(double xCenter) {

	std::string componentName("uniq_detector");
	V3D pos = m_loader.getComponentPosition(m_localWorkspace, componentName);
	// TODO confirm!
	pos.setX(pos.X() - xCenter);
	m_loader.moveComponent(m_localWorkspace, componentName, pos);

}

void LoadILLReflectometry::placeDetector(
				double distance /* meter */,
				double angle /* degree */) {

	std::string componentName("uniq_detector");
	V3D pos = m_loader.getComponentPosition(m_localWorkspace, componentName);

	double r, theta, phi;
	pos.getSpherical(r, theta, phi);


	V3D newpos;
	newpos.spherical(distance, angle, phi);

	m_loader.moveComponent(m_localWorkspace, componentName, newpos);

	// Apply a local rotation to stay perpendicular to the beam
	const V3D axis(0.0,1.0,0.0);
	Quat rotation(angle, axis);
	m_loader.rotateComponent(m_localWorkspace, componentName, rotation);

}
} // namespace DataHandling
} // namespace Mantid
