// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/address_tracker_linux.h"

#include <errno.h>
#include <linux/if.h>
#include <stdint.h>
#include <sys/ioctl.h>

#include "base/bind_helpers.h"
#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/message_loop/message_loop_current.h"
#include "base/optional.h"
#include "base/posix/eintr_wrapper.h"
#include "base/threading/scoped_blocking_call.h"
#include "net/base/network_interfaces_linux.h"

namespace net {
namespace internal {

// static
char* AddressTrackerLinux::GetInterfaceName(int interface_index, char* buf) {
  NOTIMPLEMENTED();
  return NULL;
}

AddressTrackerLinux::AddressTrackerLinux()
    : get_interface_name_(GetInterfaceName),
      address_callback_(base::DoNothing()),
      link_callback_(base::DoNothing()),
      tunnel_callback_(base::DoNothing()),
      netlink_fd_(-1),
      watcher_(FROM_HERE),
      ignored_interfaces_(),
      connection_type_initialized_(false),
      connection_type_initialized_cv_(&connection_type_lock_),
      current_connection_type_(NetworkChangeNotifier::CONNECTION_NONE),
      tracking_(false),
      threads_waiting_for_connection_type_initialization_(0) {}

AddressTrackerLinux::AddressTrackerLinux(
    const base::Closure& address_callback,
    const base::Closure& link_callback,
    const base::Closure& tunnel_callback,
    const std::unordered_set<std::string>& ignored_interfaces)
    : get_interface_name_(GetInterfaceName),
      address_callback_(address_callback),
      link_callback_(link_callback),
      tunnel_callback_(tunnel_callback),
      netlink_fd_(-1),
      watcher_(FROM_HERE),
      ignored_interfaces_(ignored_interfaces),
      connection_type_initialized_(false),
      connection_type_initialized_cv_(&connection_type_lock_),
      current_connection_type_(NetworkChangeNotifier::CONNECTION_NONE),
      tracking_(true),
      threads_waiting_for_connection_type_initialization_(0) {
  DCHECK(!address_callback.is_null());
  DCHECK(!link_callback.is_null());
}

AddressTrackerLinux::~AddressTrackerLinux() {
  CloseSocket();
}

void AddressTrackerLinux::Init() {
NOTIMPLEMENTED();
AbortAndForceOnline();
}

void AddressTrackerLinux::AbortAndForceOnline() {
  CloseSocket();
  AddressTrackerAutoLock lock(*this, connection_type_lock_);
  current_connection_type_ = NetworkChangeNotifier::CONNECTION_UNKNOWN;
  connection_type_initialized_ = true;
  connection_type_initialized_cv_.Broadcast();
}

NetworkChangeNotifier::ConnectionType
AddressTrackerLinux::GetCurrentConnectionType() {
  // http://crbug.com/125097
  base::ScopedAllowBaseSyncPrimitivesOutsideBlockingScope allow_wait;
  AddressTrackerAutoLock lock(*this, connection_type_lock_);
  // Make sure the initial connection type is set before returning.
  threads_waiting_for_connection_type_initialization_++;
  while (!connection_type_initialized_) {
    connection_type_initialized_cv_.Wait();
  }
  threads_waiting_for_connection_type_initialization_--;
  return current_connection_type_;
}

void AddressTrackerLinux::ReadMessages(bool* address_changed,
                                       bool* link_changed,
                                       bool* tunnel_changed) {
  *address_changed = false;
  *link_changed = false;
  *tunnel_changed = false;
  char buffer[4096];
  bool first_loop = true;
  {
    base::Optional<base::ScopedBlockingCall> blocking_call;
    if (tracking_) {
      // If the loop below takes a long time to run, a new thread should added
      // to the current thread pool to ensure forward progress of all tasks.
      blocking_call.emplace(base::BlockingType::MAY_BLOCK);
    }

    for (;;) {
      int rv = HANDLE_EINTR(recv(netlink_fd_, buffer, sizeof(buffer),
                                 // Block the first time through loop.
                                 first_loop ? 0 : MSG_DONTWAIT));
      first_loop = false;
      if (rv == 0) {
        LOG(ERROR) << "Unexpected shutdown of NETLINK socket.";
        return;
      }
      if (rv < 0) {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
          break;
        PLOG(ERROR) << "Failed to recv from netlink socket";
        return;
      }
      HandleMessage(buffer, rv, address_changed, link_changed, tunnel_changed);
    }
  }
  if (*link_changed || *address_changed)
    UpdateCurrentConnectionType();
}

void AddressTrackerLinux::HandleMessage(char* buffer,
                                        size_t length,
                                        bool* address_changed,
                                        bool* link_changed,
                                        bool* tunnel_changed) {
  NOTIMPLEMENTED();
}

void AddressTrackerLinux::OnFileCanReadWithoutBlocking(int fd) {
  DCHECK_EQ(netlink_fd_, fd);
  bool address_changed;
  bool link_changed;
  bool tunnel_changed;
  ReadMessages(&address_changed, &link_changed, &tunnel_changed);
  if (address_changed)
    address_callback_.Run();
  if (link_changed)
    link_callback_.Run();
  if (tunnel_changed)
    tunnel_callback_.Run();
}

void AddressTrackerLinux::OnFileCanWriteWithoutBlocking(int /* fd */) {}

void AddressTrackerLinux::CloseSocket() {
  if (netlink_fd_ >= 0 && IGNORE_EINTR(close(netlink_fd_)) < 0)
    PLOG(ERROR) << "Could not close NETLINK socket.";
  netlink_fd_ = -1;
}

bool AddressTrackerLinux::IsTunnelInterface(int interface_index) const {
  char buf[IFNAMSIZ] = {0};
  return IsTunnelInterfaceName(get_interface_name_(interface_index, buf));
}

// static
bool AddressTrackerLinux::IsTunnelInterfaceName(const char* name) {
  // Linux kernel drivers/net/tun.c uses "tun" name prefix.
  return strncmp(name, "tun", 3) == 0;
}

void AddressTrackerLinux::UpdateCurrentConnectionType() {
  NOTIMPLEMENTED();
}

int AddressTrackerLinux::GetThreadsWaitingForConnectionTypeInitForTesting()
{
  AddressTrackerAutoLock lock(*this, connection_type_lock_);
  return threads_waiting_for_connection_type_initialization_;
}

AddressTrackerLinux::AddressTrackerAutoLock::AddressTrackerAutoLock(
    const AddressTrackerLinux& tracker,
    base::Lock& lock)
    : tracker_(tracker), lock_(lock) {
  if (tracker_.tracking_) {
    lock_.Acquire();
  } else {
    DCHECK(tracker_.thread_checker_.CalledOnValidThread());
  }
}

AddressTrackerLinux::AddressTrackerAutoLock::~AddressTrackerAutoLock() {
  if (tracker_.tracking_) {
    lock_.AssertAcquired();
    lock_.Release();
  }
}

}  // namespace internal
}  // namespace net
