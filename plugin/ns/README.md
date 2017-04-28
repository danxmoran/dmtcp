# DMTCP Control Group Plugin
Authors: Daniel Moran, Drew Pomerleau, Matthew Piorko

## <a name="overview"></a>Overview of Functionality
This plugin enables DMTCP to capture the
[Namespaces](http://man7.org/linux/man-pages/man7/namespaces.7.html) of
a process during checkpointing, and then restore the process into the same namespaces upon restart.
The plugin is currently a work-in-progess proof-of-concept to demonstrate one possible way to capture
namepsaces and restore them.

## <a name="cgroup-api"></a>Overview of Namespace API
While there is no direct system call to create a namespace, the
`clone` call offers a parameter for flags that allow you to create
different types of namespaces.

### Types of namespaces
There are several different types of namespaces to allow for isolating different 
parts of the operating system. The linux kernal currently supports namespaces for:
 - CGroup
 - IPC
 - Network
 - Mount
 - PID
 - User
 - UTS

An example call for creating a new PID namespace is:
```
clone([FUNCTION], [CHILD_STACK_LOCATION], CLONE_NEWPID | SIGCHLD, NULL);
```
where `[FUNCTION]` is a pointer to a function to invoke in the new namespace
and `[CHILD_STACK_LOCATION]` is a pointer to the address where the new process'
stack will be. `CLONE_NEWPID` is the flag to specify that new process should be 
in new PID namespace.

Existing processes can also be moved into namespaces through the `setns` function.

## Plugin Design
This plugin wraps the clone system call to account for new namespace creation.
Our wrapper checks the flags and stores them in our checkpoint file. When built,
there will be a new `libdmtcp_ns.so` in `lib/dmtcp` which can be linked with DMTCP.

#### Checkpoint location and structure
Currently, checkpointed cgroup information is dumped into a file `ns_ckpt.dmtcp`
in the directory of the process being checkpointed. The contents of the file are currently
just the flags used to invoke clone for the process.

### Restoring Namespaces
Restoring is not supported at the moment but this could be done by reading the checkpoint file,
creating the new namespace, and moving the process into it.

## Future Work
Creating the restoration system would allow for this plugin to actually be used for checkpointing namespaces.
This would simply invole the steps mentioned in `Restoring Namespaces`. We also explored the idea of reading `proc/[PID]/ns/*`
to find the current status on checkpoint and use that information rather than wrapping clone. Wrapping system calls can be tricky
and using this method may be a better longterm solution.
