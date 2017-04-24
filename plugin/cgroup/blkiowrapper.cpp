#include "jassert.h"

#include "wrappers.h"

using namespace dmtcp;


BlkIOWrapper::BlkIOWrapper(std::string name)
  : CGroupWrapper("blkio", name)
{
  numCtrlFiles = /*8*/2;
  std::string blkioFiles[numCtrlFiles] = {
    "blkio.weight",
    "blkio.leaf_weight",
    // TODO trying to set device-specific parameters throws
    // "Operation not permitted" errors. It's not documented, but
    // you might need CAP_SYS_ADMIN to set these parameters in
    // the same way you need it to set any values in the devices
    // subsystem.
    /*"blkio.weight_device",
    "blkio.leaf_weight_device",
    "blkio.throttle.read_bps_device",
    "blkio.throttle.write_bps_device",
    "blkio.throttle.read_iops_device",
    "blkio.throttle.write_iops_device"*/
  };
  ctrlFilePaths = pathList(blkioFiles, blkioFiles + numCtrlFiles);
}
