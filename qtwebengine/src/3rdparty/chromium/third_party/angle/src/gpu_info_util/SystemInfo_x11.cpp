//
// Copyright (c) 2013-2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SystemInfo_x11.cpp: implementation of the X11-specific parts of SystemInfo.h

#include "gpu_info_util/SystemInfo_internal.h"

#include <GL/glx.h>
#include <GL/glxext.h>
#include <X11/Xlib.h>

#include "common/debug.h"
#include "third_party/libXNVCtrl/NVCtrl.h"
#include "third_party/libXNVCtrl/NVCtrlLib.h"

#if !defined(GPU_INFO_USE_X11)
#    error SystemInfo_x11.cpp compiled without GPU_INFO_USE_X11
#endif

#define GLX_RENDERER_VENDOR_ID_MESA	0x8183
#define GLX_RENDERER_DEVICE_ID_MESA	0x8184

namespace angle
{

bool CollectMesaCardInfo(std::vector<GPUDeviceInfo> *devices)
{

    unsigned int vid[3], did[3];

    Display *display = XOpenDisplay(NULL);
    if (!display) {
        return false;
    }

    PFNGLXQUERYRENDERERINTEGERMESAPROC queryInteger =
        (PFNGLXQUERYRENDERERINTEGERMESAPROC) glXGetProcAddressARB((const GLubyte *)
        "glXQueryRendererIntegerMESA");

    if (!queryInteger)
        return false;

    bool vendor_ret =
        queryInteger(display, 0, 0, GLX_RENDERER_VENDOR_ID_MESA, vid);
    bool device_ret =
        queryInteger(display, 0, 0, GLX_RENDERER_DEVICE_ID_MESA, did);

    if (vendor_ret && device_ret) {
        GPUDeviceInfo info;
        info.vendorId = vid[0];
        info.deviceId = did[0];
        devices->push_back(info);
    }

    return true;
}


bool GetNvidiaDriverVersionWithXNVCtrl(std::string *version)
{
    *version = "";

    int eventBase = 0;
    int errorBase = 0;

    Display *display = XOpenDisplay(nullptr);

    if (display && XNVCTRLQueryExtension(display, &eventBase, &errorBase))
    {
        int screenCount = ScreenCount(display);
        for (int screen = 0; screen < screenCount; ++screen)
        {
            char *buffer = nullptr;
            if (XNVCTRLIsNvScreen(display, screen) &&
                XNVCTRLQueryStringAttribute(display, screen, 0,
                                            NV_CTRL_STRING_NVIDIA_DRIVER_VERSION, &buffer))
            {
                ASSERT(buffer != nullptr);
                *version = buffer;
                XFree(buffer);
                return true;
            }
        }
    }

    return false;
}
}  // namespace angle
