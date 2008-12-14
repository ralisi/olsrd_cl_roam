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

#include "tc_set.h"
#include "link_set.h"
#include "lq_plugin.h"
#include "olsr_spf.h"
#include "lq_packet.h"
#include "olsr.h"
#include "lq_plugin_etx_ff.h"
#include "parser.h"
#include "mid_set.h"
#include "scheduler.h"

#define LQ_PLUGIN_RELEVANT_COSTCHANGE_FF 16

static void lq_etxff_initialize(void);

static olsr_linkcost lq_etxff_calc_link_entry_cost(struct link_entry *);
static olsr_linkcost lq_etxff_calc_lq_hello_neighbor_cost(
    struct lq_hello_neighbor *);
static olsr_linkcost lq_etxff_calc_tc_mpr_addr_cost(struct tc_mpr_addr *);
static olsr_linkcost lq_etxff_calc_tc_edge_entry_cost(struct tc_edge_entry *);

static bool lq_etxff_is_relevant_costchange(olsr_linkcost c1, olsr_linkcost c2);

static olsr_linkcost lq_etxff_packet_loss_handler(struct link_entry *, bool);

static void lq_etxff_memorize_foreign_hello(struct link_entry *,
    struct lq_hello_neighbor *);
static void lq_etxff_copy_link_entry_lq_into_tc_mpr_addr(
    struct tc_mpr_addr *target, struct link_entry *source);
static void lq_etxff_copy_link_entry_lq_into_tc_edge_entry(
    struct tc_edge_entry *target, struct link_entry *source);
static void lq_etxff_copy_link_lq_into_neighbor(struct lq_hello_neighbor *target,
    struct link_entry *source);

static void lq_etxff_clear_link_entry(struct link_entry *);

static int lq_etxff_serialize_hello_lq(unsigned char *buff,
    struct lq_hello_neighbor *lq);
static int lq_etxff_serialize_tc_lq(unsigned char *buff, struct tc_mpr_addr *lq);
static void lq_etxff_deserialize_hello_lq(uint8_t const ** curr,
    struct lq_hello_neighbor *lq);
static void lq_etxff_deserialize_tc_lq(uint8_t const ** curr,
    struct tc_edge_entry *lq);

static char *lq_etxff_print_link_entry_lq(struct link_entry *entry, char separator,
    struct lqtextbuffer *buffer);
static char *lq_etxff_print_tc_edge_entry_lq(struct tc_edge_entry *ptr,
    char separator, struct lqtextbuffer * buffer);
static char *lq_etxff_print_cost(olsr_linkcost cost, struct lqtextbuffer * buffer);

static struct olsr_cookie_info *default_lq_ff_timer_cookie = NULL;

/* etx lq plugin (freifunk fpm version) settings */
struct lq_handler lq_etxff_handler = {
  &lq_etxff_initialize,

  &lq_etxff_calc_link_entry_cost,
  &lq_etxff_calc_lq_hello_neighbor_cost,
  &lq_etxff_calc_tc_mpr_addr_cost,
  &lq_etxff_calc_tc_edge_entry_cost,

  &lq_etxff_is_relevant_costchange,

  &lq_etxff_packet_loss_handler,

  &lq_etxff_memorize_foreign_hello,
  &lq_etxff_copy_link_entry_lq_into_tc_mpr_addr,
  &lq_etxff_copy_link_entry_lq_into_tc_edge_entry,
  &lq_etxff_copy_link_lq_into_neighbor,

  &lq_etxff_clear_link_entry,
  NULL,
  NULL,
  NULL,

  &lq_etxff_serialize_hello_lq,
  &lq_etxff_serialize_tc_lq,
  &lq_etxff_deserialize_hello_lq,
  &lq_etxff_deserialize_tc_lq,

  &lq_etxff_print_link_entry_lq,
  &lq_etxff_print_tc_edge_entry_lq,
  &lq_etxff_print_cost,

  sizeof(struct lq_etxff_tc_edge),
  sizeof(struct lq_etxff_tc_mpr_addr),
  sizeof(struct lq_etxff_lq_hello_neighbor),
  sizeof(struct lq_etxff_link_entry),

