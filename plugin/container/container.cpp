#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "dmtcp.h"
#include "util.h"
#include "jassert.h"

#include "proccgroup.h"
#include "procselfcgroup.h"

using namespace dmtcp;

#define CKPT_FILE   "./ckpt_container.dmtcp"
#define CKPT_FLAGS  O_CREAT | O_TRUNC | O_WRONLY
#define CKPT_PERM   0600

static int
read_from_ckpt(void* buffer, size_t buffer_size) {
  int fd = _real_open(CKPT_FILE, O_RDONLY);
  JASSERT(fd != 0);
  int read_result = _real_read(fd, buffer, buffer_size);
  JASSERT(read_result != 0);
  return read_result;
}

EXTERNC int
dmtcp_container_enabled() { return 1; }

static void
container_event_hook(DmtcpEvent_t event, DmtcpEventData_t *data)
{
  /* NOTE:  See warning in plugin/README about calls to printf here. */
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

  int ckpt_fd = _real_open(CKPT_FILE, CKPT_FLAGS, CKPT_PERM);
  JASSERT(ckpt_fd != -1);

  size_t numGroups = procSelfCGroup.getNumCGroups();
  Util::writeAll(ckpt_fd, &numGroups, sizeof(size_t));

  while ((group = procSelfCGroup.getNextCGroup())) {
    ProcCGroupHeader grpHdr;
    group->getHeader(grpHdr);
    Util::writeAll(ckpt_fd, &grpHdr, sizeof(ProcCGroupHeader));

    CtrlFileHeader hdr;
    void *fileBuf;
    while ((fileBuf = group->getNextCtrlFile(hdr))) {
      Util::writeAll(ckpt_fd, &hdr, sizeof(CtrlFileHeader));
      Util::writeAll(ckpt_fd, fileBuf, hdr.fileSize);
      JALLOC_HELPER_FREE(fileBuf);
    }
    delete group;
  }

  _real_close(ckpt_fd);
  printf("\n*** The plugin has finished checkpointing. ***\n");
}

static void
resume()
{
  printf("*** The application has now been checkpointed. ***\n");
}

static void
restart()
{
  printf("*** The application has now been checkpointed. ***\n");
}

static DmtcpBarrier barriers[] = {
  { DMTCP_GLOBAL_BARRIER_PRE_CKPT, checkpoint, "checkpoint" },
  { DMTCP_GLOBAL_BARRIER_RESUME, resume, "resume" },
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
