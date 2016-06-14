#ifndef TESTHISTOGRAM1D_
#define TESTHISTOGRAM1D_

#include <vector>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/Histogram1D.h"

using Mantid::DataObjects::Histogram1D;
using Mantid::MantidVec;
using Mantid::Kernel::make_cow;
using namespace Mantid::HistogramData;

class Histogram1DTest : public CxxTest::TestSuite {
private:
  int nel; // Number of elements in the array
  Histogram1D h{Histogram::XMode::Points};
  Histogram1D h2{Histogram::XMode::Points};
  MantidVec x1, y1, e1; // vectors
  boost::shared_ptr<HistogramY> pa;
  boost::shared_ptr<HistogramE> pb;
public:
  void setUp() override {
    nel = 100;
    x1.resize(nel);
    std::fill(x1.begin(), x1.end(), rand());
    y1.resize(nel);
    std::fill(y1.begin(), y1.end(), rand());
    e1.resize(nel);
    pa = boost::make_shared<HistogramY>(nel);
    std::fill(pa->begin(), pa->end(), rand());
    pb = boost::make_shared<HistogramE>(nel);
    std::fill(pb->begin(), pb->end(), rand());
    h.setHistogram(Histogram(Points(100)));
    h2.setHistogram(Histogram(Points(100)));
    h.setCounts(100);
    h.setCountStandardDeviations(100);
    h2.setCounts(100);
    h2.setCountStandardDeviations(100);
  }

  void testsetgetXvector() {
    h.setPoints(x1);
    TS_ASSERT_EQUALS(x1, h.dataX());
  }
  void testcopyX() {
    h2.setPoints(x1);
    h.dataX() = h2.dataX();
    TS_ASSERT_EQUALS(h.dataX(), x1);
  }
  void testsetgetDataYVector() {
    h.setCounts(y1);
    TS_ASSERT_EQUALS(h.dataY(), y1);
  }
  void testsetgetDataYEVector() {
    h.setCounts(y1);
    h.setCountStandardDeviations(e1);
    TS_ASSERT_EQUALS(h.dataY(), y1);
    TS_ASSERT_EQUALS(h.dataE(), e1);
  }
  void testmaskSpectrum() {
    h.clearData();
    TS_ASSERT_EQUALS(h.dataY()[5], 0.0);
    TS_ASSERT_EQUALS(h.dataE()[12], 0.0);
  }
  void testsetgetXPointer() {
    auto px = boost::make_shared<HistogramX>(0);
    h.setX(px);
    TS_ASSERT_EQUALS(&(*h.ptrX()), &(*px));
  }
  void testsetgetDataYPointer() {
    h.setCounts(pa);
    TS_ASSERT_EQUALS(h.dataY(), pa->rawData());
  }
  void testsetgetDataYEPointer() {
    h.setCounts(pa);
    h.setCountStandardDeviations(pb);
    TS_ASSERT_EQUALS(h.dataY(), pa->rawData());
    TS_ASSERT_EQUALS(h.dataE(), pb->rawData());
  }
  void testgetXindex() {
    h.setPoints(x1);
    TS_ASSERT_EQUALS(h.dataX()[4], x1[4]);
  }
  void testgetYindex() {
    h.setCounts(y1);
    TS_ASSERT_EQUALS(h.dataY()[4], y1[4]);
  }
  void testgetEindex() {
    h.setCounts(y1);
    h.setCountStandardDeviations(e1);
    TS_ASSERT_EQUALS(h.dataE()[4], e1[4]);
  }
  void testrangeexceptionX() {
    h.setPoints(x1);
    TS_ASSERT_THROWS(h.dataX().at(nel), std::out_of_range);
  }
  void testrangeexceptionY() {
    h.setCounts(y1);
    TS_ASSERT_THROWS(h.dataY().at(nel), std::out_of_range);
  }
  void testrangeexceptionE() {
    h.setCounts(y1);
    h.setCountStandardDeviations(e1);
    TS_ASSERT_THROWS(h.dataE().at(nel), std::out_of_range);
  }

  void test_copy_constructor() {
    const Histogram1D source(Histogram::XMode::Points);
    Histogram1D clone(source);
    TS_ASSERT_EQUALS(&clone.readX(), &source.readX());
    TS_ASSERT_EQUALS(&clone.readY(), &source.readY());
    TS_ASSERT_EQUALS(&clone.readE(), &source.readE());
  }

  void test_move_constructor() {
    Histogram1D source(Histogram::XMode::Points);
    auto oldX = &source.readX();
    auto oldY = &source.readY();
    auto oldE = &source.readE();
    Histogram1D clone(std::move(source));
    TS_ASSERT(!source.ptrX());
    TS_ASSERT_EQUALS(&clone.readX(), oldX);
    TS_ASSERT_EQUALS(&clone.readY(), oldY);
    TS_ASSERT_EQUALS(&clone.readE(), oldE);
  }

  void test_constructor_from_ISpectrum() {
    Histogram1D resource(Histogram::XMode::Points);
    resource.dataX() = {0.1};
    resource.dataY() = {0.2};
    resource.dataE() = {0.3};
    const Mantid::API::ISpectrum &source = resource;
    Histogram1D clone(source);
    // X is shared...
    TS_ASSERT_EQUALS(&clone.readX(), &source.readX());
    // .. but not Y and E, since they are not part of ISpectrum.
    TS_ASSERT_DIFFERS(&clone.readY(), &source.readY());
    TS_ASSERT_DIFFERS(&clone.readE(), &source.readE());
    TS_ASSERT_EQUALS(clone.readX()[0], 0.1);
    TS_ASSERT_EQUALS(clone.readY()[0], 0.2);
    TS_ASSERT_EQUALS(clone.readE()[0], 0.3);
  }

  void test_copy_assignment() {
    const Histogram1D source(Histogram::XMode::Points);
    Histogram1D clone(Histogram::XMode::Points);
    clone = source;
    TS_ASSERT_EQUALS(&clone.readX(), &source.readX());
    TS_ASSERT_EQUALS(&clone.readY(), &source.readY());
    TS_ASSERT_EQUALS(&clone.readE(), &source.readE());
  }

  void test_move_assignment() {
    Histogram1D source(Histogram::XMode::Points);
    auto oldX = &source.readX();
    auto oldY = &source.readY();
    auto oldE = &source.readE();
    Histogram1D clone(Histogram::XMode::Points);
    clone = std::move(source);
    TS_ASSERT(!source.ptrX());
    TS_ASSERT_EQUALS(&clone.readX(), oldX);
    TS_ASSERT_EQUALS(&clone.readY(), oldY);
    TS_ASSERT_EQUALS(&clone.readE(), oldE);
  }

  void test_assign_ISpectrum() {
    Histogram1D resource(Histogram::XMode::Points);
    resource.dataX() = {0.1};
    resource.dataY() = {0.2};
    resource.dataE() = {0.3};
    const Mantid::API::ISpectrum &source = resource;
    Histogram1D clone(Histogram::XMode::Points);
    clone = source;
    // X is shared...
    TS_ASSERT_EQUALS(&clone.readX(), &source.readX());
    // .. but not Y and E, since they are not part of ISpectrum.
    TS_ASSERT_DIFFERS(&clone.readY(), &source.readY());
    TS_ASSERT_DIFFERS(&clone.readE(), &source.readE());
    TS_ASSERT_EQUALS(clone.readX()[0], 0.1);
    TS_ASSERT_EQUALS(clone.readY()[0], 0.2);
    TS_ASSERT_EQUALS(clone.readE()[0], 0.3);
  }
};
#endif /*TESTHISTOGRAM1D_*/
