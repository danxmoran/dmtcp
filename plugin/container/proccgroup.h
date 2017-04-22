#ifndef PROCCGROUP_H
#define PROCCGROUP_H

#include <sys/types.h>
#include <dirent.h>

#include "jalloc.h"

#define CGROUP_PREFIX std::string("/sys/fs/cgroup/")
#define NAMESIZE 1024

typedef struct CtrlFileHeader {
  char name[NAMESIZE];
  size_t fileSize;
} CtrlFileHeader;

typedef std::vector<std::string> pathList;

namespace dmtcp
{
class ProcCGroup
{
  public:
#ifdef JALIB_ALLOCATOR
    static void *operator new(size_t nbytes, void *p) { return p; }
    static void *operator new(size_t nbytes) { JALLOC_HELPER_NEW(nbytes); }
    static void operator delete(void *p) { JALLOC_HELPER_DELETE(p); }
#endif // ifdef JALIB_ALLOCATOR

    ProcCGroup(std::string subsystem, std::string name);
    ~ProcCGroup();

    size_t getNumCtrlFiles();
    void *getNextCtrlFile(CtrlFileHeader &fileHdr);

  private:
    std::string subsystem;
    std::string name;
    size_t numFiles;
    pathList ctrlFilePaths;
    pathList::iterator ctrlFileIterator;
};
}

#endif // ifndef PROCCGROUP_H
