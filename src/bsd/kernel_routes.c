
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

#include "kernel_routes.h"
#include "olsr.h"
#include "defs.h"
#include "process_routes.h"
#include "net_olsr.h"
#include "ipcalc.h"
#include "olsr_logging.h"

#include <errno.h>
#include <unistd.h>
#include <net/if_dl.h>

#ifdef _WRS_KERNEL
#include <net/ifaddrs.h>
#include <wrn/coreip/net/route.h>
#include <m2Lib.h>
#define OLSR_PID taskIdSelf ()
#else
#include <ifaddrs.h>
#define OLSR_PID getpid ()
#endif

/**
 *
 * Calculate the kernel route flags.
 * Called before enqueuing a change/delete operation
 *
 */
static uint8_t
olsr_rt_flags(const struct rt_entry *rt)
{
  const struct rt_nexthop *nh;
  uint8_t flags = RTF_UP;

  /* destination is host */
  if (rt->rt_dst.prefix_len == 8 * olsr_cnf->ipsize) {
    flags |= RTF_HOST;
  }

  nh = olsr_get_nh(rt);

  if (olsr_ipcmp(&rt->rt_dst.prefix, &nh->gateway) != 0) {
    flags |= RTF_GATEWAY;
  }

  return flags;
}

static unsigned int seq = 0;

/*
 * Sends an add or delete message via the routing socket.
 * The message consists of:
 *  - a header i.e. struct rt_msghdr
 *  - 0-8 socket address structures
 */
static int
add_del_route(const struct rt_entry *rt, int add)
{
  struct rt_msghdr *rtm;               /* message to send to the routing socket */
  unsigned char buff[512];
  unsigned char *walker;               /* points within the buffer */
  struct sockaddr_in sin4;             /* internet style sockaddr */
  struct sockaddr_dl *sdl;             /* link level sockaddr */
  struct ifaddrs *addrs;
  struct ifaddrs *awalker;
  const struct rt_nexthop *nexthop;
  union olsr_ip_addr mask;             /* netmask as ip address */
  int sin_size, sdl_size;              /* payload of the message */
  int len;                             /* message size written to routing socket */

  if (add) {
    OLSR_DEBUG(LOG_ROUTING, "KERN: Adding %s\n", olsr_rtp_to_string(rt->rt_best));
  } else {
    OLSR_DEBUG(LOG_ROUTING, "KERN: Deleting %s\n", olsr_rt_to_string(rt));
  }

  memset(buff, 0, sizeof(buff));
  memset(&sin4, 0, sizeof(sin4));

  sin4.sin_len = sizeof(sin4);
  sin4.sin_family = AF_INET;

  sin_size = 1 + ((sizeof(struct sockaddr_in) - 1) | 3);
  sdl_size = 1 + ((sizeof(struct sockaddr_dl) - 1) | 3);

  /**********************************************************************
   *                  FILL THE ROUTING MESSAGE HEADER
   **********************************************************************/

  /* position header to the beginning of the buffer */
  rtm = (struct rt_msghdr *)buff;

  rtm->rtm_version = RTM_VERSION;
  rtm->rtm_type = add ? RTM_ADD : RTM_DELETE;
  rtm->rtm_index = 0;           /* is ignored in outgoing messages */
  rtm->rtm_flags = olsr_rt_flags(rt);
  rtm->rtm_pid = OLSR_PID;
  rtm->rtm_seq = ++seq;

  /* walk to the end of the header */
  walker = buff + sizeof(struct rt_msghdr);

  /**********************************************************************
   *                  SET  DESTINATION OF THE ROUTE
   **********************************************************************/

#ifdef _WRS_KERNEL
  /*
   * vxWorks: change proto or tos
   */
  OLSR_DEBUG(LOG_ROUTING, "\t- Setting Protocol: 0\n");
  ((struct sockaddr_rt *)(&sin4))->srt_proto = 0;
  OLSR_DEBUG(LOG_ROUTING, "\t- Setting TOS: 0\n");
  ((struct sockaddr_rt *)(&sin4))->srt_tos = 0;
#endif

  sin4.sin_addr = rt->rt_dst.prefix.v4;
  memcpy(walker, &sin4, sizeof(sin4));
  walker += sin_size;
  rtm->rtm_addrs = RTA_DST;

  /**********************************************************************
   *                  SET GATEWAY OF THE ROUTE
   **********************************************************************/

#ifdef _WRS_KERNEL
  /*
   * vxWorks: Route with no gateway is deleted
   */
  if (add) {
#endif
    nexthop = olsr_get_nh(rt);
    if (0 != (rtm->rtm_flags & RTF_GATEWAY)) {
      sin4.sin_addr = nexthop->gateway.v4;
      memcpy(walker, &sin4, sizeof(sin4));
      walker += sin_size;
      rtm->rtm_addrs |= RTA_GATEWAY;
    } else {
      /*
       * Host is directly reachable, so add
       * the output interface MAC address.
       */
      if (getifaddrs(&addrs)) {
        OLSR_WARN(LOG_ROUTING, "\ngetifaddrs() failed\n");
        return -1;
      }

      for (awalker = addrs; awalker != NULL; awalker = awalker->ifa_next)
        if (awalker->ifa_addr->sa_family == AF_LINK && strcmp(awalker->ifa_name, nexthop->interface->int_name) == 0)
          break;

      if (awalker == NULL) {
        OLSR_WARN(LOG_ROUTING, "\nInterface %s not found\n", nexthop->interface->int_name);
        freeifaddrs(addrs);
        return -1;
      }

      /* sdl is "struct sockaddr_dl" */
      sdl = (struct sockaddr_dl *)awalker->ifa_addr;
      memcpy(walker, sdl, sdl->sdl_len);
      walker += sdl_size;
      rtm->rtm_addrs |= RTA_GATEWAY;
#ifdef RTF_CLONING
      rtm->rtm_flags |= RTF_CLONING;
#endif
#ifndef _WRS_KERNEL
      rtm->rtm_flags &= ~RTF_HOST;
#endif
      freeifaddrs(addrs);
    }
#ifdef _WRS_KERNEL
  }
#endif

  /**********************************************************************
   *                         SET  NETMASK
   **********************************************************************/

  if (0 == (rtm->rtm_flags & RTF_HOST)) {
    olsr_prefix_to_netmask(&mask, rt->rt_dst.prefix_len);
    sin4.sin_addr = mask.v4;
    memcpy(walker, &sin4, sizeof(sin4));
    walker += sin_size;
    rtm->rtm_addrs |= RTA_NETMASK;
  }

  /**********************************************************************
   *           WRITE CONFIGURATION MESSAGE TO THE ROUTING SOCKET
   **********************************************************************/

  rtm->rtm_msglen = (unsigned short)(walker - buff);
  len = write(olsr_cnf->rts_bsd, buff, rtm->rtm_msglen);
  if (0 != rtm->rtm_errno || len < rtm->rtm_msglen) {
    OLSR_WARN(LOG_ROUTING, "\nCannot write to routing socket: (rtm_errno= 0x%x) (last error message: %s)\n", rtm->rtm_errno,
              strerror(errno));
  }
  return 0;
}

