/*
 * OLSR ad-hoc routing table management protocol
 * Copyright (C) 2003 Andreas T�nnesen (andreto@ifi.uio.no)
 *
 * This file is part of the olsr.org OLSR daemon.
 *
 * olsr.org is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * olsr.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with olsr.org; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * 
 * $Id: neighbor_table.c,v 1.11 2004/11/08 23:25:57 tlopatic Exp $
 *
 */




#include "defs.h"
#include "two_hop_neighbor_table.h"
#include "mid_set.h"
#include "mpr.h"
#include "olsr.h"
#include "scheduler.h"
#include "link_set.h"

void
olsr_init_neighbor_table()
{
  int i;

  olsr_register_timeout_function(&olsr_time_out_neighborhood_tables);

  for(i = 0; i < HASHSIZE; i++)
    {
      neighbortable[i].next = &neighbortable[i];
      neighbortable[i].prev = &neighbortable[i];
    }
}


/**
 *Delete a two hop neighbor from a neighbors two 
 *hop neighbor list.
 *
 *@param neighbor the neighbor to delete the two hop 
 *neighbor from.
 *@param address the IP address of the two hop neighbor 
 *to delete.
 *
 *@return positive if entry deleted
 */
int
olsr_delete_neighbor_2_pointer(struct neighbor_entry *neighbor, union olsr_ip_addr *address)
{

  struct neighbor_2_list_entry *entry;
  
  entry = neighbor->neighbor_2_list.next;

  while(entry != &neighbor->neighbor_2_list)
    {
      
      if(COMP_IP(&entry->neighbor_2->neighbor_2_addr, address))
	{
	  /* Dequeue */
	  DEQUEUE_ELEM(entry);
	  /* Delete */
	  free(entry);
	  return 1;	  
	}
      entry = entry->next;      
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
 *@return a pointer to the neighbor_2_list_entry struct
 *representing the two hop neighbor if found. NULL if not found.
 */
struct neighbor_2_list_entry *
olsr_lookup_my_neighbors(struct neighbor_entry *neighbor, union olsr_ip_addr *neighbor_main_address)
{
  
  struct neighbor_2_list_entry *entry;
  
  for(entry = neighbor->neighbor_2_list.next;
      entry != &neighbor->neighbor_2_list;
      entry = entry->next)
    {
      
      if(COMP_IP(&entry->neighbor_2->neighbor_2_addr, neighbor_main_address))
	return entry;
      
    }
  return NULL;
}



/**
 *Delete a neighbr table entry.
 *
 *Remember: Deleting a neighbor entry results 
 *the deletion of its 2 hop neighbors list!!!
 *@param neighbor the neighbor entry to delete
 *
 *@return nada
 */

int
olsr_delete_neighbor_table(union olsr_ip_addr *neighbor_addr)
{  
  struct  neighbor_2_list_entry *two_hop_list, *two_hop_to_delete;
  struct  neighbor_2_entry      *two_hop_entry;
  olsr_u32_t                    hash;
  struct neighbor_entry         *entry;

  //printf("inserting neighbor\n");

  hash = olsr_hashing(neighbor_addr);

  entry = neighbortable[hash].next;

  /*
   * Find neighbor entry
   */
  while(entry != &neighbortable[hash])
    {
      if(COMP_IP(&entry->neighbor_main_addr, neighbor_addr))
	break;
      
      entry = entry->next;
    }

  if(entry == &neighbortable[hash])
    return 0;


  two_hop_list = entry->neighbor_2_list.next;

  while(two_hop_list != &entry->neighbor_2_list)
    {
      two_hop_entry = two_hop_list->neighbor_2;
      
      two_hop_entry->neighbor_2_pointer--;

      olsr_delete_neighbor_pointer(two_hop_entry, &entry->neighbor_main_addr);

      /* Delete entry if it has no more one hop neighbors pointing to it */
      if(two_hop_entry->neighbor_2_pointer < 1)
	{
	  DEQUEUE_ELEM(two_hop_entry);

	  free(two_hop_entry);
	}


      two_hop_to_delete = two_hop_list;
      two_hop_list = two_hop_list->next;
      /* Delete entry */
      free(two_hop_to_delete);
      
    }


  /* Dequeue */
  DEQUEUE_ELEM(entry);

  free(entry);

  changes_neighborhood = OLSR_TRUE;
  return 1;

}



/**
 *Insert a neighbor entry in the neighbor table
 *
 *@param main_addr the main address of the new node
 *
 *@return 0 if neighbor already exists 1 if inserted
 */
struct neighbor_entry *
olsr_insert_neighbor_table(union olsr_ip_addr *main_addr)
{
  olsr_u32_t             hash;
  struct neighbor_entry  *new_neigh;
  
  hash = olsr_hashing(main_addr);

  /* Check if entry exists */
  
  for(new_neigh = neighbortable[hash].next;
      new_neigh != &neighbortable[hash];
      new_neigh = new_neigh->next)
    {
      if(COMP_IP(&new_neigh->neighbor_main_addr, main_addr))
	return new_neigh;
    }
  
  //printf("inserting neighbor\n");
  
  new_neigh = olsr_malloc(sizeof(struct neighbor_entry), "New neighbor entry");
  
  /* Set address, willingness and status */
  COPY_IP(&new_neigh->neighbor_main_addr, main_addr);
  new_neigh->willingness = WILL_NEVER;
  new_neigh->status = NOT_SYM;

  new_neigh->neighbor_2_list.next = &new_neigh->neighbor_2_list;
  new_neigh->neighbor_2_list.prev = &new_neigh->neighbor_2_list;
  
  new_neigh->linkcount = 0;
  new_neigh->is_mpr = OLSR_FALSE;
  new_neigh->was_mpr = OLSR_FALSE;

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
struct neighbor_entry *
olsr_lookup_neighbor_table(union olsr_ip_addr *dst)
{
  struct neighbor_entry  *entry;
  olsr_u32_t             hash;
  //struct addresses       *adr;
  union olsr_ip_addr     *tmp_ip;

  /*
   *Find main address of node
   */
  if((tmp_ip = mid_lookup_main_addr(dst)) != NULL)
    dst = tmp_ip;
  
  hash = olsr_hashing(dst);

  
  //printf("\nLookup %s\n", olsr_ip_to_string(dst));
  for(entry = neighbortable[hash].next;
      entry != &neighbortable[hash];
      entry = entry->next)
    {
      //printf("Checking %s\n", olsr_ip_to_string(&neighbor_table_tmp->neighbor_main_addr));
      if(COMP_IP(&entry->neighbor_main_addr, dst))
	return entry;
      
    }
  //printf("NOPE\n\n");

  return NULL;

}


/**
 *Lookup a neighbor entry in the neighbortable based on an address.
 *
 *@param dst the IP address of the neighbor to look up
 *
 *@return a pointer to the neighbor struct registered on the given 
 *address. NULL if not found.
 */
struct neighbor_entry *
olsr_lookup_neighbor_table_alias(union olsr_ip_addr *dst)
{
  struct neighbor_entry  *entry;
  olsr_u32_t             hash;
  //struct addresses       *adr;
  
  hash = olsr_hashing(dst);

  
  //printf("\nLookup %s\n", olsr_ip_to_string(dst));
  for(entry = neighbortable[hash].next;
      entry != &neighbortable[hash];
      entry = entry->next)
    {
      //printf("Checking %s\n", olsr_ip_to_string(&neighbor_table_tmp->neighbor_main_addr));
      if(COMP_IP(&entry->neighbor_main_addr, dst))
	return entry;
      
    }
  //printf("NOPE\n\n");

  return NULL;

}



int
update_neighbor_status(struct neighbor_entry *entry, int link)
{
  struct neighbor_2_entry *two_hop_neighbor;

  /*
   * Update neighbor entry
   */
 
  if(link == SYM_LINK)
    {
      /* N_status is set to SYM */
      if(entry->status == NOT_SYM)
	{
	  
	  /* Delete posible 2 hop entry on this neighbor */
	  if((two_hop_neighbor = olsr_lookup_two_hop_neighbor_table(&entry->neighbor_main_addr))!=NULL)
	    {
	      olsr_delete_two_hop_neighbor_table(two_hop_neighbor);
	    }
  
	  changes_neighborhood = OLSR_TRUE;
	  changes_topology = OLSR_TRUE;
	  if(olsr_cnf->tc_redundancy > 1)
	    changes = OLSR_TRUE;
	}
      entry->status = SYM;
    }
  else
    {
      if(entry->status == SYM)
	{
	  changes_neighborhood = OLSR_TRUE;
	  changes_topology = OLSR_TRUE;
	  if(olsr_cnf->tc_redundancy > 1)
	    changes = OLSR_TRUE;
	}
      /* else N_status is set to NOT_SYM */
      entry->status = NOT_SYM;
      /* remove neighbor from routing list */
    }

  return entry->status;
}



/**
 *Times out the entries in the two hop neighbor table and 
 *deletes those who have exceeded their time to live since 
 *last update.
 *
 *@return nada
 */

void
olsr_time_out_two_hop_neighbors(struct neighbor_entry  *neighbor)
{
  struct  neighbor_2_list_entry *two_hop_list, *two_hop_to_delete;
  struct  neighbor_2_entry      *two_hop_entry;


  two_hop_list = neighbor->neighbor_2_list.next;

  //neighbor->neighbor_2_list = NULL;

  while(two_hop_list != &neighbor->neighbor_2_list)
    {
      if(TIMED_OUT(&two_hop_list->neighbor_2_timer))
	{
	  two_hop_entry = two_hop_list->neighbor_2;
	  two_hop_entry->neighbor_2_pointer--;
	  olsr_delete_neighbor_pointer(two_hop_entry, &neighbor->neighbor_main_addr);

	  if(two_hop_entry->neighbor_2_pointer < 1)
	    {
	      /* FIX THIS (fix what?)*/
	      DEQUEUE_ELEM(two_hop_entry);

	      free((void *)two_hop_entry);
	    }

	  two_hop_to_delete = two_hop_list;
	  two_hop_list = two_hop_list->next;

	  /* Dequeue */
	  DEQUEUE_ELEM(two_hop_to_delete);

	  free(two_hop_to_delete);

	  /* This flag is set to OLSR_TRUE to recalculate the MPR set and the routing table*/
	  changes_neighborhood = OLSR_TRUE;
	  changes_topology = OLSR_TRUE;
	  
	}
      else
	two_hop_list = two_hop_list->next;

    }
}





void
olsr_time_out_neighborhood_tables()
{
  olsr_u8_t              index;
  struct neighbor_entry  *entry;
  
  for(index=0;index<HASHSIZE;index++)
    {
      entry = neighbortable[index].next;

      while(entry != &neighbortable[index])
	{	  
	  olsr_time_out_two_hop_neighbors(entry);

	  entry = entry->next;
	}
    }
}





/**
 *Prints the registered neighbors and two hop neighbors
 *to STDOUT.
 *
 *@return nada
 */
void
olsr_print_neighbor_table()
{
  int i;
  struct neighbor_entry *neigh;
#if defined USE_LINK_QUALITY
  struct link_entry *link;
#endif
  double best_lq, inv_best_lq, total_lq;

  olsr_printf(1, "\n---------------------------------------------- NEIGHBORS\n\n");
  olsr_printf(1, "IP address       LQ     NLQ    TLQ    SYM  MPR  will\n");

  for (i = 0; i < HASHSIZE; i++)
    {
      for(neigh = neighbortable[i].next; neigh != &neighbortable[i];
	  neigh = neigh->next)
	{
#if defined USE_LINK_QUALITY
          link = olsr_neighbor_best_link(&neigh->neighbor_main_addr);
          best_lq = link->neigh_link_quality;

          link = olsr_neighbor_best_inverse_link(&neigh->neighbor_main_addr);
          inv_best_lq = link->loss_link_quality;
#else
          best_lq = 0.0;
          inv_best_lq = 0.0;
#endif

          total_lq = best_lq * inv_best_lq;

          olsr_printf(1, "%-15s  %5.3f  %5.3f  %5.3f  %s  %s  %d\n",
                      olsr_ip_to_string(&neigh->neighbor_main_addr),
                      best_lq, inv_best_lq, total_lq,
                      (neigh->status == SYM) ? "SYM" : "   ",
                      neigh->is_mpr ? "MPR" : "   ",
                      neigh->willingness);
	}
    }
}










