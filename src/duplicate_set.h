
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

#ifndef DUPLICATE_SET_H_
#define DUPLICATE_SET_H_

#include "defs.h"
#include "olsr.h"
#include "olsr_time.h"
#include "common/avl.h"

#define DUPLICATE_CLEANUP_INTERVAL 30000
#define DUPLICATE_CLEANUP_JITTER 25
#define DUPLICATE_VTIME 120000

struct dup_entry {
  struct avl_node avl;
  union olsr_ip_addr ip;
  uint16_t seqnr;
  uint16_t too_low_counter;
  uint32_t array;
  uint32_t valid_until;
};

enum duplicate_status {
  RESET_SEQNO_OLSR_MESSAGE,
  NEW_OLSR_MESSAGE,
  OLD_OLSR_MESSAGE,
  DUPLICATE_OLSR_MESSAGE,
  TOO_OLD_OLSR_MESSAGE
};

AVLNODE2STRUCT(duptree2dupentry, struct dup_entry, avl);

int EXPORT(olsr_seqno_diff)(uint16_t reference, uint16_t other);
void olsr_init_duplicate_set(void);
bool olsr_is_duplicate_message(union olsr_message *m, bool forward_set, enum duplicate_status *status);
void olsr_print_duplicate_table(void);
void olsr_flush_duplicate_entries(void);

#define OLSR_FOR_ALL_DUP_ENTRIES(dup, forward) \
{ \
  struct avl_node *dup_tree_node, *next_dup_tree_node; \
  for (dup_tree_node = avl_walk_first(forward ? &forward_set : &processing_set); \
    dup_tree_node; dup_tree_node = next_dup_tree_node) { \
    next_dup_tree_node = avl_walk_next(dup_tree_node); \
    dup = duptree2dupentry(dup_tree_node);
#define OLSR_FOR_ALL_DUP_ENTRIES_END(dup) }}

#endif /*DUPLICATE_SET_H_ */

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
