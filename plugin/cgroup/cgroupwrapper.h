#ifndef CGroupWrapper_H
#define CGroupWrapper_H

#include <fcntl.h>
#include <sys/types.h>
#include <vector>

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

typedef struct CGroupHeader {
  char name[NAMESIZE];
  char subsystem[NAMESIZE];
  size_t numFiles;
} CGroupHeader;

typedef std::vector<std::string> pathList;

namespace dmtcp
{
class CGroupWrapper
{
  public:
#ifdef JALIB_ALLOCATOR
    static void *operator new(size_t nbytes, void *p) { return p; }
    static void *operator new(size_t nbytes) { JALLOC_HELPER_NEW(nbytes); }
    static void operator delete(void *p) { JALLOC_HELPER_DELETE(p); }
#endif // ifdef JALIB_ALLOCATOR

    static CGroupWrapper *build(std::string subsystem, std::string name);
    static CGroupWrapper *build(CGroupHeader &groupHdr);

    void getHeader(CGroupHeader &groupHdr);
    void createIfMissing();
    void *getNextCtrlFile(CtrlFileHeader &fileHdr);
    void addPid(pid_t pid);
    void initCtrlFiles();

    virtual void writeCtrlFile(CtrlFileHeader &fileHdr, void *contentBuf);

  protected:
    CGroupWrapper(std::string subsystem, std::string name);

    std::string subsystem;
    std::string name;
    std::string path;
    pathList ctrlFilePaths;
    size_t numCtrlFiles;
    pathList::iterator ctrlFileIterator;
};
}

#endif // ifndef CGroupWrapper_H
