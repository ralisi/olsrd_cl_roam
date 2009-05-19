
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

#include "ipcalc.h"
#include "defs.h"
#include "two_hop_neighbor_table.h"
#include "mid_set.h"
#include "mpr.h"
#include "neighbor_table.h"
#include "olsr.h"
#include "scheduler.h"
#include "link_set.h"
#include "mpr_selector_set.h"
#include "net_olsr.h"
#include "olsr_logging.h"

#include <stdlib.h>

/* Root of the one hop neighbor database */
struct avl_tree nbr_tree;
struct nbr_entry neighbortable[HASHSIZE];

/* Some cookies for stats keeping */
struct olsr_cookie_info *nbr2_list_timer_cookie = NULL;

void
olsr_init_neighbor_table(void)
{
  OLSR_INFO(LOG_NEIGHTABLE, "Initialize neighbor table...\n");
  avl_init(&nbr_tree, avl_comp_default);

  nbr2_list_timer_cookie = olsr_alloc_cookie("2-Hop Neighbor List", OLSR_COOKIE_TYPE_TIMER);
}

/**
 * Unlink, delete and free a nbr2_list entry.
 */
static void
olsr_del_nbr2_list(struct nbr2_list_entry *nbr2_list)
{
  struct neighbor_2_entry *nbr2;

  nbr2 = nbr2_list->neighbor_2;

  if (nbr2->neighbor_2_pointer < 1) {
    DEQUEUE_ELEM(nbr2);
    free(nbr2);
  }

  /*
   * Kill running timers.
   */
  olsr_stop_timer(nbr2_list->nbr2_list_timer);
  nbr2_list->nbr2_list_timer = NULL;

  /* Dequeue */
  DEQUEUE_ELEM(nbr2_list);

  free(nbr2_list);

  /* Set flags to recalculate the MPR set and the routing table */
  changes_neighborhood = true;
  changes_topology = true;
}

/**
 * Delete a two hop neighbor from a neighbors two hop neighbor list.
 *
 * @param neighbor the neighbor to delete the two hop neighbor from.
 * @param address the IP address of the two hop neighbor to delete.
 *
 * @return positive if entry deleted
 */
int
olsr_delete_nbr2_list_entry(struct nbr_entry *neighbor, struct neighbor_2_entry *neigh2)
{
  struct nbr2_list_entry *nbr2_list;

  nbr2_list = neighbor->neighbor_2_list.next;

  while (nbr2_list != &neighbor->neighbor_2_list) {
    if (nbr2_list->neighbor_2 == neigh2) {
      olsr_del_nbr2_list(nbr2_list);
      return 1;
    }
    nbr2_list = nbr2_list->next;
  }
  return 0;
}


/**
 *Check if a two hop neighbor is reachable via a given
 *neighbor.
 *
 *@param neighbor neighbor-entry to check via
 *@param neighbor_main_address the addres of the two hop neighbor
 *to find.
 *
 *@return a pointer to the nbr2_list_entry struct
 *representing the two hop neighbor if found. NULL if not found.
 */
struct nbr2_list_entry *
olsr_lookup_nbr2_list_entry(struct nbr_entry *nbr,
                            const union olsr_ip_addr *addr)
{
  struct avl_node *node;

  node = avl_find(&nbr->nbr2_list_tree, addr);
  if (node) {
    return nbr2_list_node_to_nbr2_list(node);
  }
  return NULL;
}  


/**
 *Delete a neighbor table entry.
 *
 *Remember: Deleting a neighbor entry results
 *the deletion of its 2 hop neighbors list!!!
 *@param neighbor the neighbor entry to delete
 *
 *@return nada
 */

int
olsr_delete_nbr_entry(const union olsr_ip_addr *neighbor_addr)
{
  struct nbr2_list_entry *two_hop_list, *two_hop_to_delete;
  uint32_t hash;
  struct nbr_entry *entry;

#if !defined REMOVE_LOG_DEBUG
  struct ipaddr_str buf;
#endif
  OLSR_DEBUG(LOG_NEIGHTABLE, "delete neighbor: %s\n", olsr_ip_to_string(&buf, neighbor_addr));

  hash = olsr_ip_hashing(neighbor_addr);

  entry = neighbortable[hash].next;

  /*
   * Find neighbor entry
   */
  while (entry != &neighbortable[hash]) {
    if (olsr_ipcmp(&entry->neighbor_main_addr, neighbor_addr) == 0)
      break;

    entry = entry->next;
  }

  if (entry == &neighbortable[hash])
    return 0;


  two_hop_list = entry->neighbor_2_list.next;

  while (two_hop_list != &entry->neighbor_2_list) {
    two_hop_to_delete = two_hop_list;
    two_hop_list = two_hop_list->next;

    two_hop_to_delete->neighbor_2->neighbor_2_pointer--;
    olsr_delete_neighbor_pointer(two_hop_to_delete->neighbor_2, entry);

    olsr_del_nbr2_list(two_hop_to_delete);
  }


  /* Dequeue */
  DEQUEUE_ELEM(entry);

  free(entry);

  changes_neighborhood = true;
  return 1;

}



/**
 *Insert a neighbor entry in the neighbor table
 *
 *@param main_addr the main address of the new node
 *
 *@return 0 if neighbor already exists 1 if inserted
 */
