/* arm_features.c -- ARM processor features detection.
 *
 * Copyright 2018 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the Chromium source repository LICENSE file.
 */
#include "arm_features.h"

#include "zutil.h"

#include <pthread.h>
#include <stdint.h>
#include <machine/armreg.h>
#include <sys/types.h>

int ZLIB_INTERNAL arm_cpu_enable_crc32 = 0;
int ZLIB_INTERNAL arm_cpu_enable_pmull = 0;

static pthread_once_t cpu_check_inited_once = PTHREAD_ONCE_INIT;

static void init_arm_features(void)
{
#if defined (__aarch64__)
    uint64_t id_aa64isar0;

    id_aa64isar0 = READ_SPECIALREG(ID_AA64ISAR0_EL1);
    if (ID_AA64ISAR0_AES(id_aa64isar0) == ID_AA64ISAR0_AES_PMULL)
        arm_cpu_enable_pmull = 1;
    if (ID_AA64ISAR0_CRC32(id_aa64isar0) == ID_AA64ISAR0_CRC32_BASE)
        arm_cpu_enable_crc32 = 1;
#endif
}

void ZLIB_INTERNAL arm_check_features(void)
{
    pthread_once(&cpu_check_inited_once, init_arm_features);
}
