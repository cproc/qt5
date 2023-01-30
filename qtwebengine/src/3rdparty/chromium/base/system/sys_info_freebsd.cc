// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/system/sys_info.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/sysctl.h>

#include "base/logging.h"
#include "base/strings/string_util.h"

namespace base {

int64_t SysInfo::AmountOfPhysicalMemoryImpl() {
#if 0
  int pages, page_size, r = 0;
  size_t size = sizeof(pages);
  if(r == 0)
    r = sysctlbyname("vm.stats.vm.v_page_count", &pages, &size, NULL, 0);
  if(r == 0)
    r =sysctlbyname("vm.stats.vm.v_page_size", &page_size, &size, NULL, 0);
  if(r == -1) {
    NOTREACHED();
    return 0;
  }
  return static_cast<int64_t>(pages) * page_size;
#else
  return 256 * 1024 * 1024;
#endif
}

int64_t SysInfo::AmountOfAvailablePhysicalMemoryImpl() {
  int page_size, r = 0;
  unsigned pgfree, pginact, pgcache;
  size_t size = sizeof(page_size);
  size_t szpg = sizeof(pgfree);
  if(r == 0)
    r = sysctlbyname("vm.stats.vm.v_page_size", &page_size, &size, NULL, 0);
  if(r == 0)
    r = sysctlbyname("vm.stats.vm.v_free_count", &pgfree, &szpg, NULL, 0);
  if(r == 0)
    r = sysctlbyname("vm.stats.vm.v_inactive_count", &pginact, &szpg, NULL, 0);
  if(r == 0)
    r = sysctlbyname("vm.stats.vm.v_cache_count", &pgcache, &szpg, NULL, 0);
  if(r == -1) {
    NOTREACHED();
    return 0;
  }
  return static_cast<int64_t>((pgfree + pginact + pgcache) * page_size);
}

// static
std::string SysInfo::CPUModelName() {
  int mib[] = { CTL_HW, HW_MODEL };
  char name[256];
  size_t size = base::size(name);
  if (sysctl(mib, base::size(mib), &name, &size, NULL, 0) == 0)
    return name;
  return std::string();
}

// static
uint64_t SysInfo::MaxSharedMemorySize() {
  size_t limit;
  size_t size = sizeof(limit);

  if (sysctlbyname("kern.ipc.shmmax", &limit, &size, NULL, 0) < 0) {
    NOTREACHED();
    return 0;
  }

  return static_cast<uint64_t>(limit);
}

SysInfo::HardwareInfo SysInfo::GetHardwareInfoSync() {
  HardwareInfo info;
  // Set the manufacturer to "FreeBSD" and the model to
  // an empty string.
  info.manufacturer = "FreeBSD";
  info.model = HardwareModelName();
  DCHECK(IsStringUTF8(info.manufacturer));
  DCHECK(IsStringUTF8(info.model));
  return info;
}

}  // namespace base
