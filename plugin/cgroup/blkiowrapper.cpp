#include "jassert.h"

#include "wrappers.h"

using namespace dmtcp;


BlkIOWrapper::BlkIOWrapper(std::string name)
  : CGroupWrapper("blkio", name)
{
  // TODO
  numCtrlFiles = /*8*/2;
  std::string blkioFiles[numCtrlFiles] = {
    "blkio.weight",
    "blkio.leaf_weight",
    /*"blkio.weight_device",
    "blkio.leaf_weight_device",
    "blkio.throttle.read_bps_device",
    "blkio.throttle.write_bps_device",
    "blkio.throttle.read_iops_device",
    "blkio.throttle.write_iops_device"*/
  };
  ctrlFilePaths = pathList(blkioFiles, blkioFiles + numCtrlFiles);
}
