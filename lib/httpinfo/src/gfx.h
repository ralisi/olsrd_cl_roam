/*
 * The olsr.org Optimized Link-State Routing daemon(olsrd)
 * Copyright (c) 2004-2009, the olsr.org team - see HISTORY file
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

/*
 * Dynamic linked library for the olsr.org olsr daemon
 */

#ifndef _GFX_H
#define _GFX_H




static unsigned char favicon_ico[] = {
  0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x68, 0x05, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x28, 0x00,
  0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x31, 0x73,
  0xde, 0x00, 0x42, 0xff, 0xff, 0x00, 0x52, 0x52, 0x52, 0x00, 0xad, 0xad,
  0xad, 0x00, 0x21, 0x21, 0x9c, 0x00, 0x9c, 0xef, 0xff, 0x00, 0x39, 0xb5,
  0xff, 0x00, 0x29, 0x29, 0x29, 0x00, 0xd6, 0xd6, 0xd6, 0x00, 0x7b, 0x7b,
  0x7b, 0x00, 0x29, 0x31, 0xce, 0x00, 0xc6, 0xff, 0xff, 0x00, 0x94, 0x94,
  0x94, 0x00, 0xc6, 0xbd, 0xbd, 0x00, 0x29, 0x52, 0xce, 0x00, 0x94, 0xa5,
  0xbd, 0x00, 0x7b, 0x84, 0x9c, 0x00, 0x52, 0x63, 0x6b, 0x00, 0x29, 0xce,
  0xff, 0x00, 0x18, 0x18, 0x18, 0x00, 0xe7, 0xe7, 0xe7, 0x00, 0x39, 0x39,
  0x39, 0x00, 0x31, 0x9c, 0xf7, 0x00, 0x6b, 0x6b, 0x6b, 0x00, 0x4a, 0xe7,
  0xff, 0x00, 0x42, 0xce, 0xff, 0x00, 0x8c, 0xf7, 0xff, 0x00, 0x31, 0xb5,
  0xef, 0x00, 0xb5, 0xff, 0xff, 0x00, 0x84, 0x84, 0x84, 0x00, 0x08, 0x08,
  0x08, 0x00, 0x10, 0x10, 0x10, 0x00, 0xf7, 0xf7, 0xf7, 0x00, 0xef, 0xef,
  0xef, 0x00, 0x31, 0x31, 0x31, 0x00, 0xde, 0xde, 0xde, 0x00, 0x4a, 0x4a,
  0x4a, 0x00, 0xce, 0xce, 0xce, 0x00, 0x5a, 0x5a, 0x5a, 0x00, 0xb5, 0xb5,
  0xb5, 0x00, 0x73, 0x73, 0x73, 0x00, 0xa5, 0xa5, 0xa5, 0x00, 0x9c, 0x9c,
  0x9c, 0x00, 0x8c, 0x8c, 0x8c, 0x00, 0xc6, 0xc6, 0xc6, 0x00, 0x29, 0x4a,
  0xc6, 0x00, 0x29, 0x5a, 0xd6, 0x00, 0xe7, 0xde, 0xde, 0x00, 0xbd, 0xbd,
  0xbd, 0x00, 0xf7, 0xff, 0xff, 0x00, 0x73, 0x6b, 0x6b, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x08, 0x18, 0x02, 0x0c, 0x05, 0x25, 0x25, 0x30, 0x10, 0x2f, 0x06, 0x12,
  0x00, 0x00, 0x00, 0x00, 0x1c, 0x1c, 0x1a, 0x1b, 0x13, 0x2a, 0x34, 0x1e,
  0x0d, 0x07, 0x1d, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x2d,
  0x0e, 0x2b, 0x0e, 0x2a, 0x19, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x05, 0x32, 0x2e, 0x27, 0x2e, 0x32, 0x2b, 0x2d, 0x2a, 0x2a, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x09, 0x25, 0x00, 0x26, 0x00, 0x0b,
  0x1f, 0x2d, 0x2a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2e, 0x0a, 0x00,
  0x26, 0x16, 0x00, 0x2d, 0x00, 0x2c, 0x2d, 0x0e, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x27, 0x24, 0x23, 0x26, 0x22, 0x00, 0x0e, 0x00, 0x05, 0x04, 0x24,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x2d, 0x16, 0x22, 0x22, 0x22, 0x22, 0x16,
  0x0a, 0x32, 0x15, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22,
  0x22, 0x01, 0x22, 0x23, 0x25, 0x24, 0x21, 0x15, 0x00, 0x00, 0x00, 0x00,
  0x20, 0x17, 0x17, 0x2c, 0x32, 0x03, 0x14, 0x33, 0x09, 0x00, 0x21, 0x04,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x0a, 0x0a, 0x31, 0x0f, 0x22, 0x16,
  0x27, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x28, 0x16, 0x23,
  0x16, 0x28, 0x19, 0x22, 0x16, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x19, 0x01, 0x01, 0x01, 0x32, 0x27, 0x01, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x0a, 0x29, 0x2d, 0x0b, 0x19,
  0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x2c, 0x05, 0x1f, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
  0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0xf0, 0x0f,
  0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0xc0, 0x03,
  0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x03,
  0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0xc0, 0x07,
  0x00, 0x00, 0xe0, 0x0f, 0x00, 0x00, 0xf0, 0x1f, 0x00, 0x00, 0xfc, 0x3f,
  0x00, 0x00
};


