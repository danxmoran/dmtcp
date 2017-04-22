#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "dmtcp.h"
#include "procselfcgroup.h"
#include "util.h"
#include "jassert.h"

#define _real_open   NEXT_FNC(open)
#define _real_read   NEXT_FNC(read)

using namespace dmtcp;

#define DEFAULT_CONTAINER_CKPT_FILE "./ckpt_container.dmtcp"

int
write_to_ckpt(void* buffer, size_t buffer_size) {
  int flags = O_CREAT | O_TRUNC | O_WRONLY;
  int fd = _real_open(DEFAULT_CONTAINER_CKPT_FILE, flags, 0600);
  return Util::writeAll(fd, buffer, buffer_size);
}

int
read_from_ckpt(void* buffer, size_t buffer_size) {
  int fd = _real_open(DEFAULT_CONTAINER_CKPT_FILE, O_RDONLY);
  JASSERT(fd != 0);

  // find the number of subsystems
  size_t num_systems;
  int read_result = _real_read(fd, &num_systems, sizeof(size_t));
  JASSERT(read_result != 0); 
 
  printtf("Found %i subsystems in header file\n", num_systems);
  int i;
  for (i = 0; i < numSystems; i++) {
    read_ckpt_header(fd);
  }
}

int
read_ckpt_header(int fd) {
  ProcCGroupHeader header; 
  int read_result = _real_read(fd, &header, sizeof(header));
  JASSERT(read_result != 0);

  printtf("Reading subsystem header: %s\n", header.name);
  size_t number_files = header.numFiles;
  int i;
  for (i = 0; i < number_files; i++) {
    read_ckpt_file(fd);
  }

  printtf("Finished reading subsystem header: %s\n", header.name);
}

read_ckpt_file(int fd) {
  CtrlFileHeader header; 
  int read_result = _real_read(fd, &header, sizeof(header));
  JASSERT(read_result != 0);

  size_t file_size = header.fileSize;

  void* file_contents = malloc(file_size);
  read_result = _real_read(fd, file_contents, file_size);
  JASSERT(read_result != 0);
  printf("Read file: %s\n", header.name);
}

const char* key = "testkey";
const char* value = "we got data!";

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
//  ProcSelfCGroup procSelfCGroup;
//  ProcCGroup groupBuf;

//  while (procSelfCGroup.getNextCGroup(&groupBuf)) {
//    printf("Read: %s\n", groupBuf.name);
//  }
  write_to_ckpt((void*)value, strlen(value) + 1);
}

static void
resume()
{
  size_t buf_size = strlen(value) + 1;
  char* buf = (char*)malloc(buf_size);
  read_from_ckpt(buf, buf_size);
  printf("*** The application has now been checkpointed. ***\n");
  printf("Got some data: %s.\n", buf);
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