  LQ_HELLO_MESSAGE,
  LQ_TC_MESSAGE
};

static void lq_etxff_packet_parser(struct olsr *olsr, struct interface *in_if,
    union olsr_ip_addr *from_addr) {
  const union olsr_ip_addr *main_addr;
  struct lq_etxff_link_entry *lnk;
  uint32_t seq_diff;

  /* Find main address */
  main_addr = olsr_lookup_main_addr_by_alias(from_addr);

  /* Loopup link entry */
  lnk = (struct lq_etxff_link_entry *) lookup_link_entry(from_addr, main_addr,
      in_if);
  if (lnk == NULL) {
    return;
  }

  if (lnk->last_seq_nr > olsr->olsr_seqno) {
    seq_diff = (uint32_t) olsr->olsr_seqno + 65536 - lnk->last_seq_nr;
  } else {
    seq_diff = olsr->olsr_seqno - lnk->last_seq_nr;
  }

  /* Jump in sequence numbers ? */
  if (seq_diff > 256) {
    seq_diff = 1;
  }

  lnk->received[lnk->activePtr]++;
  lnk->lost[lnk->activePtr] += (seq_diff - 1);

  lnk->last_seq_nr = olsr->olsr_seqno;
}

static void lq_etxff_timer(void __attribute__((unused)) *context) {
  struct link_entry *link;
  OLSR_FOR_ALL_LINK_ENTRIES(link)
{    struct lq_etxff_link_entry *lq_link;
    uint32_t ratio;
    uint16_t i, received, lost;

#if !defined(NODEBUG) && defined(DEBUG)
    struct ipaddr_str buf;
    struct lqtextbuffer lqbuffer;
#endif

    lq_link = (struct lq_etxff_link_entry *)link;

#if !defined(NODEBUG) && defined(DEBUG)
    OLSR_PRINTF(3, "LQ-FF new entry for %s: rec: %d lost: %d",
        olsr_ip_to_string(&buf, &link->neighbor_iface_addr),
        lq_link->received[lq_link->activePtr], lq_link->lost[lq_link->activePtr]);
#endif

    lq_link = (struct lq_etxff_link_entry *)link;

    received = 0;
    lost = 0;

    /* enlarge window if still in quickstart phase */
    if (lq_link->windowSize < LQ_FF_WINDOW) {
      lq_link->windowSize++;
    }
    for (i=0; i < lq_link->windowSize; i++) {
      received += lq_link->received[i];
      lost += lq_link->lost[i];
    }

#if !defined(NODEBUG) && defined(DEBUG)
    OLSR_PRINTF(3, " total-rec: %d total-lost: %d", received, lost);
#endif
    /* calculate link quality */
    if (received + lost == 0) {
      lq_link->lq.valueLq = 0;
    }
    else {
      // start with link-loss-factor
      ratio = link->loss_link_multiplier;

      // calculate received/(received + loss) factor
      ratio = ratio * received;
      ratio = ratio / (received + lost);
      ratio = (ratio * 255) >> 16;

      lq_link->lq.valueLq = (uint8_t)(ratio);
    }
    link->linkcost = lq_etxff_calc_link_entry_cost(link);

#if !defined(NODEBUG) && defined(DEBUG)
    OLSR_PRINTF(3, " linkcost: %s\n", lq_etxff_print_cost(link->linkcost, &lqbuffer));
#endif

    // shift buffer
    lq_link->activePtr = (lq_link->activePtr + 1) % LQ_FF_WINDOW;
    lq_link->lost[lq_link->activePtr] = 0;
    lq_link->received[lq_link->activePtr] = 0;
  }OLSR_FOR_ALL_LINK_ENTRIES_END(link);
}

static void lq_etxff_initialize(void) {
  /* Some cookies for stats keeping */
  olsr_packetparser_add_function(&lq_etxff_packet_parser);
  default_lq_ff_timer_cookie = olsr_alloc_cookie("Default Freifunk LQ",
      OLSR_COOKIE_TYPE_TIMER);
  olsr_start_timer(1000, 0, OLSR_TIMER_PERIODIC, &lq_etxff_timer, NULL,
      default_lq_ff_timer_cookie->ci_id);
}

