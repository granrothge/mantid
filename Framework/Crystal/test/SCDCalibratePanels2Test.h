// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

// NOTE: Generating a synthetic peakworkspace for testing is very time
//       consuming, therefore only one wholesome unittest with calibration
//       is enabled for regression test.
//       To test other testing targets, change the lead word from
//                 run_ to test_
//       to run them within the ctest framework.
//       You might need to do one at a time to avoid ctest timeout error
//       locally.

// DEVNOTE:
//  - cos, sin func uses radians
//  - Quat class uses degrees

#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/SCDCalibratePanels2.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/Logger.h"

#include <cxxtest/TestSuite.h>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/math/constants/constants.hpp>

using namespace Mantid::API;
using namespace Mantid::Crystal;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace {
/// static logger
  Logger g_log("SCDCalibratePanels2Test");
} // namespace

class SCDCalibratePanels2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SCDCalibratePanels2Test *createSuite() { return new SCDCalibratePanels2Test(); }
  static void destroySuite( SCDCalibratePanels2Test *suite ) { delete suite; }

  // ----------------- //
  // ----- Setup ----- //
  // ----------------- //
  /**
   * @brief Construct a new SCDCalibratePanels2Test object
   *
   */
  SCDCalibratePanels2Test()
      : wsname("wsSCDCalibratePanels2Test"),
        pwsname("pwsSCDCalibratePanels2Test"),
        tmppwsname("tmppwsSCDCalibratePanels2Test"), // fixed workspace name
        bank_xtop("bank73/sixteenpack"),             //
        bank_xcenter("bank12/sixteenpack"),          //
        bank_xbottom("bank11/sixteenpack"),          //
        bank_yright("bank59/sixteenpack"),           //
        bank_yleft("bank58/sixteenpack"),            //
        bank_ytop("bank88/sixteenpack"),             //
        bank_ybottom("bank26/sixteenpack"),          //
        silicon_a(5.431), silicon_b(5.431), silicon_c(5.431),   // angstrom
        silicon_alpha(90), silicon_beta(90), silicon_gamma(90), // degree
        silicon_cs(CrystalStructure("5.431 5.431 5.431", "F d -3 m",
                                    "Si 0 0 0 1.0 0.02")),
        dspacing_min(1.0), dspacing_max(10.0),   //
        wavelength_min(0.1), wavelength_max(10), //
        omega_step(3.0),                         //
        TOLERANCE_L(1e-16), // this calibration has intrinsic accuracy limit of
                            // 1mm for translation
        TOLERANCE_R(1e-8),  // this calibration has intrinsic accuracy limit of
                            // 1e-4 deg for rotation
        LOGCHILDALG(false) {
    // NOTE:
    //  The MAGIC PIECE, basically we need to let AlgorithmFactory
    //  to load a non-related algorithm, then somehow AlgorithmFactory
    //  can find the Fit algorithm for the remaining test
    std::shared_ptr<Algorithm> darkmagic =
        AlgorithmFactory::Instance().create("LoadIsawPeaks", 1);
    darkmagic->initialize();
    darkmagic->setLogging(false);  // don't really care its output
    darkmagic->setPropertyValue("Filename", "Peaks5637.integrate");
    darkmagic->setPropertyValue("OutputWorkspace", "TOPAZ_5637");
    TS_ASSERT(darkmagic->execute());

    m_ws = generateSimulatedWorkspace();
  }


  // ---------------------- //
  // ----- Unit Tests ----- //
  // ---------------------- //

  /**
   * @brief test name
   *
   */
  void test_Name() {
    SCDCalibratePanels2 alg;
    TS_ASSERT_EQUALS(alg.name(), "SCDCalibratePanels");
  }

  /**
   * @brief test init
   *
   */
  void test_Init() {
    SCDCalibratePanels2 alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  /**
   * @brief Trivial case where all components are in ideal/starting position
   *        Therefore the calibration results should be close to a zero
   *        vector.
   *
   * NOTE: Change the name from run_Null_Case to test_Null_Case to run it
   *       within the ctest framework.
   */
  void run_Null_Case() {
    g_log.notice() << "test: !Null case!\n";

    // Generate unique temp files
    auto isawFile = boost::filesystem::temp_directory_path();
    isawFile /= boost::filesystem::unique_path("nullcase_%%%%%%%%.DetCal");
    auto xmlFile = boost::filesystem::temp_directory_path();
    xmlFile /= boost::filesystem::unique_path("nullcase_%%%%%%%%.xml");

    g_log.notice() << "-- generate simulated workspace\n";
    MatrixWorkspace_sptr ws = m_ws->clone();
    MatrixWorkspace_sptr wsraw = ws->clone();

    // Trivial case, no component undergoes any affine transformation
    g_log.notice() << "-- trivial case, no components moved\n";

    g_log.notice() << "-- generate peaks\n";
    PeaksWorkspace_sptr pws = generateSimulatedPeaksWorkspace(ws);
    PeaksWorkspace_sptr pwsref = pws->clone();

    // Pretend we don't know the answer
    g_log.notice() << "-- reset instrument positions&orientations\n";
    pws->setInstrument(wsraw->getInstrument());

    // Perform the calibration
    g_log.notice() << "-- start calibration\n";
    runCalibration(isawFile.string(), xmlFile.string(), pws, false, true, true);

    // Check if the calibration returns the same instrument as we put in
    g_log.notice() << "-- validate calibration output\n";
    TS_ASSERT(validateCalibrationResults(pwsref, wsraw, xmlFile.string()));

    // Cleanup
    doCleanup();
  }

  /**
   * @brief Only adjust T0
   *
   */
  void run_T0_Shift() {
    g_log.notice() << "test: !T0 Shift!\n";

    // prescribed shift
    const double dT0 = boost::math::constants::e<double>() / 2;

    // Generate unique temp files
    auto isawFile = boost::filesystem::temp_directory_path();
    isawFile /= boost::filesystem::unique_path("changeT0_%%%%%%%%.DetCal");
    auto xmlFile = boost::filesystem::temp_directory_path();
    xmlFile /= boost::filesystem::unique_path("changeT0_%%%%%%%%.xml");

    g_log.notice() << "-- generate simulated workspace\n";
    MatrixWorkspace_sptr ws = m_ws->clone();
    MatrixWorkspace_sptr wsraw = ws->clone();

    // Trivial case, no component undergoes any affine transformation
    g_log.notice() << "-- generate peaks\n";
    PeaksWorkspace_sptr pws = generateSimulatedPeaksWorkspace(ws);
    PeaksWorkspace_sptr pwsref = pws->clone();

    // Adjust T0
    adjustT0(dT0, pws);
  }

  /**
   * @brief Single variant case where only global var is adjusted.
   *
   * NOTE: technically we should also check T0, but the client, CORELLI
   *       team does not seem to care about using T0, therefore we are
   *       not implmenting T0 calibration here.
   *
   * NOTE: change the name from run_L1_Shift to test_L1_Shift to run
   *       it within the ctest framework
   */
  void run_L1_Shift() {
    g_log.notice() << "test: !Source Shift (L1 change)!\n";

    g_log.notice() << "Tolerance of Distance (meter) :" << TOLERANCE_L << "\n";

    // prescribed shift
    // NOTE: the common range for dL1 is +-10cm
    const double dL1 = boost::math::constants::e<double>() / 100;

    // Generate unique temp files
    auto isawFile = boost::filesystem::temp_directory_path();
    isawFile /= boost::filesystem::unique_path("changeL1_%%%%%%%%.DetCal");
    auto xmlFile = boost::filesystem::temp_directory_path();
    xmlFile /= boost::filesystem::unique_path("changeL1_%%%%%%%%.xml");

    g_log.notice() << "-- generate simulated workspace\n";
    MatrixWorkspace_sptr ws = m_ws->clone();
    MatrixWorkspace_sptr wsraw = ws->clone();

    // move source
    adjustComponent(0.0, 0.0, dL1, 1.0, 0.0, 0.0, 0.0,
                    ws->getInstrument()->getSource()->getName(), ws);

    g_log.notice() << "-- generate peaks\n";
    PeaksWorkspace_sptr pws = generateSimulatedPeaksWorkspace(ws);
    PeaksWorkspace_sptr pwsref = pws->clone();

    // Pretend we don't know the answer
    g_log.notice() << "-- reset instrument positions&orientations\n";
    g_log.notice() << "    * before reset L1 = "
                   << pws->getInstrument()->getSource()->getPos().Z() << "\n";
    pws->setInstrument(wsraw->getInstrument());
    g_log.notice() << "    * after reset L1 = "
                   << pws->getInstrument()->getSource()->getPos().Z() << "\n";

    // Perform the calibration
    g_log.notice() << "-- start calibration\n";
    runCalibration(isawFile.string(), xmlFile.string(), pws, false, true,
                   false);

    // Check if the calibration returns the same instrument as we put in
    g_log.notice() << "-- validate calibration output\n";
    TS_ASSERT(validateCalibrationResults(pwsref, wsraw, xmlFile.string()));

    // this is just for documentation purpose, the validation func above
    // is better for robust testing
    double L1_prescribed = ws->getInstrument()->getSource()->getPos().Z();
    double L1_calibrated = pws->getInstrument()->getSource()->getPos().Z();
    double dl = std::abs(L1_prescribed - L1_calibrated);
    g_log.notice() << "-- |L1_prescribed-L1_calibrated| = " << dl << "\n";

    TS_ASSERT(false);
    // Cleanup
    doCleanup();
  }

  /**
   * @brief Moving (rotation and translation) single panel
   *
   */
  void test_bank_moved() {
    g_log.notice() << "test: !single bank moved!\n";

    g_log.notice() << "Tolerance of Distance (meter) :" << TOLERANCE_L << "\n";
    g_log.notice() << "Tolerance of Rotation (degree) :" << TOLERANCE_R << "\n";

    // prescribed shift
    // NOTE: the common range for dx, dy ,dz is +-5cm
    double dx = 1.1e-2;
    double dy = -0.9e-2;
    double dz = 1.5e-2;

    // prescribed rotation
    double theta = PI / 3;
    double phi = PI / 8;
    double rvx = sin(theta) * cos(phi);
    double rvy = sin(theta) * sin(phi);
    double rvz = cos(theta);
    double ang = 1.414; // degrees

    // Generate unique temp files
    auto isawFile = boost::filesystem::temp_directory_path();
    isawFile /= boost::filesystem::unique_path("panelMove_%%%%%%%%.DetCal");
    auto xmlFile = boost::filesystem::temp_directory_path();
    xmlFile /= boost::filesystem::unique_path("panelMove_%%%%%%%%.xml");

    g_log.notice() << "-- generate simulated workspace\n";
    MatrixWorkspace_sptr ws = m_ws->clone();
    MatrixWorkspace_sptr wsraw = ws->clone();

    g_log.notice() << "-- for x(top) - bank73\n"
                   << "   translated by (" << dx << "," << dy << "," << dz
                   << ")\n"
                   << "   rotated by " << ang << "@(" << rvx << "," << rvy
                   << "," << rvz << ")\n";
    adjustComponent(dx, dy, dz, rvx, rvy, rvz, ang, bank_xtop, ws);

    g_log.notice() << "-- generate peaks\n";
    PeaksWorkspace_sptr pws = generateSimulatedPeaksWorkspace(ws);
    PeaksWorkspace_sptr pwsref = pws->clone();

    // Pretend we don't know the answer
    g_log.notice() << "-- reset instrument positions&orientations\n";
    g_log.notice()
        << "    * before reset x(top) - bank73:\n"
        << "    pos(abs) = "
        << pws->getInstrument()->getComponentByName(bank_xtop)->getPos() << "\n"
        << "    quat(rel) = "
        << pws->getInstrument()->getComponentByName(bank_xtop)->getRelativeRot()
        << "\n";
    Quat q0 = pws->getInstrument()
                  ->getComponentByName(bank_xtop)
                  ->getRelativeRot(); // this is the answer
    pws->setInstrument(wsraw->getInstrument());
    g_log.notice()
        << "    * after reset x(top) - bank73:\n"
        << "    pos(abs) = "
        << pws->getInstrument()->getComponentByName(bank_xtop)->getPos() << "\n"
        << "    quat(rel) = "
        << pws->getInstrument()->getComponentByName(bank_xtop)->getRelativeRot()
        << "\n";

    // Perform the calibration
    g_log.notice() << "-- start calibration\n";
    runCalibration(isawFile.string(), xmlFile.string(), pws, false, false,
                   true);

    // Check if the calibration returns the same instrument as we put in
    g_log.notice() << "-- validate calibration output\n";
    TS_ASSERT(validateCalibrationResults(pwsref, wsraw, xmlFile.string()));

    // Cleanup
    doCleanup();
  }

  /**
   * @brief Everything goes
   *
   * NOTE: not enough peaks on the y_panels, so we have to work with only the
   *       x_panels
   */
  void test_Exec() {
    g_log.notice() << "test: !calibrate L1 and two panels at the same time!\n";

    g_log.notice() << "Tolerance of Distance (meter) :" << TOLERANCE_L << "\n";
    g_log.notice() << "Tolerance of Rotation (degree) :" << TOLERANCE_R << "\n";

    // ----------------------------- //
    // ----- Precribe movement ----- //
    // ----------------------------- //
    //-- source
    const double dL1 = boost::math::constants::e<double>() / 100;
    //-- xtop
    double dx1 = 1.1e-2;
    double dy1 = -0.9e-2;
    double dz1 = 1.5e-2;
    double theta1 = PI / 3;
    double phi1 = PI / 8;
    double rvx1 = sin(theta1) * cos(phi1);
    double rvy1 = sin(theta1) * sin(phi1);
    double rvz1 = cos(theta1);
    double ang1 = 1.414; // degrees
    //-- xbottom
    double dx2 = 0.5e-2;
    double dy2 = 1.3e-2;
    double dz2 = -0.9e-2;
    double theta2 = PI / 4;
    double phi2 = PI / 3;
    double rvx2 = sin(theta2) * cos(phi2);
    double rvy2 = sin(theta2) * sin(phi2);
    double rvz2 = cos(theta2);
    double ang2 = 3.3; // degrees

    // ----------------------------------- //
    // ----- Generate Synthetic Data ----- //
    // ----------------------------------- //
    // Generate unique temp files
    auto isawFile = boost::filesystem::temp_directory_path();
    isawFile /= boost::filesystem::unique_path("testExec_%%%%%%%%.DetCal");
    auto xmlFile = boost::filesystem::temp_directory_path();
    xmlFile /= boost::filesystem::unique_path("testExec_%%%%%%%%.xml");

    g_log.notice() << "-- generate simulated workspace\n";
    MatrixWorkspace_sptr ws = m_ws->clone();
    MatrixWorkspace_sptr wsraw = ws->clone();

    // Source
    adjustComponent(0.0, 0.0, dL1, 1.0, 0.0, 0.0, 0.0,
                    ws->getInstrument()->getSource()->getName(), ws);
    g_log.notice() << "--Shift source by dL1 = " << dL1 << "\n";

    // Bank73
    adjustComponent(dx1, dy1, dz1, rvx1, rvy1, rvz1, ang1, bank_xtop, ws);
    g_log.notice() << "-- for x(top) - bank73\n"
                   << "   translated by (" << dx1 << "," << dy1 << "," << dz1
                   << ")\n"
                   << "   rotated by " << ang1 << "@(" << rvx1 << "," << rvy1
                   << "," << rvz1 << ")\n";

    // Bank11
    adjustComponent(dx2, dy2, dz2, rvx2, rvy2, rvz2, ang2, bank_xbottom, ws);
    g_log.notice() << "-- for x(bottom) - bank11\n"
                   << "   translated by (" << dx2 << "," << dy2 << "," << dz2
                   << ")\n"
                   << "   rotated by " << ang2 << "@(" << rvx2 << "," << rvy2
                   << "," << rvz2 << ")\n";

    g_log.notice() << "-- generate peaks\n";
    PeaksWorkspace_sptr pws = generateSimulatedPeaksWorkspace(ws);
    PeaksWorkspace_sptr pwsref = pws->clone();

    g_log.notice() << "-- reset instrument positions&orientations\n";
    pws->setInstrument(wsraw->getInstrument());

    // Perform the calibration
    g_log.notice() << "-- start calibration\n";
    runCalibration(isawFile.string(), xmlFile.string(), pws, false, true, true);

    // Check if the calibration returns the same instrument as we put in
    g_log.notice() << "-- validate calibration output\n";
    TS_ASSERT(validateCalibrationResults(pwsref, wsraw, xmlFile.string()));

    TS_ASSERT(false);

    // Cleanup
    doCleanup();
  }

private:
  // ---------------------------- //
  // ----- Helper Functions ----- //
  // -----------------------------//

  /**
   * @brief generate a simulated workspace for each testing case
   *
   * @return MatrixWorkspace_sptr
   */
  MatrixWorkspace_sptr generateSimulatedWorkspace() {

    // create simulated workspace
    IAlgorithm_sptr csws_alg =
        AlgorithmFactory::Instance().create("CreateSimulationWorkspace", 1);
    csws_alg->initialize();
    csws_alg->setLogging(LOGCHILDALG);
    csws_alg->setProperty("Instrument", "CORELLI");
    csws_alg->setProperty("BinParams", "1,100,10000");
    csws_alg->setProperty("UnitX", "TOF");
    csws_alg->setProperty("OutputWorkspace", wsname);
    csws_alg->execute();
    TS_ASSERT(csws_alg->isExecuted());

    // set UB
    IAlgorithm_sptr sub_alg = AlgorithmFactory::Instance().create("SetUB", 1);
    sub_alg->initialize();
    sub_alg->setLogging(LOGCHILDALG);
    sub_alg->setProperty("Workspace", wsname);
    sub_alg->setProperty("u", "1,0,0");
    sub_alg->setProperty("v", "0,1,0");
    sub_alg->setProperty("a", silicon_a);
    sub_alg->setProperty("b", silicon_b);
    sub_alg->setProperty("c", silicon_c);
    sub_alg->setProperty("alpha", silicon_alpha);
    sub_alg->setProperty("beta", silicon_beta);
    sub_alg->setProperty("gamma", silicon_gamma);
    sub_alg->execute();
    TS_ASSERT(sub_alg->isExecuted());

    MatrixWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsname);

    auto &sample = ws->mutableSample();
    sample.setCrystalStructure(silicon_cs);

    return ws;
  }

  /**
   * @brief populate peaks for the post adjustment simulated workspace
   * 
   * @return PeaksWorkspace_sptr 
   */
  PeaksWorkspace_sptr generateSimulatedPeaksWorkspace(MatrixWorkspace_sptr ws) {
    // prepare the algs pointer
    IAlgorithm_sptr sg_alg =
        AlgorithmFactory::Instance().create("SetGoniometer", 1);
    IAlgorithm_sptr pp_alg =
        AlgorithmFactory::Instance().create("PredictPeaks", 1);
    IAlgorithm_sptr cpw_alg =
        AlgorithmFactory::Instance().create("CombinePeaksWorkspaces", 1);

    // generate peaks for a range of omega values
    for (double omega = 0; omega <= 180; omega = omega + omega_step) {
      std::ostringstream os;
      os << omega << ",0,1,0,1";

      // set the SetGoniometer
      sg_alg->initialize();
      sg_alg->setLogging(LOGCHILDALG);
      sg_alg->setProperty("Workspace", ws);
      sg_alg->setProperty("Axis0", os.str());
      sg_alg->execute();

      // predict peak positions
      pp_alg->initialize();
      pp_alg->setLogging(LOGCHILDALG);
      pp_alg->setProperty("InputWorkspace", ws);
      pp_alg->setProperty("WavelengthMin", wavelength_min);
      pp_alg->setProperty("wavelengthMax", wavelength_max);
      pp_alg->setProperty("MinDSpacing", dspacing_min);
      pp_alg->setProperty("MaxDSpacing", dspacing_max);
      pp_alg->setProperty("ReflectionCondition", "All-face centred");

      if (omega < omega_step) {
        pp_alg->setProperty("OutputWorkspace", pwsname);
        pp_alg->execute();
      } else {
        pp_alg->setProperty("OutputWorkspace", tmppwsname);
        pp_alg->execute();

        // add the peaks to output peaks workspace
        cpw_alg->initialize();
        cpw_alg->setLogging(LOGCHILDALG);
        cpw_alg->setProperty("LHSWorkspace", tmppwsname);
        cpw_alg->setProperty("RHSWorkspace", pwsname);
        cpw_alg->setProperty("OutputWorkspace", pwsname);
        cpw_alg->execute();
      }
    }

    return AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(pwsname);
  }

  /**
   * @brief Adjust the position of a component through translation and rotation
   *
   * @param dx
   * @param dy
   * @param dz
   * @param rvx  x-component of rotation axis
   * @param rvy  y-component of rotation axis
   * @param rvz  z-component of rotation axis
   * @param drotang  rotation angle
   * @param cmptName
   * @param ws
   */
  void adjustComponent(double dx, double dy, double dz, double rvx, double rvy,
                       double rvz, double drotang, std::string cmptName,
                       MatrixWorkspace_sptr ws) {

    // rotation
    IAlgorithm_sptr rot_alg = Mantid::API::AlgorithmFactory::Instance().create(
        "RotateInstrumentComponent", -1);
    rot_alg->initialize();
    rot_alg->setLogging(LOGCHILDALG);
    rot_alg->setProperty("Workspace", ws);
    rot_alg->setProperty("ComponentName", cmptName);
    rot_alg->setProperty("X", rvx);
    rot_alg->setProperty("Y", rvy);
    rot_alg->setProperty("Z", rvz);
    rot_alg->setProperty("Angle", drotang);
    rot_alg->setProperty("RelativeRotation", true);
    rot_alg->executeAsChildAlg();

    // translation
    IAlgorithm_sptr mv_alg = Mantid::API::AlgorithmFactory::Instance().create(
        "MoveInstrumentComponent", -1);
    mv_alg->initialize();
    mv_alg->setLogging(LOGCHILDALG);
    mv_alg->setProperty("Workspace", ws);
    mv_alg->setProperty("ComponentName", cmptName);
    mv_alg->setProperty("X", dx);
    mv_alg->setProperty("Y", dy);
    mv_alg->setProperty("Z", dz);
    mv_alg->setProperty("RelativePosition", true);
    mv_alg->executeAsChildAlg();
  }

  /**
   * @brief shift T0 for both peakworkspace and all peaks
   *
   * @param dT0
   * @param pws
   */
  void adjustT0(double dT0, PeaksWorkspace_sptr pws) {
    // update the T0 record in peakworkspace
    Mantid::API::Run &run = pws->mutableRun();
    double T0 = 0.0;
    if (run.hasProperty("T0")) {
      T0 = run.getPropertyValueAsType<double>("T0");
    }
    T0 += dT0;
    run.addProperty<double>("T0", T0, true);

    // update wavelength of each peak using new T0
    for (int i = 0; i < pws->getNumberPeaks(); ++i) {
      Peak &pk = pws->getPeak(i);
      Units::Wavelength wl;
      wl.initialize(pk.getL1(), pk.getL2(), pk.getScattering(), 0,
                    pk.getInitialEnergy(), 0.0);
      pk.setWavelength(wl.singleFromTOF(pk.getTOF() + dT0));
    }
  }

  /**
   * @brief Run the calibration algorithm
   *
   * @param isawFilename
   * @param xmlFilename
   */
  void runCalibration(const std::string &isawFilename,
                      const std::string &xmlFilename, PeaksWorkspace_sptr pws,
                      bool calibrateT0, bool calibrateL1, bool calibrateBanks) {
    SCDCalibratePanels2 alg;
    alg.initialize();
    alg.setProperty("PeakWorkspace", pws);
    alg.setProperty("a", silicon_a);
    alg.setProperty("b", silicon_b);
    alg.setProperty("c", silicon_c);
    alg.setProperty("alpha", silicon_alpha);
    alg.setProperty("beta", silicon_beta);
    alg.setProperty("gamma", silicon_gamma);
    alg.setProperty("CalibrateT0", calibrateT0);
    alg.setProperty("CalibrateL1", calibrateL1);
    alg.setProperty("CalibrateBanks", calibrateBanks);
    alg.setProperty("DetCalFilename", isawFilename);
    alg.setProperty("XmlFilename", xmlFilename);
    alg.execute();
    TS_ASSERT(alg.isExecuted());
  }

  /**
   * @brief validate the calibration results by comparing component
   *        positions from reference Peakworkspace and workspace adjusted 
   *        using calibration output (xml)
   * 
   * @param refpws 
   * @param refws 
   * @param xmlFileName 
   * @return true 
   * @return false 
   */
  bool validateCalibrationResults(PeaksWorkspace_sptr refpws,
                                  MatrixWorkspace_sptr refws,
                                  const std::string &xmlFileName) {
    // Adjust components in reference workspace using calibration results
    IAlgorithm_sptr lpf_alg =
        AlgorithmFactory::Instance().create("LoadParameterFile", 1);
    lpf_alg->initialize();
    lpf_alg->setLogging(LOGCHILDALG);
    lpf_alg->setProperty("Workspace", refws);
    lpf_alg->setProperty("Filename", xmlFileName);
    TS_ASSERT(lpf_alg->execute());

    // compare each bank
    // -- get the names
    boost::container::flat_set<std::string> BankNames;
    for (int i = 0; i < refpws->getNumberPeaks(); ++i) {
      std::string bname = refpws->getPeak(i).getBankName();
      if (bname != "None")
        BankNames.insert(bname);
    }
    // -- perform per bank comparison
    Instrument_sptr inst1 = std::const_pointer_cast<Instrument>(
        refws->getInstrument()); // based on calibration
    Instrument_sptr inst2 = std::const_pointer_cast<Instrument>(
        refpws->getInstrument()); // reference one

    for (auto bankname : BankNames) {
      if (!compareComponent(inst1, inst2, bankname)) {
        g_log.error() << "--" << bankname << " mismatch\n";
        return false;
      }
    }

    // all banks are the same, now the source check will make the call
    if (!compareComponent(inst1, inst2, inst1->getSource()->getName())) {
        g_log.error() << "-- " << inst1->getSource()->getName()
                      << " mismatch\n";
        return false;
      }

    return true;
  }

  /**
   * @brief compare if two components to see if they have similar 
   *        translation and rotation
   *
   * @param instr1
   * @param instr2
   * @param componentName
   * @return true
   * @return false
   */
  bool compareComponent(std::shared_ptr<Instrument> & instr1,
                        std::shared_ptr<Instrument> & instr2,
                        std::string componentName) {

    std::shared_ptr<const IComponent> cmpt1 =
        instr1->getComponentByName(componentName);
    std::shared_ptr<const IComponent> cmpt2 =
        instr2->getComponentByName(componentName);

    V3D p1 = cmpt1->getRelativePos();
    V3D p2 = cmpt2->getRelativePos();

    Quat q1 = cmpt1->getRelativeRot();
    Quat q2 = cmpt2->getRelativeRot();

    q2.inverse();
    Quat dq = q1 * q2;
    double ang, ax0, ax1, ax2;
    dq.getAngleAxis(ang, ax0, ax1, ax2); // what is the unit of this angle?

    return (std::abs(p1.X() - p2.X()) < TOLERANCE_L) &&
           (std::abs(p1.Y() - p2.Y()) < TOLERANCE_L) &&
           (std::abs(p1.Z() - p2.Z()) < TOLERANCE_L) &&
           (std::abs(ang) < TOLERANCE_R);
  }

  /**
   * @brief remove all workspace memory after one test is done
   *
   */
  void doCleanup() {
    AnalysisDataService::Instance().remove(pwsname);
    AnalysisDataService::Instance().remove(tmppwsname);
  }

  // ------------------- //
  // ----- members ----- //
  // ------------------- //
  // workspace names
  const std::string wsname;
  const std::string pwsname;
  const std::string tmppwsname;

  MatrixWorkspace_sptr m_ws;

  // bank&panel names selected for testing
  // batch_1: high order zone selection
  const std::string bank_xtop;
  const std::string bank_xcenter;
  const std::string bank_xbottom;

  // batch_2: low order zone selection
  // NOTE: limited reflections from experiment, often
  //       considered as a chanllegening case
  const std::string bank_yright;
  const std::string bank_yleft;
  const std::string bank_ytop;
  const std::string bank_ybottom;

  // lattice constants of silicon
  const double silicon_a;
  const double silicon_b;
  const double silicon_c;
  const double silicon_alpha;
  const double silicon_beta;
  const double silicon_gamma;

  // silicon crystal structure
  CrystalStructure silicon_cs;

  // constants that select the recriprocal space
  const double dspacing_min;
  const double dspacing_max;
  const double wavelength_min;
  const double wavelength_max;
  const double omega_step;

  // check praramerter
  const double TOLERANCE_L; // distance
  const double TOLERANCE_R; // rotation angle
  const bool LOGCHILDALG; // whether to show individual alg log
  const double PI{3.141592653589793238462643383279502884};
};
