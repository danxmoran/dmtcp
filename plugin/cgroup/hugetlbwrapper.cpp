#include <dirent.h>
#include <jassert.h>
#include <string>

#include "wrappers.h"

using namespace dmtcp;


HugeTLBWrapper::HugeTLBWrapper(std::string name)
  : CGroupWrapper("hugetlb", name)
{
  numCtrlFiles = 0;
  ctrlFilePaths = pathList();

  DIR *dir = opendir(path.c_str());
  JASSERT(dir != NULL) (path);

  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL) {
    std::string entName = std::string(ent->d_name);
    if (entName.find(subsystem) == 0 &&
        entName.find("limit_in_bytes") != std::string::npos) {
      ctrlFilePaths.push_back(entName);
      numCtrlFiles++;
    }
  }

  closedir(dir);
}
