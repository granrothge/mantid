// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidCrystal/SCDCalibratePanels2.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCrystal/SCDCalibratePanels2ObjFunc.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Logger.h"
#include <boost/container/flat_set.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace Mantid {
namespace Crystal {

    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    /// Config logger
    namespace {
    Logger logger("SCDCalibratePanels2");
    }

    DECLARE_ALGORITHM(SCDCalibratePanels2)

    /**
     * @brief Initialization
     * 
     */
    void SCDCalibratePanels2::init() {
        // Input peakworkspace
        declareProperty(
            std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                "PeakWorkspace", 
                "",
                Kernel::Direction::Input),
            "Workspace of Indexed Peaks");

        // Lattice constant group
        auto mustBePositive = std::make_shared<BoundedValidator<double>>();
        mustBePositive->setLower(0.0);
        declareProperty("a", EMPTY_DBL(), mustBePositive,
                        "Lattice Parameter a (Leave empty to use lattice constants "
                        "in peaks workspace)");
        declareProperty("b", EMPTY_DBL(), mustBePositive,
                        "Lattice Parameter b (Leave empty to use lattice constants "
                        "in peaks workspace)");
        declareProperty("c", EMPTY_DBL(), mustBePositive,
                        "Lattice Parameter c (Leave empty to use lattice constants "
                        "in peaks workspace)");
        declareProperty("alpha", EMPTY_DBL(), mustBePositive,
                        "Lattice Parameter alpha in degrees (Leave empty to use "
                        "lattice constants in peaks workspace)");
        declareProperty("beta", EMPTY_DBL(), mustBePositive,
                        "Lattice Parameter beta in degrees (Leave empty to use "
                        "lattice constants in peaks workspace)");
        declareProperty("gamma", EMPTY_DBL(), mustBePositive,
                        "Lattice Parameter gamma in degrees (Leave empty to use "
                        "lattice constants in peaks workspace)");
        const std::string LATTICE("Lattice Constants");
        setPropertyGroup("a", LATTICE);
        setPropertyGroup("b", LATTICE);
        setPropertyGroup("c", LATTICE);
        setPropertyGroup("alpha", LATTICE);
        setPropertyGroup("beta", LATTICE);
        setPropertyGroup("gamma", LATTICE);

        // Calibration options group
        declareProperty("CalibrateT0", false, "Calibrate the T0 (initial TOF)");
        declareProperty("CalibrateL1", true, "Change the L1(source to sample) distance");
        declareProperty("CalibrateBanks", true, "Calibrate position and orientation of each bank.");
        // TODO:
        //     Once the core functionality of calibration is done, we can consider adding the
        //     following control calibration parameters.
        // declareProperty("EdgePixels", 0, "Remove peaks that are at pixels this close to edge. ");
        // declareProperty("ChangePanelSize", true, 
        //                 "Change the height and width of the "
        //                 "detectors.  Implemented only for "
        //                 "RectangularDetectors.");
        // declareProperty("CalibrateSNAPPanels", false,
        //                 "Calibrate the 3 X 3 panels of the "
        //                 "sides of SNAP.");
        const std::string PARAMETERS("Calibration Parameters");
        setPropertyGroup("CalibrateT0" ,PARAMETERS);
        setPropertyGroup("CalibrateL1" ,PARAMETERS);
        setPropertyGroup("CalibrateBanks" ,PARAMETERS);

        // Output options group
        const std::vector<std::string> detcalExts{".DetCal", ".Det_Cal"};
        declareProperty(
            std::make_unique<FileProperty>("DetCalFilename", "SCDCalibrate2.DetCal",
                                           FileProperty::OptionalSave, detcalExts),
            "Path to an ISAW-style .detcal file to save.");

