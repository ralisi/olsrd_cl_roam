/*
 * OLSR plugin
 * Copyright (C) 2004 Andreas T�nnesen (andreto@olsr.org)
 *
 * This file is part of the olsrd dynamic gateway detection.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This plugin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with olsrd-unik; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * 
 * $ Id $
 *
 */

/*
 * Dynamic linked library example for UniK OLSRd
 */


#include "olsrd_plugin.h"
#include <stdio.h>


/* Data to sent to the plugin with the register_olsr_function call 
 * THIS STRUCT MUST MATCH ITS SIBLING IN plugin_loader.h IN OLSRD
 */
struct olsr_plugin_data
{
  int ipversion;
  union olsr_ip_addr *main_addr;
  int (*olsr_plugin_io)(int, void *, size_t);
};


/**
 * "Private" declarations
 */

void __attribute__ ((constructor)) 
my_init(void);

void __attribute__ ((destructor)) 
my_fini(void);

int
register_olsr_data(struct olsr_plugin_data *);

int
fetch_olsrd_data();

/*
 * Defines the version of the plugin interface that is used
 * THIS IS NOT THE VERSION OF YOUR PLUGIN!
 * Do not alter unless you know what you are doing!
 */
int plugin_interface_version;

/**
 *Constructor
 */
void
my_init()
{
  /* Print plugin info to stdout */
  printf("%s\n", MOD_DESC);
  /* Set interface version */
  plugin_interface_version = PLUGIN_INTERFACE_VERSION;

  return;
}

/**
 *Destructor
 */
void
my_fini()
{

  /* Calls the destruction function
   * olsr_plugin_exit()
   * This function should be present in your
   * sourcefile and all data destruction
   * should happen there - NOT HERE!
   */
  olsr_plugin_exit();

  return;
}


/**
 *Register needed functions and pointers
 *
 *This function should not be changed!
 *
 */
int
register_olsr_data(struct olsr_plugin_data *data)
{
  /* IPversion */
  ipversion = data->ipversion;
  /* Main address */
  main_addr = data->main_addr;

  /* Multi-purpose function */
  olsr_plugin_io = data->olsr_plugin_io;

  /* Set size of IP address */
  if(ipversion == AF_INET)
    {
      ipsize = sizeof(olsr_u32_t);
    }
  else
    {
      ipsize = sizeof(struct in6_addr);
    }

  if(!fetch_olsrd_data())
    {
      fprintf(stderr, "Could not fetch the neccessary functions from olsrd!\n");
      return 0;
    }

  /* Calls the initialization function
   * olsr_plugin_init()
   * This function should be present in your
   * sourcefile and all data initialization
   * should happen there - NOT HERE!
   */
  if(!olsr_plugin_init())
    {
      fprintf(stderr, "Could not initialize plugin!\n");
      return 0;
    }

  if(!plugin_ipc_init())
    {
      fprintf(stderr, "Could not initialize plugin IPC!\n");
      return 0;
    }

  return 1;

}



int
fetch_olsrd_data()
{
  int retval = 1;

  
  /* Neighbor table */
  if(!olsr_plugin_io(GETD__NEIGHBORTABLE, 
		     &neighbortable, 
		     sizeof(neighbortable)))
  {
    neighbortable = NULL;
    retval = 0;
  }
  
  /* Two hop neighbor table */
  if(!olsr_plugin_io(GETD__TWO_HOP_NEIGHBORTABLE, 
		     &two_hop_neighbortable, 
		     sizeof(two_hop_neighbortable)))
  {
    two_hop_neighbortable = NULL;
    retval = 0;
  }

  /* Topoloy table */
  if(!olsr_plugin_io(GETD__TC_TABLE, 
		     &tc_table, 
		     sizeof(tc_table)))
  {
    tc_table = NULL;
    retval = 0;
  }

  /* HNA table */
  if(!olsr_plugin_io(GETD__HNA_SET, 
		     &hna_set, 
		     sizeof(hna_set)))
  {
    hna_set = NULL;
    retval = 0;
  }

  /* Olsr debug output function */
  if(!olsr_plugin_io(GETF__OLSR_PRINTF, 
		     &olsr_printf, 
		     sizeof(olsr_printf)))
  {
    olsr_printf = NULL;
    retval = 0;
  }

  /* Olsr malloc wrapper */
  if(!olsr_plugin_io(GETF__OLSR_MALLOC, 
		     &olsr_malloc, 
		     sizeof(olsr_malloc)))
  {
    olsr_malloc = NULL;
    retval = 0;
  }

  /* "ProcessChanges" event registration */
  if(!olsr_plugin_io(GETF__REGISTER_PCF, 
		     &register_pcf, 
		     sizeof(register_pcf)))
  {
    register_pcf = NULL;
    retval = 0;
  }



  /* Add socket to OLSR select function */
  if(!olsr_plugin_io(GETF__ADD_OLSR_SOCKET, &add_olsr_socket, sizeof(add_olsr_socket)))
  {
    add_olsr_socket = NULL;
    retval = 0;
  }

  /* Remove socket from OLSR select function */
  if(!olsr_plugin_io(GETF__REMOVE_OLSR_SOCKET, &remove_olsr_socket, sizeof(remove_olsr_socket)))
  {
    remove_olsr_socket = NULL;
    retval = 0;
  }

  return retval;

}
