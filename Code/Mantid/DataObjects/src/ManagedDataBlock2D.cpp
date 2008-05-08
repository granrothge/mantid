//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataObjects/ManagedDataBlock2D.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/WorkspaceFactory.h"
#include <iostream>

namespace Mantid
{
namespace DataObjects
{

// Get a reference to the logger
Kernel::Logger& ManagedDataBlock2D::g_log = Kernel::Logger::get("ManagedDataBlock2D");

/** Constructor.
 *  @param minIndex The index of the workspace that this data block starts at
 *  @param NVectors The number of Histogram1D's in this data block
 *  @param XLength  The number of elements in the X data
 *  @param YLength  The number of elements in the Y/E/E2 data
 */
ManagedDataBlock2D::ManagedDataBlock2D(const int &minIndex, const int &NVectors, 
    const int &XLength, const int &YLength) :
  m_data(NVectors), m_XLength(XLength), m_YLength(YLength), m_minIndex(minIndex), m_hasChanges(false)
{
  if (minIndex < 0 || NVectors < 0 || XLength < 0 || YLength < 0)
  {
    throw std::out_of_range("All arguments to ManagedDataBlock constructor must be positive");
  }

  // Set all the internal vectors to the right size
  for (std::vector<Histogram1D>::iterator it = m_data.begin(); it != m_data.end(); ++it)
  {
    it->dataX().resize(m_XLength);
    it->dataY().resize(m_YLength);
    it->dataE().resize(m_YLength);
    it->dataE2().resize(m_YLength);
  }
}

/// Virtual destructor
ManagedDataBlock2D::~ManagedDataBlock2D()
{
}

/// The minimum index of the workspace data that this data block contains
int ManagedDataBlock2D::minIndex() const
{
  return m_minIndex;
}

/** Flags whether the data has changed since being read in.
 *  In fact indicates whether the data has been accessed in a non-const fashion.
 *  @return True if the data has been changed.
 */
bool ManagedDataBlock2D::hasChanges() const
{
  return m_hasChanges;
}

/**
 Set the x values
 @param index Index to the histogram to set
 @param vec Vector to set
 */
void ManagedDataBlock2D::setX(const int index, const std::vector<double>& vec)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::setX, histogram number out of range");

  m_data[index-m_minIndex].dataX()=vec;
  m_hasChanges = true;
  return;
}

/**
 Set the x values
 @param index :: Index to the histogram
 @param vec :: Shared ptr base object
 */
void ManagedDataBlock2D::setX(const int index, const Histogram1D::RCtype::ptr_type& vec)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::setX, histogram number out of range");

  m_data[index-m_minIndex].setX(vec);
  m_hasChanges = true;
  return;
}

/**
 Set the x values
 @param index :: Index to the histogram
 @param PA :: Reference counted histogram
 */
void ManagedDataBlock2D::setX(const int index, const Histogram1D::RCtype& PA)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::setX, histogram number out of range");

  m_data[index-m_minIndex].setX(PA);
  m_hasChanges = true;
  return;
}

/**
 Sets the data in the workspace
 @param index The histogram to be set
 @param vec A vector containing the data 
 */
void ManagedDataBlock2D::setData(const int index, const std::vector<double>& vec)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::setData, histogram number out of range");

  m_data[index-m_minIndex].dataY()=vec;
  m_hasChanges = true;
  return;
}

/**
 Sets the data in the workspace (including errors)
 @param index The histogram to be set
 @param vec A vector containing the data 
 @param vecErr A vector containing the corresponding errors
 */
void ManagedDataBlock2D::setData(const int index, const std::vector<double>& vec,
    const std::vector<double>& vecErr)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::setData, histogram number out of range");

  m_data[index-m_minIndex].dataY()=vec;
  m_data[index-m_minIndex].dataE()=vecErr;
  m_hasChanges = true;
  return;
}

/**
 Sets the data in the workspace (including errors)
 @param index The histogram to be set
 @param vec A vector containing the data 
 @param vecErr A vector containing the corresponding errors
 @param vecErr2 A vector containing the 2nd error values
 */
void ManagedDataBlock2D::setData(const int index, const std::vector<double>& vec,
    const std::vector<double>& vecErr, const std::vector<double>& vecErr2)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::setData, histogram number out of range");

  m_data[index-m_minIndex].dataY()=vec;
  m_data[index-m_minIndex].dataE()=vecErr;
  m_data[index-m_minIndex].dataE2()=vecErr2;  
  m_hasChanges = true;
  return;
}

/**
 Sets the data in the workspace
 @param index The histogram to be set
 @param PY A reference counted data range  
 */
void ManagedDataBlock2D::setData(const int index, const Histogram1D::RCtype& PY)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::setData, histogram number out of range");

  m_data[index-m_minIndex].setData(PY);
  m_hasChanges = true;
  return;
}

