
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

#include "hna_set.h"
#include "ipcalc.h"
#include "defs.h"
#include "parser.h"
#include "olsr.h"
#include "scheduler.h"
#include "net_olsr.h"
#include "tc_set.h"
#include "olsr_ip_prefix_list.h"
#include "olsr_logging.h"

/* Some cookies for stats keeping */
static struct olsr_cookie_info *hna_net_timer_cookie = NULL;
static struct olsr_cookie_info *hna_net_mem_cookie = NULL;

/**
 * Initialize the HNA set
 */
void
olsr_init_hna_set(void)
{
  OLSR_INFO(LOG_HNA, "Initialize HNA set...\n");

  hna_net_timer_cookie = olsr_alloc_cookie("HNA Network", OLSR_COOKIE_TYPE_TIMER);

  hna_net_mem_cookie = olsr_alloc_cookie("hna_net", OLSR_COOKIE_TYPE_MEMORY);
  olsr_cookie_set_memory_size(hna_net_mem_cookie, sizeof(struct hna_net));
}

/**
 * Lookup a network entry in the HNA subtree.
 *
 * @param tc the HNA hookup point
 * @param prefic the prefix to look for
 *
 * @return the localized entry or NULL of not found
 */
static struct hna_net *
olsr_lookup_hna_net(struct tc_entry *tc, const struct olsr_ip_prefix *prefix)
{
  return (hna_tc_tree2hna(avl_find(&tc->hna_tree, prefix)));
}

/**
 * Adds a network entry to a HNA gateway.
 *
 * @param tc the gateway entry to add the network to
 * @param net the nework prefix to add
 * @param prefixlen the prefix length
 *
 * @return the newly created entry
 */
static struct hna_net *
olsr_add_hna_net(struct tc_entry *tc, const struct olsr_ip_prefix *prefix)
{
  /* Add the net */
  struct hna_net *new_net = olsr_cookie_malloc(hna_net_mem_cookie);

  /* Fill struct */
  new_net->hna_prefix = *prefix;

  /* Set backpointer */
  new_net->hna_tc = tc;
  olsr_lock_tc_entry(tc);

  /*
   * Insert into the per-tc hna subtree.
   */
  new_net->hna_tc_node.key = &new_net->hna_prefix;
  avl_insert(&tc->hna_tree, &new_net->hna_tc_node, AVL_DUP_NO);

  return new_net;
}

/**
 * Delete a single HNA network.
 *
 * @param hna_net the hna_net to delete.
 */
static void
olsr_delete_hna_net(struct hna_net *hna_net)
{
  struct tc_entry *tc = hna_net->hna_tc;

  /*
   * Delete the rt_path for the hna_net.
   */
  olsr_delete_routing_table(&hna_net->hna_prefix.prefix, hna_net->hna_prefix.prefix_len, &tc->addr, OLSR_RT_ORIGIN_HNA);

  /*
   * Remove from the per-tc tree.
   */
  avl_delete(&tc->hna_tree, &hna_net->hna_tc_node);

  /*
   * Unlock and free.
   */
  olsr_unlock_tc_entry(tc);
  olsr_cookie_free(hna_net_mem_cookie, hna_net);
}

/**
 * Delete all the HNA nets hanging off a tc entry.
 *
 * @param entry the tc entry holding the HNA networks.
 */
void
olsr_flush_hna_nets(struct tc_entry *tc)
{
  struct hna_net *hna_net;

  OLSR_FOR_ALL_TC_HNA_ENTRIES(tc, hna_net) {
    olsr_delete_hna_net(hna_net);
  } OLSR_FOR_ALL_TC_HNA_ENTRIES_END(tc, hna_net);
}

/**
 * Callback for the hna_net timer.
 */
static void
olsr_expire_hna_net_entry(void *context)
{
  struct hna_net *hna_net = context;
#if !defined REMOVE_LOG_DEBUG
  struct ipaddr_str buf;
  struct ipprefix_str prefixstr;
#endif

  OLSR_DEBUG(5, "HNA: timeout %s via hna-gw %s\n",
             olsr_ip_prefix_to_string(&prefixstr, &hna_net->hna_prefix), olsr_ip_to_string(&buf, &hna_net->hna_tc->addr));

  hna_net->hna_net_timer = NULL;        /* be pedandic */

  olsr_delete_hna_net(hna_net);
}

/**
 * Update a HNA entry. If it does not exist it
 * is created.
 * This is the only function that should be called
 * from outside concerning creation of HNA entries.
 *
 *@param gw address of the gateway
 *@param net address of the network
 *@param mask the netmask
 *@param vtime the validitytime of the entry
 *
 *@return nada
 */
