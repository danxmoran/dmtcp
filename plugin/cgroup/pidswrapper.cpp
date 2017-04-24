#include "wrappers.h"

using namespace dmtcp;


PidsWrapper::PidsWrapper(std::string name)
  : CGroupWrapper("pids", name)
{
  numCtrlFiles = 1;
  ctrlFilePaths = pathList(1, "pids.max");
}