/**
 Sets the data in the workspace
 @param index The histogram to be set
 @param PY A reference counted data range  
 @param PE A reference containing the corresponding errors
 */
void ManagedDataBlock2D::setData(const int index, const Histogram1D::RCtype& PY,
    const Histogram1D::RCtype& PE)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::setData, histogram number out of range");

  m_data[index-m_minIndex].setData(PY, PE);
  m_hasChanges = true;
  return;
}

/**
 Sets the data in the workspace
 @param index The histogram to be set
 @param PY A reference counted data range  
 @param PE A reference containing the corresponding errors
 @param PE2 A reference containing the 2nd error values
 */
void ManagedDataBlock2D::setData(const int index, const Histogram1D::RCtype& PY,
    const Histogram1D::RCtype& PE, const Histogram1D::RCtype& PE2)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::setData, histogram number out of range");

  m_data[index-m_minIndex].setData(PY, PE, PE2);
  m_hasChanges = true;
  return;
}

/**
 Sets the data in the workspace
 @param index The histogram to be set
 @param PY A reference counted data range  
 @param PE A reference containing the corresponding errors
 */
void ManagedDataBlock2D::setData(const int index, const Histogram1D::RCtype::ptr_type& PY,
    const Histogram1D::RCtype::ptr_type& PE)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::setData, histogram number out of range");

  m_data[index-m_minIndex].setData(PY, PE);
  m_hasChanges = true;
  return;
}

/**
 Sets the data in the workspace
 @param index The histogram to be set
 @param PY A reference counted data range  
 @param PE A reference containing the corresponding errors
 @param PE2 A reference containing the second error values
 */
void ManagedDataBlock2D::setData(const int index, const Histogram1D::RCtype::ptr_type& PY,
    const Histogram1D::RCtype::ptr_type& PE, const Histogram1D::RCtype::ptr_type& PE2)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::setData, histogram number out of range");

  m_data[index-m_minIndex].setData(PY, PE, PE2);
  m_hasChanges = true;
  return;
}

/**
  Get the x data of a specified histogram
  @param index The number of the histogram
  @return A vector of doubles containing the x data
*/
std::vector<double>& ManagedDataBlock2D::dataX(const int index)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::dataX, histogram number out of range");

  m_hasChanges = true;  
  return m_data[index-m_minIndex].dataX();
}

/**
  Get the y data of a specified histogram
  @param index The number of the histogram
  @return A vector of doubles containing the y data
*/
std::vector<double>& ManagedDataBlock2D::dataY(const int index)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::dataY, histogram number out of range");

  m_hasChanges = true;
  return m_data[index-m_minIndex].dataY();
}

/**
  Get the error data for a specified histogram
  @param index The number of the histogram
  @return A vector of doubles containing the error data
*/
std::vector<double>& ManagedDataBlock2D::dataE(const int index)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::dataE, histogram number out of range");

  m_hasChanges = true;
  return m_data[index-m_minIndex].dataE();
}

/**
  Get the error data for a specified histogram
  @param index The number of the histogram
  @return A vector of doubles containing the error (second value) data
*/
std::vector<double>& ManagedDataBlock2D::dataE2(const int index)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::dataE2, histogram number out of range");

  m_hasChanges = true;
  return m_data[index-m_minIndex].dataE2();
}

/**
  Get the x data of a specified histogram
  @param index The number of the histogram
  @return A vector of doubles containing the x data
*/
const std::vector<double>& ManagedDataBlock2D::dataX(const int index) const
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::dataX, histogram number out of range");

  return m_data[index-m_minIndex].dataX();
}

/**
  Get the y data of a specified histogram
  @param index The number of the histogram
  @return A vector of doubles containing the y data
*/
const std::vector<double>& ManagedDataBlock2D::dataY(const int index) const
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::dataY, histogram number out of range");

  return m_data[index-m_minIndex].dataY();
}

/**
  Get the error data for a specified histogram
  @param index The number of the histogram
  @return A vector of doubles containing the error data
*/
const std::vector<double>& ManagedDataBlock2D::dataE(const int index) const
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::dataE, histogram number out of range");

  return m_data[index-m_minIndex].dataE();
}

/**
  Get the error data for a specified histogram
  @param index The number of the histogram
  @return A vector of doubles containing the error (second value) data
*/
const std::vector<double>& ManagedDataBlock2D::dataE2(const int index) const
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::dataE2, histogram number out of range");

  return m_data[index-m_minIndex].dataE2();
}

///Returns the ErrorHelper applicable for this spectra
const API::IErrorHelper* ManagedDataBlock2D::errorHelper(int const index) const
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::errorHelper, histogram number out of range");

  return m_data[index-m_minIndex].errorHelper();
}

