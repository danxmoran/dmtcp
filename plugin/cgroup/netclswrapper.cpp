#include "wrappers.h"

using namespace dmtcp;


NetClsWrapper::NetClsWrapper(std::string name)
  : CGroupWrapper("net_cls", name)
{
  numCtrlFiles = 1;
  ctrlFilePaths = pathList(1, "net_cls.classid");
}
