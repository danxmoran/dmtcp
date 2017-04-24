#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "dmtcp.h"
#include "util.h"
#include "jassert.h"

#include "cgroupwrapper.h"
#include "procselfcgroup.h"

#define CKPT_FILE "ckpt_cgroup.dmtcp"

using namespace dmtcp;


EXTERNC int
dmtcp_cgroup_enabled() { return 1; }

static void
cgroupEventHook(DmtcpEvent_t event, DmtcpEventData_t *data)
{
  switch (event) {
  case DMTCP_EVENT_INIT:
    printf("The plugin containing %s has been initialized.\n", __FILE__);
    break;
  case DMTCP_EVENT_EXIT:
    printf("The plugin is being called before exiting.\n");
    break;
  default:
    break;
  }
}

static void
checkpoint()
{
  printf("\n*** The plugin is being called before checkpointing. ***\n");
  ProcSelfCGroup procSelfCGroup;
  CGroupWrapper *group;

  int ckpt_fd = _real_open(CKPT_FILE, O_CREAT | O_TRUNC | O_WRONLY, 0600);
  JASSERT(ckpt_fd != -1);

  size_t numGroups = procSelfCGroup.getNumCGroups();
  Util::writeAll(ckpt_fd, &numGroups, sizeof(size_t));

  while ((group = procSelfCGroup.getNextCGroup())) {
    CGroupHeader grpHdr;
    group->initCtrlFiles();
    group->getHeader(grpHdr);
    Util::writeAll(ckpt_fd, &grpHdr, sizeof(CGroupHeader));

    CtrlFileHeader hdr;
    void *fileBuf;
    while ((fileBuf = group->getNextCtrlFile(hdr))) {
      Util::writeAll(ckpt_fd, &hdr, sizeof(CtrlFileHeader));
      Util::writeAll(ckpt_fd, fileBuf, hdr.fileSize);
      JALLOC_HELPER_FREE(fileBuf);
    }
    JALLOC_HELPER_DELETE(group);
  }

  _real_close(ckpt_fd);
  printf("*** The plugin has finished checkpointing. ***\n");
}

static void
restart()
{
  int fd = _real_open(CKPT_FILE, O_RDONLY);
  JASSERT(fd != 0);

  int readResult;
  size_t numGroups;
  readResult = _real_read(fd, &numGroups, sizeof(size_t));
  JASSERT(readResult > 0) (JASSERT_ERRNO);

  CGroupHeader groupHeader;
  CtrlFileHeader fileHeader;
  pid_t realPid = dmtcp_virtual_to_real_pid(getpid());
  for (size_t i = 0; i < numGroups; i++) {
    readResult = _real_read(fd, &groupHeader, sizeof(CGroupHeader));
    JASSERT(readResult > 0) (JASSERT_ERRNO);

    CGroupWrapper *group = CGroupWrapper::build(groupHeader);
    group->createIfMissing();

    size_t fileCount = groupHeader.numFiles;
    for (size_t j = 0; j < fileCount; j++) {
      readResult = _real_read(fd, &fileHeader, sizeof(CtrlFileHeader));
      JASSERT(readResult > 0) (JASSERT_ERRNO);

      size_t fileSize = fileHeader.fileSize;
      void* fileContents = JALLOC_HELPER_MALLOC(fileSize);
      readResult = _real_read(fd, fileContents, fileSize);
      JASSERT(readResult != -1) (JASSERT_ERRNO);

      group->writeCtrlFile(fileHeader, fileContents);
      JALLOC_HELPER_FREE(fileContents);
    }

    group->addPid(realPid);
    JALLOC_HELPER_DELETE(group);
  }
  _real_close(fd);
  printf("*** The application has restarted. ***\n");
}

static DmtcpBarrier barriers[] = {
  { DMTCP_GLOBAL_BARRIER_PRE_CKPT, checkpoint, "checkpoint" },
  { DMTCP_GLOBAL_BARRIER_RESTART, restart, "restart" }
};

DmtcpPluginDescriptor_t cgroupPlugin = {
  DMTCP_PLUGIN_API_VERSION,
  DMTCP_PACKAGE_VERSION,
  "Control Group Plugin",
  "Daniel Moran, Drew Pomerleau, Matthew Piorko",
  "moran.dan@husky.neu.edu, pomerleau.dr@husky.neu.edu",
  "Saves and restores cgroup names and parameters",
  DMTCP_DECL_BARRIERS(barriers),
  cgroupEventHook
};

DMTCP_DECL_PLUGIN(cgroupPlugin);
