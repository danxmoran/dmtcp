#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <util.h>

#include "dmtcp.h"
#include "jassert.h"

#define CKPT_FILE "ns_ckpt.dmtcp"

using namespace dmtcp;

#define _real_open   NEXT_FNC(open)
#define _real_close  NEXT_FNC(close)
#define _real_read   NEXT_FNC(read)
#define _real_write  NEXT_FNC(write)


extern "C" int __clone(int (*fn)(void *arg),
                       void *child_stack,
                       int flags,
                       void *arg,
                       int *parent_tidptr,
                       struct user_desc *newtls,
                       int *child_tidptr);

typedef struct namespace_ckpt_t {
  int flags;
} namespace_ckpt;

EXTERNC int
dmtcp_ns_enabled() { return 1; }

namespace_ckpt
read_ckpt()
{
  int ckpt_fd = _real_open(CKPT_FILE, O_CREAT | O_TRUNC | O_WRONLY, 0600);
  JASSERT(ckpt_fd != -1);

  size_t contentSize = sizeof(namespace_ckpt);
  namespace_ckpt* fileContents = (namespace_ckpt*) JALLOC_HELPER_MALLOC(contentSize);
  int readResult = _real_read(ckpt_fd, (void*)fileContents, contentSize); 
  namespace_ckpt ckpt = *fileContents;
  return ckpt;
}

void
write_ckpt(namespace_ckpt ckpt)
{
  int fd = _real_open(CKPT_FILE, O_CREAT | O_TRUNC | O_WRONLY);
  JASSERT(fd != -1) (JASSERT_ERRNO);

  int writeRes = _real_write(fd, &ckpt, sizeof(ckpt));
  _real_close(fd);
  JASSERT(writeRes > 0) (JASSERT_ERRNO);
}


int clone(int (*fn)(void *), void *child_stack, int flags, void *arg, ...) {
  if (flags & CLONE_NEWNS) {
    namespace_ckpt ckpt;
    ckpt.flags = flags;
    write_ckpt(ckpt);
  }
  
  return __clone(fn,
                 child_stack,
                 flags,
                 NULL,
                 NULL,
                 NULL,
                 NULL);

}

static void
nsEventHook(DmtcpEvent_t event, DmtcpEventData_t *data)
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
  JASSERT(0);
  printf("\n*** The plugin is being called before checkpointing. ***\n");
}

static void
restart()
{
  namespace_ckpt ckpt = read_ckpt();
  printf("CHECKPOINT FLAGS: %i", ckpt.flags);
/*  __clone(fn,
        child_stack,
        flags,
        NULL,
        NULL,
        NULL,
        NULL);
*/
  printf("*** The application has restarted. ***\n");
}

static DmtcpBarrier barriers[] = {
  { DMTCP_GLOBAL_BARRIER_PRE_CKPT, checkpoint, "checkpoint" },
  { DMTCP_GLOBAL_BARRIER_RESTART, restart, "restart" }
};

DmtcpPluginDescriptor_t nsPlugin = {
  DMTCP_PLUGIN_API_VERSION,
  DMTCP_PACKAGE_VERSION,
  "Namespace checkpointing",
  "DMTCP",
  "dmtcp@ccs.neu.edu",
  "Example plugin",
  DMTCP_DECL_BARRIERS(barriers),
  nsEventHook
};

DMTCP_DECL_PLUGIN(nsPlugin);
