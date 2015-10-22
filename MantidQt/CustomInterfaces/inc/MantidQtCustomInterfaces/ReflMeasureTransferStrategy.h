#ifndef MANTID_CUSTOMINTERFACES_REFLMEASURETRANSFERSTRATEGY_H_
#define MANTID_CUSTOMINTERFACES_REFLMEASURETRANSFERSTRATEGY_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/ReflTransferStrategy.h"

#include <vector>
#include <map>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** ReflMeasureTransferStrategy : Transfer strategy that uses the measurement
  information
  from the loaded workspaces to complete the transfer.

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_CUSTOMINTERFACES_DLL ReflMeasureTransferStrategy
    : public ReflTransferStrategy {
public:
  ReflMeasureTransferStrategy();

  virtual std::vector<std::map<std::string, std::string>>
  transferRuns(const SearchResultMap &searchResults,
               Mantid::Kernel::ProgressBase &progress);

  virtual ReflMeasureTransferStrategy *clone() const;

  virtual bool knownFileType(const std::string &filename) const;

  virtual ~ReflMeasureTransferStrategy();
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_REFLMEASURETRANSFERSTRATEGY_H_ */
