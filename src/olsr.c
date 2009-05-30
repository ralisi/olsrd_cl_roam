
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

/**
 * All these functions are global
 */

#include "defs.h"
#include "olsr.h"
#include "link_set.h"
#include "tc_set.h"
#include "duplicate_set.h"
#include "mid_set.h"
#include "lq_mpr.h"
#include "olsr_spf.h"
#include "scheduler.h"
#include "apm.h"
#include "misc.h"
#include "neighbor_table.h"
#include "log.h"
#include "lq_packet.h"
#include "common/avl.h"
#include "net_olsr.h"
#include "lq_plugin.h"
#include "olsr_logging.h"

#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>

static void olsr_update_willingness(void *);
static void olsr_trigger_forced_update(void *);

bool changes_topology;
bool changes_neighborhood;
bool changes_hna;
bool changes_force;

/**
 * Process changes functions
 */

struct pcf {
  int (*function) (int, int, int);
  struct pcf *next;
};

static struct pcf *pcf_list;

static uint16_t message_seqno;

/**
 *Initialize the message sequence number as a random value
 */
void
init_msg_seqno(void)
{
  message_seqno = random() & 0xFFFF;
  OLSR_DEBUG(LOG_MAIN, "Settings initial message sequence number to %u\n", message_seqno);
}

/**
 * Get and increment the message sequence number
 *
 *@return the seqno
 */
uint16_t
get_msg_seqno(void)
{
  return message_seqno++;
}


void
register_pcf(int (*f) (int, int, int))
{
  struct pcf *new_pcf;

  OLSR_DEBUG(LOG_MAIN, "Registering pcf function\n");

  new_pcf = olsr_malloc(sizeof(struct pcf), "New PCF");

  new_pcf->function = f;
  new_pcf->next = pcf_list;
  pcf_list = new_pcf;

}


/**
 *Process changes in neighborhood or/and topology.
 *Re-calculates the neighborhood/topology if there
 *are any updates - then calls the right functions to
 *update the routing table.
 *@return 0
 */
void
olsr_process_changes(void)
{
  struct pcf *tmp_pc_list;

  if (changes_neighborhood)
    OLSR_DEBUG(LOG_MAIN, "CHANGES IN NEIGHBORHOOD\n");
  if (changes_topology)
    OLSR_DEBUG(LOG_MAIN, "CHANGES IN TOPOLOGY\n");
  if (changes_hna)
    OLSR_DEBUG(LOG_MAIN, "CHANGES IN HNA\n");

  if (!changes_force && 0 >= olsr_cnf->lq_dlimit)
    return;

  if (!changes_neighborhood && !changes_topology && !changes_hna)
    return;

  if (olsr_cnf->log_target_stderr && olsr_cnf->clear_screen && isatty(STDOUT_FILENO)) {
    clear_console();
    printf("       *** %s (%s on %s) ***\n", olsrd_version, build_date, build_host);
  }

  if (changes_neighborhood) {
    olsr_calculate_lq_mpr();
  }

  /* calculate the routing table */
  if (changes_neighborhood || changes_topology || changes_hna) {
    olsr_calculate_routing_table();
  }

  olsr_print_link_set();
  olsr_print_neighbor_table();
  olsr_print_tc_table();
  olsr_print_mid_set();
  olsr_print_duplicate_table();
  olsr_print_hna_set();

  for (tmp_pc_list = pcf_list; tmp_pc_list != NULL; tmp_pc_list = tmp_pc_list->next) {
    tmp_pc_list->function(changes_neighborhood, changes_topology, changes_hna);
  }

  changes_neighborhood = false;
  changes_topology = false;
  changes_hna = false;
  changes_force = false;
}

/*
 * Callback for the periodic route calculation.
 */
static void
olsr_trigger_forced_update(void *unused __attribute__ ((unused)))
{

  changes_force = true;
  changes_neighborhood = true;
  changes_topology = true;
  changes_hna = true;

  olsr_process_changes();
}

/**
 *Initialize all the tables used(neighbor,
 *topology, MID,  HNA, MPR, dup).
 *Also initalizes other variables
 */
