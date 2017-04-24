#ifndef __DMTCP_PROCSELFCGROUP_H__
#define __DMTCP_PROCSELFCGROUP_H__

#include <map>
#include <vector>
#include <string>

#include "jalloc.h"
#include "cgroupwrapper.h"

typedef std::vector<std::string> GroupNameList;
typedef std::map<std::string, GroupNameList> GroupMap;

namespace dmtcp
{
class ProcSelfCGroup
{
  public:
#ifdef JALIB_ALLOCATOR
    static void *operator new(size_t nbytes, void *p) { return p; }
    static void *operator new(size_t nbytes) { JALLOC_HELPER_NEW(nbytes); }
    static void operator delete(void *p) { JALLOC_HELPER_DELETE(p); }
#endif // ifdef JALIB_ALLOCATOR

    ProcSelfCGroup();
    ~ProcSelfCGroup();

    size_t getNumCGroups();
    CGroupWrapper *getNextCGroup();

  private:
    int parseGroupLine();

    GroupMap groupsBySubsystem;
    GroupMap::iterator groupIterator;
    GroupNameList::iterator groupNameIterator;
    char *data;
    char *dataTokPtr;
    size_t numBytes;
    size_t numGroups;
    int fd;
};
}

#endif // ifndef __DMTCP_PROCSELFCGROUP_H__
