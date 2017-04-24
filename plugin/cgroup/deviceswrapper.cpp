#include "wrappers.h"

using namespace dmtcp;


DevicesWrapper::DevicesWrapper(std::string name)
  : CGroupWrapper("devices", name)
{
  // TODO: the devices subgroup has 2 writeable files and 1 read-only file
  // which summarizes the writes to the other 2. We should copy the read-only
  // file and then dump to the write-only files.
  numCtrlFiles = 0;
  ctrlFilePaths = pathList();
}
