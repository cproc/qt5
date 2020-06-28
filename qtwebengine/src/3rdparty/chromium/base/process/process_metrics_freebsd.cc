// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/process/process_metrics.h"

#include <stddef.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <unistd.h>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/process/process_metrics_iocounters.h"
#include "base/stl_util.h"

#include <unistd.h> /* getpagesize() */
#include <fcntl.h>  /* O_RDONLY */
#include <kvm.h>
#include <libutil.h>

namespace base {

ProcessMetrics::ProcessMetrics(ProcessHandle process)
    : process_(process) {}

// static
std::unique_ptr<ProcessMetrics> ProcessMetrics::CreateProcessMetrics(
    ProcessHandle process) {
  return WrapUnique(new ProcessMetrics(process));
}

double ProcessMetrics::GetPlatformIndependentCPUUsage() {
  struct kinfo_proc info;
  int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, process_};
  size_t length = sizeof(info);

  if (sysctl(mib, base::size(mib), &info, &length, NULL, 0) < 0)
    return 0;

  return (info.ki_pctcpu / FSCALE) * 100.0;
}

TimeDelta ProcessMetrics::GetCumulativeCPUUsage() {
  NOTREACHED();
  return TimeDelta();
}

bool ProcessMetrics::GetIOCounters(IoCounters* io_counters) const {
  return false;
}

size_t GetSystemCommitCharge() {
  int mib[2], pagesize;
  unsigned long mem_total, mem_free, mem_inactive;
  size_t length = sizeof(mem_total);

  if (sysctl(mib, base::size(mib), &mem_total, &length, NULL, 0) < 0)
    return 0;

  length = sizeof(mem_free);
  if (sysctlbyname("vm.stats.vm.v_free_count", &mem_free, &length, NULL, 0) < 0)
    return 0;

  length = sizeof(mem_inactive);
  if (sysctlbyname("vm.stats.vm.v_inactive_count", &mem_inactive, &length,
      NULL, 0) < 0) {
    return 0;
  }

  pagesize = getpagesize();

  return mem_total - (mem_free*pagesize) - (mem_inactive*pagesize);
}

int GetNumberOfThreads(ProcessHandle process) {
  // Taken from FreeBSD top (usr.bin/top/machine.c)

  kvm_t* kd = kvm_open(NULL, "/dev/null", NULL, O_RDONLY, "kvm_open");
  if (kd == NULL)
    return 0;

  struct kinfo_proc* pbase;
  int nproc;
  pbase = kvm_getprocs(kd, KERN_PROC_PID, process, &nproc);
  if (pbase == NULL)
    return 0;

  if (kvm_close(kd) == -1)
    return 0;

  return nproc;
}

bool GetSystemMemoryInfo(SystemMemoryInfoKB *meminfo) {
  unsigned int mem_total, mem_free, swap_total, swap_used;
  size_t length;
  int pagesizeKB;

  pagesizeKB = getpagesize() / 1024;

  length = sizeof(mem_total);
  if (sysctlbyname("vm.stats.vm.v_page_count", &mem_total,
      &length, NULL, 0) != 0 || length != sizeof(mem_total))
    return false;

  length = sizeof(mem_free);
  if (sysctlbyname("vm.stats.vm.v_free_count", &mem_free, &length, NULL, 0)
      != 0 || length != sizeof(mem_free))
    return false;

  length = sizeof(swap_total);
  if (sysctlbyname("vm.swap_size", &swap_total, &length, NULL, 0)
      != 0 || length != sizeof(swap_total))
    return false;

  length = sizeof(swap_used);
  if (sysctlbyname("vm.swap_anon_use", &swap_used, &length, NULL, 0)
      != 0 || length != sizeof(swap_used))
    return false;

  meminfo->total = mem_total * pagesizeKB;
  meminfo->free = mem_free * pagesizeKB;
  meminfo->swap_total = swap_total * pagesizeKB;
  meminfo->swap_free = (swap_total - swap_used) * pagesizeKB;

  return true;
}

int ProcessMetrics::GetOpenFdCount() const {
  struct kinfo_file * kif;
  int cnt;

  if ((kif = kinfo_getfile(process_, &cnt)) == NULL)
    return -1;

  free(kif);

  return cnt;
}

int ProcessMetrics::GetOpenFdSoftLimit() const {
  size_t length;
  int total_count = 0;
  int mib[] = { CTL_KERN, KERN_MAXFILESPERPROC };

  length = sizeof(total_count);

  if (sysctl(mib, base::size(mib), &total_count, &length, NULL, 0) < 0) {
    total_count = -1;
  }

  return total_count;
}

uint64_t ProcessMetrics::GetVmSwapBytes() const {
   NOTIMPLEMENTED();
   return 0;
}

int ProcessMetrics::GetIdleWakeupsPerSecond() {
  NOTIMPLEMENTED();
  return 0;
}
}  // namespace base