        declareProperty(
            std::make_unique<FileProperty>("XmlFilename", "SCDCalibrate2.xml",
                                            FileProperty::OptionalSave, ".xml"),
            "Path to an Mantid .xml description(for LoadParameterFile) file to "
            "save.");
        // NOTE: we need to make some significant changes to the output interface considering
        //       50% of the time is spent on writing to file for the version 1.
        // Tentative options: all calibration output should be stored as a group workspace
        //                    for interactive analysis
        //  - peak positions comparison between theoretical and measured
        //  - TOF comparision between theoretical and measured
        const std::string OUTPUT("Output");
        setPropertyGroup("DetCalFilename", OUTPUT);
        setPropertyGroup("XmlFilename", OUTPUT);
    }

    /**
     * @brief validate inputs
     * 
     * @return std::map<std::string, std::string> 
     */
    std::map<std::string, std::string>
    SCDCalibratePanels2::validateInputs() {
        std::map<std::string, std::string> issues;

        return issues;
    }

    /**
     * @brief execute calibration
     * 
     */
    void SCDCalibratePanels2::exec() {
        // parse all inputs
        PeaksWorkspace_sptr m_pws = getProperty("PeakWorkspace");

        parseLatticeConstant(m_pws);

        bool calibrateT0 = getProperty("CalibrateT0");
        bool calibrateL1 = getProperty("CalibrateL1");
        bool calibrateBanks = getProperty("CalibrateBanks");

        const std::string DetCalFilename = getProperty("DetCalFilename");
        const std::string XmlFilename = getProperty("XmlFilename");

        // STEP_0: sort the peaks
        // NOTE: why??
        std::vector<std::pair<std::string, bool>> criteria{{"BankName", true}};
        m_pws->sort(criteria);

        // STEP_1: preparation
        // get names of banks that can be calibrated
        getBankNames(m_pws);

        // STEP_2: optimize T0,L1,L2,etc.
        // calibrate T0 if required
        if (calibrateT0)
            optimizeT0(m_pws);

        // calibrate L1 if required
        if (calibrateL1)
            optimizeL1(m_pws);

        // calibrate each bank
        if (calibrateBanks)
            optimizeBanks(m_pws);

        // STEP_3: Write to disk if required
        Instrument_sptr instCalibrated =
            std::const_pointer_cast<Geometry::Instrument>(m_pws->getInstrument());

        if (!XmlFilename.empty())
            saveXmlFile(XmlFilename, m_BankNames, instCalibrated);

        if (!DetCalFilename.empty())
            saveIsawDetCal(DetCalFilename, m_BankNames, instCalibrated, m_T0);

        // STEP_4: Cleanup
    }

    /// ------------------------------------------- ///
    /// Core functions for Calibration&Optimizatoin ///
    /// ------------------------------------------- ///

    /**
     * @brief 
     * 
     * @param pws 
     */
    void SCDCalibratePanels2::optimizeT0(std::shared_ptr<PeaksWorkspace> pws){

    }

    /**
     * @brief 
     * 
     * @param pws 
     */
    void SCDCalibratePanels2::optimizeL1(std::shared_ptr<PeaksWorkspace> pws){

    }

    /**
     * @brief 
     * 
     * @param pws 
     */
    void SCDCalibratePanels2::optimizeBanks(std::shared_ptr<PeaksWorkspace> pws){

    }

    /// ---------------- ///
    /// helper functions ///
    /// ---------------- ///

    /**
     * @brief get lattice constants from either inputs or the
     *        input peak workspace
     * 
     */
    void SCDCalibratePanels2::parseLatticeConstant(std::shared_ptr<PeaksWorkspace> pws) {
        m_a = getProperty("a");
        m_b = getProperty("b");
        m_c = getProperty("c");
        m_alpha = getProperty("alpha");
        m_beta = getProperty("beta");
        m_gamma = getProperty("gamma");
        // if any one of the six lattice constants is missing, try to get
        // one from the workspace
        if((m_a == EMPTY_DBL() ||
            m_b == EMPTY_DBL() ||
            m_c == EMPTY_DBL() ||
            m_alpha == EMPTY_DBL() ||
            m_beta == EMPTY_DBL() ||
            m_gamma == EMPTY_DBL()) &&
           (pws->sample().hasOrientedLattice())) {
            OrientedLattice lattice = pws->mutableSample().getOrientedLattice();
            m_a = lattice.a();
            m_b = lattice.b();
            m_c = lattice.c();
            m_alpha = lattice.alpha();
            m_beta = lattice.beta();
            m_gamma = lattice.gamma();
        }
    }

    /**
     * @brief Gather names for bank for calibration
     * 
     * @param pws 
     */
    void SCDCalibratePanels2::getBankNames(std::shared_ptr<PeaksWorkspace> pws) {
        int npeaks = static_cast<int>(pws->getNumberPeaks());
        for (int i=0; i<npeaks; ++i){
            std::string bname = pws->getPeak(i).getBankName();
            if (bname != "None")
                m_BankNames.insert(bname);
        }
    }

    /**
     * Saves the new instrument to an xml file that can be used with the
     * LoadParameterFile Algorithm. If the filename is empty, nothing gets done.
     *
     * @param FileName     The filename to save this information to
     *
     * @param AllBankNames The names of the banks in each group whose values are
     *                     to be saved to the file
     *
     * @param instrument   The instrument with the new values for the banks in
     *                     Groups
     * 
     * TODO: 
     *  - Need to find a way to add the information regarding calibrated T0
    */
    void SCDCalibratePanels2::saveXmlFile(
        const std::string &FileName,
        boost::container::flat_set<std::string> &AllBankNames,
        std::shared_ptr<Instrument> &instrument) {
        g_log.notice() << "Generating xml tree" << "\n";

        using boost::property_tree::ptree;
        ptree root;
        ptree parafile;

        // configure root node
        parafile.put("<xmlattr>.instrument", instrument->getName());
        parafile.put("<xmlattr>.valid-from", instrument->getValidFromDate().toISO8601String());

        // cnofigure and add each bank
        for (auto bankName : AllBankNames) {
            // Prepare data for node
            if (instrument->getName().compare("CORELLI") == 0)
                bankName.append("/sixteenpack");
            std::shared_ptr<const IComponent> bank = instrument->getComponentByName(bankName);
            Quat relRot = bank->getRelativeRot();
            std::vector<double> relRotAngles = relRot.getEulerAngles("XYZ");
            V3D pos1 = bank->getRelativePos();
            // TODO: no handling of scaling for now, will add back later
            double scalex = 1.0;
            double scaley = 1.0;

            // prepare node
            ptree bank_root;
            ptree bank_dx, bank_dy, bank_dz;
            ptree bank_dx_val, bank_dy_val, bank_dz_val;
            ptree bank_drotx, bank_droty, bank_drotz;
            ptree bank_drotx_val, bank_droty_val, bank_drotz_val;
            ptree bank_sx, bank_sy;
            ptree bank_sx_val, bank_sy_val;

            // add data to node
            bank_dx_val.put("<xmlattr>.val", pos1.X());
            bank_dy_val.put("<xmlattr>.val", pos1.Y());
            bank_dz_val.put("<xmlattr>.val", pos1.Z());
            bank_dx.put("<xmlattr>.name", "x");
            bank_dy.put("<xmlattr>.name", "y");
            bank_dz.put("<xmlattr>.name", "z");

            bank_drotx_val.put("<xmlattr>.val", relRot[0]);
            bank_droty_val.put("<xmlattr>.val", relRot[1]);
            bank_drotz_val.put("<xmlattr>.val", relRot[2]);
            bank_drotx.put("<xmlattr>.name", "rotx");
            bank_droty.put("<xmlattr>.name", "roty");
            bank_drotz.put("<xmlattr>.name", "rotz");

            bank_sx_val.put("<xmlattr>.val", scalex);
            bank_sy_val.put("<xmlattr>.val", scaley);
            bank_sx.put("<xmlattr>.name", "scalex");
            bank_sy.put("<xmlattr>.name", "scaley");

            bank_root.put("<xmlattr>.name", bankName);

            // configure structure
            bank_dx.add_child("value", bank_dx_val);
            bank_dy.add_child("value", bank_dy_val);
            bank_dz.add_child("value", bank_dz_val);

            bank_drotx.add_child("value", bank_drotx_val);
            bank_droty.add_child("value", bank_droty_val);
            bank_drotz.add_child("value", bank_drotz_val);

            bank_sx.add_child("value", bank_sx_val);
            bank_sy.add_child("value", bank_sy_val);

            bank_root.add_child("parameter", bank_drotx);
            bank_root.add_child("parameter", bank_droty);
            bank_root.add_child("parameter", bank_drotz);
            bank_root.add_child("parameter", bank_dx);
            bank_root.add_child("parameter", bank_dy);
            bank_root.add_child("parameter", bank_dz);
            bank_root.add_child("parameter", bank_sx);
            bank_root.add_child("parameter", bank_sy);

            parafile.add_child("component-link", bank_root);
        }

        // get L1 info for source
        ptree src;
        ptree src_dx, src_dy, src_dz;
        ptree src_dx_val, src_dy_val, src_dz_val;
        // -- get positional data from source
        IComponent_const_sptr source = instrument->getSource();
        V3D sourceRelPos = source->getRelativePos();
        // -- add date to node
        src_dx_val.put("<xmlattr>.val", sourceRelPos.X());
        src_dy_val.put("<xmlattr>.val", sourceRelPos.Y());
        src_dz_val.put("<xmlattr>.val", sourceRelPos.Z());
        src_dx.put("<xmlattr>.name", "x");
        src_dy.put("<xmlattr>.name", "y");
        src_dz.put("<xmlattr>.name", "z");
        src.put("<xmlattr>.name", source->getName());

        src_dx.add_child("value", src_dx_val);
        src_dy.add_child("value", src_dy_val);
        src_dz.add_child("value", src_dz_val);
        src.add_child("parameter", src_dx);
        src.add_child("parameter", src_dy);
        src.add_child("parameter", src_dz);

        parafile.add_child("component-link", src);

        // give everything to root
        root.add_child("parameter-file", parafile);
        // write the xml tree to disk
        g_log.notice() << "\tSaving parameter file as " << FileName << "\n";
        boost::property_tree::write_xml(
            FileName, root, std::locale(),
            boost::property_tree::xml_writer_settings<std::string>(' ', 2));
    }

    /**
     * Really this is the operator SaveIsawDetCal but only the results of the given
     * banks are saved.  L1 and T0 are also saved.
     *
     * @param filename     -The name of the DetCal file to save the results to
     * @param AllBankName  -the set of the NewInstrument names of the banks(panels)
     * @param instrument   -The instrument with the correct panel geometries
     *                      and initial path length
     * @param T0           -The time offset from the DetCal file
     */
    void SCDCalibratePanels2::saveIsawDetCal(
        const std::string &filename,
        boost::container::flat_set<std::string> &AllBankName,
        std::shared_ptr<Instrument> &instrument,
        double T0) {
        g_log.notice() << "Saving DetCal file in " << filename << "\n";

        // create a workspace to pass to SaveIsawDetCal
        const size_t number_spectra = instrument->getNumberDetectors();
        Workspace2D_sptr wksp =
            std::dynamic_pointer_cast<Workspace2D>(
                WorkspaceFactory::Instance().create("Workspace2D", number_spectra, 2,
                                                    1));
        wksp->setInstrument(instrument);
        wksp->rebuildSpectraMapping(true /* include monitors */);

        // convert the bank names into a vector
        std::vector<std::string> banknames(AllBankName.begin(), AllBankName.end());

        // call SaveIsawDetCal
        API::IAlgorithm_sptr alg = createChildAlgorithm("SaveIsawDetCal");
        alg->setProperty("InputWorkspace", wksp);
        alg->setProperty("Filename", filename);
        alg->setProperty("TimeOffset", T0);
        alg->setProperty("BankNames", banknames);
        alg->executeAsChildAlg();
    }

} // namespace Crystal
} // namespace Mantid
