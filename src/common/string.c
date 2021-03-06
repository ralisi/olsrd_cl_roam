
/*
 * The olsr.org Optimized Link-State Routing daemon(olsrd)
 * Copyright (c) 2008, Bernd Petrovitsch <bernd-at-firmix.at>
 * Copyright (c) 2008, Sven-Ola Tuecke <sven-ola-at-gmx.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of olsr.org, olsrd nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Visit http://www.olsr.org for more information.
 *
 * If you find this software useful feel free to make a donation
 * to the project. For more information see the website or contact
 * the copyright holders.
 *
 */

#include "common/string.h"

#include <string.h>
#include <assert.h>

const char *OLSR_YES = "yes";
const char *OLSR_NO = "no";

/*
 * A somewhat safe version of strncpy and strncat. Note, that
 * BSD/Solaris strlcpy()/strlcat() differ in implementation, while
 * the BSD compiler prints out a warning if you use plain strcpy().
 */
char *
strscpy(char *dest, const char *src, size_t size)
{
  size_t l = 0;
  assert(dest != NULL);
  assert(src != NULL);
  if (NULL != dest && NULL != src) {
    /* src does not need to be null terminated */
    if (0 < size--) {
      while (l < size && 0 != src[l])
        l++;
    }
    dest[l] = 0;
  }
  return strncpy(dest, src, l);
}

/*
 * A somewhat safe version of strncat. Note, that the
 * size parameter denotes the complete size of dest,
 * which is different from the strncat semantics.
 */
char *
strscat(char *dest, const char *src, size_t size)
{
  const size_t l = strlen(dest);
  return strscpy(dest + l, src, size > l ? size - l : 0);
}

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
