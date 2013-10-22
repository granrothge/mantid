#ifndef SAVEASCIITEST_H_
#define SAVEASCIITEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/SaveAscii2.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FrameworkManager.h"
#include <fstream>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

// This test tests that SaveAscii produces a file of the expected form
// It does not test that the file can be loaded by loadAscii.
// The test LoadSaveAscii does that and should be run in addition to this test
// if you modify SaveAscii.

class SaveAscii2Test : public CxxTest::TestSuite
{

public:

  static SaveAscii2Test *createSuite() { return new SaveAscii2Test(); }
  static void destroySuite(SaveAscii2Test *suite) { delete suite; }

  SaveAscii2Test()
  {

  }
  ~SaveAscii2Test()
  {
  }

  void testExec()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave = boost::dynamic_pointer_cast<
      Mantid::DataObjects::Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", 2, 3, 3));
    for (int i = 0; i < 2; i++)
    {
      std::vector<double>& X = wsToSave->dataX(i);
      std::vector<double>& Y = wsToSave->dataY(i);
      std::vector<double>& E = wsToSave->dataE(i);
      for (int j = 0; j < 3; j++)
      {
        X[j] = 1.5 * j / 0.9;
        Y[j] = (i + 1) * (2. + 4. * X[j]);
        E[j] = 1.;
      }
    }
    const std::string name = "SaveAsciiWS";
    AnalysisDataService::Instance().add(name, wsToSave);

    std::string filename = "SaveAsciiTestFile.dat";
    std::string filename_nohead ="SaveAsciiTestFileWithoutHeader.dat";

    SaveAscii2 save;
    TS_ASSERT_THROWS_NOTHING(save.initialize());
    TS_ASSERT(save.isInitialized());
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Filename", filename));
    filename = save.getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("InputWorkspace", name));
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT( Poco::File(filename).exists() );

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());
    int specID;
    std::string header1, header2, header3, separator, comment;

    // Test that the first few column headers, separator and first two bins are as expected
    in >> comment >> header1 >> separator >> header2 >> separator >> header3 >> specID;
    TS_ASSERT_EQUALS(specID, 1 );
    TS_ASSERT_EQUALS(comment,"#" );
    TS_ASSERT_EQUALS(separator,"," );
    TS_ASSERT_EQUALS(header1,"X" );
    TS_ASSERT_EQUALS(header2,"Y" );
    TS_ASSERT_EQUALS(header3,"E" );

    std::string binlines;
    std::vector<std::string> binstr;
    std::vector<double> bins;
    std::getline(in,binlines);
    std::getline(in,binlines);

    boost::split(binstr, binlines,boost::is_any_of(","));
    for (int i = 0; i < binstr.size(); i++)
    {
      bins.push_back(boost::lexical_cast<double>(binstr.at(i)));
    }
    TS_ASSERT_EQUALS(bins[0], 0 );
    TS_ASSERT_EQUALS(bins[1], 2 );
    TS_ASSERT_EQUALS(bins[2], 1 );

    std::getline(in,binlines);
    bins.clear();
    boost::split(binstr, binlines,boost::is_any_of(","));
    for (int i = 0; i < binstr.size(); i++)
    {
      bins.push_back(boost::lexical_cast<double>(binstr.at(i)));
    }
    TS_ASSERT_EQUALS(bins[0], 1.66667 );
    TS_ASSERT_EQUALS(bins[1], 8.66667 );
    TS_ASSERT_EQUALS(bins[2], 1 );

    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(name);
  }

  void testExec_no_header()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave = boost::dynamic_pointer_cast<
      Mantid::DataObjects::Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", 2, 3, 3));
    for (int i = 0; i < 2; i++)
    {
      std::vector<double>& X = wsToSave->dataX(i);
      std::vector<double>& Y = wsToSave->dataY(i);
      std::vector<double>& E = wsToSave->dataE(i);
      for (int j = 0; j < 3; j++)
      {
        X[j] = 1.5 * j / 0.9;
        Y[j] = (i + 1) * (2. + 4. * X[j]);
        E[j] = 1.;
      }
    }
    const std::string name = "SaveAsciiWS";
    AnalysisDataService::Instance().add(name, wsToSave);

    std::string filename = "SaveAsciiTestFile.dat";
    std::string filename_nohead ="SaveAsciiTestFileWithoutHeader.dat";

    SaveAscii2 save;
    TS_ASSERT_THROWS_NOTHING(save.initialize());
    TS_ASSERT(save.isInitialized());
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Filename", filename));
    filename = save.getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("InputWorkspace", name));
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT( Poco::File(filename).exists() );

    // Test ColumnHeader property - perhaps this should be a separate test
    save.setPropertyValue("Filename", filename_nohead);
    save.setPropertyValue("InputWorkspace", name);
    TS_ASSERT_THROWS_NOTHING(save.setProperty("ColumnHeader", false));
    filename_nohead = save.getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT( Poco::File(filename_nohead).exists() );

    // Now we check that the first line of the file without header matches the second line of the file with header
    std::ifstream in1(filename.c_str());
    std::string line2header;
    std::ifstream in2(filename_nohead.c_str());
    std::string line1noheader;
    getline(in1,line2header);
    getline(in1,line2header);
    getline(in1,line2header); // 3rd line of file with header
    getline(in2,line1noheader);
    getline(in2,line1noheader); // 2nd line of file without header
    TS_ASSERT_EQUALS(line1noheader,line2header);
    in1.close();
    in2.close();
    // Remove files
    Poco::File(filename).remove();
    Poco::File(filename_nohead).remove();
    AnalysisDataService::Instance().remove(name);
  }
};


#endif /*SAVEASCIITEST_H_*/