static olsr_linkcost lq_etxff_calc_linkcost(struct lq_etxff_linkquality *lq) {
  olsr_linkcost cost;

  if (lq->valueLq < (unsigned int) (255 * MINIMAL_USEFUL_LQ) || lq->valueNlq
      < (unsigned int) (255 * MINIMAL_USEFUL_LQ)) {
    return LINK_COST_BROKEN;
  }

  cost = 65536 * lq->valueLq / 255 * lq->valueNlq / 255;

  if (cost > LINK_COST_BROKEN)
    return LINK_COST_BROKEN;
  if (cost == 0)
    return 1;
  return cost;
}

static olsr_linkcost lq_etxff_calc_link_entry_cost(struct link_entry *link) {
  struct lq_etxff_link_entry *lq_link = (struct lq_etxff_link_entry *) link;

  return lq_etxff_calc_linkcost(&lq_link->lq);
}

static olsr_linkcost lq_etxff_calc_lq_hello_neighbor_cost(
    struct lq_hello_neighbor *neigh) {
  struct lq_etxff_lq_hello_neighbor *lq_neigh =
      (struct lq_etxff_lq_hello_neighbor *) neigh;

  return lq_etxff_calc_linkcost(&lq_neigh->lq);
}

static olsr_linkcost lq_etxff_calc_tc_mpr_addr_cost(struct tc_mpr_addr *mpr) {
  struct lq_etxff_tc_mpr_addr *lq_mpr = (struct lq_etxff_tc_mpr_addr *) mpr;

  return lq_etxff_calc_linkcost(&lq_mpr->lq);
}

static olsr_linkcost lq_etxff_calc_tc_edge_entry_cost(struct tc_edge_entry *edge) {
  struct lq_etxff_tc_edge *lq_edge = (struct lq_etxff_tc_edge *) edge;

  return lq_etxff_calc_linkcost(&lq_edge->lq);
}

static bool lq_etxff_is_relevant_costchange(olsr_linkcost c1, olsr_linkcost c2) {
  if (c1 > c2) {
    return c2 - c1 > LQ_PLUGIN_RELEVANT_COSTCHANGE_FF;
  }
  return c1 - c2 > LQ_PLUGIN_RELEVANT_COSTCHANGE_FF;
}

static olsr_linkcost lq_etxff_packet_loss_handler(struct link_entry *link,
    bool loss __attribute__((unused))) {
  return link->linkcost;
}

static void lq_etxff_memorize_foreign_hello(struct link_entry *target,
    struct lq_hello_neighbor *source) {
  struct lq_etxff_link_entry *lq_target = (struct lq_etxff_link_entry *) target;
  struct lq_etxff_lq_hello_neighbor *lq_source =
      (struct lq_etxff_lq_hello_neighbor *) source;

  if (source) {
    lq_target->lq.valueNlq = lq_source->lq.valueLq;
  } else {
    lq_target->lq.valueNlq = 0;
  }

}

static void lq_etxff_copy_link_entry_lq_into_tc_mpr_addr(
    struct tc_mpr_addr *target, struct link_entry *source) {
  struct lq_etxff_tc_mpr_addr *lq_target = (struct lq_etxff_tc_mpr_addr *) target;
  struct lq_etxff_link_entry *lq_source = (struct lq_etxff_link_entry *) source;

  lq_target->lq = lq_source->lq;
}

static void lq_etxff_copy_link_entry_lq_into_tc_edge_entry(
    struct tc_edge_entry *target, struct link_entry *source) {
  struct lq_etxff_tc_edge *lq_target = (struct lq_etxff_tc_edge *) target;
  struct lq_etxff_link_entry *lq_source = (struct lq_etxff_link_entry *) source;

  lq_target->lq = lq_source->lq;
}

static void lq_etxff_copy_link_lq_into_neighbor(struct lq_hello_neighbor *target,
    struct link_entry *source) {
  struct lq_etxff_lq_hello_neighbor *lq_target =
      (struct lq_etxff_lq_hello_neighbor *) target;
  struct lq_etxff_link_entry *lq_source = (struct lq_etxff_link_entry *) source;

  lq_target->lq = lq_source->lq;
}

