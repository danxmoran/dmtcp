#include <dirent.h>
#include <jassert.h>
#include <algorithm>

#include "wrappers.h"

using namespace dmtcp;


MemoryWrapper::MemoryWrapper(std::string name)
  : CGroupWrapper("memory", name)
{
  // TODO
  size_t maxCtrlFiles = 9;
  std::string memoryFiles[maxCtrlFiles] = {
    "memory.limit_in_bytes",
    "memory.soft_limit_in_bytes",
    "memory.use_hierarchy",
    "memory.swappiness",
    "memory.move_charge_at_immigrate",
    "memory.oom_control",
    "memory.kmem.limit_in_bytes",
    "memory.kmem.tcp.limit_in_bytes",
    // Note: this file might not exist
    "memory.memsw.limit_in_bytes"
  };

  numCtrlFiles = 0;
  ctrlFilePaths = pathList();

  DIR *dir = opendir(path.c_str());
  JASSERT(dir != NULL) (path);

  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL) {
    std::string entName = std::string(ent->d_name);
    std::string *entLoc = std::find(memoryFiles,
                                    memoryFiles + maxCtrlFiles,
                                    entName);
    if (entLoc != memoryFiles + maxCtrlFiles) {
      ctrlFilePaths.push_back(entName);
      numCtrlFiles++;
    }
  }

  closedir(dir);
}

void
MemoryWrapper::writeCtrlFile(CtrlFileHeader &fileHdr, void *contentBuf)
{
  std::string basename = std::string(fileHdr.name);
  if (basename == "memory.oom_control") {
    // The file `memory.oom_control` contains summary information in addition
    // to a single tunable parameter.
    // Extract the tunable parameter, and write it.
    CtrlFileHeader modifiedHdr;
    strcpy(modifiedHdr.name, fileHdr.name);

    (void)strtok((char *)contentBuf, " \t");
    char *oomKillDisabled = strtok(NULL, "\n");
    modifiedHdr.fileSize = strlen(oomKillDisabled);
    CGroupWrapper::writeCtrlFile(modifiedHdr, (void *)oomKillDisabled);
  } else {
    CGroupWrapper::writeCtrlFile(fileHdr, contentBuf);
  }
}
