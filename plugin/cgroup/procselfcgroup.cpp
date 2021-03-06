#include <fcntl.h>
#include <jassert.h>
#include <util.h>
#include <string>

#include "cgroupwrapper.h"
#include "procselfcgroup.h"

using namespace dmtcp;


ProcSelfCGroup::ProcSelfCGroup()
  : groupsBySubsystem(GroupMap()),
  data(NULL),
  dataTokPtr(NULL),
  numBytes(0),
  numGroups(0),
  fd(-1)
{
  char buf[4096];

  fd = _real_open("/proc/self/cgroup", O_RDONLY);
  JASSERT(fd != -1) (JASSERT_ERRNO);
  ssize_t numRead = 0;

  // Get an approximation of the required buffer size.
  do {
    numRead = Util::readAll(fd, buf, sizeof(buf));
    if (numRead > 0) {
      numBytes += numRead;
    }
  } while (numRead > 0);

  // Now allocate a buffer.
  size_t size = numBytes + 4096; // Add a one page buffer.
  data = (char *)JALLOC_HELPER_MALLOC(size);
  JASSERT(lseek(fd, 0, SEEK_SET) == 0);

  numBytes = Util::readAll(fd, data, size);
  JASSERT(numBytes > 0) (numBytes);

  // TODO(dan): Replace this assert with more robust code that would
  // reallocate the buffer with an extended size.
  JASSERT(numBytes < size) (numBytes) (size);
  _real_close(fd);

  // Parse all groups.
  while (parseGroupLine()) {}

  groupIterator = groupsBySubsystem.begin();
  groupNameIterator = groupIterator->second.begin();
}

ProcSelfCGroup::~ProcSelfCGroup()
{
  JALLOC_HELPER_FREE(data);
  dataTokPtr = NULL;
  numBytes = 0;
  fd = -1;
}

int
ProcSelfCGroup::parseGroupLine()
{
  char *line;
  char *lineTokPtr;

  char *toTok = (dataTokPtr == NULL) ? data : NULL;
  if ((line = strtok_r(toTok, "\n", &dataTokPtr)) == NULL) {
    return 0;
  }

  // Group lines have the format <#>:<subsystem>:<name>.
  JASSERT(strtok_r(line, ":", &lineTokPtr) != NULL); // Ignore group #
  char *subsystems = strtok_r(NULL, ":", &lineTokPtr);
  JASSERT(subsystems != NULL);

  GroupNameList groups = GroupNameList();
  std::string groupPath = std::string(lineTokPtr);
  size_t pathLen = groupPath.length();
  size_t idx = 0;
  while (idx < pathLen) {
    idx = groupPath.find_first_of("/", idx);
    if (idx == std::string::npos) {
      idx = pathLen;
    }
    if (idx != 0) {
      groups.push_back(groupPath.substr(0, idx));
    }
    idx++;
  }

  if (groups.size() > 0) {
    char *subsystem;
    char *subsystemPtr;

    while ((subsystem = strtok_r(subsystems, ",", &subsystemPtr)) != NULL) {
      subsystems = NULL;
      std::string std_subsys = std::string(subsystem);

      // CGroups not bound to a subsystem are prefixed with "name=".
      if (std_subsys.find("name=") == 0) {
        std_subsys = std_subsys.substr(5);
      }
      groupsBySubsystem[std_subsys] = groups;
      numGroups += groups.size();
    }
  }

  return 1;
}

size_t
ProcSelfCGroup::getNumCGroups()
{
  return numGroups;
}

CGroupWrapper *
ProcSelfCGroup::getNextCGroup()
{
  if (groupIterator == groupsBySubsystem.end()) {
    return NULL;
  }

  CGroupWrapper *group = CGroupWrapper::build(groupIterator->first,
                                              *groupNameIterator);

  groupNameIterator++;
  if (groupNameIterator == groupIterator->second.end()) {
    groupIterator++;
    if (groupIterator != groupsBySubsystem.end()) {
      groupNameIterator = groupIterator->second.begin();
    }
  }

  return group;
}
