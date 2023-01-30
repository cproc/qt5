// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <sys/soundcard.h>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "media/base/audio_timestamp_helper.h"
#include "media/audio/genode/audio_manager_genode.h"
#include "media/audio/audio_manager.h"
#include "media/audio/oss/oss_input.h"

namespace media {

static const SampleFormat kSampleFormat = kSampleFormatS16;

void *OssAudioInputStream::ThreadEntry(void *arg) {
  OssAudioInputStream* self = static_cast<OssAudioInputStream*>(arg);

  self->ThreadLoop();
  return NULL;
}

OssAudioInputStream::OssAudioInputStream(AudioManagerBase* manager,
                                             const std::string& device_name,
                                             const AudioParameters& params)
    : manager(manager),
      params(params),
      audio_bus(AudioBus::Create(params)),
      state(kClosed) {
}

OssAudioInputStream::~OssAudioInputStream() {
  if (state != kClosed)
    Close();
}

bool OssAudioInputStream::Open() {

  if (state != kClosed)
    return false;

  if (params.format() != AudioParameters::AUDIO_PCM_LINEAR &&
      params.format() != AudioParameters::AUDIO_PCM_LOW_LATENCY) {
    LOG(WARNING) << "Unsupported audio format.";
    return false;
  }

  read_fd = open("/dev/dsp", O_RDONLY);
  if (read_fd == -1) {
    LOG(ERROR) << "Couldn't open audio device.";
    return false;
  }

  state = kStopped;
  buffer = new char[audio_bus->frames() * params.GetBytesPerFrame(kSampleFormat)];
  return true;
}

void OssAudioInputStream::Start(AudioInputCallback* cb) {

  StartAgc();

  state = kRunning;
  hw_delay = 0;
  callback = cb;
  if (pthread_create(&thread, NULL, &ThreadEntry, this) != 0) {
    LOG(ERROR) << "Failed to create real-time thread for recording.";
    state = kStopped;
  }
}

void OssAudioInputStream::Stop() {

  if (state == kStopped)
    return;

  state = kStopWait;
  pthread_join(thread, NULL);
  state = kStopped;

  StopAgc();
}

void OssAudioInputStream::Close() {

  if (state == kClosed)
    return;

  if (state == kRunning)
    Stop();

  state = kClosed;
  delete [] buffer;
  close(read_fd);

  manager->ReleaseInputStream(this);
}

double OssAudioInputStream::GetMaxVolume() {
  // Not supported
  return 0.0;
}

void OssAudioInputStream::SetVolume(double volume) {
  // Not supported. Do nothing.
}

double OssAudioInputStream::GetVolume() {
  // Not supported.
  return 0.0;
}

bool OssAudioInputStream::IsMuted() {
  // Not supported.
  return false;
}

void OssAudioInputStream::SetOutputDeviceForAec(
    const std::string& output_device_id) {
  // Not supported.
}

void OssAudioInputStream::ThreadLoop(void) {
  size_t todo, n;
  char *data;
  unsigned int nframes;
  double normalized_volume = 0.0;

  nframes = audio_bus->frames();

  while (state == kRunning) {

    GetAgcVolume(&normalized_volume);

    // read one block
    todo = nframes * params.GetBytesPerFrame(kSampleFormat);
    data = buffer;
    while (todo > 0) {
      n = read(read_fd, data, todo);
      if (n == 0)
        return;	// unrecoverable I/O error
      todo -= n;
      data += n;
    }
    //hw_delay -= nframes;

    // convert frames count to TimeDelta
    const base::TimeDelta delay = AudioTimestampHelper::FramesToTime(hw_delay,
      params.sample_rate());

    // push into bus
    audio_bus->FromInterleaved(buffer, nframes, SampleFormatToBytesPerChannel(kSampleFormat));

    // invoke callback
    callback->OnData(audio_bus.get(), base::TimeTicks::Now() - delay, 1.);
  }
}

}  // namespace media
