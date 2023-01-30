// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <sys/soundcard.h>

#include "base/logging.h"
#include "base/time/time.h"
#include "base/time/default_tick_clock.h"
#include "media/audio/audio_manager_base.h"
#include "media/base/audio_timestamp_helper.h"
#include "media/audio/oss/oss_output.h"

namespace media {

static const SampleFormat kSampleFormat = kSampleFormatS16;

void *OssAudioOutputStream::ThreadEntry(void *arg) {
  OssAudioOutputStream* self = static_cast<OssAudioOutputStream*>(arg);

  self->ThreadLoop();
  return NULL;
}

OssAudioOutputStream::OssAudioOutputStream(const AudioParameters& params,
                                               AudioManagerBase* manager)
    : manager(manager),
      params(params),
      audio_bus(AudioBus::Create(params)),
      up_mixing(false),
      state(kClosed),
      mutex(PTHREAD_MUTEX_INITIALIZER) {
}

OssAudioOutputStream::~OssAudioOutputStream() {
  if (state != kClosed)
    Close();
}

bool OssAudioOutputStream::Open() {

  if (params.format() != AudioParameters::AUDIO_PCM_LINEAR &&
      params.format() != AudioParameters::AUDIO_PCM_LOW_LATENCY) {
    LOG(WARNING) << "Unsupported audio format.";
    return false;
  }

  if (params.channels() < 2)
    up_mixing = true;

  write_fd = open("/dev/dsp", O_WRONLY);
  if (write_fd == -1) {
    LOG(ERROR) << "Couldn't open audio device.";
    return false;
  }

  int fragment = (16 << 16) | 11;
  ioctl(write_fd, SNDCTL_DSP_SETFRAGMENT, &fragment);

  state = kStopped;
  volpending = 0;
  vol = 0;
  buffer = new char[audio_bus->frames() * params.GetBytesPerFrame(kSampleFormat)];
  return true;
}

void OssAudioOutputStream::Close() {
  if (state == kClosed)
    return;
  if (state == kRunning)
    Stop();
  state = kClosed;
  delete [] buffer;
  close(write_fd);
  manager->ReleaseOutputStream(this);  // Calls the destructor
}

void OssAudioOutputStream::Start(AudioSourceCallback* callback) {
  state = kRunning;
  source = callback;
  if (pthread_create(&thread, NULL, &ThreadEntry, this) != 0) {
    LOG(ERROR) << "Failed to create real-time thread.";
    state = kStopped;
  }
}

void OssAudioOutputStream::Stop() {
  if (state == kStopped)
    return;
  state = kStopWait;
  pthread_join(thread, NULL);
  state = kStopped;
}

void OssAudioOutputStream::SetVolume(double v) {
  pthread_mutex_lock(&mutex);
  vol = v * 255;
  volpending = 1;
  pthread_mutex_unlock(&mutex);
}

void OssAudioOutputStream::GetVolume(double* v) {
  pthread_mutex_lock(&mutex);
  *v = vol * (1. / 255);
  pthread_mutex_unlock(&mutex);
}

void OssAudioOutputStream::Flush() {}

void OssAudioOutputStream::ThreadLoop(void) {
  int avail, count, result;

  static short up_mixing_buffer[16u << 10];

  while (state == kRunning) {
    // Update volume if needed
    pthread_mutex_lock(&mutex);
    if (volpending) {
      volpending = 0;
    }
    pthread_mutex_unlock(&mutex);

    // Get data to play
    const base::TimeDelta delay =
        AudioTimestampHelper::FramesToTime(0 /* hw_delay */, params.sample_rate());
    count = source->OnMoreData(delay, base::TimeTicks::Now(), 0, audio_bus.get());
    audio_bus->ToInterleaved(count, SampleFormatToBytesPerChannel(kSampleFormat), buffer);
    if (count == 0) {
      // We have to submit something to the device
      count = audio_bus->frames();
      memset(buffer, 0, count * params.GetBytesPerFrame(kSampleFormat));
      LOG(WARNING) << "No data to play, running empty cycle.";
    }

    // Submit data to the device
    if (up_mixing) {
      short *p = (short*)buffer;
      int const avail = count * params.GetBytesPerFrame(kSampleFormat);
      for (int j = 0, i = 0; i < avail / 2; i++) {
        up_mixing_buffer[j++] = p[i];
        up_mixing_buffer[j++] = p[i];
      }
      result = write(write_fd, up_mixing_buffer, avail * 2);
    } else {
      avail = count * params.GetBytesPerFrame(kSampleFormat);
      result = write(write_fd, buffer, avail);
    }
    if (result == 0) {
      LOG(WARNING) << "Audio device disconnected.";
      break;
    }
  }
}

}  // namespace media