static int
add_del_route6(const struct rt_entry *rt, int add)
{
  struct rt_msghdr *rtm;
  unsigned char buff[512];
  unsigned char *walker;
  struct sockaddr_in6 sin6;
  struct sockaddr_dl sdl;
  const struct rt_nexthop *nexthop;
  int sin_size, sdl_size;
  int len;

  if (add) {
    OLSR_DEBUG(LOG_ROUTING, "KERN: Adding %s\n", olsr_rtp_to_string(rt->rt_best));
  } else {
    OLSR_DEBUG(LOG_ROUTING, "KERN: Deleting %s\n", olsr_rt_to_string(rt));
  }

  memset(buff, 0, sizeof(buff));
  memset(&sin6, 0, sizeof(sin6));
  memset(&sdl, 0, sizeof(sdl));

  sin6.sin6_len = sizeof(sin6);
  sin6.sin6_family = AF_INET6;
  sdl.sdl_len = sizeof(sdl);
  sdl.sdl_family = AF_LINK;

  sin_size = 1 + ((sizeof(struct sockaddr_in6) - 1) | 3);
  sdl_size = 1 + ((sizeof(struct sockaddr_dl) - 1) | 3);

  /**********************************************************************
   *                  FILL THE ROUTING MESSAGE HEADER
   **********************************************************************/

  /* position header to the beginning of the buffer */
  rtm = (struct rt_msghdr *)buff;
  rtm->rtm_version = RTM_VERSION;
  rtm->rtm_type = (add != 0) ? RTM_ADD : RTM_DELETE;
  rtm->rtm_index = 0;
  rtm->rtm_flags = olsr_rt_flags(rt);
  rtm->rtm_pid = OLSR_PID;
  rtm->rtm_seq = ++seq;

  /* walk to the end of the header */
  walker = buff + sizeof(struct rt_msghdr);

  /**********************************************************************
   *                  SET  DESTINATION OF THE ROUTE
   **********************************************************************/

  memcpy(&sin6.sin6_addr.s6_addr, &rt->rt_dst.prefix.v6, sizeof(struct in6_addr));
  memcpy(walker, &sin6, sizeof(sin6));
  walker += sin_size;
  rtm->rtm_addrs = RTA_DST;

  /**********************************************************************
   *                  SET GATEWAY OF THE ROUTE
   **********************************************************************/

  nexthop = olsr_get_nh(rt);
  if (0 != (rtm->rtm_flags & RTF_GATEWAY)) {
    memcpy(&sin6.sin6_addr.s6_addr, &nexthop->gateway.v6, sizeof(struct in6_addr));
    memset(&sin6.sin6_addr.s6_addr, 0, 8);
    sin6.sin6_addr.s6_addr[0] = 0xfe;
    sin6.sin6_addr.s6_addr[1] = 0x80;
    sin6.sin6_scope_id = nexthop->interface->if_index;
#ifdef __KAME__
    *(u_int16_t *) & sin6.sin6_addr.s6_addr[2] = htons(sin6.sin6_scope_id);
    sin6.sin6_scope_id = 0;
#endif
    memcpy(walker, &sin6, sizeof(sin6));
    walker += sin_size;
    rtm->rtm_addrs |= RTA_GATEWAY;
  } else {
    /*
     * Host is directly reachable, so add
     * the output interface MAC address.
     */
    memcpy(&sin6.sin6_addr.s6_addr, &rt->rt_dst.prefix.v6, sizeof(struct in6_addr));
    memset(&sin6.sin6_addr.s6_addr, 0, 8);
    sin6.sin6_addr.s6_addr[0] = 0xfe;
    sin6.sin6_addr.s6_addr[1] = 0x80;
    sin6.sin6_scope_id = nexthop->interface->if_index;
#ifdef __KAME__
    *(u_int16_t *) & sin6.sin6_addr.s6_addr[2] = htons(sin6.sin6_scope_id);
    sin6.sin6_scope_id = 0;
#endif
    memcpy(walker, &sin6, sizeof(sin6));
    walker += sin_size;
    rtm->rtm_addrs |= RTA_GATEWAY;
    rtm->rtm_flags |= RTF_GATEWAY;
  }

  /**********************************************************************
   *                         SET  NETMASK
   **********************************************************************/

  if (0 == (rtm->rtm_flags & RTF_HOST)) {
    olsr_prefix_to_netmask((union olsr_ip_addr *)&sin6.sin6_addr, rt->rt_dst.prefix_len);
    memcpy(walker, &sin6, sizeof(sin6));
    walker += sin_size;
    rtm->rtm_addrs |= RTA_NETMASK;
  }

  /**********************************************************************
   *           WRITE CONFIGURATION MESSAGE TO THE ROUTING SOCKET
   **********************************************************************/

  rtm->rtm_msglen = (unsigned short)(walker - buff);
  len = write(olsr_cnf->rts_bsd, buff, rtm->rtm_msglen);
  if (len < 0 && !(errno == EEXIST || errno == ESRCH)) {
    OLSR_WARN(LOG_ROUTING, "cannot write to routing socket: %s\n", strerror(errno));
  }

  /*
   * If we get an EEXIST error while adding, delete and retry.
   */
  if (len < 0 && errno == EEXIST && rtm->rtm_type == RTM_ADD) {
    struct rt_msghdr *drtm;
    unsigned char dbuff[512];

    memset(dbuff, 0, sizeof(dbuff));
    drtm = (struct rt_msghdr *)dbuff;
    drtm->rtm_version = RTM_VERSION;
    drtm->rtm_type = RTM_DELETE;
    drtm->rtm_index = 0;
    drtm->rtm_flags = olsr_rt_flags(rt);
    drtm->rtm_seq = ++seq;

    walker = dbuff + sizeof(struct rt_msghdr);
    memcpy(&sin6.sin6_addr.s6_addr, &rt->rt_dst.prefix.v6, sizeof(struct in6_addr));
    memcpy(walker, &sin6, sizeof(sin6));
    walker += sin_size;
    drtm->rtm_addrs = RTA_DST;
    drtm->rtm_msglen = (unsigned short)(walker - dbuff);
    len = write(olsr_cnf->rts_bsd, dbuff, drtm->rtm_msglen);
    if (len < 0) {
      OLSR_WARN(LOG_ROUTING, "cannot delete route: %s\n", strerror(errno));
    }
    rtm->rtm_seq = ++seq;
    len = write(olsr_cnf->rts_bsd, buff, rtm->rtm_msglen);
    if (len < 0) {
      OLSR_WARN(LOG_ROUTING, "still cannot add route: %s\n", strerror(errno));
    }
  }
  return 0;
}

int
olsr_kernel_add_route(const struct rt_entry *rt, int ip_version)
{
  return AF_INET == ip_version ? add_del_route(rt, 1) : add_del_route6(rt, 1);
}

int
olsr_kernel_del_route(const struct rt_entry *rt, int ip_version)
{
  return AF_INET == ip_version ? add_del_route(rt, 0) : add_del_route6(rt, 0);
}

int
olsr_lo_interface(union olsr_ip_addr *ip __attribute__ ((unused)), bool create __attribute__ ((unused)))
{
  return 0;
}

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
