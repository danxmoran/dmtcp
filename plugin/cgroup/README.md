# DMTCP Control Group Plugin
Authors: Daniel Moran, Drew Pomerleau, Matthew Piorko

## <a name="overview"></a>Overview of Functionality
This plugin enables DMTCP to capture the
[Control Groups](http://man7.org/linux/man-pages/man7/cgroups.7.html) (cgroups) of
a process during checkpointing, and then restore the process into those same groups
(creating the groups and resetting their tunable parameters if needed) upon restart.
The plugin is currently a proof-of-concept with many [limitations](#limitations),
but it is a good first step towards enabling DMTCP to checkpoint applications which
run under cgroup hierarchies, such as Docker containers.

## <a name="cgroup-api"></a>Overview of CGroup API
Unlike other mechanisms for containerization such as [namespaces](http://man7.org/linux/man-pages/man7/namespaces.7.html),
the cgroup API has no dedicated system calls. Instead, cgroups are exposed through
a virtual file system, and their parameters are tuned by writing into that file system.

### Terminology
**Subsystem:** A cgroup subsystem models a machine resource to be managed /
partitioned. Most subsystems expose tunable parameters through virtual files, but
some are built solely for accounting purposes. The Linux kernel implements subsystems
for:

- [CPU sets](http://www.tamacom.com/tour/kernel/linux/S/619.html)
- [CPU shares](https://access.redhat.com/documentation/en-US/Red_Hat_Enterprise_Linux/6/html/Resource_Management_Guide/sec-cpu.html)
- [CPU accounting](http://www.tamacom.com/tour/kernel/linux/S/618.html)
- [Block IO](http://www.tamacom.com/tour/kernel/linux/S/616.html)
- [Memory](http://www.tamacom.com/tour/kernel/linux/S/624.html)
- [Devices](http://www.tamacom.com/tour/kernel/linux/S/620.html)
- [Freezer](http://www.tamacom.com/tour/kernel/linux/S/621.html)
- [Network classification](http://www.tamacom.com/tour/kernel/linux/S/625.html)
- [Network priority](http://www.tamacom.com/tour/kernel/linux/S/626.html)
- [Perf events](https://access.redhat.com/documentation/en-US/Red_Hat_Enterprise_Linux/6/html/Resource_Management_Guide/sec-perf_event.html)
- [Huge TLB](http://www.tamacom.com/tour/kernel/linux/S/622.html)
- [PIDs](https://www.mjmwired.net/kernel/Documentation/cgroups/pids.txt)

Users cannot define their own subsystems, but they can create hierarchies without
an underlying subsystem for the purposes of process grouping.

**Hierarchy:** A cgroup hierarchy is a tree of named groups, each with their own
tunable parameters and process lists. Each subsystem provides a root cgroup upon
which a hierarchy can be built. Hierarchies in different subsystems are not connected,
so they can share names without conflicting with one another; on the flip side, a
process can belong to a differently-named hierarchy within each subsystem.

### Exposing a Subsystem
The first step towards working with cgroups is to expose the subsystem(s) you wish
to extend / tune. This is done by `mount`ing the virtual filesystem of the subsystem.
By convention, cgroups are typically mounted within `/sys/fs/cgroup`:

```bash
$ mkdir /sys/fs/cgroup/memory
$ mount -t cgroup -o memory /sys/fs/cgroup/memory
```

Multiple cgroups can also be mounted into the same directory:

```bash
$ mkdir /sys/fs/cgroup/cpu,cpuacct
$ mount -t cgroup -o cpu,cpuacct /sys/fs/cgroup/cpu,cpuacct
```

Upon mounting a subsystem to a directory, the kernel will automatically generate
virtual files within the directory corresponding to the tunable parameters / accounting
reports exposed by that subsystem.

```bash
# Restrict the memory available to all processes to 1 million pages.
$ echo 4096000000 > /sys/fs/cgroup/memory/memory.limit_in_bytes
```

On Ubuntu (and potentially other distributions), `systemd` will auto-mount all
subsystems into `/sys/fs/cgroup` on boot.

### Creating a Hierarchy
Once you've mounted a subsystem, you can create a new cgroup hierarchy within that
system using `mkdir`:

```bash
$ mkdir /sys/fs/cgroup/memory/my-cgroup
```

As it does on mount, the kernel will automatically generate virtual files within
the new directory to allow for tuning the new group.

### Moving Processes Into a CGroup
Regardless of the underlying subsystem (or lack thereof), every layer of cgroup
hierarchy will contain a virtual file `cgroup.procs`. To add a process to a group
within the hierarchy, you write its pid into this file:

```bash
$ echo 12345 > /sys/fs/cgroup/memory/my-cgroup/cgroup.procs
```

The kernel will automatically remove the PID from its previous group. It will
also raise an error if you attempt to move an invalid PID into a group.

### Checking the Current CGroups of a Process
The current cgroups of all live processes are exposed through virtual files at
`/proc/<pid>/cgroup`, and a process can determine its own cgroups by reading
`/proc/self/cgroup`. These files contain one line per mounted subsystem, with the
format: `<subsystem #>:<subsystem name>:<hierarchy path>`. For example, the process
moved into the `my-cgroup` group within the `memory` subsystem in the section above
would have a line like: `4:memory:/my-cgroup`.

## <a name="design"></a>Plugin Design
[The plugin](./cgroup.cpp) hooks into DMTCP's `checkpoint()` and `restart()` events
to enable saving and restoring cgroups. When built, it generates the shared library
`libdmtcp_cgroup.so` in the `lib/dmtcp` directory of the top-level project.

### Checkpointing CGroups
On checkpoint, the plugin first reads `/proc/self/cgroup` to build an
[iterator](procselfcgroup.cpp) over all cgroups in all subsystems to which the
checkpointed process belongs. Root-level cgroups are ignored by the iterator, and
nested cgroups are translated into a set of groups, one for each layer of the
hierarchy.

After building the iterator over [group objects](cgroupwrapper.cpp), the plugin then
iterates through the objects to save their names and tunable parameters. Group
objects themselves serve as iterators over file names indicating the relevant virtual
files whose contents should be captured as part of checkpointing. In order to be
checkpointed, the virtual files must have both read- and write-access (otherwise
they either cannot be saved or cannot be restored).

We experimented with various methods of capturing the necessary files for each
subsystem before settling on a subclass-based architecture, in which objects which
wrap subsystems with tunable parameters must inherit from the base group-wrapping
object and override the list of filenames to save. This design gave us the flexibility
to handle differences in naming and I/O conventions between subsystems, and allowed
us to narrow the impact of system-imposed limitations. In the end, we defined the
following subclasses:

- [CPU set wrapper](cpusetwrapper.cpp), [CPU wrapper](cpuwrapper.cpp),
  [Network classification wrapper](netclswrapper.cpp),
  [Network priority wrapper](netpriowrapper.cpp), [PIDs wrapper](pidswrapper.cpp):
  These wrappers capture all of the control files for their respective subsystems
  as hard-coded vectors.

- [Block IO wrapper](blkiowrapper.cpp): Because of [permissions issues](#device-limitations),
  this wrapper only captures two of the eight possible tunable files within the
  blkio subsystem. If the limitations could be resolved, this wrapper could emit
  all eight filenames.

- [Devices wrapper](deviceswrapper.cpp): Currently, this wrapper emits no file names
  for checkpointing. This is because of [permissions limitations](#device-limitations), not
  because the devices subsystem is read-only. If the limitations could be resolved,
  this wrapper could emit device-related files to checkpoint.

- [Huge TLB wrapper](hugetlbwrapper.cpp): Since huge TLB control file names depend
  on the number and sizes of huge TLBs configured in the system, this wrapper cannot
  contain a hard-coded list of filenames to save. Instead, it opens and scans the
  group directory on checkpoint to collect all files matching the expected naming
  pattern, and emits those files.

- [Memory wrapper](memorywrapper.cpp): This wrapper contains a hard-coded list of
  valid control files for the memory subsystem. However, since one of the files
  might or might not be exposed (depending on swap settings), the wrapper opens
  and scans the group directory to determine if the file is missing and avoid
  emitting it if so.

- [No-files wrapper](noctrlfileswrapper.cpp): This instance wraps cgroups which
  contain no tunable parameters, so it is solely used for saving the paths of
  groups.

#### Why no freezer wrapper?
The freezer subsystem contains a "tunable" file, but it is treated as a subsystem
without tunable parameters. This is because the one parameter of a freezer cgroup
determines whether or not any of the processes it contains are currently running /
runnable, or if they're instead "frozen". During checkpointing, it must be the case
that the process under DMTCP is not frozen (otherwise the checkpointing logic couldn't
run), so there's no value in saving the freezer-state parameter.

#### Checkpoint location and structure
Currently, checkpointed cgroup information is dumped into a file `ckpt_cgroup.dmtcp`
in the directory of the process being checkpointed. The contents of the file are
ordered as follows:

1. Number of saved groups (`size_t`)
2. For each group:
  1. Header containing subsystem name, hierarchy path, and number of control
     files (`CGroupHeader`)
  2. For each of the control files:
    1. Header containing the file name relative to the hierarchy, and the
       number of bytes saved from the file (`CtrlFileHeader`)
    2. Bytes saved from the file

### Restoring CGroups
On restart, the plugin will attempt to open `ckpt_cgroup.dmtcp` in the working
directory of the running process. It will then iterate through the saved group
information and:

1. Create the group's directory within `/sys/fs/cgroup/<subsystem>` if it doesn't
   already exist.
2. Write the saved contents of the group's tunable parameters (if any) back to their
   corresponding virtual files.

In all cases but one, restoring saved parameters is done by writing the full blob
of saved bytes into the corresponding file. The one exception is made for the file
`memory.oom_control`, which accepts only `0` or `1` as input but, when read, prints
out multiple lines of text:

```bash
$ cat /sys/fs/cgroup/memory/memory.oom_control
oom_kill_disable 0
under_oom 0
```

The first line of input reflects the `1` or `0` previously written into the file,
while the second line is for accounting. The [memory wrapper](memorywrapper.cpp)
class therefore includes special-case logic for parsing the `1` or `0` out of the
saved file and restoring only it.

## <a name="limitations"></a>Assumptions and Limitations
As mentioned above, this plugin is currently just a proof-of-concept built on
many assumptions.

### Finding Mount Points
First, the plugin assumes that cgroup filesystems will be mounted in `/sys/fs/cgroup`.
This isn't a very radical assumption since it follows the documented convention,
but it could be made more resilient by reading `/proc/mounts` to locate the mount-
points of the cgroup filesystem with certainty.

Second, the plugin assumes every mounted subsystem will be mounted to a directory
named after the subsystem. This assumption also largely follows convention, but it
could cause problems in situations when multiple cgroups are mounted together, i.e.

```bash
$ mount -t cgroup -o cpu,cpuacct /sys/fs/cgroup/cpu,cpuacct
```

By default, systemd on Ubuntu does mount certain subsystems together into the same
direcories; however, it also creates symlinks to these shared directories for each
individual subsystem within, enabling use of the plugin:

```bash
$ ls -l /sys/fs/cgroup
...
cpu -> cpu,cpuacct
cpuacct -> cpu,cpuacct
...
```

A more robust solution would again likely use `/proc/mounts` to find the exact
path to each mounted subsystem.

Finally, the plugin assumes that every subsystem which was mounted on checkpoint
will be present on restart. An alternative solution could have been for DMTCP to
mount any subsystems not present on restart; however, this would have required
running the restart script as the superuser. We chose against this route since
DMTCP firmly warns users against running it as root, and plenty of other tools
exist for configuring the mounting of cgroups.

### Restoring CGroup Hierarchies and Parameters
The most serious assumption made by the plugin is that on restart, the user running
the restart script will have the permissions necessary to make directories and write
to files in all of the saved control groups. **This requires pre-configuration of
cgroup file ownership, since the virtual files within mounted groups belong to the
root user by default.** As with mounting, we chose to make this assumption because
we did not want to force DMTCP users to run restart scripts as root, and because
a variety of other services for configuring cgroup access exist which would be much
better suited for addressing cgroup access problems that would DMTCP.

### <a name="device-limitations"></a>Device and Block IO Limitations
Unfortunately, even if cgroup files are pre-configured to give write access to users running
DMTCP restart scripts, our plugin is unable to restore parameters for the devices
subsystem, as well as device-specific parameters for the blkio subsystem. Both of
these subsystems require `CAP_SYS_ADMIN` permissions for any user attempting to
change their parameters, regardless of the owner of the mounted virtual files.

### Checkpoint Names
Finally, since the plugin currently dumps saved cgroup data to a hard-coded path,
checkpointing a multi-process application, or checkpointing multiple processes running
in the same working directory, will currently clobber the saved checkpoint data.
The plugin should parameterize its image names by the current DMTCP computation ID
and integrate with the DMTCP coordinator process to avoid these problems.

## <a name="extensions"></a>Potential Extensions
Given the stated assumptions and limitations of this plugin, this plugin is not
realistically ready for widespread distribution with DMTCP. An obvious extension
of this proof-of-concept would be to narrow the assumptions it makes and find a
sane way around the device permissions issues it currently does not address, to
make it more generally useful. A few ideas for addressing these points were mentioned
in the sections above. Another possibility worth exploring would be to integrate
this work with parallel development in running DMTCP within its own PID, mount,
and user namespaces. Within these namespaces, the user running DMTCP should have
full permissions to mount cgroup subsystems and tune device-related cgroup parameters.
The checkpoint-restart process for cgroups could then confidently proceed knowing
exactly where subsystems should be located and exactly which virtual files it should be
allowed to read and write.

Another possible extension to this plugin would be to add safety checks when restoring
a cgroup that already exists. Currently, the plugin will blindly overwrite any
parameters within the group to match the checkpointed information. This is the desired
behavior when re-creating a group, but if the group already exists upon restart and
contains different parameters from what was saved, overwriting the parameters could
easily disrupt other processes running within the group. The plugin could be augmented
to compare saved parameters to current parameters for existing groups and raise an
error if it detects a mismatch. Alternatively, the plugin could always restore to a
brand-new hierarchy, ensuring that restarting a process cannot interfere with the
resources available to other independently-running processes.

Lastly, cgroups are just one mechanism provided by Linux for containerization. A
natural follow-up to this work would be to develop similar checkpoint-restart
functionality for namespaces, another key container mechanism. While building
this plugin, we began preliminary work on a [namespaces plugin](../ns), but were
unable to make much progress. See the README of that plugin for additional details.