///Returns the spectrum number to which a given index refers
int ManagedDataBlock2D::spectraNo(int const index) const
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::spectraNo, histogram number out of range");

  return m_data[index-m_minIndex].spectraNo();
}

///Returns the spectrum number to which a given index refers
int& ManagedDataBlock2D::spectraNo(int const index)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::spectraNo, histogram number out of range");

  return m_data[index-m_minIndex].spectraNo();
}

///Sets the ErrorHelper for this spectra
void ManagedDataBlock2D::setErrorHelper(int const index,API::IErrorHelper* errorHelper)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::setErrorHelper, histogram number out of range");

  m_data[index-m_minIndex].setErrorHelper(errorHelper);
}

///Sets the ErrorHelper for this spectra
void ManagedDataBlock2D::setErrorHelper(int const index,const API::IErrorHelper* errorHelper)
{
  if ( ( index < m_minIndex ) 
      || ( index >= static_cast<int>(m_minIndex + m_data.size()) ) )
    throw std::range_error("ManagedDataBlock2D::setErrorHelper, histogram number out of range");

  m_data[index-m_minIndex].setErrorHelper(errorHelper);
}


/** Output file stream operator.
 *  @param fs The stream to write to
 *  @param data The object to write to file
 */
std::fstream& operator<<(std::fstream& fs, ManagedDataBlock2D& data)
{
  for (std::vector<Histogram1D>::iterator it = data.m_data.begin(); it != data.m_data.end(); ++it)
  {
    // If anyone's gone and changed the size of the vectors then get them back to the
    // correct size, removing elements or adding zeroes as appropriate.
    if (it->dataX().size() != static_cast<unsigned int>(data.m_XLength))
    {
      it->dataX().resize(data.m_XLength, 0.0);
      ManagedDataBlock2D::g_log.warning() << "X vector resized to " << data.m_XLength << " elements.";
    }
    fs.write((char *) &*it->dataX().begin(), data.m_XLength * sizeof(double));
    if (it->dataY().size() != static_cast<unsigned int>(data.m_YLength))
    {
      it->dataY().resize(data.m_YLength, 0.0);
      ManagedDataBlock2D::g_log.warning() << "Y vector resized to " << data.m_YLength << " elements.";
    }
    fs.write((char *) &*it->dataY().begin(), data.m_YLength * sizeof(double));
    if (it->dataE().size() != static_cast<unsigned int>(data.m_YLength))
    {
      it->dataE().resize(data.m_YLength, 0.0);
      ManagedDataBlock2D::g_log.warning() << "E vector resized to " << data.m_YLength << " elements.";
    }
    fs.write((char *) &*it->dataE().begin(), data.m_YLength * sizeof(double));
    if (it->dataE2().size() != static_cast<unsigned int>(data.m_YLength))
    {
      it->dataE2().resize(data.m_YLength, 0.0);
      ManagedDataBlock2D::g_log.warning() << "E2 vector resized to " << data.m_YLength << " elements.";
    }
    fs.write((char *) &*it->dataE2().begin(), data.m_YLength * sizeof(double));
    
    // Write out the spectrum number member of Histogram1D
    fs.write((char *) &it->spectraNo(), sizeof(int));
    // N.B. ErrorHelper member not stored to file so will always be Gaussian default
  }
  return fs;
}

/** Input file stream operator.
 *  @param fs The stream to read from
 *  @param data The object to fill with the read-in data
 */
std::fstream& operator>>(std::fstream& fs, ManagedDataBlock2D& data)
{ 
  // Assumes that the ManagedDataBlock2D passed in is the same size as the data to be read in
  // Will be ManagedWorkspace2D's job to ensure that this is so
  for (std::vector<Histogram1D>::iterator it = data.m_data.begin(); it != data.m_data.end(); ++it)
  {
    it->dataX().resize(data.m_XLength, 0.0);
    fs.read((char *) &*it->dataX().begin(), data.m_XLength * sizeof(double));
    it->dataY().resize(data.m_YLength, 0.0);
    fs.read((char *) &*it->dataY().begin(), data.m_YLength * sizeof(double));
    it->dataE().resize(data.m_YLength, 0.0);
    fs.read((char *) &*it->dataE().begin(), data.m_YLength * sizeof(double));
    it->dataE2().resize(data.m_YLength, 0.0);
    fs.read((char *) &*it->dataE2().begin(), data.m_YLength * sizeof(double));
    
    // Read in the spectrum number member of Histogram1D
    fs.read((char *) &it->spectraNo(), sizeof(int));
    // N.B. ErrorHelper member not stored to file so will always be Gaussian default
  }
  return fs;
}

} // namespace DataObjects
} // namespace Mantid
