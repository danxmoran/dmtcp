#include "wrappers.h"

using namespace dmtcp;


CpuWrapper::CpuWrapper(std::string name)
  : CGroupWrapper("cpu", name)
{
  numCtrlFiles = 3;
  std::string cpuFiles[numCtrlFiles] = {
    "cpu.cfs_period_us",
    "cpu.cfs_quota_us",
    "cpu.shares"
  };
  ctrlFilePaths = pathList(cpuFiles, cpuFiles + numCtrlFiles);
}
