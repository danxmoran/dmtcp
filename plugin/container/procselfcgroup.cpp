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

#include <fcntl.h>
#include <jassert.h>
#include <util.h>
#include <string.h>

#include "proccgroup.h"
#include "procselfcgroup.h"

using namespace dmtcp;


ProcSelfCGroup::ProcSelfCGroup()
  : dataIdx(0),
  numGroups(0),
  numBytes(0),
  fd(-1)
{
  char buf[4096];

  fd = open("/proc/self/cgroup", O_RDONLY);
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

  close(fd);

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
      c -= '0';
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
      buf[i] = '\0';
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

    char c = data[dataIdx];
    if (c == '\n') {
      if (buf[i-1] != '/') {
        buf[i++] = '/';
      }
      buf[i] = '\0';
      break;
    }
    buf[i++] = c;
    dataIdx++;
  }
  return i;
}

size_t
ProcSelfCGroup::readControl(char *pathBuf, const char *ctrlName,
                            void *dest, size_t size) {
  int tmp_fd;
  size_t numRead = 0;

  strcpy(pathBuf, ctrlName);
  pathBuf[strlen(ctrlName) + 1] = '\0';
  if (access(pathBuf, F_OK) != -1) {
    tmp_fd = open(pathBuf, O_RDONLY);
    JASSERT(tmp_fd != -1) (JASSERT_ERRNO);
    numRead = read(tmp_fd, dest, size);
    close(tmp_fd);
  }

  return numRead;
}

void
ProcSelfCGroup::readMemoryLimits(ProcCGroup *group)
{
  char buf[2 * FILENAMESIZE];

  if (strcmp(group->name, "/") == 0) {
    group->memory.limit_in_bytes = -1;
  } else {
    const char *prefix = "/sys/fs/cgroup/memory";
    size_t prefixLen = strlen(prefix);

    strcpy(buf, prefix);
    strcpy(buf + prefixLen + 1, group->name);
    size_t bufIdx = prefixLen + strlen(group->name) + 2;
    strcpy(buf + bufIdx, "memory.");
    bufIdx += 7;

    // Save limit-in-bytes.
    if (!readControl(buf + bufIdx, "limit_in_bytes",
                     &group->memory.limit_in_bytes,
                     sizeof(ssize_t))) {
      group->memory.limit_in_bytes = -1;
    }

    // Save limit with swap.
    if (!readControl(buf + bufIdx, "memsw.limit_in_bytes",
                     &group->memory.memsw_limit_in_bytes,
                     sizeof(ssize_t))) {
      group->memory.memsw_limit_in_bytes = -1;
    }

    // Save soft limit.
    if (!readControl(buf + bufIdx, "soft_limit_in_bytes",
                     &group->memory.soft_limit_in_bytes,
                     sizeof(ssize_t))) {
      group->memory.soft_limit_in_bytes = -1;
    }
  }
}

void
ProcSelfCGroup::readPIDSLimits(ProcCGroup *group)
{
  char buf[2 * FILENAMESIZE];

  if (strcmp(group->name, "/") == 0) {
    group->pids.max = -1;
  } else {
    const char *prefix = "/sys/fs/cgroup/pids";
    size_t prefixLen = strlen(prefix);

    strcpy(buf, prefix);
    strcpy(buf + prefixLen + 1, group->name);
    size_t bufIdx = prefixLen + strlen(group->name) + 2;
    strcpy(buf + bufIdx, "pids.");
    bufIdx += 5;

    // Save max pids.
    if (!readControl(buf + bufIdx, "max",
                     &group->pids.max, sizeof(ssize_t))) {
      group->pids.max = -1;
    }
  }
}

int
ProcSelfCGroup::getNextCGroup(ProcCGroup *group)
{
  char buf[FILENAMESIZE];
  size_t numRead;

  while (dataIdx < numBytes && data[dataIdx] != 0) {
    size_t groupNum = readDec();
    JASSERT(groupNum > 0);
    JASSERT(data[dataIdx++] == ':');

    numRead = readSubsystem(buf, FILENAMESIZE);
    JASSERT(numRead > 0);
    JASSERT(data[dataIdx++] == ':');

    bool skip = false;
    if (strcmp(buf, "memory") == 0) {
      group->subsystem = DMTCP_CGROUP_MEMORY;
    } else if (strcmp(buf, "pids") == 0) {
      group->subsystem = DMTCP_CGROUP_PIDS;
    } else {
      // TODO: Add other groups
      printf("Skipping cgroup '%s'\n", buf);
      skip = true;
    }

    numRead = readName(group->name, FILENAMESIZE);
    JASSERT(numRead > 0);
    JASSERT(data[dataIdx++] == '\n');
    if (skip) continue;

    switch (group->subsystem) {
      case DMTCP_CGROUP_MEMORY:
        readMemoryLimits(group);
        break;
      case DMTCP_CGROUP_PIDS:
        readPIDSLimits(group);
        break;
      default:
        JASSERT(0);
    }

    return 1;
  }

  return 0;
}
