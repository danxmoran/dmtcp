#include "wrappers.h"

using namespace dmtcp;


MemoryWrapper::MemoryWrapper(std::string name)
  : CGroupWrapper("memory", name)
{
  // TODO
  numCtrlFiles = /*9*/8;
  std::string memoryFiles[numCtrlFiles] = {
    "memory.limit_in_bytes",
    "memory.memsw.limit_in_bytes",
    "memory.soft_limit_in_bytes",
    "memory.use_hierarchy",
    "memory.swappiness",
    "memory.move_charge_at_immigrate",
    /*"memory.oom_control",*/
    "memory.kmem.limit_in_bytes",
    "memory.kmem.tcp.limit_in_bytes"
  };
  ctrlFilePaths = pathList(memoryFiles, memoryFiles + numCtrlFiles);
}