void
olsr_init_tables(void)
{
  /* Some cookies for stats keeping */
  static struct olsr_cookie_info *periodic_spf_timer_cookie = NULL;

  changes_topology = false;
  changes_neighborhood = false;
  changes_hna = false;

  /* Initialize link set */
  olsr_init_link_set();

  /* Initialize duplicate table */
  olsr_init_duplicate_set();

  /* Initialize neighbor table */
  olsr_init_neighbor_table();

  /* Initialize routing table */
  olsr_init_routing_table();

  /* Initialize topology */
  olsr_init_tc();

  /* Initialize MID set */
  olsr_init_mid_set();

  /* Initialize HNA set */
  olsr_init_hna_set();

  /* Start periodic SPF and RIB recalculation */
  if (olsr_cnf->lq_dinter > 0.0) {
    periodic_spf_timer_cookie = olsr_alloc_cookie("Periodic SPF", OLSR_COOKIE_TYPE_TIMER);
    olsr_start_timer((unsigned int)(olsr_cnf->lq_dinter * MSEC_PER_SEC), 5,
                     OLSR_TIMER_PERIODIC, &olsr_trigger_forced_update, NULL, periodic_spf_timer_cookie);
  }
}

/**
 *Check if a message is to be forwarded and forward
 *it if necessary.
 *
 *@param m the OLSR message recieved
 *
 *@returns positive if forwarded
 */
int
olsr_forward_message(union olsr_message *m, struct interface *in_if, union olsr_ip_addr *from_addr)
{
  union olsr_ip_addr *src;
  struct nbr_entry *neighbor;
  int msgsize;
  struct interface *ifn;
#if !defined REMOVE_LOG_DEBUG
  struct ipaddr_str buf;
#endif

  /*
   * Sven-Ola: We should not flood the mesh with overdue messages. Because
   * of a bug in parser.c:parse_packet, we have a lot of messages because
   * all older olsrd's have lq_fish enabled.
   */
  if (AF_INET == olsr_cnf->ip_version) {
    if (m->v4.ttl < 2 || 255 < (int)m->v4.hopcnt + (int)m->v4.ttl)
      return 0;
  } else {
    if (m->v6.ttl < 2 || 255 < (int)m->v6.hopcnt + (int)m->v6.ttl)
      return 0;
  }

  /* Lookup sender address */
  src = olsr_lookup_main_addr_by_alias(from_addr);
  if (!src)
    src = from_addr;

  neighbor = olsr_lookup_nbr_entry(src, true);
  if (!neighbor) {
    OLSR_DEBUG(LOG_PACKET_PARSING, "Not forwarding message type %d because no nbr entry found for %s\n",
        m->v4.olsr_msgtype, olsr_ip_to_string(&buf, src));
    return 0;
  }
  if (!neighbor->is_sym) {
    OLSR_DEBUG(LOG_PACKET_PARSING, "Not forwarding message type %d because received by non-symmetric neighbor %s\n",
        m->v4.olsr_msgtype, olsr_ip_to_string(&buf, src));
    return 0;
  }

  /* Check MPR */
  if (neighbor->mprs_count == 0) {
    OLSR_DEBUG(LOG_PACKET_PARSING, "Not forwarding message type %d because we are no MPR for %s\n",
        m->v4.olsr_msgtype, olsr_ip_to_string(&buf, src));
    /* don't forward packages if not a MPR */
    return 0;
  }

  /* check if we already forwarded this message */
  if (olsr_message_is_duplicate(m, true)) {
    OLSR_DEBUG(LOG_PACKET_PARSING, "Not forwarding message type %d from %s because we already forwarded it.\n",
        m->v4.olsr_msgtype, olsr_ip_to_string(&buf, src));
    return 0;                   /* it's a duplicate, forget about it */
  }

  /* Treat TTL hopcnt */
  if (olsr_cnf->ip_version == AF_INET) {
    /* IPv4 */
    m->v4.hopcnt++;
    m->v4.ttl--;
  } else {
    /* IPv6 */
    m->v6.hopcnt++;
    m->v6.ttl--;
  }

  /* Update packet data */
  msgsize = ntohs(m->v4.olsr_msgsize);

  OLSR_DEBUG(LOG_PACKET_PARSING, "Forwarding message type %d from %s.\n",
      m->v4.olsr_msgtype, olsr_ip_to_string(&buf, src));

  /* looping trough interfaces */
  OLSR_FOR_ALL_INTERFACES(ifn) {
    if (net_output_pending(ifn)) {
      /* dont forward to incoming interface if interface is mode ether */
      if (in_if->mode == IF_MODE_ETHER && ifn == in_if)
        continue;

      /*
       * Check if message is to big to be piggybacked
       */
      if (net_outbuffer_push(ifn, m, msgsize) != msgsize) {
        /* Send */
        net_output(ifn);
        /* Buffer message */
        set_buffer_timer(ifn);

        if (net_outbuffer_push(ifn, m, msgsize) != msgsize) {
          OLSR_WARN(LOG_NETWORKING, "Received message to big to be forwarded in %s(%d bytes)!", ifn->int_name, msgsize);
        }
      }
    } else {
      /* No forwarding pending */
      set_buffer_timer(ifn);

      if (net_outbuffer_push(ifn, m, msgsize) != msgsize) {
        OLSR_WARN(LOG_NETWORKING, "Received message to big to be forwarded in %s(%d bytes)!", ifn->int_name, msgsize);
      }
    }
  }
  OLSR_FOR_ALL_INTERFACES_END(ifn);

  return 1;
}

