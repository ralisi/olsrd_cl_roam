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

#include "tc_set.h"
#include "link_set.h"
#include "olsr_spf.h"
#include "lq_packet.h"
#include "olsr.h"
#include "lq_plugin_rfc.h"

#define LQ_PLUGIN_LC_MULTIPLIER 1024

static void lq_rfc_initialize(void);
static void lq_rfc_deinitialize(void);

static olsr_linkcost lq_rfc_calc_link_entry_cost(struct link_entry *);
static olsr_linkcost lq_rfc_calc_lq_hello_neighbor_cost(
    struct lq_hello_neighbor *);
static olsr_linkcost lq_rfc_calc_tc_mpr_addr_cost(struct tc_mpr_addr *);
static olsr_linkcost lq_rfc_calc_tc_edge_entry_cost(struct tc_edge_entry *);

static bool lq_rfc_is_relevant_costchange(olsr_linkcost c1, olsr_linkcost c2);

static olsr_linkcost lq_rfc_packet_loss_handler(struct link_entry *, bool);

static void lq_rfc_memorize_foreign_hello(struct link_entry *,
    struct lq_hello_neighbor *);
static void lq_rfc_copy_link_entry_lq_into_tc_mpr_addr(
    struct tc_mpr_addr *target, struct link_entry *source);
static void lq_rfc_copy_link_entry_lq_into_tc_edge_entry(
    struct tc_edge_entry *target, struct link_entry *source);
static void lq_rfc_copy_link_lq_into_neighbor(struct lq_hello_neighbor *target,
    struct link_entry *source);

static int lq_rfc_serialize_hello_lq(unsigned char *buff,
    struct lq_hello_neighbor *lq);
static int lq_rfc_serialize_tc_lq(unsigned char *buff, struct tc_mpr_addr *lq);
static void lq_rfc_deserialize_hello_lq(uint8_t const ** curr,
    struct lq_hello_neighbor *lq);
static void lq_rfc_deserialize_tc_lq(uint8_t const ** curr,
    struct tc_edge_entry *lq);

static char *lq_rfc_print_link_entry_lq(struct link_entry *entry, char separator,
    struct lqtextbuffer *buffer);
static char *lq_rfc_print_tc_edge_entry_lq(struct tc_edge_entry *ptr,
    char separator, struct lqtextbuffer * buffer);
static char *lq_rfc_print_cost(olsr_linkcost cost, struct lqtextbuffer * buffer);

/* etx lq plugin (freifunk fpm version) settings */
struct lq_handler lq_rfc_handler = {
  &lq_rfc_initialize,
  &lq_rfc_deinitialize,

  &lq_rfc_calc_link_entry_cost,
  &lq_rfc_calc_lq_hello_neighbor_cost,
  &lq_rfc_calc_tc_mpr_addr_cost,
  &lq_rfc_calc_tc_edge_entry_cost,

  &lq_rfc_is_relevant_costchange,

  &lq_rfc_packet_loss_handler,

  &lq_rfc_memorize_foreign_hello,
  &lq_rfc_copy_link_entry_lq_into_tc_mpr_addr,
  &lq_rfc_copy_link_entry_lq_into_tc_edge_entry,
  &lq_rfc_copy_link_lq_into_neighbor,

  NULL,
  NULL,
  NULL,
  NULL,

  &lq_rfc_serialize_hello_lq,
  &lq_rfc_serialize_tc_lq,
  &lq_rfc_deserialize_hello_lq,
  &lq_rfc_deserialize_tc_lq,

  &lq_rfc_print_link_entry_lq,
  &lq_rfc_print_tc_edge_entry_lq,
  &lq_rfc_print_cost,

  sizeof(struct tc_edge_entry),
  sizeof(struct tc_mpr_addr),
  sizeof(struct lq_hello_neighbor),
  sizeof(struct lq_rfc_link_entry),

