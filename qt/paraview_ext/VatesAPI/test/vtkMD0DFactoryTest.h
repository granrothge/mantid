// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <cxxtest/TestSuite.h>

#include "MantidVatesAPI/vtkMD0DFactory.h"
#include "MockObjects.h"

using namespace Mantid::VATES;

class vtkMD0DFactoryTest : public CxxTest::TestSuite {
public:
  void testCreatesA0DDataSet() {
    // Arrange
    FakeProgressAction progressUpdater;
    vtkMD0DFactory factory;

    vtkSmartPointer<vtkDataSet> dataSet;

    // Assert
    TSM_ASSERT_THROWS_NOTHING("0D factory should create data set without exceptions",
                              dataSet = factory.create(progressUpdater));
    TSM_ASSERT("Should have exactly one point", dataSet->GetNumberOfPoints() == 1);
    TSM_ASSERT("Should have exactly one cell", dataSet->GetNumberOfCells() == 1);
  }
};
