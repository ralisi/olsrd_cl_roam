/*
 * The olsr.org Optimized Link-State Routing daemon(olsrd)
 * Copyright (c) 2004, Andreas T�nnesen(andreto@olsr.org)
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
 * $Id: olsrd_plugin.h,v 1.9 2005/01/05 20:39:50 kattemat Exp $
 */

/*
 * Dynamic linked library for the olsr.org olsr daemon
 */

#ifndef _OLSRD_PLUGIN_DEFS
#define _OLSRD_PLUGIN_DEFS


#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

#include "olsr_plugin_io.h"


/*****************************************************************************
 *                               Plugin data                                 *
 *                       ALTER THIS TO YOUR OWN NEED                         *
 *****************************************************************************/

#define PLUGIN_NAME    "Httpinfo olsrd plugin"
#define PLUGIN_VERSION "0.1"
#define PLUGIN_AUTHOR   "Andreas T�nnesen"
#define MOD_DESC PLUGIN_NAME " " PLUGIN_VERSION " by " PLUGIN_AUTHOR
#define PLUGIN_INTERFACE_VERSION 2

int http_port;

/****************************************************************************
 *           Various datastructures and definitions from olsrd              *
 ****************************************************************************/

/*
 *Neighbor status
 */

#define NOT_SYM               0
#define SYM                   1


/*
 * TYPES SECTION
 */

typedef enum
{
    OLSR_FALSE = 0,
    OLSR_TRUE
}olsr_bool;



/* types */
#include <sys/types.h>

#ifndef WIN32
typedef u_int8_t        olsr_u8_t;
typedef u_int16_t       olsr_u16_t;
typedef u_int32_t       olsr_u32_t;
typedef int8_t          olsr_8_t;
typedef int16_t         olsr_16_t;
typedef int32_t         olsr_32_t;
#else
typedef unsigned char olsr_u8_t;
typedef unsigned short olsr_u16_t;
typedef unsigned int olsr_u32_t;
typedef char olsr_8_t;
typedef short olsr_16_t;
typedef int olsr_32_t;
#endif


/*
 * VARIOUS DEFINITIONS
 */

union olsr_ip_addr
{
  olsr_u32_t v4;
  struct in6_addr v6;
};

union hna_netmask
{
  olsr_u32_t v4;
  olsr_u16_t v6;
};

/*
 * Hashing
 */

#define	HASHSIZE	32
#define	HASHMASK	(HASHSIZE - 1)

#define MAXIFS         8 /* Maximum number of interfaces (from defs.h) in uOLSRd */


struct allowed_host
{
  union olsr_ip_addr       host;
  struct allowed_host     *next;
};

struct allowed_net
{
  union olsr_ip_addr       net;
  union olsr_ip_addr       mask;
  struct allowed_net      *next;
};

struct allowed_host   *allowed_hosts;
struct allowed_net    *allowed_nets;


/*
 * Neighbor structures
 */

/* One hop neighbor */

struct neighbor_2_list_entry 
{
  struct neighbor_2_entry      *neighbor_2;
  struct timeval	       neighbor_2_timer;
  struct neighbor_2_list_entry *next;
  struct neighbor_2_list_entry *prev;
};

struct neighbor_entry
{
  union olsr_ip_addr           neighbor_main_addr;
  olsr_u8_t                    status;
  olsr_u8_t                    willingness;
  olsr_bool                    is_mpr;
  olsr_bool                    was_mpr; /* Used to detect changes in MPR */
  olsr_bool                    skip;
  int                          neighbor_2_nocov;
  int                          linkcount;
  struct neighbor_2_list_entry neighbor_2_list; 
  struct neighbor_entry        *next;
  struct neighbor_entry        *prev;
};


/* Two hop neighbor */


struct neighbor_list_entry 
{
  struct	neighbor_entry *neighbor;
  double path_link_quality;
  double saved_path_link_quality;
  struct	neighbor_list_entry *next;
  struct	neighbor_list_entry *prev;
};

struct neighbor_2_entry
{
  union olsr_ip_addr         neighbor_2_addr;
  olsr_u8_t      	     mpr_covered_count;    /*used in mpr calculation*/
  olsr_u8_t      	     processed;            /*used in mpr calculation*/
  olsr_16_t                  neighbor_2_pointer;   /* Neighbor count */
  struct neighbor_list_entry neighbor_2_nblist; 
  struct neighbor_2_entry    *prev;
  struct neighbor_2_entry    *next;
};


/* Link entry */
struct link_entry
{
  union olsr_ip_addr local_iface_addr;
  union olsr_ip_addr neighbor_iface_addr;
  struct timeval SYM_time;
  struct timeval ASYM_time;
  struct timeval time;
  struct neighbor_entry *neighbor;