/**
 * Wrapper for the timer callback.
 */
static void
olsr_expire_buffer_timer(void *context)
{
  struct interface *ifn;

  ifn = (struct interface *)context;

  /*
   * Clear the pointer to indicate that this timer has
   * been expired and needs to be restarted in case there
   * will be another message queued in the future.
   */
  ifn->buffer_hold_timer = NULL;

  /*
   * Do we have something to emit ?
   */
  if (!net_output_pending(ifn)) {
    return;
  }

  OLSR_DEBUG(LOG_NETWORKING, "Buffer Holdtimer for %s timed out, sending data.\n", ifn->int_name);

  net_output(ifn);
}

/*
 * set_buffer_timer
 *
 * Kick a hold-down timer which defers building of a message.
 * This has the desired effect that olsr messages get bigger.
 */
void
set_buffer_timer(struct interface *ifn)
{

  /*
   * Bail if there is already a timer running.
   */
  if (ifn->buffer_hold_timer) {
    return;
  }

  /*
   * This is the first message since the last time this interface has
   * been drained. Flush the buffer in second or so.
   */
  ifn->buffer_hold_timer =
    olsr_start_timer(OLSR_BUFFER_HOLD_TIME, OLSR_BUFFER_HOLD_JITTER,
                     OLSR_TIMER_ONESHOT, &olsr_expire_buffer_timer, ifn, buffer_hold_timer_cookie);
}

void
olsr_init_willingness(void)
{
  /* Some cookies for stats keeping */
  static struct olsr_cookie_info *willingness_timer_cookie = NULL;

  if (olsr_cnf->willingness_auto) {
    OLSR_INFO(LOG_MAIN, "Initialize automatic willingness...\n");
    /* Run it first and then periodic. */
    olsr_update_willingness(NULL);

    willingness_timer_cookie = olsr_alloc_cookie("Update Willingness", OLSR_COOKIE_TYPE_TIMER);
    olsr_start_timer((unsigned int)olsr_cnf->will_int * MSEC_PER_SEC, 5,
                     OLSR_TIMER_PERIODIC, &olsr_update_willingness, NULL, willingness_timer_cookie);
  }
}

static void
olsr_update_willingness(void *foo __attribute__ ((unused)))
{
  int tmp_will = olsr_cnf->willingness;

  /* Re-calculate willingness */
  olsr_cnf->willingness = olsr_calculate_willingness();

  if (tmp_will != olsr_cnf->willingness) {
    OLSR_INFO(LOG_MAIN, "Local willingness updated: old %d new %d\n", tmp_will, olsr_cnf->willingness);
  }
}


