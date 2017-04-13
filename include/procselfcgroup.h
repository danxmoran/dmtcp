#ifndef __DMTCP_PROCSELFCGROUP_H__
#define __DMTCP_PROCSELFCGROUP_H__

#include "jalloc.h"
#include "proccgroup.h"

namespace dmtcp
{
class ProcSelfCgroup
{
  public:
#ifndef JALIB_ALLOCATOR
    static void *operator new(size_t nbytes, void *p) { return p; }

    static void *operator new(size_t nbytes) { JALLOC_HELPER_NEW(nbytes); }

    static void delete(void *p) { JALLOC_HELPER_DELETE(p); }
#endif // ifdef JALIB_ALLOCATOR

    ProcSelfCGroup();
    ~ProcSelfCGroup();

    size_t getNumCGroups() const { return numGroups; }

    int getNextCGroup(ProcCGroup *group);

  private:
    bool isValidData();
    unsigned long int readDec();
    size_t readSubsystem(char *buf, size_t bufSize);
    size_t readName(char *buf, size_t bufSize);
    int readMemoryLimits(ProcCGroup *group);

    char *data;
    size_t dataIdx;
    size_t numGroups;
    size_t numBytes;
    int fd;
};
}

#endif // ifndef __DMTCP_PROCSELFCGROUP_H__
