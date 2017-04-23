#ifndef PROCCGROUP_H
#define PROCCGROUP_H

#include <sys/types.h>

#include "dmtcp.h"
#include "jalloc.h"

#define CGROUP_PREFIX std::string("/sys/fs/cgroup/")
#define NAMESIZE 1024

#define _real_open   NEXT_FNC(open)
#define _real_close  NEXT_FNC(close)
#define _real_read   NEXT_FNC(read)
#define _real_write  NEXT_FNC(write)

typedef struct CtrlFileHeader {
  char name[NAMESIZE];
  size_t fileSize;
} CtrlFileHeader;

typedef struct ProcCGroupHeader {
  char name[NAMESIZE];
  char subsystem[NAMESIZE];
  size_t numFiles;
} ProcCGroupHeader;

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
    ProcCGroup(ProcCGroupHeader &groupHdr);
    ~ProcCGroup();

    void getHeader(ProcCGroupHeader &groupHdr);
    void createIfNotExist();
    void initCtrlFiles();
    void *getNextCtrlFile(CtrlFileHeader &fileHdr);
    void writeCtrlFile(CtrlFileHeader &fileHdr, void *contentBuf);
    void addPid(pid_t pid);

  private:
    std::string subsystem;
    std::string name;
    std::string path;
    size_t numFiles;
    pathList ctrlFilePaths;
    pathList::iterator ctrlFileIterator;
};
}

#endif // ifndef PROCCGROUP_H