  /*
   *Hysteresis
   */
  float L_link_quality;
  int L_link_pending;
  struct timeval L_LOST_LINK_time;
  struct timeval hello_timeout; /* When we should receive a new HELLO */
  double last_htime;
  olsr_u16_t olsr_seqno;
  olsr_bool olsr_seqno_valid;

  /*
   * packet loss
   */

  double loss_hello_int;
  struct timeval loss_timeout;

  olsr_u16_t loss_seqno;
  int loss_seqno_valid;
  int loss_missed_hellos;

  int lost_packets;
  int total_packets;

  double loss_link_quality;

  int loss_window_size;
  int loss_index;

  unsigned char loss_bitmap[16];

  double neigh_link_quality;

  double saved_loss_link_quality;
  double saved_neigh_link_quality;

  /*
   * Spy
   */
  olsr_u8_t                    spy_activated;

  struct link_entry *next;
};


/* Topology entry */

struct topo_dst
{
  union olsr_ip_addr T_dest_addr;
  struct timeval T_time;
  olsr_u16_t T_seq;
  struct topo_dst *next;
  struct topo_dst *prev;
  double link_quality;
  double inverse_link_quality;
  double saved_link_quality;
  double saved_inverse_link_quality;
};

struct tc_entry
{
  union olsr_ip_addr T_last_addr;
  struct topo_dst destinations;
  struct tc_entry *next;
  struct tc_entry *prev;
};

/* HNA */

/* hna_netmask declared in packet.h */

struct hna_net
{
  union olsr_ip_addr A_network_addr;
  union hna_netmask  A_netmask;
  struct timeval     A_time;
  struct hna_net     *next;
  struct hna_net     *prev;
};

struct hna_entry
{
  union olsr_ip_addr A_gateway_addr;
  struct hna_net     networks;
  struct hna_entry   *next;
  struct hna_entry   *prev;
};

/*
 * Generic address list elem
 */
struct addresses 
{
  union olsr_ip_addr address;
  struct addresses *next;
};

/* MID set */
struct mid_entry
{
  union olsr_ip_addr main_addr;
  struct addresses *aliases;
  struct mid_entry *prev;
  struct mid_entry *next;
  struct timeval ass_timer;  
};

/* Routing table */
struct rt_entry
{
  union olsr_ip_addr    rt_dst;
  union olsr_ip_addr    rt_router;
  union hna_netmask     rt_mask;
  olsr_u8_t  	        rt_flags; 
  olsr_u16_t 	        rt_metric;
  struct interface      *rt_if;
  struct rt_entry       *prev;
  struct rt_entry       *next;
};


/* The lists */

struct neighbor_entry *neighbortable;
struct neighbor_2_entry *two_hop_neighbortable;
struct link_entry *link_set;
struct tc_entry *tc_table;
struct hna_entry *hna_set;
struct mid_entry *mid_set;
struct rt_entry *host_routes;
struct rt_entry *hna_routes;


/* Buffer for olsr_ip_to_string */

char ipv6_buf[100]; /* buffer for IPv6 inet_htop */

/* MPR set entry */

struct mpr_selector
{
  union olsr_ip_addr MS_main_addr;
  struct timeval MS_time;
  struct mpr_selector *next;
  struct mpr_selector *prev;
};

/****************************************************************************
 *                Function pointers to functions in olsrd                   *
 *              These allow direct access to olsrd functions                *
 ****************************************************************************/

/* The multi-purpose funtion. All other functions are fetched trough this */
int (*olsr_plugin_io)(int, void *, size_t);

/* Add a socket to the main olsrd select loop */
void (*add_olsr_socket)(int, void(*)(int));

/* Lookup MPR entry */
struct mpr_selector *(*olsr_lookup_mprs_set)(union olsr_ip_addr *);


/* olsrd printf wrapper */
int (*olsr_printf)(int, char *, ...);

/* olsrd malloc wrapper */
void *(*olsr_malloc)(size_t, const char *);


/****************************************************************************
 *                             Data from olsrd                              *
 *           NOTE THAT POINTERS POINT TO THE DATA USED BY OLSRD!            *
 *               NEVER ALTER DATA POINTED TO BY THESE POINTERS              * 
 *                   UNLESS YOU KNOW WHAT YOU ARE DOING!!!                  *
 ****************************************************************************/

/* These two are set automatically by olsrd at load time */
int                ipversion;  /* IPversion in use */
union olsr_ip_addr *main_addr; /* Main address */


size_t             ipsize;     /* Size of the ipadresses used */

/****************************************************************************
 *                Functions that the plugin MUST provide                    *
 ****************************************************************************/


/* Initialization function */
int
olsr_plugin_init(void);

/* IPC initialization function */
int
plugin_ipc_init(void);

int
register_olsr_param(char *, char *);

/* Destructor function */
void
olsr_plugin_exit(void);

/* Mulitpurpose funtion */
int
plugin_io(int, void *, size_t);

int 
get_plugin_interface_version(void);

#endif