/**
 *Calculate this nodes willingness to act as a MPR
 *based on either a fixed value or the power status
 *of the node using APM
 *
 *@return a 8bit value from 0-7 representing the willingness
 */

uint8_t
olsr_calculate_willingness(void)
{
  struct olsr_apm_info ainfo;

  /* If fixed willingness */
  if (!olsr_cnf->willingness_auto)
    return olsr_cnf->willingness;

  if (apm_read(&ainfo) < 1)
    return WILL_DEFAULT;

  apm_printinfo(&ainfo);

  /* If AC powered */
  if (ainfo.ac_line_status == OLSR_AC_POWERED)
    return 6;

  /* If battery powered
   *
   * juice > 78% will: 3
   * 78% > juice > 26% will: 2
   * 26% > juice will: 1
   */
  return (ainfo.battery_percentage / 26);
}

const char *
olsr_msgtype_to_string(uint8_t msgtype)
{
  static char type[20];

  switch (msgtype) {
  case (HELLO_MESSAGE):
    return "HELLO";
  case (TC_MESSAGE):
    return "TC";
  case (MID_MESSAGE):
    return "MID";
  case (HNA_MESSAGE):
    return "HNA";
  case (LQ_HELLO_MESSAGE):
    return ("LQ-HELLO");
  case (LQ_TC_MESSAGE):
    return ("LQ-TC");
  default:
    break;
  }

  snprintf(type, sizeof(type), "UNKNOWN(%d)", msgtype);
  return type;
}


const char *
olsr_link_to_string(uint8_t linktype)
{
  static char type[20];

  switch (linktype) {
  case (UNSPEC_LINK):
    return "UNSPEC";
  case (ASYM_LINK):
    return "ASYM";
  case (SYM_LINK):
    return "SYM";
  case (LOST_LINK):
    return "LOST";
  case (HIDE_LINK):
    return "HIDE";
  default:
    break;
  }

  snprintf(type, sizeof(type), "UNKNOWN(%d)", linktype);
  return type;
}


const char *
olsr_status_to_string(uint8_t status)
{
  static char type[20];

  switch (status) {
  case (NOT_NEIGH):
    return "NOT NEIGH";
  case (SYM_NEIGH):
    return "NEIGHBOR";
  case (MPR_NEIGH):
    return "MPR";
  default:
    break;
  }

  snprintf(type, sizeof(type), "UNKNOWN(%d)", status);
  return type;
}


/**
 *Termination function to be called whenever a error occures
 *that requires the daemon to terminate
 *
 *@param val the exit code for OLSR
 */

void
olsr_exit(int val)
{
  fflush(stdout);
  olsr_cnf->exit_value = val;
  if (app_state == STATE_INIT) {
    exit(val);
  }
  app_state = STATE_SHUTDOWN;
}


/**
 * Wrapper for malloc(3) that does error-checking
 *
 * @param size the number of bytes to allocalte
 * @param caller a string identifying the caller for
 * use in error messaging
 *
 * @return a void pointer to the memory allocated
 */
void *
olsr_malloc(size_t size, const char *id __attribute__ ((unused)))
{
  void *ptr;

  /*
   * Not all the callers do a proper cleaning of memory.
   * Clean it on behalf of those.
   */
  ptr = calloc(1, size);

  if (!ptr) {
    OLSR_ERROR(LOG_MAIN, "Out of memory for id '%s': %s\n", id, strerror(errno));
    olsr_exit(EXIT_FAILURE);
  }
  return ptr;
}

/*
 * Same as strdup but works with olsr_malloc
 */
char *
olsr_strdup(const char *s)
{
  char *ret = olsr_malloc(1 + strlen(s), "olsr_strdup");
  strcpy(ret, s);
  return ret;
}

/*
 * Same as strndup but works with olsr_malloc
 */
char *
olsr_strndup(const char *s, size_t n)
{
  size_t len = n < strlen(s) ? n : strlen(s);
  char *ret = olsr_malloc(1 + len, "olsr_strndup");
  strncpy(ret, s, len);
  ret[len] = 0;
  return ret;
}

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