static unsigned char logo_gif[] = {
  0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x50, 0x00, 0x50, 0x00, 0xf7, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x84, 0x10, 0x18, 0x7b, 0x10,
  0x18, 0x7b, 0x42, 0x4a, 0x84, 0x84, 0x94, 0x5a, 0x5a, 0x63, 0x7b, 0x7b,
  0x84, 0x6b, 0x6b, 0x73, 0x84, 0x84, 0x8c, 0x94, 0x94, 0x9c, 0xbd, 0xbd,
  0xc6, 0xb5, 0xb5, 0xbd, 0xde, 0xde, 0xe7, 0xc6, 0xc6, 0xce, 0xe7, 0xe7,
  0xef, 0x39, 0x42, 0x4a, 0x5a, 0x63, 0x63, 0x52, 0x5a, 0x5a, 0xd6, 0xde,
  0xde, 0xa5, 0xad, 0x73, 0xff, 0xff, 0x42, 0xff, 0xff, 0x4a, 0xff, 0xff,
  0x52, 0xff, 0xff, 0x63, 0xff, 0xff, 0x7b, 0xde, 0xde, 0x73, 0xff, 0xff,
  0x94, 0xff, 0xff, 0x9c, 0xa5, 0xa5, 0x6b, 0x7b, 0x7b, 0x52, 0xde, 0xde,
  0x9c, 0xff, 0xff, 0xb5, 0xff, 0xff, 0xc6, 0x73, 0x73, 0x5a, 0xef, 0xef,
  0xbd, 0xd6, 0xd6, 0xad, 0x7b, 0x7b, 0x63, 0x84, 0x84, 0x6b, 0xde, 0xde,
  0xb5, 0xe7, 0xe7, 0xbd, 0xef, 0xef, 0xc6, 0xf7, 0xf7, 0xce, 0xa5, 0xa5,
  0x8c, 0xad, 0xad, 0x94, 0x7b, 0x7b, 0x6b, 0xde, 0xde, 0xc6, 0xc6, 0xc6,
  0xb5, 0x73, 0x73, 0x6b, 0xf7, 0xf7, 0xe7, 0x9c, 0x9c, 0x94, 0xff, 0xff,
  0xf7, 0xff, 0xf7, 0x31, 0xff, 0xf7, 0x4a, 0xef, 0xe7, 0x6b, 0xff, 0xf7,
  0x73, 0xff, 0xef, 0x29, 0xff, 0xef, 0x39, 0xff, 0xf7, 0x8c, 0xff, 0xef,
  0x6b, 0xd6, 0xce, 0x7b, 0xff, 0xf7, 0xad, 0xff, 0xf7, 0xb5, 0xff, 0xe7,
  0x31, 0xff, 0xe7, 0x39, 0xff, 0xe7, 0x42, 0xff, 0xe7, 0x4a, 0xff, 0xef,
  0x73, 0xff, 0xef, 0x7b, 0xff, 0xf7, 0xbd, 0xf7, 0xde, 0x42, 0xe7, 0xce,
  0x42, 0xf7, 0xde, 0x4a, 0xff, 0xe7, 0x5a, 0xbd, 0xad, 0x52, 0xf7, 0xe7,
  0x8c, 0xd6, 0xce, 0x9c, 0xef, 0xce, 0x29, 0xe7, 0xce, 0x5a, 0xf7, 0xe7,
  0x94, 0xff, 0xef, 0x9c, 0xff, 0xd6, 0x21, 0xff, 0xd6, 0x31, 0xce, 0xb5,
  0x4a, 0xa5, 0x94, 0x4a, 0x9c, 0x94, 0x73, 0xff, 0xd6, 0x39, 0xf7, 0xde,
  0x84, 0xff, 0xce, 0x29, 0xf7, 0xc6, 0x31, 0xf7, 0xce, 0x4a, 0xff, 0xd6,
  0x52, 0xd6, 0xb5, 0x4a, 0xf7, 0xd6, 0x6b, 0xff, 0xe7, 0x9c, 0x73, 0x6b,
  0x52, 0xff, 0xce, 0x42, 0xff, 0xe7, 0xa5, 0xe7, 0xde, 0xc6, 0xd6, 0xbd,
  0x7b, 0xef, 0xde, 0xb5, 0xe7, 0xad, 0x29, 0xef, 0xb5, 0x31, 0xef, 0xb5,
  0x39, 0xef, 0xc6, 0x6b, 0x6b, 0x63, 0x52, 0xff, 0xb5, 0x21, 0xf7, 0xb5,
  0x31, 0xef, 0xbd, 0x5a, 0xde, 0xb5, 0x63, 0xef, 0xc6, 0x73, 0xef, 0xad,
  0x39, 0xbd, 0x94, 0x4a, 0xef, 0xc6, 0x7b, 0xff, 0xad, 0x29, 0xef, 0xa5,
  0x31, 0xff, 0xb5, 0x39, 0xf7, 0xb5, 0x4a, 0xef, 0xb5, 0x52, 0xef, 0xa5,
  0x39, 0xf7, 0xa5, 0x31, 0xbd, 0xa5, 0x84, 0xde, 0xc6, 0xa5, 0xde, 0x8c,
  0x29, 0xf7, 0x9c, 0x31, 0xef, 0x9c, 0x39, 0xd6, 0x84, 0x29, 0xe7, 0x8c,
  0x31, 0xf7, 0x9c, 0x42, 0xd6, 0x8c, 0x42, 0xc6, 0x8c, 0x52, 0xde, 0xce,
  0xbd, 0x73, 0x6b, 0x63, 0x7b, 0x73, 0x6b, 0xef, 0x8c, 0x31, 0xf7, 0x8c,
  0x31, 0xce, 0x73, 0x29, 0xe7, 0x84, 0x31, 0xde, 0x84, 0x39, 0xce, 0x94,
  0x63, 0xde, 0x7b, 0x31, 0xce, 0x7b, 0x42, 0xbd, 0xa5, 0x94, 0xde, 0x73,
  0x31, 0x7b, 0x52, 0x39, 0xb5, 0x8c, 0x73, 0xbd, 0x5a, 0x21, 0xde, 0x6b,
  0x29, 0xc6, 0x63, 0x29, 0xc6, 0x6b, 0x39, 0xef, 0x73, 0x31, 0xd6, 0x6b,
  0x31, 0xe7, 0x73, 0x39, 0xe7, 0x63, 0x29, 0x6b, 0x5a, 0x52, 0xd6, 0x5a,
  0x29, 0xc6, 0x63, 0x39, 0xce, 0x6b, 0x42, 0xb5, 0x4a, 0x21, 0x9c, 0x84,
  0x7b, 0xce, 0x52, 0x29, 0xbd, 0x42, 0x21, 0xb5, 0x42, 0x21, 0xc6, 0x5a,
  0x39, 0xd6, 0x4a, 0x29, 0xc6, 0x4a, 0x29, 0xad, 0x84, 0x7b, 0xa5, 0x31,
  0x21, 0xb5, 0x4a, 0x39, 0xb5, 0x31, 0x21, 0xad, 0x31, 0x21, 0xa5, 0x5a,
  0x52, 0xad, 0x6b, 0x63, 0x9c, 0x39, 0x31, 0xa5, 0x42, 0x39, 0xad, 0x4a,
  0x42, 0xce, 0x31, 0x29, 0x9c, 0x21, 0x21, 0x94, 0x31, 0x31, 0x8c, 0x31,
  0x31, 0x84, 0x31, 0x31, 0xa5, 0x52, 0x52, 0x94, 0x63, 0x63, 0x94, 0x73,
  0x73, 0xa5, 0x94, 0x94, 0xc6, 0xbd, 0xbd, 0xd6, 0xce, 0xce, 0xf7, 0xf7,
  0xf7, 0xef, 0xef, 0xef, 0xe7, 0xe7, 0xe7, 0xde, 0xde, 0xde, 0xd6, 0xd6,
  0xd6, 0xce, 0xce, 0xce, 0xc6, 0xc6, 0xc6, 0xbd, 0xbd, 0xbd, 0xb5, 0xb5,
  0xb5, 0xad, 0xad, 0xad, 0xa5, 0xa5, 0xa5, 0x9c, 0x9c, 0x9c, 0x94, 0x94,
  0x94, 0x8c, 0x8c, 0x8c, 0x84, 0x84, 0x84, 0x7b, 0x7b, 0x7b, 0x73, 0x73,
  0x73, 0x6b, 0x6b, 0x6b, 0x63, 0x63, 0x63, 0x5a, 0x5a, 0x5a, 0x52, 0x52,
  0x52, 0x4a, 0x4a, 0x4a, 0x42, 0x42, 0x42, 0x39, 0x39, 0x39, 0x31, 0x31,
  0x31, 0x29, 0x29, 0x29, 0x21, 0x21, 0x21, 0x18, 0x18, 0x18, 0x10, 0x10,
  0x10, 0x08, 0x08, 0x08, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x21, 0xf9, 0x04, 0x01, 0x00, 0x00, 0xd9, 0x00, 0x2c, 0x00, 0x00,
  0x00, 0x00, 0x50, 0x00, 0x50, 0x00, 0x00, 0x08, 0xff, 0x00, 0xb3, 0x09,
  0x1c, 0x48, 0xb0, 0xa0, 0xc1, 0x83, 0x08, 0x13, 0x2a, 0x5c, 0xc8, 0xb0,
  0xa1, 0xc3, 0x87, 0x10, 0x23, 0x4a, 0x9c, 0x48, 0xb1, 0xa2, 0xc5, 0x8b,
  0x18, 0x33, 0x6a, 0xdc, 0xc8, 0xb1, 0xa3, 0xc7, 0x8f, 0x20, 0x43, 0x8a,
  0x1c, 0x49, 0xb2, 0xa4, 0x49, 0x87, 0xc7, 0x96, 0x35, 0x6b, 0xe6, 0xac,
  0x19, 0x33, 0x65, 0xc7, 0x4e, 0x76, 0x2c, 0x76, 0xcc, 0x98, 0x31, 0x62,
  0x38, 0x8b, 0xd1, 0x4c, 0xc6, 0x2c, 0x5a, 0x34, 0x65, 0x32, 0x2f, 0xea,
  0x2c, 0x46, 0x4c, 0xa7, 0xcd, 0x63, 0x48, 0x8f, 0x21, 0x5b, 0xca, 0x4c,
  0x9a, 0x35, 0x67, 0x41, 0x25, 0x0e, 0xb5, 0x49, 0xf5, 0x28, 0xd2, 0x64,
  0xc8, 0x92, 0x29, 0xdb, 0xfa, 0xec, 0x1a, 0xb6, 0x65, 0x51, 0x51, 0x26,
  0x1d, 0x3b, 0x76, 0x69, 0x32, 0xad, 0xca, 0x96, 0x31, 0x63, 0xd6, 0xcc,
  0x1a, 0x80, 0x69, 0x61, 0x15, 0x22, 0x53, 0x7a, 0x8c, 0xd8, 0xaf, 0x5e,
  0xbd, 0x7e, 0x0d, 0xa3, 0xbb, 0x34, 0xeb, 0x56, 0xb5, 0x6c, 0x9b, 0x75,
  0x05, 0x80, 0x0d, 0x59, 0x5c, 0x83, 0x58, 0x93, 0x1d, 0xf3, 0x15, 0xa0,
  0xb1, 0xe3, 0x5e, 0xc6, 0x12, 0xa3, 0x4d, 0xcb, 0xcc, 0x99, 0xb3, 0x67,
  0xcf, 0xa2, 0x01, 0x20, 0x0c, 0xf4, 0xb0, 0x40, 0x65, 0x5a, 0x8f, 0xf1,
  0x72, 0x4c, 0x3a, 0xc0, 0x2e, 0x63, 0xa0, 0xb5, 0xaa, 0x75, 0xb9, 0x1a,
  0x5a, 0xb4, 0x6a, 0x9b, 0x01, 0x74, 0x8e, 0xfb, 0x72, 0x2b, 0xe3, 0xd2,
  0xa4, 0x79, 0xa1, 0x55, 0x7b, 0x0c, 0x58, 0xaf, 0x5d, 0xbb, 0x7a, 0x01,
  0x43, 0x26, 0x2d, 0x36, 0x80, 0x64, 0x71, 0x59, 0x33, 0x23, 0xd6, 0x18,
  0x38, 0xee, 0xc6, 0xc2, 0x96, 0x49, 0x07, 0xf6, 0x3c, 0xc0, 0xaf, 0x69,
  0xb1, 0xb1, 0x15, 0x8b, 0x8a, 0x2c, 0x9a, 0xda, 0x65, 0x8c, 0x9d, 0x8b,
  0xff, 0xcf, 0xad, 0xf6, 0x57, 0x69, 0xe0, 0xce, 0x7f, 0xc1, 0xde, 0x4c,
  0x2d, 0x6a, 0xb5, 0x95, 0xcd, 0x92, 0xf1, 0x42, 0xbf, 0xcb, 0x74, 0xfd,
  0xfa, 0xf3, 0x03, 0x20, 0x63, 0xee, 0xf8, 0x3e, 0x70, 0x5e, 0xf3, 0x19,
  0x63, 0x5c, 0x33, 0x32, 0x41, 0x33, 0xcd, 0x4a, 0xce, 0x20, 0xf3, 0x1f,
  0x7d, 0xce, 0xf9, 0x32, 0x0c, 0x32, 0xc2, 0x0c, 0x03, 0x4c, 0x30, 0xa3,
  0x31, 0x08, 0x20, 0x5e, 0xd7, 0x65, 0x67, 0x8c, 0x49, 0xc6, 0x60, 0x03,
  0x8d, 0x65, 0xcf, 0x20, 0xc3, 0x4b, 0x2f, 0x00, 0x02, 0x88, 0x5e, 0x2f,
  0xc7, 0xf4, 0x32, 0x8c, 0x33, 0xc6, 0x0c, 0xb3, 0x56, 0x2f, 0xf6, 0xed,
  0x72, 0x61, 0x2f, 0xbe, 0xf8, 0xf2, 0x8c, 0x71, 0xd1, 0x98, 0x44, 0x8d,
  0x35, 0x98, 0x3d, 0x03, 0x8d, 0x88, 0x78, 0x91, 0x58, 0xe2, 0x2e, 0xbf,
  0xb8, 0xc4, 0x96, 0x74, 0x3d, 0x51, 0x27, 0xe3, 0x8c, 0xbe, 0xfc, 0x02,
  0xcc, 0x8d, 0x9b, 0x61, 0x83, 0x4d, 0x4c, 0x23, 0x09, 0x48, 0x0d, 0x34,
  0xd0, 0xf8, 0x08, 0x9e, 0x2f, 0x41, 0xe2, 0x35, 0x9f, 0x2f, 0x2c, 0x59,
  0xa6, 0x56, 0x34, 0xcd, 0xf8, 0xb2, 0xe4, 0x88, 0x34, 0x3a, 0x19, 0x4c,
  0x71, 0x51, 0x62, 0x93, 0xe3, 0x48, 0xd0, 0x00, 0x70, 0x25, 0x96, 0xd0,
  0x34, 0x03, 0xcc, 0x2f, 0xbf, 0xd4, 0xc8, 0xa5, 0x97, 0xc5, 0x60, 0xa6,
  0x0c, 0x33, 0xc5, 0x08, 0xf3, 0xcb, 0x92, 0x78, 0xd5, 0xa8, 0xa6, 0x31,
  0xd7, 0xb4, 0x79, 0xcd, 0x35, 0x1b, 0x8a, 0x94, 0x68, 0x35, 0x3e, 0xb9,
  0x06, 0x8d, 0x5d, 0xc0, 0xdc, 0x99, 0x67, 0x9e, 0x0d, 0x3c, 0xb0, 0x00,
  0x03, 0x0b, 0x48, 0x80, 0xc0, 0x7c, 0x0f, 0x84, 0x4a, 0x63, 0x93, 0x13,
  0x0a, 0xa3, 0x59, 0x94, 0x8b, 0x5e, 0x43, 0x60, 0x48, 0xca, 0x60, 0x03,
  0x80, 0x35, 0xd2, 0xf8, 0xff, 0xe4, 0x13, 0x33, 0xc1, 0xd4, 0x5a, 0xa9,
  0xa5, 0x2e, 0xa0, 0x90, 0x82, 0x0a, 0x29, 0x9c, 0x40, 0x82, 0x18, 0x29,
  0xa4, 0x80, 0xc2, 0x09, 0x4d, 0x3a, 0x09, 0x8c, 0x30, 0x50, 0x2a, 0x0a,
  0xab, 0x48, 0x9a, 0x49, 0x39, 0x8d, 0x34, 0xb1, 0xfa, 0x04, 0xa1, 0x30,
  0xd4, 0xd6, 0xea, 0xc0, 0x07, 0x15, 0x64, 0x6b, 0xc1, 0xb6, 0x15, 0xd0,
  0xf0, 0x43, 0x14, 0x6c, 0x04, 0xe3, 0x40, 0x30, 0xc2, 0x1c, 0x83, 0x5d,
  0x76, 0xd8, 0x2c, 0x6a, 0x4d, 0x35, 0x8d, 0x7e, 0xe4, 0xd5, 0x66, 0xd5,
  0x40, 0x1b, 0xab, 0x34, 0xd0, 0xd4, 0x45, 0xcc, 0x30, 0xc3, 0x08, 0xb3,
  0xc0, 0x02, 0x1a, 0x64, 0xeb, 0x6f, 0x0e, 0x56, 0x60, 0xc1, 0x06, 0x30,
  0x13, 0xf4, 0x22, 0x0c, 0x9b, 0xc6, 0x11, 0x76, 0x8d, 0x35, 0x0c, 0x83,
  0xf5, 0x51, 0x87, 0x52, 0x12, 0xf6, 0xac, 0xbc, 0xd2, 0x4c, 0xb3, 0x9f,
  0x31, 0x3a, 0x11, 0x23, 0xcc, 0x04, 0x19, 0xd0, 0x90, 0x6d, 0x0e, 0x40,
  0x60, 0x51, 0x87, 0x2e, 0xbc, 0xcc, 0x00, 0xdc, 0x30, 0xd4, 0x24, 0x2c,
  0xe5, 0xc2, 0xd5, 0x54, 0xf3, 0x0c, 0x48, 0xcb, 0x10, 0x16, 0x9b, 0x35,
  0xd3, 0x4c, 0x3c, 0x0d, 0x33, 0xbf, 0x19, 0x6b, 0xa6, 0x69, 0x18, 0xd0,
  0x00, 0x32, 0x16, 0x6f, 0x0c, 0xf2, 0xc0, 0x7f, 0x5f, 0xc6, 0xd9, 0x66,
  0xba, 0xeb, 0x42, 0x2a, 0xcc, 0x47, 0x37, 0x46, 0xcc, 0x5e, 0xcd, 0xd5,
  0x2c, 0x73, 0xdf, 0x73, 0x31, 0xd4, 0x80, 0x83, 0x14, 0x72, 0xe8, 0x31,
  0x34, 0xd1, 0x24, 0xf6, 0x62, 0xb4, 0x94, 0x48, 0x57, 0x43, 0x4d, 0x35,
  0xc3, 0x78, 0x04, 0xb1, 0xab, 0xc6, 0x8d, 0xdd, 0x0c, 0x83, 0xfe, 0xd5,
  0x17, 0x80, 0x0a, 0x52, 0xa8, 0xa1, 0x35, 0x7a, 0x33, 0x06, 0x79, 0x2a,
  0xd2, 0xd6, 0x8c, 0xad, 0x6a, 0x47, 0xc7, 0x64, 0xff, 0x97, 0x30, 0x00,
  0xd1, 0xfc, 0x36, 0x1f, 0x7d, 0xa6, 0x91, 0x46, 0xc6, 0x18, 0x0d, 0xd0,
  0xcd, 0x64, 0x8d, 0xc0, 0x60, 0x97, 0xee, 0x35, 0x62, 0x57, 0xe3, 0xd6,
  0xaa, 0x19, 0xc5, 0x6c, 0x1c, 0xda, 0x51, 0x0a, 0x23, 0xe3, 0xe6, 0x84,
  0x8f, 0x47, 0x5f, 0x89, 0x85, 0x16, 0xeb, 0x0b, 0x31, 0x5e, 0xb1, 0xdc,
  0xb2, 0x35, 0xae, 0x42, 0x93, 0x51, 0x33, 0x7f, 0x27, 0xac, 0x4c, 0x70,
  0x42, 0xb2, 0x5d, 0x38, 0xdb, 0x75, 0x1b, 0x5a, 0xe9, 0x2f, 0x31, 0x9b,
  0x2e, 0x39, 0xda, 0x70, 0x59, 0x74, 0x6a, 0xeb, 0xec, 0xdd, 0xe5, 0xcb,
  0x88, 0x43, 0xfa, 0x67, 0x9f, 0x7d, 0xf3, 0xd5, 0x8e, 0xe7, 0x84, 0xe4,
  0x4e, 0x03, 0x79, 0xcb, 0xbb, 0xc7, 0x56, 0x4d, 0x45, 0x08, 0x03, 0x0f,
  0x00, 0x32, 0x34, 0x06, 0x69, 0xe2, 0xe0, 0x6c, 0x2b, 0x1e, 0xa4, 0xed,
  0xa5, 0x06, 0x93, 0x8c, 0xee, 0xa8, 0x1b, 0x67, 0xcd, 0x44, 0x29, 0x5b,
  0x0f, 0x6f, 0x9e, 0x35, 0x7a, 0x69, 0x62, 0xf7, 0x8a, 0xa3, 0x69, 0xa8,
  0x9a, 0xe4, 0x0e, 0x43, 0x8c, 0x53, 0x2d, 0x8f, 0x8d, 0x79, 0x94, 0x54,
  0x3e, 0xa4, 0x7e, 0x6c, 0xe0, 0xc1, 0x53, 0xfb, 0x84, 0xf4, 0x3e, 0xda,
  0xd5, 0x8e, 0x54, 0xb5, 0x8a, 0x10, 0xbe, 0x98, 0x91, 0x34, 0xc9, 0x79,
  0x05, 0x69, 0xd4, 0xa0, 0xc6, 0xcb, 0x22, 0xb2, 0x13, 0xca, 0xb8, 0x25,
  0x3b, 0x76, 0x11, 0xe0, 0x9e, 0x08, 0xc8, 0xbd, 0x25, 0x71, 0x6d, 0x54,
  0xc6, 0xaa, 0x9f, 0xfd, 0x88, 0x61, 0x2e, 0x6a, 0x3c, 0x8b, 0x1a, 0xc9,
  0xc0, 0x97, 0x30, 0x6c, 0x95, 0x11, 0xa3, 0x6d, 0x66, 0x1a, 0xb7, 0xfa,
  0x85, 0x33, 0xb0, 0xa3, 0x2a, 0x00, 0x15, 0x03, 0x00, 0xd0, 0x18, 0x92,
  0x30, 0xce, 0x05, 0x80, 0x66, 0xa0, 0xc8, 0x38, 0xcf, 0x88, 0x50, 0x51,
  0xff, 0x8a, 0x61, 0x8c, 0x67, 0x50, 0x03, 0x5a, 0xd3, 0x20, 0xc6, 0x47,
  0x62, 0x86, 0x36, 0x3b, 0x55, 0x2a, 0x66, 0x65, 0x12, 0x50, 0x33, 0x78,
  0x21, 0xa0, 0x67, 0xd0, 0x8d, 0x66, 0xc3, 0x18, 0xd1, 0x30, 0x7c, 0x81,
  0x0c, 0xc0, 0x01, 0x83, 0x19, 0xb2, 0xb1, 0x1f, 0x11, 0x8d, 0xc1, 0x8c,
  0x67, 0x4d, 0x23, 0x1a, 0x65, 0xf3, 0xc8, 0x0d, 0x63, 0x83, 0x8c, 0x09,
  0x01, 0x23, 0x1a, 0xd7, 0x68, 0x9f, 0x5b, 0xa8, 0x88, 0xc3, 0x12, 0xf1,
  0xe2, 0x2d, 0xbe, 0x19, 0x55, 0x17, 0xa3, 0x01, 0x8c, 0x64, 0x00, 0x60,
  0x19, 0x45, 0x39, 0x8a, 0x32, 0xcc, 0xf8, 0x8c, 0x60, 0x80, 0xe4, 0x82,
  0xd7, 0x08, 0x94, 0x30, 0x80, 0x01, 0x38, 0xf6, 0xc5, 0xa9, 0x26, 0x75,
  0x2c, 0x91, 0xd1, 0x9a, 0x11, 0x8c, 0x1a, 0xed, 0x91, 0x18, 0x29, 0xc3,
  0x18, 0x11, 0xaf, 0x12, 0x0d, 0x69, 0x50, 0x63, 0x19, 0xbf, 0x00, 0x89,
  0xd1, 0xa8, 0x91, 0xaf, 0x5a, 0x35, 0xb2, 0x46, 0x71, 0xb2, 0x49, 0x24,
  0xbd, 0x34, 0xc8, 0xcd, 0x20, 0xe3, 0x17, 0x5d, 0x04, 0x60, 0x20, 0x91,
  0xb2, 0x14, 0x03, 0x51, 0xa3, 0x7f, 0x1e, 0xf1, 0x23, 0xe0, 0x54, 0x18,
  0x8c, 0x46, 0xe6, 0xe9, 0x91, 0x02, 0x82, 0x46, 0x97, 0x0a, 0x45, 0xba,
  0x3f, 0xfa, 0x51, 0x1a, 0x37, 0x42, 0xc6, 0x18, 0x95, 0x72, 0x16, 0x23,
  0xa2, 0xd1, 0x51, 0x38, 0x8c, 0xd0, 0x0a, 0xe1, 0x88, 0xa7, 0x5f, 0xb8,
  0xe5, 0x87, 0xc2, 0xfc, 0x9e, 0x9e, 0xf6, 0xe8, 0xc7, 0x68, 0xdc, 0x30,
  0x91, 0xc6, 0x60, 0x66, 0x33, 0xdf, 0x03, 0x0c, 0x91, 0xdc, 0x08, 0x1a,
  0xf8, 0xca, 0x17, 0x18, 0x9d, 0x01, 0x4b, 0x00, 0x38, 0xc3, 0x17, 0x7d,
  0x13, 0x66, 0xfb, 0xa8, 0xa1, 0x8c, 0xa5, 0xb8, 0x65, 0x19, 0xc7, 0x1c,
  0x06, 0x6c, 0x96, 0xa1, 0xff, 0x14, 0xbf, 0x28, 0x03, 0x1a, 0xd5, 0xc0,
  0xe5, 0x47, 0x6e, 0x88, 0xce, 0x7b, 0xad, 0x90, 0x19, 0x29, 0xb3, 0xc6,
  0x96, 0xfa, 0x66, 0x1c, 0x68, 0xa4, 0x4f, 0x4e, 0xc9, 0x08, 0x86, 0x32,
  0x00, 0x20, 0x0d, 0x62, 0xe8, 0x32, 0x31, 0x7f, 0xf1, 0x89, 0x21, 0x47,
  0x72, 0x46, 0xfb, 0xf1, 0xf2, 0x76, 0xd5, 0xd4, 0x93, 0x48, 0xab, 0x59,
  0xa9, 0x04, 0x8a, 0x91, 0x26, 0xe2, 0x4c, 0x4b, 0x33, 0xa4, 0x91, 0x8c,
  0x50, 0x8e, 0xa4, 0x18, 0x15, 0x25, 0x0a, 0xb5, 0x56, 0x18, 0x0c, 0x90,
  0x56, 0x93, 0xa4, 0x21, 0x24, 0x97, 0x10, 0x8b, 0x42, 0xcb, 0xb3, 0x50,
  0xe6, 0x32, 0x4b, 0x2b, 0x89, 0x32, 0x30, 0x66, 0x50, 0x9a, 0xd6, 0x34,
  0x86, 0x77, 0xba, 0x15, 0xf3, 0x44, 0x18, 0xc8, 0x70, 0x66, 0x45, 0x35,
  0x95, 0x79, 0x86, 0x40, 0xab, 0x44, 0x94, 0x7b, 0xe5, 0xcb, 0xa8, 0x35,
  0xcd, 0x6a, 0x49, 0x13, 0xa8, 0xc0, 0xa6, 0xa6, 0x74, 0x35, 0xcf, 0x58,
  0xc6, 0x46, 0x4f, 0x92, 0x31, 0x15, 0xce, 0xf4, 0xac, 0xe4, 0xd2, 0x29,
  0xb5, 0xf0, 0xe5, 0x55, 0xb3, 0xa8, 0x46, 0x25, 0xcf, 0x70, 0x46, 0x1a,
  0x83, 0x52, 0x14, 0x9c, 0x58, 0xd5, 0xaa, 0xe9, 0xbc, 0x2a, 0x5b, 0xdb,
  0xea, 0xcf, 0xb4, 0xc0, 0xf5, 0x19, 0xed, 0x0a, 0x0a, 0x51, 0xaa, 0x3a,
  0x94, 0xc2, 0x56, 0xd5, 0xae, 0xb3, 0x34, 0xcb, 0x5f, 0x90, 0x24, 0x18,
  0x68, 0x20, 0xe7, 0x30, 0x37, 0x19, 0xa3, 0x0c, 0xfa, 0x39, 0x97, 0x9a,
  0xd8, 0x64, 0x99, 0xcc, 0xf4, 0xeb, 0x5a, 0xd6, 0xd2, 0xd8, 0x88, 0x7a,
  0x26, 0x1b, 0x0a, 0x50, 0x00, 0x32, 0x0a, 0x90, 0x80, 0x03, 0x28, 0xa3,
  0x05, 0x5b, 0xc1, 0x4a, 0x65, 0x7b, 0x4a, 0x99, 0xc0, 0x38, 0x43, 0x02,
  0xcf, 0x80, 0x00, 0x04, 0x92, 0x01, 0x0c, 0x5f, 0xff, 0x7c, 0x96, 0x05,
  0x1d, 0xa0, 0x40, 0x15, 0x4c, 0x00, 0x83, 0x08, 0x18, 0xc0, 0x00, 0xcc,
  0x58, 0x46, 0x6a, 0xfa, 0x82, 0x16, 0x04, 0x34, 0x43, 0x02, 0x2d, 0x29,
  0x44, 0x0b, 0x4a, 0xe0, 0x01, 0x13, 0xac, 0x80, 0x05, 0x2f, 0x08, 0x0b,
  0x07, 0x38, 0xa0, 0x83, 0x0c, 0x60, 0xa0, 0x02, 0x3f, 0xc8, 0x81, 0x11,
  0x8e, 0x30, 0x05, 0x2a, 0x7c, 0x41, 0x02, 0xc0, 0x15, 0x2e, 0x68, 0xfe,
  0xc2, 0x0c, 0x03, 0x88, 0xa0, 0x03, 0x4c, 0x18, 0x41, 0x08, 0xd6, 0x0b,
  0x02, 0x0e, 0x64, 0xe0, 0x02, 0xda, 0x75, 0xc2, 0x18, 0x48, 0xa2, 0x83,
  0xfa, 0xea, 0x60, 0x03, 0xf6, 0x1d, 0xc2, 0x10, 0x84, 0x20, 0x05, 0x2b,
  0x58, 0x01, 0x0c, 0x60, 0xa8, 0x03, 0x1c, 0xd2, 0xe0, 0x8c, 0xe0, 0x8a,
  0x77, 0x19, 0x08, 0x20, 0x84, 0x12, 0x6c, 0x00, 0x82, 0xf5, 0xae, 0xb7,
  0x08, 0x3e, 0xe8, 0x01, 0x14, 0x88, 0x80, 0x84, 0x26, 0x98, 0xc1, 0x0c,
  0x7b, 0x68, 0x84, 0x23, 0x40, 0x52, 0x01, 0x0c, 0xec, 0xe0, 0x06, 0x37,
  0xd0, 0xef, 0x0e, 0x92, 0x90, 0x84, 0x2e, 0x68, 0x21, 0x0b, 0x66, 0xb8,
  0xc3, 0x1d, 0xfa, 0xd0, 0x07, 0x40, 0x04, 0x42, 0x13, 0x12, 0x30, 0x12,
  0x5b, 0xd2, 0xc0, 0x85, 0x20, 0x0c, 0xe1, 0x09, 0x61, 0xc8, 0x71, 0x18,
  0xbc, 0xf0, 0x84, 0x25, 0x5c, 0x01, 0x0d, 0x67, 0x28, 0xc3, 0x1f, 0xfc,
  0x40, 0x09, 0x4f, 0xa8, 0xc2, 0x14, 0x1e, 0xe1, 0x01, 0x0d, 0x8c, 0x80,
  0x04, 0x12, 0x77, 0x21, 0x09, 0x5b, 0x70, 0x82, 0x16, 0xce, 0x60, 0x86,
  0x39, 0xcc, 0xe1, 0x0f, 0x89, 0x60, 0xc4, 0x25, 0x30, 0x81, 0x09, 0x48,
  0x40, 0x00, 0x33, 0x97, 0x91, 0xc0, 0x17, 0xde, 0x70, 0x87, 0x37, 0xd8,
  0x01, 0x0d, 0x6e, 0x88, 0x83, 0x9a, 0xdd, 0xd0, 0x06, 0x3b, 0xf4, 0xc1,
  0xce, 0x0f, 0x7e, 0x40, 0x84, 0x24, 0x44, 0x71, 0x0a, 0x01, 0x10, 0xe0,
  0x23, 0x41, 0x90, 0x42, 0x13, 0xb2, 0xc0, 0xe7, 0x33, 0x9c, 0x61, 0x0d,
  0x78, 0x98, 0x83, 0x21, 0x12, 0xf1, 0x88, 0x47, 0x6c, 0x02, 0x14, 0xa0,
  0x28, 0x05, 0x2a, 0x0a, 0x21, 0x9d, 0xad, 0x98, 0x16, 0x19, 0x82, 0x18,
  0x32, 0x9c, 0xff, 0xa0, 0x88, 0x4a, 0x33, 0x62, 0x12, 0x88, 0xa0, 0x04,
  0x25, 0x4a, 0xe1, 0x89, 0x53, 0xc4, 0x62, 0x00, 0x03, 0xb8, 0x05, 0x48,
  0xda, 0x40, 0x07, 0x3a, 0xe0, 0xe1, 0xd4, 0x7c, 0x38, 0x84, 0x21, 0x2c,
  0xf1, 0x88, 0x4c, 0x64, 0x82, 0x14, 0xb0, 0x80, 0x45, 0x2a, 0x56, 0x41,
  0x0c, 0x07, 0xb0, 0xaf, 0x7d, 0xbe, 0x70, 0x40, 0x24, 0x2a, 0xb1, 0x89,
  0x43, 0x23, 0x3a, 0xd1, 0xa5, 0x28, 0x45, 0x28, 0xe8, 0x1c, 0x8b, 0x4f,
  0xd3, 0xe2, 0x13, 0x23, 0xc9, 0xc3, 0x22, 0x04, 0xd1, 0x88, 0x46, 0x74,
  0xa2, 0x13, 0x9c, 0x18, 0xc5, 0x2b, 0xa6, 0xbd, 0x8a, 0x4f, 0xd4, 0x9a,
  0x21, 0xb8, 0x60, 0x45, 0x2d, 0x5c, 0xc1, 0xed, 0x56, 0xb8, 0x42, 0x16,
  0xe0, 0x9e, 0x05, 0x01, 0x6c, 0x21, 0x83, 0xa0, 0x96, 0x24, 0x18, 0xb9,
  0x98, 0x69, 0x2e, 0xd6, 0x5d, 0x53, 0x89, 0xa0, 0xfb, 0xac, 0x0b, 0x58,
  0xe1, 0x9d, 0x3e, 0x4b, 0xef, 0x7a, 0xdb, 0xfb, 0xde, 0xf8, 0xce, 0xb7,
  0xbe, 0xf7, 0xcd, 0xef, 0x7e, 0xfb, 0xfb, 0xdf, 0x00, 0x0f, 0xb8, 0xc0,
  0x39, 0x12, 0x10, 0x00, 0x3b
};

static unsigned char grayline_gif[] = {
  0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x01, 0x00, 0x01, 0x00, 0x80, 0x00,
  0x00, 0xcc, 0xcc, 0xcc, 0x00, 0x00, 0x00, 0x21, 0xf9, 0x04, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
  0x00, 0x02, 0x02, 0x44, 0x01, 0x00, 0x3b
};

#endif

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
