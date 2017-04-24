#include "wrappers.h"

using namespace dmtcp;


NetPrioWrapper::NetPrioWrapper(std::string name)
  : CGroupWrapper("net_prio", name)
{
  numCtrlFiles = 1;
  ctrlFilePaths = pathList(1, "net_prio.ifpriomap");
}
