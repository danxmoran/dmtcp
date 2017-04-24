#include "wrappers.h"

using namespace dmtcp;


DevicesWrapper::DevicesWrapper(std::string name)
  : CGroupWrapper("devices", name)
{
  // TODO the devices subsystem requires CAP_SYS_ADMIN to modify any parameters,
  // even if the virtual files are owned by a non-root user.
  // DMTCP currently shies away from running as root, so it can't do anything
  // useful for this subsystem other than restore the name.
  numCtrlFiles = 0;
  ctrlFilePaths = pathList();
}