  HELLO_MESSAGE,
  TC_MESSAGE
};

static void lq_rfc_initialize(void) {
}

static void lq_rfc_deinitialize(void) {
}

static olsr_linkcost lq_rfc_calc_link_entry_cost(struct link_entry __attribute__((unused)) *link) {
  return 1;
}

static olsr_linkcost lq_rfc_calc_lq_hello_neighbor_cost(
    struct lq_hello_neighbor __attribute__((unused)) *neigh) {
  return 1;
}

static olsr_linkcost lq_rfc_calc_tc_mpr_addr_cost(struct tc_mpr_addr __attribute__((unused)) *mpr) {
  return 1;
}

static olsr_linkcost lq_rfc_calc_tc_edge_entry_cost(struct tc_edge_entry __attribute__((unused)) *edge) {
	  return 1;
}

static bool lq_rfc_is_relevant_costchange(olsr_linkcost c1, olsr_linkcost c2) {
  return c1 != c2;
}

static olsr_linkcost lq_rfc_packet_loss_handler(struct link_entry *link, bool loss) {
  struct lq_rfc_link_entry *link_entry = (struct lq_rfc_link_entry *)link;

  if (!use_hysteresis)
    return 1;

  link_entry->hysteresis *= (1 - scaling);
  if(!loss) {
    link_entry->hysteresis += scaling;
  }

  if (link_entry->active && link_entry->hysteresis < thr_low) {
    link_entry->active = false;
  }
  else if (!link_entry->active && link_entry->hysteresis > thr_high) {
    link_entry->active = true;
  }

  return link_entry->active ? 1 : LINK_COST_BROKEN;
}

static void lq_rfc_memorize_foreign_hello(struct link_entry __attribute__((unused)) *target,
    struct lq_hello_neighbor __attribute__((unused)) *source) {
}

static void lq_rfc_copy_link_entry_lq_into_tc_mpr_addr(
    struct tc_mpr_addr __attribute__((unused)) *target, struct link_entry __attribute__((unused)) *source) {
}

static void lq_rfc_copy_link_entry_lq_into_tc_edge_entry(
    struct tc_edge_entry __attribute__((unused)) *target, struct link_entry __attribute__((unused)) *source) {
}

static void lq_rfc_copy_link_lq_into_neighbor(struct lq_hello_neighbor __attribute__((unused)) *target,
    struct link_entry __attribute__((unused)) *source) {
}

static int lq_rfc_serialize_hello_lq(unsigned char __attribute__((unused)) *buff,
    struct lq_hello_neighbor __attribute__((unused)) *neigh) {
  return 0;
}
static int lq_rfc_serialize_tc_lq(unsigned char __attribute__((unused)) *buff, struct tc_mpr_addr __attribute__((unused)) *mpr) {
  return 0;
}

static void lq_rfc_deserialize_hello_lq(uint8_t const __attribute__((unused)) ** curr,
    struct lq_hello_neighbor __attribute__((unused)) *neigh) {
}

static void lq_rfc_deserialize_tc_lq(uint8_t const __attribute__((unused)) ** curr,
    struct tc_edge_entry __attribute__((unused)) *edge) {
}

static char *lq_rfc_print_link_entry_lq(struct link_entry __attribute__((unused)) *link, char __attribute__((unused)) separator,
    struct lqtextbuffer *buffer) {
    strncpy(buffer->buf, "", 0);
    return buffer->buf;
}

static char *lq_rfc_print_tc_edge_entry_lq(struct tc_edge_entry __attribute__((unused)) *edge,
    char __attribute__((unused)) separator, struct lqtextbuffer * buffer) {
    strncpy(buffer->buf, "", 0);
    return buffer->buf;
}

static char *lq_rfc_print_cost(olsr_linkcost cost, struct lqtextbuffer * buffer) {
    sprintf(buffer->buf, "%d", cost);
    return buffer->buf;
}

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
