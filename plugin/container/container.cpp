#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "dmtcp.h"
#include "util.h"
#include "jassert.h"

#include "proccgroup.h"
#include "procselfcgroup.h"

#define CKPT_FILE "ckpt_container.dmtcp"

using namespace dmtcp;


static int
restoreCtrlFile(ProcCGroup *group, int fd)
{
  CtrlFileHeader header;
  int readResult = _real_read(fd, &header, sizeof(header));
  JASSERT(readResult > 0) (JASSERT_ERRNO);

  size_t fileSize = header.fileSize;
  void* fileContents = JALLOC_HELPER_MALLOC(fileSize);
  readResult = _real_read(fd, fileContents, fileSize);
  JASSERT(readResult > 0) (JASSERT_ERRNO);

  group->writeCtrlFile(header, fileContents);
  JALLOC_HELPER_FREE(fileContents);
}

static int
restoreCGroup(int fd, pid_t realPid)
{
  ProcCGroupHeader header;
  int readResult = _real_read(fd, &header, sizeof(ProcCGroupHeader));
  JASSERT(readResult > 0) (JASSERT_ERRNO);

  ProcCGroup *group = new ProcCGroup(header);
  group->createIfNotExist();

  size_t fileCount = header.numFiles;
  for (size_t i = 0; i < fileCount; i++) {
    restoreCtrlFile(group, fd);
  }

  group->addPid(realPid);
  JALLOC_HELPER_DELETE(group);
}

static int
restoreFromCkpt(int fd)
{
  // Find the number of groups that were checkpointed.
  size_t numGroups;
  int readResult = _real_read(fd, &numGroups, sizeof(size_t));
  JASSERT(readResult > 0) (JASSERT_ERRNO);

  pid_t realPid = dmtcp_virtual_to_real_pid(getpid());
  for (size_t i = 0; i < numGroups; i++) {
    restoreCGroup(fd, realPid);
  }
}

EXTERNC int
dmtcp_container_enabled() { return 1; }

static void
container_event_hook(DmtcpEvent_t event, DmtcpEventData_t *data)
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
  ProcCGroup *group;

  int ckpt_fd = _real_open(CKPT_FILE, O_CREAT | O_TRUNC | O_WRONLY, 0600);
  JASSERT(ckpt_fd != -1);

  size_t numGroups = procSelfCGroup.getNumCGroups();
  Util::writeAll(ckpt_fd, &numGroups, sizeof(size_t));

  while ((group = procSelfCGroup.getNextCGroup())) {
    ProcCGroupHeader grpHdr;
    group->initCtrlFiles();
    group->getHeader(grpHdr);
    Util::writeAll(ckpt_fd, &grpHdr, sizeof(ProcCGroupHeader));

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
  restoreFromCkpt(fd);
  _real_close(fd);
  printf("*** The application has restarted. ***\n");
}

static DmtcpBarrier barriers[] = {
  { DMTCP_GLOBAL_BARRIER_PRE_CKPT, checkpoint, "checkpoint" },
  { DMTCP_GLOBAL_BARRIER_RESTART, restart, "restart" }
};

DmtcpPluginDescriptor_t container_plugin = {
  DMTCP_PLUGIN_API_VERSION,
  DMTCP_PACKAGE_VERSION,
  "Container checkpointing (namespaces and cgroups)",
  "DMTCP",
  "dmtcp@ccs.neu.edu",
  "Example plugin",
  DMTCP_DECL_BARRIERS(barriers),
  container_event_hook
};

DMTCP_DECL_PLUGIN(container_plugin);
