
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


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "defs.h"
#include "scheduler.h"
#include "nameservice.h"
#include "mid_set.h"
#include "tc_set.h"
#include "ipcalc.h"
#include "lq_plugin.h"
#include "olsr_logging.h"

#include "mapwrite.h"

static char my_latlon_str[48];

/**
 * lookup a nodes position
 */
static char *
lookup_position_latlon(union olsr_ip_addr *ip)
{
  int hash;
  struct db_entry *entry;
  struct list_node *list_head, *list_node;

  if (olsr_ipcmp(ip, &olsr_cnf->router_id) == 0) {
    return my_latlon_str;
  }

  for (hash = 0; hash < HASHSIZE; hash++) {
    list_head = &latlon_list[hash];
    for (list_node = list_head->next; list_node != list_head; list_node = list_node->next) {

      entry = list2db(list_node);

      if (entry->names && olsr_ipcmp(&entry->originator, ip) == 0) {
        return entry->names->name;
      }
    }
  }
  return NULL;
}

/**
 * write latlon positions to a file
 */
void
mapwrite_work(FILE * fmap)
{
  int hash;
  struct olsr_if_config *ifs;
  union olsr_ip_addr ip;
  struct ipaddr_str strbuf1, strbuf2;
  struct tc_entry *tc;
  struct tc_edge_entry *tc_edge;

  if (!my_names || !fmap)
    return;

  for (ifs = olsr_cnf->if_configs; ifs; ifs = ifs->next) {
    if (0 != ifs->interf) {
      if (olsr_cnf->ip_version == AF_INET) {
        if (ip4cmp((struct in_addr *)&olsr_cnf->router_id, &ifs->interf->int_addr.sin_addr) != 0) {
          if (0 > fprintf(fmap, "Mid('%s','%s');\n",
                          olsr_ip_to_string(&strbuf1, &olsr_cnf->router_id),
                          olsr_ip_to_string(&strbuf2, (union olsr_ip_addr *)&ifs->interf->int_addr.sin_addr))) {
            return;
          }
        }
      } else if (ip6cmp((struct in6_addr *)&olsr_cnf->router_id, &ifs->interf->int6_addr.sin6_addr) != 0) {
        if (0 > fprintf(fmap, "Mid('%s','%s');\n",
                        olsr_ip_to_string(&strbuf1, &olsr_cnf->router_id),
                        olsr_ip_to_string(&strbuf2, (union olsr_ip_addr *)&ifs->interf->int6_addr.sin6_addr))) {
          return;
        }
      }
    }
  }

  OLSR_FOR_ALL_TC_ENTRIES(tc) {
    struct mid_entry *alias;
    OLSR_FOR_ALL_TC_MID_ENTRIES(tc, alias) {
      if (0 > fprintf(fmap, "Mid('%s','%s');\n",
                      olsr_ip_to_string(&strbuf1, &tc->addr), olsr_ip_to_string(&strbuf2, &alias->mid_alias_addr))) {
        return;
      }
    }
    OLSR_FOR_ALL_TC_MID_ENTRIES_END(tc, alias);
  }
  OLSR_FOR_ALL_TC_ENTRIES_END(tc);

  lookup_defhna_latlon(&ip);
  sprintf(my_latlon_str, "%f,%f,%d", my_lat, my_lon, get_isdefhna_latlon());
  if (0 > fprintf(fmap, "Self('%s',%s,'%s','%s');\n",
                  olsr_ip_to_string(&strbuf1, &olsr_cnf->router_id), my_latlon_str,
                  olsr_ip_to_string(&strbuf2, &ip), my_names->name)) {
    return;
  }
  for (hash = 0; hash < HASHSIZE; hash++) {
    struct db_entry *entry;
    struct list_node *list_head, *list_node;

    list_head = &latlon_list[hash];
    for (list_node = list_head->next; list_node != list_head; list_node = list_node->next) {

      entry = list2db(list_node);

      if (NULL != entry->names) {
        if (0 > fprintf(fmap, "Node('%s',%s,'%s','%s');\n",
                        olsr_ip_to_string(&strbuf1, &entry->originator),
                        entry->names->name, olsr_ip_to_string(&strbuf2, &entry->names->ip),
                        lookup_name_latlon(&entry->originator))) {
          return;
        }
      }
    }
  }

  OLSR_FOR_ALL_TC_ENTRIES(tc) {
    OLSR_FOR_ALL_TC_EDGE_ENTRIES(tc, tc_edge) {
      char *lla = lookup_position_latlon(&tc->addr);
      char *llb = lookup_position_latlon(&tc_edge->T_dest_addr);
      if (NULL != lla && NULL != llb) {
        char lqbuffer[LQTEXT_MAXLENGTH];

        /*
         * To speed up processing, Links with both positions are named PLink()
         */
        if (0 > fprintf(fmap, "PLink('%s','%s',%s,%s,%s);\n",
                        olsr_ip_to_string(&strbuf1, &tc_edge->T_dest_addr),
                        olsr_ip_to_string(&strbuf2, &tc->addr),
                        olsr_get_linkcost_text(tc_edge->cost, false, lqbuffer, sizeof(lqbuffer)), lla, llb)) {
          return;
        }
      } else {
        char lqbuffer[LQTEXT_MAXLENGTH];

        /*
         * If one link end pos is unkown, only send Link()
         */
        if (0 > fprintf(fmap, "Link('%s','%s',%s);\n",
                        olsr_ip_to_string(&strbuf1, &tc_edge->T_dest_addr),
                        olsr_ip_to_string(&strbuf2, &tc->addr),
                        olsr_get_linkcost_text(tc_edge->cost, false, lqbuffer, sizeof(lqbuffer)))) {
          return;
        }
      }
    }
    OLSR_FOR_ALL_TC_EDGE_ENTRIES_END();
  }
  OLSR_FOR_ALL_TC_ENTRIES_END();
}

#ifndef WIN32

/*
 * Windows doesn't know fifo's AFAIK. We better write
 * to a file (done in nameservice.c, see #ifdef WIN32)
 */

static const char *the_fifoname = 0;

static void
mapwrite_poll(void *context __attribute__ ((unused)))
{
  FILE *fout;
  /* Non-blocking means: fail open if no pipe reader */
  int fd = open(the_fifoname, O_WRONLY | O_NONBLOCK);
  if (0 <= fd) {
    /*
     * Change to blocking, otherwhise expect fprintf errors
     */
    fcntl(fd, F_SETFL, O_WRONLY);
    fout = fdopen(fd, "w");
    if (0 != fout) {
      mapwrite_work(fout);
      fclose(fout);
      /* Give pipe reader cpu slot to detect EOF */
      usleep(1);
    } else {
      close(fd);
    }
  }
}

int
mapwrite_init(const char *fifoname)
{
  the_fifoname = fifoname;
  if (0 != fifoname && 0 != *fifoname) {
    unlink(fifoname);
    if (0 > mkfifo(fifoname, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH)) {
      OLSR_WARN(LOG_PLUGINS, "mkfifo(%s): %s", fifoname, strerror(errno));
      return false;
    } else {
      olsr_start_timer(800, 5, OLSR_TIMER_PERIODIC, &mapwrite_poll, NULL, map_poll_timer_cookie);
    }
  }
  return true;
}

void
mapwrite_exit(void)
{
  if (0 != the_fifoname) {
    unlink(the_fifoname);
    /* Ignore any Error */
    the_fifoname = 0;
  }
}
#endif

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
