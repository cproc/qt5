// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include <memory>

#include "base/files/file_path_watcher.h"
#if 0
#include "base/files/file_path_watcher_kqueue.h"
#endif

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"

namespace base {

namespace {

class FilePathWatcherImpl : public FilePathWatcher::PlatformDelegate {
 public:
  FilePathWatcherImpl() = default;
  ~FilePathWatcherImpl() override = default;

  bool Watch(const FilePath& path,
             bool recursive,
             const FilePathWatcher::Callback& callback) override {
#if defined(OS_BSD) && 0
    DCHECK(!impl_.get());
    if (recursive) {
      return false;
    } else {
      impl_ = std::make_unique<FilePathWatcherKQueue>();
    }
    DCHECK(impl_.get());
    return impl_->Watch(path, recursive, callback);
#else
    return false;
#endif
  }

  void Cancel() override {
#if 0
    if (impl_.get())
      impl_->Cancel();
    set_cancelled();
#endif
  }

 private:
#if 0
  std::unique_ptr<PlatformDelegate> impl_;
#endif

  DISALLOW_COPY_AND_ASSIGN(FilePathWatcherImpl);
};

}  // namespace

FilePathWatcher::FilePathWatcher() {
  sequence_checker_.DetachFromSequence();
  impl_ = std::make_unique<FilePathWatcherImpl>();
}

}  // namespace base
