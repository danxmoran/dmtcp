#include "wrappers.h"

using namespace dmtcp;


CpuSetWrapper::CpuSetWrapper(std::string name)
  : CGroupWrapper("cpuset", name)
{
  numCtrlFiles = 10;
  std::string cpusetFiles[numCtrlFiles] = {
    "cpuset.cpu_exclusive",
    "cpuset.cpus",
    "cpuset.mem_exclusive",
    "cpuset.mem_hardwall",
    "cpuset.memory_migrate",
    "cpuset.memory_spread_page",
    "cpuset.memory_spread_slab",
    "cpuset.mems",
    "cpuset.sched_load_balance",
    "cpuset.sched_relax_domain_level"
  };
  ctrlFilePaths = pathList(cpusetFiles, cpusetFiles + numCtrlFiles);
}
