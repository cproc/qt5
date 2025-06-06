/* Copyright (c) 2016, Google Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <openssl/cpu.h>

#if defined(OPENSSL_AARCH64)

#include <openssl/arm_arch.h>

#include "internal.h"

extern uint32_t OPENSSL_armcap_P;

#include <machine/armreg.h>

#ifndef ID_AA64ISAR0_AES_VAL
#define ID_AA64ISAR0_AES_VAL ID_AA64ISAR0_AES
#endif
#ifndef ID_AA64ISAR0_AES_VAL
#define ID_AA64ISAR0_AES_VAL ID_AA64ISAR0_AES
#endif
#ifndef ID_AA64ISAR0_SHA1_VAL
#define ID_AA64ISAR0_SHA1_VAL ID_AA64ISAR0_SHA1
#endif
#ifndef ID_AA64ISAR0_SHA2_VAL
#define ID_AA64ISAR0_SHA2_VAL ID_AA64ISAR0_SHA2
#endif

void OPENSSL_cpuid_setup(void) {
  uint64_t id_aa64isar0;

  id_aa64isar0 = READ_SPECIALREG(id_aa64isar0_el1);

  OPENSSL_armcap_P |= ARMV7_NEON;

  if (ID_AA64ISAR0_AES_VAL(id_aa64isar0) >= ID_AA64ISAR0_AES_BASE) {
    OPENSSL_armcap_P |= ARMV8_AES;
  }
  if (ID_AA64ISAR0_AES_VAL(id_aa64isar0) == ID_AA64ISAR0_AES_PMULL) {
    OPENSSL_armcap_P |= ARMV8_PMULL;
  }
  if (ID_AA64ISAR0_SHA1_VAL(id_aa64isar0) == ID_AA64ISAR0_SHA1_BASE) {
    OPENSSL_armcap_P |= ARMV8_SHA1;
  }
  if(ID_AA64ISAR0_SHA2_VAL(id_aa64isar0) >= ID_AA64ISAR0_SHA2_BASE) {
    OPENSSL_armcap_P |= ARMV8_SHA256;
  }
}
#endif  // OPENSSL_AARCH64
