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

#ifndef PROCCGROUP_H
#define PROCCGROUP_H

#define FILENAMESIZE 1024

// TODO(dan) Add support for all of these.
typedef enum CGroupSubsystem {
  // DMTCP_CGROUP_BLKIO,
  // DMTCP_CGROUP_CPU,
  // DMTCP_CGROUP_CPUACCT,
  // DMTCP_CGROUP_CPUSETS,
  // DMTCP_CGROUP_DEVICES,
  // DMTCP_CGROUP_FREEZER,
  // DMTCP_CGROUP_HUGETLB,
  DMTCP_CGROUP_MEMORY,
  // DMTCP_CGROUP_NET_CLS,
  // DMTCP_CGROPU_NET_PRIO,
  // DMTCP_CGROUP_PERF_EVENT,
  // DMTCP_CGROUP_PIDS,
  // DMTCP_CGROUP_NO_SUBSYSTEM // i.e. name=systemd cgroup
} CGroupSubsystem;

typedef struct ProcCGroup {
  CGroupSubsystem subsystem;

  // TODO(dan): Add ability to also track parameters of
  // parent cgroups.

  union {
    struct memory {
      ssize_t limit_in_bytes;
      ssize_t memsw_limit_in_bytes;
      ssize_t soft_limit_in_bytes;
      int use_hierarchy;
      int swappiness;
      int move_charge_at_immigrate;
      int oom_control;
      ssize_t kmem_limit_in_bytes;
      ssize_t kmem_tcp_limit_in_bytes;
    };
  }

  char name[FILENAMESIZE];
};

#endif // ifndef PROCCGROUP_H
