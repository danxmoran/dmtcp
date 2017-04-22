#include <fcntl.h>
#include <jassert.h>
#include <util.h>
#include <sys/stat.h>
#include <string>

#include "proccgroup.h"

using namespace dmtcp;


ProcCGroup::ProcCGroup(std::string subsystem, std::string name)
  : subsystem(subsystem),
  name(name),
  numFiles(0),
  ctrlFilePaths(pathList())
{
  std::string cgroupPath = CGROUP_PREFIX + subsystem + name;
  DIR *dir = opendir(cgroupPath.c_str());
  JASSERT(dir != NULL) (cgroupPath);

  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL) {
    std::string entName = std::string(ent->d_name);
    // Only try to dump control files for the current subsystem.
    if (entName.find(subsystem) == 0) {
      std::string entPath = cgroupPath + "/" + std::string(ent->d_name);
      struct stat fStat;
      // Only dump files with read-write access.
      int statRes = stat(entPath.c_str(), &fStat);
      JASSERT(statRes != -1) (JASSERT_ERRNO);
      if ((fStat.st_mode & S_IRUSR) && (fStat.st_mode & S_IWUSR)) {
        ctrlFilePaths.push_back(entPath);
      }
    }
  }

  closedir(dir);
  numFiles = ctrlFilePaths.size();
  ctrlFileIterator = ctrlFilePaths.begin();
}

ProcCGroup::~ProcCGroup()
{
  numFiles = 0;
}

size_t
ProcCGroup::getNumCtrlFiles()
{
  return numFiles;
}

void *
ProcCGroup::getNextCtrlFile(CtrlFileHeader &fileHdr)
{
  if (ctrlFileIterator == ctrlFilePaths.end()) {
    return NULL;
  }

  char buf[4096];
  ssize_t numRead = 0;

  strcpy(fileHdr.name, (*ctrlFileIterator).c_str());
  fileHdr.fileSize = 0;

  int fd = open(fileHdr.name, O_RDONLY);
  JASSERT(fd != -1) (JASSERT_ERRNO);

  do {
    numRead = Util::readAll(fd, buf, sizeof(buf));
    if (numRead > 0) {
      fileHdr.fileSize += numRead;
    }
  } while (numRead > 0);

  // Now allocate a buffer.
  size_t size = fileHdr.fileSize + 4096; // Add a one page buffer.
  void *data = JALLOC_HELPER_MALLOC(size);
  JASSERT(lseek(fd, 0, SEEK_SET) == 0);

  fileHdr.fileSize = Util::readAll(fd, data, size);
  JASSERT(fileHdr.fileSize < size) (fileHdr.name) (fileHdr.fileSize) (size);
  close(fd);

  ctrlFileIterator++;

  return data;
}