static void
olsr_update_hna_entry(const union olsr_ip_addr *gw, const struct olsr_ip_prefix *prefix, uint32_t vtime,
    uint16_t msg_seq)
{
  struct tc_entry *tc = olsr_locate_tc_entry(gw);
  struct hna_net *net_entry = olsr_lookup_hna_net(tc, prefix);

  if (net_entry == NULL) {
    /* Need to add the net */
    net_entry = olsr_add_hna_net(tc, prefix);
    changes_hna = true;
  }

  net_entry->tc_entry_seqno = msg_seq;

  /*
   * Add the rt_path for the entry.
   */
  olsr_insert_routing_table(&net_entry->hna_prefix.prefix, net_entry->hna_prefix.prefix_len, &tc->addr, OLSR_RT_ORIGIN_HNA);

  /*
   * Start, or refresh the timer, whatever is appropriate.
   */
  olsr_set_timer(&net_entry->hna_net_timer, vtime,
                 OLSR_HNA_NET_JITTER, OLSR_TIMER_ONESHOT, &olsr_expire_hna_net_entry, net_entry, hna_net_timer_cookie);
}

/**
 * Print all HNA entries.
 *
 *@return nada
 */
void
olsr_print_hna_set(void)
{
  /* The whole function doesn't do anything else. */
#if !defined REMOVE_LOG_INFO
  struct tc_entry *tc;
  struct ipaddr_str buf;
  struct ipprefix_str prefixstr;
  struct hna_net *hna_net;

  OLSR_INFO(LOG_HNA, "\n--- %s ------------------------------------------------- HNA\n\n", olsr_wallclock_string());

  OLSR_FOR_ALL_TC_ENTRIES(tc) {
    OLSR_INFO_NH(LOG_HNA, "HNA-gw %s:\n", olsr_ip_to_string(&buf, &tc->addr));

    OLSR_FOR_ALL_TC_HNA_ENTRIES(tc, hna_net) {
      OLSR_INFO_NH(LOG_HNA, "\t%-27s\n", olsr_ip_prefix_to_string(&prefixstr, &hna_net->hna_prefix));
    } OLSR_FOR_ALL_TC_HNA_ENTRIES_END(tc, hna_net);
  } OLSR_FOR_ALL_TC_ENTRIES_END(tc);
#endif
}

static void
olsr_prune_hna_entries(struct tc_entry *tc)
{
  struct hna_net *hna_net;

  OLSR_FOR_ALL_TC_HNA_ENTRIES(tc, hna_net) {
    if (hna_net->tc_entry_seqno != tc->hna_seq) {
      olsr_delete_hna_net(hna_net);
    }
  } OLSR_FOR_ALL_TC_HNA_ENTRIES_END(tc, hna_net);
}

/**
 * Process incoming HNA message.
 * Forwards the message if that is to be done.
 */
void
olsr_input_hna(union olsr_message *msg, struct interface *in_if __attribute__ ((unused)),
    union olsr_ip_addr *from_addr, enum duplicate_status status)
{
  uint16_t msg_size, msg_seq;
  uint8_t type, ttl, msg_hops;
  uint32_t vtime;
  union olsr_ip_addr originator;
  struct tc_entry *tc;
  struct olsr_ip_prefix prefix;
  const uint8_t *curr, *curr_end;
#if !defined REMOVE_LOG_DEBUG
  struct ipaddr_str buf;
#endif

  curr = (void *)msg;

  /* We are only interested in MID message types. */
  pkt_get_u8(&curr, &type);
  if (type != HNA_MESSAGE) {
    return;
  }

  pkt_get_reltime(&curr, &vtime);
  pkt_get_u16(&curr, &msg_size);

  pkt_get_ipaddress(&curr, &originator);

  /* Copy header values */
  pkt_get_u8(&curr, &ttl);
  pkt_get_u8(&curr, &msg_hops);
  pkt_get_u16(&curr, &msg_seq);

  if (!olsr_validate_address(&originator)) {
    return;
  }

  /*
   * If the sender interface (NB: not originator) of this message
   * is not in the symmetric 1-hop neighborhood of this node, the
   * message MUST be discarded.
   */
  if (check_neighbor_link(from_addr) != SYM_LINK) {
    OLSR_DEBUG(LOG_HNA, "Received HNA from NON SYM neighbor %s\n", olsr_ip_to_string(&buf, from_addr));
    return;
  }

  tc = olsr_locate_tc_entry(&originator);
  if (status != RESET_SEQNO_OLSR_MESSAGE && olsr_seqno_diff(msg_seq, tc->mid_seq) <= 0) {
    /* this HNA is too old, discard it */
    return;
  }
  tc->hna_seq = msg_seq;

  OLSR_DEBUG(LOG_HNA, "Processing HNA from %s, seq 0x%04x\n", olsr_ip_to_string(&buf, &originator), msg_seq);

  /*
   * Now walk the list of HNA advertisements.
   */
  curr_end = (uint8_t *)msg + msg_size;

  while (curr + olsr_cnf->ipsize < curr_end) {

    pkt_get_ipaddress(&curr, &prefix.prefix);
    pkt_get_prefixlen(&curr, &prefix.prefix_len);

    if (!ip_prefix_list_find(&olsr_cnf->hna_entries, &prefix.prefix, prefix.prefix_len, olsr_cnf->ip_version)) {
      /*
       * Only update if it's not from us.
       */
      olsr_update_hna_entry(&originator, &prefix, vtime, msg_seq);
    }
  }

  /*
   * Prune the HNAs that did not get refreshed by this advertisment.
   */
  olsr_prune_hna_entries(tc);

}

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
