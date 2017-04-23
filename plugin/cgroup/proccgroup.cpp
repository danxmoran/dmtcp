#include <dirent.h>
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
  path = CGROUP_PREFIX + subsystem + name;
}

ProcCGroup::ProcCGroup(ProcCGroupHeader &groupHdr)
  : numFiles(0),
  ctrlFilePaths(pathList())
{
  name = std::string(groupHdr.name);
  subsystem = std::string(groupHdr.subsystem);
  path = CGROUP_PREFIX + subsystem + name;
}

ProcCGroup::~ProcCGroup()
{
  numFiles = 0;
}

void
ProcCGroup::getHeader(ProcCGroupHeader &hdr)
{
  strcpy(hdr.name, name.c_str());
  strcpy(hdr.subsystem, subsystem.c_str());
  hdr.numFiles = numFiles;
}

void
ProcCGroup::createIfNotExist()
{
  int mkRes = mkdir(path.c_str(), 755);
  if (mkRes == -1) {
    JASSERT(errno == EEXIST) (JASSERT_ERRNO);
  }
}

void
ProcCGroup::initCtrlFiles()
{
  DIR *dir = opendir(path.c_str());
  JASSERT(dir != NULL) (path);

  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL) {
    std::string entName = std::string(ent->d_name);
    // Only try to dump control files for the current subsystem.
    if (entName.find(subsystem) == 0) {
      // TODO(dan): How can we capture special cases like this?
      if (entName == "memory.oom_control") continue;

      std::string entPath = path + "/" + std::string(ent->d_name);
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

void *
ProcCGroup::getNextCtrlFile(CtrlFileHeader &fileHdr)
{
  if (numFiles == 0 || ctrlFileIterator == ctrlFilePaths.end()) {
    return NULL;
  }

  char buf[4096];
  ssize_t numRead = 0;

  strcpy(fileHdr.name, (*ctrlFileIterator).c_str());
  fileHdr.fileSize = 0;

  int fd = _real_open(fileHdr.name, O_RDONLY);
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
  _real_close(fd);

  ctrlFileIterator++;

  return data;
}

void
ProcCGroup::writeCtrlFile(CtrlFileHeader &fileHdr, void *contentBuf)
{
  int fd = _real_open(fileHdr.name, O_CREAT | O_TRUNC | O_WRONLY);
  JASSERT(fd != -1) (JASSERT_ERRNO);

  int writeRes = _real_write(fd, contentBuf, fileHdr.fileSize);
  _real_close(fd);
  JASSERT(writeRes > 0) (JASSERT_ERRNO) (fileHdr.name);
}

void
ProcCGroup::addPid(pid_t pid)
{
  std::string procsPath = path + "/cgroup.procs";
  int fd = _real_open(procsPath.c_str(), O_WRONLY);
  JASSERT(fd != -1) (JASSERT_ERRNO);

  char pidbuf[100];
  sprintf(pidbuf, "%d", pid);
  int writeRes = _real_write(fd, pidbuf, strlen(pidbuf));
  _real_close(fd);
  JASSERT(writeRes > 0) (JASSERT_ERRNO);
}
