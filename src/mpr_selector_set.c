/*
 * OLSR ad-hoc routing table management protocol
 * Copyright (C) 2004 Andreas T�nnesen (andreto@ifi.uio.no)
 *
 * This file is part of olsr.org.
 *
 * UniK olsrd is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * UniK olsrd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with olsr.org; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include "defs.h"
#include "mpr_selector_set.h"
#include "olsr.h"
#include "scheduler.h"

/**
 *Initialize MPR selector set
 */

int
olsr_init_mprs_set()
{
  olsr_printf(5, "MPRS: Init\n");
  /* Initial values */
  mprs_count = 0;
  ansn = 0;

  olsr_register_timeout_function(&olsr_time_out_mprs_set);
  
  mprs_list.next = &mprs_list;
  mprs_list.prev = &mprs_list;
  

  return 1;
}


/**
 *Add a MPR selector to the MPR selector set
 *
 *@param add address of the MPR selector
 *@param vtime validity time for the new entry
 *
 *@return a pointer to the new entry
 */
struct mpr_selector *
olsr_add_mpr_selector(union olsr_ip_addr *addr, float vtime)
{
  struct mpr_selector *new_entry;

  olsr_printf(1, "MPRS: adding %s\n", olsr_ip_to_string(addr));

  mprs_count++;

  new_entry = olsr_malloc(sizeof(struct mpr_selector), "Add MPR selector");

  /* Fill struct */
  COPY_IP(&new_entry->MS_main_addr, addr);
  olsr_get_timestamp((olsr_u32_t) vtime*1000, &new_entry->MS_time);

  /* Queue */
  QUEUE_ELEM(mprs_list, new_entry);
  /*
  new_entry->prev = &mprs_list;
  new_entry->next = mprs_list.next;
  mprs_list.next->prev = new_entry;
  mprs_list.next = new_entry;
  */

  return new_entry;
}



/**
 *Lookup an entry in the MPR selector table
 *based on address
 *
 *@param addr the addres to check for
 *
 *@return a pointer to the entry or NULL
 */
struct mpr_selector *
olsr_lookup_mprs_set(union olsr_ip_addr *addr)
{
  struct mpr_selector *mprs;

  if(addr == NULL)
    return NULL;
  //olsr_printf(1, "MPRS: Lookup....");

  mprs = mprs_list.next;

  while(mprs != &mprs_list)
    {

      if(COMP_IP(&mprs->MS_main_addr, addr))
	{
	  //olsr_printf(1, "MATCH\n");
	  return mprs;
	}
      mprs = mprs->next;
    }
  
  //olsr_printf(1, "NO MACH\n");
  return NULL;
}


/**
 *Update a MPR selector entry or create an new
 *one if it does not exist
 *
 *@param addr the address of the MPR selector
 *@param vtime tha validity time of the entry
 *
 *@return 1 if a new entry was added 0 if not
 */
int
olsr_update_mprs_set(union olsr_ip_addr *addr, float vtime)
{
  struct mpr_selector *mprs;
  int retval;

  olsr_printf(5, "MPRS: Update %s\n", olsr_ip_to_string(addr));

  retval = 0;

  if(NULL == (mprs = olsr_lookup_mprs_set(addr)))
    {
      olsr_add_mpr_selector(addr, vtime);
      retval = 1;
      changes = UP;
    }
  else
    {
      olsr_get_timestamp((olsr_u32_t) vtime*1000, &mprs->MS_time);
    }
  return retval;
}





/**
 *Time out MPR selector entries
 *
 *@return nada
 */
void
olsr_time_out_mprs_set()
{
  struct mpr_selector *mprs, *mprs_to_delete;

  mprs = mprs_list.next;

  while(mprs != &mprs_list)
    {

      if(TIMED_OUT(&mprs->MS_time))
	{
	  /* Dequeue */
	  mprs_to_delete = mprs;
	  mprs = mprs->next;

	  olsr_printf(1, "MPRS: Timing out %s\n", olsr_ip_to_string(&mprs_to_delete->MS_main_addr));

	  DEQUEUE_ELEM(mprs_to_delete);
	  //mprs_to_delete->prev->next = mprs_to_delete->next;
	  //mprs_to_delete->next->prev = mprs_to_delete->prev;

	  mprs_count--;

	  /* Delete entry */
	  free(mprs_to_delete);
	  changes = UP;
	}
      else
	mprs = mprs->next;
    }

}



/**
 *Print the current MPR selector set to STDOUT
 */
void
olsr_print_mprs_set()
{
  struct mpr_selector *mprs;


  mprs = mprs_list.next;
  olsr_printf(1, "MPR SELECTORS: ");

  while(mprs != &mprs_list)
    {
      olsr_printf(1, "%s ", olsr_ip_to_string(&mprs->MS_main_addr));
      mprs = mprs->next;
    }
  olsr_printf(1, "\n");
}
