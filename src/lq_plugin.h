
/*
 * The olsr.org Optimized Link-State Routing daemon(olsrd)
 * Copyright (c) 2008 Henning Rogge <rogge@fgan.de>
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

#ifndef LQPLUGIN_H_
#define LQPLUGIN_H_

#include "tc_set.h"
#include "link_set.h"
#include "olsr_spf.h"
#include "lq_packet.h"
#include "common/avl.h"

#define LINK_COST_BROKEN (1<<22)
#define ROUTE_COST_BROKEN (0xffffffff)
#define ZERO_ROUTE_COST 0

#define MINIMAL_USEFUL_LQ 0.1
#define LQ_PLUGIN_RELEVANT_COSTCHANGE 16

#define LQ_QUICKSTART_STEPS 12
#define LQ_QUICKSTART_AGING 0.25

struct lqtextbuffer {
  char buf[16];
};

struct lq_handler {
  void (*initialize)(void);
  void (*deinitialize)(void);

  olsr_linkcost (*calc_link_entry_cost) (struct link_entry *);
  olsr_linkcost (*calc_lq_hello_neighbor_cost) (struct lq_hello_neighbor *);
  olsr_linkcost (*calc_tc_mpr_addr_cost) (struct tc_mpr_addr *);
  olsr_linkcost (*calc_tc_edge_entry_cost) (struct tc_edge_entry *);

  bool (*is_relevant_costchange) (olsr_linkcost c1, olsr_linkcost c2);

  olsr_linkcost (*packet_loss_handler) (struct link_entry *, bool);

  void (*memorize_foreign_hello) (struct link_entry *, struct lq_hello_neighbor *);
  void (*copy_link_entry_lq_into_tc_mpr_addr) (struct tc_mpr_addr *, struct link_entry *);
  void (*copy_link_entry_lq_into_tc_edge_entry) (struct tc_edge_entry *, struct link_entry *);
  void (*copy_link_lq_into_neighbor) (struct lq_hello_neighbor *, struct link_entry *);

  void (*clear_link_entry) (struct link_entry *);
  void (*clear_lq_hello_neighbor) (struct lq_hello_neighbor *);
  void (*clear_tc_mpr_addr) (struct tc_mpr_addr *);
  void (*clear_tc_edge_entry) (struct tc_edge_entry *);

  int (*serialize_hello_lq) (unsigned char *, struct lq_hello_neighbor *);
  int (*serialize_tc_lq) (unsigned char *, struct tc_mpr_addr *);
  void (*deserialize_hello_lq) (uint8_t const **, struct lq_hello_neighbor *);
  void (*deserialize_tc_lq) (uint8_t const ** , struct tc_edge_entry *);

  char *(*print_link_entry_lq) (struct link_entry *, char , struct lqtextbuffer *);
  char *(*print_tc_edge_entry_lq) (struct tc_edge_entry *, char , struct lqtextbuffer *);
  char *(*print_cost) (olsr_linkcost cost, struct lqtextbuffer *);

  size_t size_tc_edge;
  size_t size_tc_mpr_addr;
  size_t size_lq_hello_neighbor;
  size_t size_link_entry;

  uint8_t messageid_hello;
  uint8_t messageid_tc;
};

struct lq_handler_node {
  struct avl_node node;
  struct lq_handler *handler;
  char name[0];
};

AVLNODE2STRUCT(lq_handler_tree2lq_handler_node, struct lq_handler_node, node);

#define OLSR_FOR_ALL_LQ_HANDLERS(lq) \
{ \
  struct avl_node *lq_tree_node, *next_lq_tree_node; \
  for (lq_tree_node = avl_walk_first(&lq_handler_tree); \
    lq_tree_node; lq_tree_node = next_lq_tree_node) { \
    next_lq_tree_node = avl_walk_next(lq_tree_node); \
    lq = lq_handler_tree2lq_handler_node(lq_tree_node);
#define OLSR_FOR_ALL_LQ_HANDLERS_END(lq) }}

void init_lq_handler_tree(void);

void activate_lq_handler(void);
void register_lq_handler(struct lq_handler *, const char *);
void deactivate_lq_handler(void);

olsr_linkcost olsr_calc_tc_cost(struct tc_edge_entry *);
bool olsr_is_relevant_costchange(olsr_linkcost c1, olsr_linkcost c2);

int olsr_serialize_hello_lq_pair(unsigned char *, struct lq_hello_neighbor *);
void olsr_deserialize_hello_lq_pair(const uint8_t **, struct lq_hello_neighbor *);
int olsr_serialize_tc_lq_pair(unsigned char *, struct tc_mpr_addr *);
void olsr_deserialize_tc_lq_pair(const uint8_t **, struct tc_edge_entry *);

void olsr_update_packet_loss_worker(struct link_entry *, bool);
void olsr_memorize_foreign_hello_lq(struct link_entry *, struct lq_hello_neighbor *);

const char *EXPORT(get_link_entry_text)(struct link_entry *,
                                        char, struct lqtextbuffer *);
const char *EXPORT(get_tc_edge_entry_text)(struct tc_edge_entry *,
				   char, struct lqtextbuffer *);
const char *EXPORT(get_linkcost_text)(olsr_linkcost, bool, struct lqtextbuffer *);

void olsr_copy_hello_lq(struct lq_hello_neighbor *, struct link_entry *);
void olsr_copylq_link_entry_2_tc_mpr_addr(struct tc_mpr_addr *,
                                          struct link_entry *);
void olsr_copylq_link_entry_2_tc_edge_entry(struct tc_edge_entry *,
					    struct link_entry *);
#if 0
void olsr_clear_tc_lq(struct tc_mpr_addr *);
#endif

struct tc_edge_entry *olsr_malloc_tc_edge_entry(void);
struct tc_mpr_addr *olsr_malloc_tc_mpr_addr(void);
struct lq_hello_neighbor *olsr_malloc_lq_hello_neighbor(void);
struct link_entry *olsr_malloc_link_entry(void);

void olsr_free_link_entry(struct link_entry *);
void olsr_free_lq_hello_neighbor(struct lq_hello_neighbor *);
void olsr_free_tc_edge_entry(struct tc_edge_entry *);
void olsr_free_tc_mpr_addr(struct tc_mpr_addr *);

uint8_t olsr_get_Hello_MessageId(void);
uint8_t olsr_get_TC_MessageId(void);

/* Externals. */
extern struct lq_handler *active_lq_handler;


#endif /*LQPLUGIN_H_ */

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
