// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_GENODE_VIDEO_CAPTURE_DEVICE_GENODE_H_
#define MEDIA_CAPTURE_VIDEO_GENODE_VIDEO_CAPTURE_DEVICE_GENODE_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/files/file.h"
#include "base/files/memory_mapped_file.h"
#include "base/macros.h"
#include "base/threading/thread.h"
#include "base/threading/thread_checker.h"
#include "media/capture/video/video_capture_device.h"

namespace media {

class VideoFileParser;

class CAPTURE_EXPORT VideoCaptureDeviceGenode : public VideoCaptureDevice {
 public:
  static bool GetVideoCaptureFormat(VideoCaptureFormat* video_format);

  explicit VideoCaptureDeviceGenode();

  // VideoCaptureDevice implementation, class methods.
  ~VideoCaptureDeviceGenode() override;
  void AllocateAndStart(
      const VideoCaptureParams& params,
      std::unique_ptr<VideoCaptureDevice::Client> client) override;
  void StopAndDeAllocate() override;
  void GetPhotoState(GetPhotoStateCallback callback) override;
  void SetPhotoOptions(mojom::PhotoSettingsPtr settings,
                       SetPhotoOptionsCallback callback) override;
  void TakePhoto(TakePhotoCallback callback) override;

 private:

  // Called on the |capture_thread_|.
  void OnAllocateAndStart(const VideoCaptureParams& params,
                          std::unique_ptr<Client> client);
  void OnStopAndDeAllocate();
  const uint8_t* GetNextFrame();
  void OnCaptureTask();

  // |thread_checker_| is used to check that destructor, AllocateAndStart() and
  // StopAndDeAllocate() are called in the correct thread that owns the object.
  base::ThreadChecker thread_checker_;

  // |capture_thread_| is used for internal operations via posting tasks to it.
  // It is active between OnAllocateAndStart() and OnStopAndDeAllocate().
  base::Thread capture_thread_;
  // The following members belong to |capture_thread_|.
  std::unique_ptr<VideoCaptureDevice::Client> client_;
  VideoCaptureFormat capture_format_;
  // Target time for the next frame.
  base::TimeTicks next_frame_time_;
  // The system time when we receive the first frame.
  base::TimeTicks first_ref_time_;

  // capture data file
  int file_ { -1 };

  // Guards the below variables from concurrent access between methods running
  // on the main thread and |capture_thread_|.
  base::Lock lock_;
  base::queue<TakePhotoCallback> take_photo_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(VideoCaptureDeviceGenode);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_GENODE_VIDEO_CAPTURE_DEVICE_GENODE_H_
