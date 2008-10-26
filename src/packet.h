/*
 * The olsr.org Optimized Link-State Routing daemon(olsrd)
 * Copyright (c) 2004, Andreas Tønnesen(andreto@olsr.org)
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

#ifndef _OLSR_PACKET
#define _OLSR_PACKET

#include "olsr_protocol.h"
#include "interfaces.h"
#include "mantissa.h"

struct hello_neighbor
{
  olsr_u8_t             status;
  olsr_u8_t             link;
  union olsr_ip_addr    main_address;
  union olsr_ip_addr    address;
  struct hello_neighbor *next;
  olsr_linkcost         cost;
  olsr_u32_t            linkquality[0];
};

struct hello_message
{
  olsr_reltime           vtime;
  olsr_reltime           htime;
  union olsr_ip_addr     source_addr;
  olsr_u16_t             packet_seq_number;
  olsr_u8_t              hop_count;
  olsr_u8_t              ttl;
  olsr_u8_t              willingness;
  struct hello_neighbor  *neighbors;
  
};

struct tc_mpr_addr
{
  union olsr_ip_addr address;
  struct tc_mpr_addr *next;
  olsr_u32_t         linkquality[0];
};

struct tc_message
{
  olsr_reltime        vtime;
  union olsr_ip_addr  source_addr;
  union olsr_ip_addr  originator;
  olsr_u16_t          packet_seq_number;
  olsr_u8_t           hop_count;
  olsr_u8_t           ttl;
  olsr_u16_t          ansn;
  struct tc_mpr_addr  *multipoint_relay_selector_address;
};


struct unknown_message
{
  olsr_u16_t         seqno;
  union olsr_ip_addr originator;
  olsr_u8_t          type;
};


void
olsr_free_hello_packet(struct hello_message *);

int
olsr_build_hello_packet(struct hello_message *, struct interface *);

#endif
