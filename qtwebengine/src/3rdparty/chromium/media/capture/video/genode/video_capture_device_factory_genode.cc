// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/genode/video_capture_device_factory_genode.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/scoped_blocking_call.h"
#include "build/build_config.h"
#include "media/base/media_switches.h"
#include "media/capture/video/genode/video_capture_device_genode.h"

namespace media {

const char kVideoCaptureDeviceGenodeName[] = "/dev/capture";

std::unique_ptr<VideoCaptureDevice> VideoCaptureDeviceFactoryGenode::CreateDevice(
    const VideoCaptureDeviceDescriptor& device_descriptor) {
  DCHECK(thread_checker_.CalledOnValidThread());
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);
  return std::unique_ptr<VideoCaptureDevice>(new VideoCaptureDeviceGenode());
}

void VideoCaptureDeviceFactoryGenode::GetDeviceDescriptors(
    VideoCaptureDeviceDescriptors* device_descriptors) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(device_descriptors->empty());
  base::ThreadRestrictions::SetIOAllowed(true);

  device_descriptors->emplace_back(
      kVideoCaptureDeviceGenodeName, kVideoCaptureDeviceGenodeName,
      VideoCaptureApi::UNKNOWN
  );
}

void VideoCaptureDeviceFactoryGenode::GetSupportedFormats(
    const VideoCaptureDeviceDescriptor& device_descriptor,
    VideoCaptureFormats* supported_formats) {
  DCHECK(thread_checker_.CalledOnValidThread());
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);
  VideoCaptureFormat capture_format;
  if (!VideoCaptureDeviceGenode::GetVideoCaptureFormat(&capture_format)) {
    return;
  }

  supported_formats->push_back(capture_format);
}

void VideoCaptureDeviceFactoryGenode::GetCameraLocationsAsync(
    std::unique_ptr<VideoCaptureDeviceDescriptors> device_descriptors,
    DeviceDescriptorsCallback result_callback) {
  std::move(result_callback).Run(std::move(device_descriptors));
}

}  // namespace media
