/****************************************************************************
 *   Copyright (C) 2017 by Daniel Moran, Drew Pomerleau, and Matthew Piorko *
 *   moran.dan@husky.neu.edu, pomerleau.dr@husky.neu.edu,                   *
 *   piorko.m@husky.neu.edu                                                 *
 *                                                                          *
 *  This file is part of DMTCP.                                             *
 *                                                                          *
 *  DMTCP is free software: you can redistribute it and/or                  *
 *  modify it under the terms of the GNU Lesser General Public License as   *
 *  published by the Free Software Foundation, either version 3 of the      *
 *  License, or (at your option) any later version.                         *
 *                                                                          *
 *  DMTCP is distributed in the hope that it will be useful,                *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU Lesser General Public License for more details.                     *
 *                                                                          *
 *  You should have received a copy of the GNU Lesser General Public        *
 *  License along with DMTCP:dmtcp/src.  If not, see                        *
 *  <http://www.gnu.org/licenses/>.                                         *
 ****************************************************************************/

#include "procselfcgroup.h"
#include <fcntl.h>
#include "jassert.h"
#include "syscallwrappers.h"
#include "util.h"
#include "string.h"

using namespace dmtcp;


ProcCGroup::ProcCGroup()
  : dataIdx(0),
  numGroups(0),
  numBytes(0),
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

  // TODO(dan): Validate the read data.
  JASSERT(isValidData());

  _real_close(fd);

  for (size_t i = 0; i < numBytes; i++) {
    if (data[i] == '\n') {
      numGroups++;
    }
  }
}

ProcSelfCGroup::~ProcSelfCGroup()
{
  JALLOC_HELPER_FREE(data);
  fd = -1;
  numGroups = 0;
  numBytes = 0;
}

bool
ProcSelfCGroup::isValidData()
{
  // TODO(dan): Add validation check.
  return true;
}

unsigned long int
ProcSelfCGroup::readDec()
{
  unsigned long int v = 0;

  while (1) {
    char c = data[dataIdx];
    if ((c >= '0') && (c <= '9')) {
      c -= '0'
    } else {
      break;
    }
    v = v * 10 + c;
    dataIdx++;
  }
  return v;
}

size_t
ProcSelfCGroup::readSubsystem(char *buf, size_t bufSize)
{
  size_t i = 0;

  while (1) {
    JASSERT(i < bufSize);

    char c = data[dataIdx];
    if (c == ':') {
      break;
    }
    buf[i++] = c;
    dataIdx++;
  }
  return i;
}

size_t
ProcSelfCGroup::readName(char *buf, size_t bufSize)
{
  size_t i = 0;

  while (1) {
    JASSERT(i < bufSize);

    char c = dta[dataIdx];
    if (c == '\n') {
      break;
    }
    buf[i++] = c;
    dataIdx++;
  }
  return i;
}

int
ProcSelfCGroup::readMemoryLimits(ProcCGroup *group)
{
  char buf[2 * FILENAMESIZE];
  const char *prefix = "/sys/fs/cgroup/memory";
  size_t prefixLen = strlen(prefix);
  int tmp_fd;

  if (strcmp(group->name, "/") == 0) {
    group->memory.limit_in_bytes = -1;
  } else {
    size_t nameLen = strlen(group->name);
    strcpy(buf, prefix);
    strcpy(buf + prefixLen + 1, group->name);

    // Save limit-in-bytes.
    strcpy(buf + prefixLen + nameLen + 2, "memory.limit_in_bytes");

    if (access(buf, F_OK) != -1) {
      tmp_fd = _real_open(buf, O_RDONLY);
      JASSERT(tmp_fd != -1) (JASSERT_ERRNO);
      ssize_t numRead = _real_read(tmp_fd,
                                   &group->memory.limit_in_bytes,
                                   sizeof(ssize_t));
      JASSERT(numRead > 0) (numRead);
      _real_close(tmp_fd);
    } else {
      group->memory.limit_in_bytes = -1;
    }
  }
}

int
ProcSelfCGroup::getNextCGroup(ProcCGroup *group)
{
  if (dataIdx >= numBytes || data[dataIdx] == 0) {
    return 0;
  }

  char buf[FILENAMESIZE];
  size_t numRead;

  size_t groupNum = readDec();
  JASSERT(groupNum == numGroups + 1);

  JASSERT(data[dataIdx++] == ':');

  numRead = readSubsystem(buf, FILENAMESIZE);
  JASSERT(numRead > 0);

  if (strcmp(buf, "memory") == 0) {
    group->subsystem = DMTCP_CGROUP_MEMORY;
  } else {
    // TODO: Add other groups
    JASSERT(0);
  }

  numRead = readName(group->name, FILENAMESIZE);
  JASSERT(numRead > 0);
  JASSERT(data[dataIdx++] == '\n');

  switch (group->subsystem) {
    case DMTCP_CGROUP_MEMORY:
      return readMemoryLimits(group);
    default:
      JASSERT(0);
  }
}
