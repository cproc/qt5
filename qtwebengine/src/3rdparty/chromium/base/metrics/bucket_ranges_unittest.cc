// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/metrics/bucket_ranges.h"

#include <stdint.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "base/sys_byteorder.h"

namespace base {
namespace {

TEST(BucketRangesTest, NormalSetup) {
  BucketRanges ranges(5);
  ASSERT_EQ(5u, ranges.size());
  ASSERT_EQ(4u, ranges.bucket_count());

  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(0, ranges.range(i));
  }
  EXPECT_EQ(0u, ranges.checksum());

  ranges.set_range(3, 100);
  EXPECT_EQ(100, ranges.range(3));
}

TEST(BucketRangesTest, Equals) {
  // Compare empty ranges.
  BucketRanges ranges1(3);
  BucketRanges ranges2(3);
  BucketRanges ranges3(5);

  EXPECT_TRUE(ranges1.Equals(&ranges2));
  EXPECT_FALSE(ranges1.Equals(&ranges3));
  EXPECT_FALSE(ranges2.Equals(&ranges3));

  // Compare full filled ranges.
  ranges1.set_range(0, 0);
  ranges1.set_range(1, 1);
  ranges1.set_range(2, 2);
  ranges1.set_checksum(100);
  ranges2.set_range(0, 0);
  ranges2.set_range(1, 1);
  ranges2.set_range(2, 2);
  ranges2.set_checksum(100);

  EXPECT_TRUE(ranges1.Equals(&ranges2));

  // Checksum does not match.
  ranges1.set_checksum(99);
  EXPECT_FALSE(ranges1.Equals(&ranges2));
  ranges1.set_checksum(100);

  // Range does not match.
  ranges1.set_range(1, 3);
  EXPECT_FALSE(ranges1.Equals(&ranges2));
}

TEST(BucketRangesTest, Checksum) {
  BucketRanges ranges(3);
  ranges.set_range(0, 0);
  ranges.set_range(1, 1);
  ranges.set_range(2, 2);

  ranges.ResetChecksum();
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  EXPECT_EQ(289217253u, ranges.checksum());
#else
  EXPECT_EQ(2767231596u, ranges.checksum());
#endif

  ranges.set_range(2, 3);
  EXPECT_FALSE(ranges.HasValidChecksum());

  ranges.ResetChecksum();
#if defined(ARCH_CPU_LITTLE_ENDIAN)
  EXPECT_EQ(2843835776u, ranges.checksum());
#else
  EXPECT_EQ(3556223738u, ranges.checksum());
#endif
  EXPECT_TRUE(ranges.HasValidChecksum());
}

}  // namespace
}  // namespace base
