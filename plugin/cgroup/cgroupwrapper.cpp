#include <jassert.h>
#include <util.h>
#include <sys/stat.h>
#include <string>

#include "cgroupwrapper.h"
#include "wrappers.h"

using namespace dmtcp;


CGroupWrapper *
CGroupWrapper::build(std::string subsystem, std::string name)
{
  if (subsystem == "cpuset") {
    return new CpuSetWrapper(name);
  } else if (subsystem == "cpu") {
    return new CpuWrapper(name);
  } else if (subsystem == "blkio") {
    return new BlkIOWrapper(name);
  } else if (subsystem == "memory") {
    return new MemoryWrapper(name);
  } else if (subsystem == "devices") {
    return new DevicesWrapper(name);
  } else if (subsystem == "net_cls") {
    return new NetClsWrapper(name);
  } else if (subsystem == "net_prio") {
    return new NetPrioWrapper(name);
  } else if (subsystem == "hugetlb") {
    return new HugeTLBWrapper(name);
  } else if (subsystem == "pids") {
    return new PidsWrapper(name);
  } else {
    return new NoCtrlFilesWrapper(subsystem, name);
  }
}

CGroupWrapper *
CGroupWrapper::build(CGroupHeader &groupHdr)
{
  return build(std::string(groupHdr.subsystem), std::string(groupHdr.name));
}

CGroupWrapper::CGroupWrapper(std::string subsystem, std::string name)
  : subsystem(subsystem),
  name(name)
{
  path = CGROUP_PREFIX + subsystem + name;
}

void
CGroupWrapper::getHeader(CGroupHeader &hdr)
{
  strcpy(hdr.name, name.c_str());
  strcpy(hdr.subsystem, subsystem.c_str());
  hdr.numFiles = numCtrlFiles;
}

void
CGroupWrapper::createIfMissing()
{
  int mkRes = mkdir(path.c_str(), 0755);
  if (mkRes == -1) {
    JASSERT(errno == EEXIST) (JASSERT_ERRNO);
  }
}

void *
CGroupWrapper::getNextCtrlFile(CtrlFileHeader &fileHdr)
{
  if (numCtrlFiles == 0 || ctrlFileIterator == ctrlFilePaths.end()) {
    return NULL;
  }

  char buf[4096];
  ssize_t numRead = 0;

  int fd = _real_open((path + "/" + *ctrlFileIterator).c_str(), O_RDONLY);
  JASSERT(fd != -1) (JASSERT_ERRNO) (*ctrlFileIterator);

  strcpy(fileHdr.name, (*ctrlFileIterator).c_str());
  fileHdr.fileSize = 0;

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
CGroupWrapper::addPid(pid_t pid)
{
  std::string procsPath = path + "/cgroup.procs";
  int fd = _real_open(procsPath.c_str(), O_WRONLY);
  JASSERT(fd != -1) (JASSERT_ERRNO) (procsPath);

  char pidbuf[100];
  sprintf(pidbuf, "%d", pid);
  int writeRes = _real_write(fd, pidbuf, strlen(pidbuf));
  _real_close(fd);
  JASSERT(writeRes != -1) (JASSERT_ERRNO);
}

void
CGroupWrapper::initCtrlFiles()
{
  ctrlFileIterator = ctrlFilePaths.begin();
}

void
CGroupWrapper::writeCtrlFile(CtrlFileHeader &fileHdr, void *contentBuf)
{
  std::string basename = std::string(fileHdr.name);
  JASSERT(basename.find(subsystem) == 0) (fileHdr.name);
  int fd = _real_open((path + "/" + basename).c_str(), O_WRONLY);
  JASSERT(fd != -1) (JASSERT_ERRNO);

  int writeRes = _real_write(fd, contentBuf, fileHdr.fileSize);
  _real_close(fd);
  JASSERT(writeRes != -1) (JASSERT_ERRNO) (fileHdr.name);
}
