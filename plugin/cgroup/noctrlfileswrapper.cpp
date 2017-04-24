#include "wrappers.h"

using namespace dmtcp;


NoCtrlFilesWrapper::NoCtrlFilesWrapper(std::string subsystem, std::string name)
  : CGroupWrapper(subsystem, name)
{
  ctrlFilePaths = pathList();
  numCtrlFiles = 0;
}