struct nbr_entry *
olsr_add_nbr_entry(const union olsr_ip_addr *main_addr)
{
  uint32_t hash;
  struct nbr_entry *new_neigh;
#if !defined REMOVE_LOG_DEBUG
  struct ipaddr_str buf;
#endif

  hash = olsr_ip_hashing(main_addr);

  /* Check if entry exists */

  for (new_neigh = neighbortable[hash].next; new_neigh != &neighbortable[hash]; new_neigh = new_neigh->next) {
    if (olsr_ipcmp(&new_neigh->neighbor_main_addr, main_addr) == 0)
      return new_neigh;
  }

  OLSR_DEBUG(LOG_NEIGHTABLE, "delete neighbor: %s\n", olsr_ip_to_string(&buf, main_addr));

  new_neigh = olsr_malloc(sizeof(struct nbr_entry), "New neighbor entry");

  /* Set address, willingness and status */
  new_neigh->neighbor_main_addr = *main_addr;
  new_neigh->willingness = WILL_NEVER;
  new_neigh->status = NOT_SYM;

  new_neigh->neighbor_2_list.next = &new_neigh->neighbor_2_list;
  new_neigh->neighbor_2_list.prev = &new_neigh->neighbor_2_list;

  new_neigh->linkcount = 0;
  new_neigh->is_mpr = false;
  new_neigh->was_mpr = false;

  /* Queue */
  QUEUE_ELEM(neighbortable[hash], new_neigh);

  return new_neigh;
}



/**
 *Lookup a neighbor entry in the neighbortable based on an address.
 *
 *@param dst the IP address of the neighbor to look up
 *
 *@return a pointer to the neighbor struct registered on the given
 *address. NULL if not found.
 */
struct nbr_entry *
olsr_lookup_nbr_entry(const union olsr_ip_addr *dst)
{
  /*
   *Find main address of node
   */
  union olsr_ip_addr *tmp_ip = olsr_lookup_main_addr_by_alias(dst);
  if (tmp_ip != NULL)
    dst = tmp_ip;
  return olsr_lookup_nbr_entry_alias(dst);
}


/**
 *Lookup a neighbor entry in the neighbortable based on an address.
 *
 *@param dst the IP address of the neighbor to look up
 *
 *@return a pointer to the neighbor struct registered on the given
 *address. NULL if not found.
 */
struct nbr_entry *
olsr_lookup_nbr_entry_alias(const union olsr_ip_addr *dst)
{
  struct nbr_entry *entry;
  uint32_t hash = olsr_ip_hashing(dst);

  for (entry = neighbortable[hash].next; entry != &neighbortable[hash]; entry = entry->next) {
    if (olsr_ipcmp(&entry->neighbor_main_addr, dst) == 0)
      return entry;
  }

  return NULL;

}



int
olsr_update_nbr_status(struct nbr_entry *entry, int lnk)
{
  /*
   * Update neighbor entry
   */

  if (lnk == SYM_LINK) {
    /* N_status is set to SYM */
    if (entry->status == NOT_SYM) {
      struct neighbor_2_entry *two_hop_neighbor;

      /* Delete posible 2 hop entry on this neighbor */
      if ((two_hop_neighbor = olsr_lookup_two_hop_neighbor_table(&entry->neighbor_main_addr)) != NULL) {
        olsr_delete_two_hop_neighbor_table(two_hop_neighbor);
      }

      changes_neighborhood = true;
      changes_topology = true;
      if (olsr_cnf->tc_redundancy > 1)
        signal_link_changes(true);
    }
    entry->status = SYM;
  } else {
    if (entry->status == SYM) {
      changes_neighborhood = true;
      changes_topology = true;
      if (olsr_cnf->tc_redundancy > 1)
        signal_link_changes(true);
    }
    /* else N_status is set to NOT_SYM */
    entry->status = NOT_SYM;
    /* remove neighbor from routing list */
  }

  return entry->status;
}


/**
 * Callback for the nbr2_list timer.
 */
void
olsr_expire_nbr2_list(void *context)
{
  struct nbr2_list_entry *nbr2_list;
  struct nbr_entry *nbr;
  struct neighbor_2_entry *nbr2;

  nbr2_list = (struct nbr2_list_entry *)context;
  nbr2_list->nbr2_list_timer = NULL;

  nbr = nbr2_list->nbr2_nbr;
  nbr2 = nbr2_list->neighbor_2;

  nbr2->neighbor_2_pointer--;
  olsr_delete_neighbor_pointer(nbr2, nbr);

  olsr_del_nbr2_list(nbr2_list);
}


/**
 *Prints the registered neighbors and two hop neighbors
 *to STDOUT.
 *
 *@return nada
 */
void
olsr_print_neighbor_table(void)
{
#if !defined REMOVE_LOG_INFO
  /* The whole function doesn't do anything else. */
  const int ipwidth = olsr_cnf->ip_version == AF_INET ? 15 : 39;
  int idx;
  OLSR_INFO(LOG_NEIGHTABLE, "\n--- %s ------------------------------------------------ NEIGHBORS\n\n"
            "%*s  LQ    SYM   MPR   MPRS  will\n", olsr_wallclock_string(), ipwidth, "IP address");

  for (idx = 0; idx < HASHSIZE; idx++) {
    struct nbr_entry *neigh;
    for (neigh = neighbortable[idx].next; neigh != &neighbortable[idx]; neigh = neigh->next) {
      struct link_entry *lnk = get_best_link_to_neighbor(&neigh->neighbor_main_addr);
      if (lnk) {
        struct ipaddr_str buf;
        OLSR_INFO_NH(LOG_NEIGHTABLE, "%-*s  %s  %s  %s  %d\n",
                     ipwidth, olsr_ip_to_string(&buf, &neigh->neighbor_main_addr),
                     neigh->status == SYM ? "YES " : "NO  ",
                     neigh->is_mpr ? "YES " : "NO  ",
                     olsr_lookup_mprs_set(&neigh->neighbor_main_addr) == NULL ? "NO  " : "YES ", neigh->willingness);
      }
    }
  }
#endif
}

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
