// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_SNDIO_SNDIO_OUTPUT_H_
#define MEDIA_AUDIO_SNDIO_SNDIO_OUTPUT_H_

#include <pthread.h>

#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "media/audio/audio_io.h"

namespace media {

class AudioManagerBase;

// Implementation of AudioOutputStream using sndio(7)
class OssAudioOutputStream : public AudioOutputStream {
 public:
  // The manager is creating this object
  OssAudioOutputStream(const AudioParameters& params,
                         AudioManagerBase* manager);
  virtual ~OssAudioOutputStream();

  // Implementation of AudioOutputStream.
  bool Open() override;
  void Close() override;
  void Start(AudioSourceCallback* callback) override;
  void Stop() override;
  void SetVolume(double volume) override;
  void GetVolume(double* volume) override;
  void Flush() override;

 private:
  enum StreamState {
    kClosed,            // Not opened yet
    kStopped,           // Device opened, but not started yet
    kRunning,           // Started, device playing
    kStopWait           // Stopping, waiting for the real-time thread to exit
  };

  // C-style call-backs
  static void OnMoveCallback(void *arg, int delta);
  static void OnVolCallback(void *arg, unsigned int vol);
  static void* ThreadEntry(void *arg);

  // Continuously moves data from the producer to the device
  void ThreadLoop(void);

  // Our creator, the audio manager needs to be notified when we close.
  AudioManagerBase* manager;
  // Parameters of the source
  AudioParameters params;
  // Source stores data here
  std::unique_ptr<AudioBus> audio_bus;
  // Call-back that produces data to play
  AudioSourceCallback* source;

  // Handle of the OSS audio device
  int write_fd;

  bool up_mixing;

  // Handle of the audio device
  struct sio_hdl* hdl;
  // Current state of the stream
  enum StreamState state;
  // High priority thread running ThreadLoop()
  pthread_t thread;
  // Protects vol, volpending and hw_delay
  pthread_mutex_t mutex;
  // Current volume in the 0..SIO_MAXVOL range
  int vol;
  // Set to 1 if volumes must be refreshed in the realtime thread
  int volpending;
  // Temporary buffer where data is stored in OSS-compatible format
  char* buffer;

  DISALLOW_COPY_AND_ASSIGN(OssAudioOutputStream);
};

}  // namespace media

#endif  // MEDIA_AUDIO_SNDIO_SNDIO_OUTPUT_H_