static void lq_etxff_clear_link_entry(struct link_entry *link) {
  struct lq_etxff_link_entry *lq_link = (struct lq_etxff_link_entry *) link;
  int i;

  lq_link->windowSize = LQ_FF_QUICKSTART_INIT;
  for (i = 0; i < LQ_FF_WINDOW; i++) {
    lq_link->lost[i] = 3;
  }
}

static int lq_etxff_serialize_hello_lq(unsigned char *buff,
    struct lq_hello_neighbor *neigh) {
  struct lq_etxff_lq_hello_neighbor *lq_neigh =
      (struct lq_etxff_lq_hello_neighbor *) neigh;

  buff[0] = (unsigned char) lq_neigh->lq.valueLq;
  buff[1] = (unsigned char) lq_neigh->lq.valueNlq;
  buff[2] = (unsigned char) (0);
  buff[3] = (unsigned char) (0);

  return 4;
}
static int lq_etxff_serialize_tc_lq(unsigned char *buff, struct tc_mpr_addr *mpr) {
  struct lq_etxff_tc_mpr_addr *lq_mpr = (struct lq_etxff_tc_mpr_addr *) mpr;

  buff[0] = (unsigned char) lq_mpr->lq.valueLq;
  buff[1] = (unsigned char) lq_mpr->lq.valueNlq;
  buff[2] = (unsigned char) (0);
  buff[3] = (unsigned char) (0);

  return 4;
}

static void lq_etxff_deserialize_hello_lq(uint8_t const ** curr,
    struct lq_hello_neighbor *neigh) {
  struct lq_etxff_lq_hello_neighbor *lq_neigh =
      (struct lq_etxff_lq_hello_neighbor *) neigh;

  pkt_get_u8(curr, &lq_neigh->lq.valueLq);
  pkt_get_u8(curr, &lq_neigh->lq.valueNlq);
  pkt_ignore_u16(curr);

}
static void lq_etxff_deserialize_tc_lq(uint8_t const ** curr,
    struct tc_edge_entry *edge) {
  struct lq_etxff_tc_edge *lq_edge = (struct lq_etxff_tc_edge *) edge;

  pkt_get_u8(curr, &lq_edge->lq.valueLq);
  pkt_get_u8(curr, &lq_edge->lq.valueNlq);
  pkt_ignore_u16(curr);
}

static char *lq_etxff_print_lq(struct lq_etxff_linkquality *lq, char separator,
    struct lqtextbuffer *buffer) {
  int i = 0;

  if (lq->valueLq == 255) {
  	strcpy(buffer->buf, "1.000");
  	i += 5;
  }
  else {
    i = sprintf(buffer->buf, "0.%03d", (lq->valueLq * 1000)/255);
  }
  buffer->buf[i++] = separator;

  if (lq->valueNlq == 255) {
    strcpy(&buffer->buf[i], "1.000");
  }
  else {
    sprintf(&buffer->buf[i], "0.%03d", (lq->valueNlq * 1000) / 255);
  }
  return buffer->buf;
}

static char *lq_etxff_print_link_entry_lq(struct link_entry *link, char separator,
    struct lqtextbuffer *buffer) {
  struct lq_etxff_link_entry *lq_link = (struct lq_etxff_link_entry *) link;

  return lq_etxff_print_lq(&lq_link->lq, separator, buffer);
}

static char *lq_etxff_print_tc_edge_entry_lq(struct tc_edge_entry *edge,
    char separator, struct lqtextbuffer * buffer) {
  struct lq_etxff_tc_edge *lq_edge = (struct lq_etxff_tc_edge *) edge;

  return lq_etxff_print_lq(&lq_edge->lq, separator, buffer);
}

static char *lq_etxff_print_cost(olsr_linkcost cost, struct lqtextbuffer * buffer) {
  // must calculate
  uint32_t roundDown = cost >> 16;
  uint32_t fraction = ((cost & 0xffff) * 1000) >> 16;

  sprintf(buffer->buf, "%u.%03u", roundDown, fraction);
  return buffer->buf;
}

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
