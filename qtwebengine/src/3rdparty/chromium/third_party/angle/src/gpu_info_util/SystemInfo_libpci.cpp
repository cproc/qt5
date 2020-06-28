//
// Copyright (c) 2013-2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SystemInfo_libpci.cpp: implementation of the libPCI-specific parts of SystemInfo.h

#include "gpu_info_util/SystemInfo_internal.h"

#include <dlfcn.h>
#include <pci/pci.h>
#include <unistd.h>

#if defined(__FreeBSD__)
#include <fcntl.h>
#include <sys/pciio.h>
#endif

#include "common/angleutils.h"
#include "common/debug.h"

#if !defined(GPU_INFO_USE_LIBPCI)
#    error SystemInfo_libpci.cpp compiled without GPU_INFO_USE_LIBPCI
#endif

namespace angle
{

namespace
{

struct LibPCI : private angle::NonCopyable
{
    LibPCI()
    {
        if (access("/sys/bus/pci/", F_OK) != 0 && access("/sys/bs/pci_express/", F_OK) != 0)
        {
            return;
        }

        mHandle = dlopen("libpci.so.3", RTLD_LAZY);

        if (mHandle == nullptr)
        {
            mHandle = dlopen("libpci.so", RTLD_LAZY);
        }

        if (mHandle == nullptr)
        {
            return;
        }

        mValid =
            (Alloc = reinterpret_cast<decltype(Alloc)>(dlsym(mHandle, "pci_alloc"))) != nullptr &&
            (Init = reinterpret_cast<decltype(Init)>(dlsym(mHandle, "pci_init"))) != nullptr &&
            (Cleanup = reinterpret_cast<decltype(Cleanup)>(dlsym(mHandle, "pci_cleanup"))) !=
                nullptr &&
            (ScanBus = reinterpret_cast<decltype(ScanBus)>(dlsym(mHandle, "pci_scan_bus"))) !=
                nullptr &&
            (FillInfo = reinterpret_cast<decltype(FillInfo)>(dlsym(mHandle, "pci_fill_info"))) !=
                nullptr &&
            (LookupName = reinterpret_cast<decltype(LookupName)>(
                 dlsym(mHandle, "pci_lookup_name"))) != nullptr;
    }

    bool IsValid() const { return mValid; }

    ~LibPCI()
    {
        if (mHandle != nullptr)
        {
            dlclose(mHandle);
        }
    }

    decltype(&::pci_alloc) Alloc            = nullptr;
    decltype(&::pci_init) Init              = nullptr;
    decltype(&::pci_cleanup) Cleanup        = nullptr;
    decltype(&::pci_scan_bus) ScanBus       = nullptr;
    decltype(&::pci_fill_info) FillInfo     = nullptr;
    decltype(&::pci_lookup_name) LookupName = nullptr;

  private:
    void *mHandle = nullptr;
    bool mValid   = false;
};

}  // anonymous namespace

#if defined(__FreeBSD__)
// Adds an entry per PCI GPU found and fills the device and vendor ID.
bool GetPCIDevicesFreeBSD(std::vector<GPUDeviceInfo> *devices)
{
    int fd;
    struct pci_conf_io conf;
    struct pci_conf *matches;
    uint32_t offset = 0;

    fd = open("/dev/pci", O_RDONLY);
    if (fd < 0)
        return false;

    matches = new struct pci_conf[32];
    conf.generation = 0;
    do {
        conf.pat_buf_len = 0;
        conf.num_patterns = 0;
        conf.patterns = NULL;
        conf.match_buf_len = 32 * sizeof(struct pci_conf);
        conf.num_matches = 32;
        conf.matches = matches;
        conf.offset = offset;
        conf.status = PCI_GETCONF_ERROR;
        if (ioctl(fd, PCIOCGETCONF, &conf) < 0) {
            if (errno == ENODEV)
                break;
        }
        /* PCI_GETCONF_LIST_CHANGED would require us to start over. */
        if (conf.status == PCI_GETCONF_ERROR || conf.status == PCI_GETCONF_LIST_CHANGED) {
            break;
        }

        for (unsigned int i = 0; i < conf.num_matches; i++) {
            uint16_t device_class = (matches[i].pc_class << 8) |  matches[i].pc_subclass;

            // Skip non-GPU devices
            switch (device_class)
            {
                case PCI_CLASS_DISPLAY_VGA:
                case PCI_CLASS_DISPLAY_XGA:
                case PCI_CLASS_DISPLAY_3D:
                    break;
                default:
                    continue;
            }

            // Skip unknown devices
            if (matches[i].pc_vendor == 0 || matches[i].pc_device == 0) {
                continue;
            }

            GPUDeviceInfo info;
            info.vendorId = matches[i].pc_vendor;
            info.deviceId = matches[i].pc_device;

            devices->push_back(info);
        }
        offset += conf.num_matches;
    } while (conf.status == PCI_GETCONF_MORE_DEVS);

    delete[] matches;

    close(fd);

    return true;
}
#endif

// Adds an entry per PCI GPU found and fills the device and vendor ID.
bool GetPCIDevicesWithLibPCI(std::vector<GPUDeviceInfo> *devices)
{
    LibPCI pci;
    if (!pci.IsValid())
    {
        return false;
    }

    pci_access *access = pci.Alloc();
    ASSERT(access != nullptr);
    pci.Init(access);
    pci.ScanBus(access);

    for (pci_dev *device = access->devices; device != nullptr; device = device->next)
    {
        pci.FillInfo(device, PCI_FILL_IDENT | PCI_FILL_CLASS);

        // Skip non-GPU devices
        switch (device->device_class)
        {
            case PCI_CLASS_DISPLAY_VGA:
            case PCI_CLASS_DISPLAY_XGA:
            case PCI_CLASS_DISPLAY_3D:
                break;
            default:
                continue;
        }

        // Skip unknown devices
        if (device->vendor_id == 0 || device->device_id == 0)
        {
            continue;
        }

        GPUDeviceInfo info;
        info.vendorId = device->vendor_id;
        info.deviceId = device->device_id;

        devices->push_back(info);
    }

    pci.Cleanup(access);

    return true;
}
}  // namespace angle
