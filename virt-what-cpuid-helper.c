/* virt-what-cpuid-helper: Are we running inside KVM or Xen HVM?
 * Copyright (C) 2008-2019 Red Hat Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* This program was suggested by Gleb Natapov and written by Paolo
 * Bonzini, with a few modifications by Richard W.M. Jones.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#if defined(__i386__) || defined(__x86_64__)

/* Known x86 hypervisor signatures.  Note that if you add a new test
 * to virt-what.in you may need to update this list.  Note the
 * signature is always 12 bytes long, plus we add \0 to the end to
 * make it 13 bytes.
 */
static int
known_signature (const char *sig)
{
  return
    strcmp (sig, "bhyve bhyve ") == 0 ||
    memcmp (sig, "KVMKVMKVM\0\0\0", 12) == 0 ||
    strcmp (sig, "LKVMLKVMLKVM") == 0 ||
    strcmp (sig, "Microsoft Hv") == 0 ||
    strcmp (sig, "OpenBSDVMM58") == 0 ||
    strcmp (sig, "TCGTCGTCGTCG") == 0 ||
    strcmp (sig, "VMwareVMware") == 0 ||
    strcmp (sig, "XenVMMXenVMM") == 0 ||
    0;
}

/* Copied from the Linux kernel definition in
 * arch/x86/include/asm/processor.h
 */
static inline void
cpuid (uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
  asm volatile ("cpuid"
                : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
                : "0" (*eax), "2" (*ecx)
                : "memory");
}

static uint32_t
cpuid_leaf (uint32_t eax, char *sig)
{
  uint32_t *sig32 = (uint32_t *) sig;

  cpuid (&eax, &sig32[0], &sig32[1], &sig32[2]);
  sig[12] = 0; /* \0-terminate the string to make string comparison possible */
  return eax;
}

static void
cpu_sig (void)
{
  char sig[13];
  const uint32_t base = 0x40000000;
  uint32_t leaf;

  /* Most hypervisors only have information in leaf 0x40000000.
   *
   * Some hypervisors have "Viridian [HyperV] extensions", and those
   * must appear in slot 0x40000000, but they will also have the true
   * hypervisor in a higher slot.
   *
   * CPUID is supposed to return the maximum leaf offset in %eax, but
   * this is not reliable.  Instead we check the returned signatures
   * against a known list (the others will be empty or garbage) and
   * only print the ones we know about.  This is OK because if we add
   * a new test in virt-what we can update the list.
   *
   * By searching backwards we only print the highest entry, thus
   * ignoring Viridian for Xen (and Nutanix).  If we ever encounter a
   * hypervisor that has more than 2 entries we may need to revisit
   * this.
   */
  for (leaf = base + 0xff00; leaf >= base; leaf -= 0x100) {
    memset (sig, 0, sizeof sig);
    cpuid_leaf (leaf, sig);
    if (known_signature (sig)) {
      puts (sig);
      break;
    }
  }
}

#else /* !i386, !x86_64 */

static void
cpu_sig (void)
{
  /* nothing for other architectures */
}

#endif

int
main()
{
  cpu_sig ();
  return 0;
}
