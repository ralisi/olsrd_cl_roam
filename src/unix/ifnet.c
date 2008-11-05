/*
 * The olsr.org Optimized Link-State Routing daemon(olsrd)
 * Copyright (c) 2004, Andreas Tønnesen(andreto@olsr.org)
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


#if defined __FreeBSD__ || defined __MacOSX__ || defined __NetBSD__ || defined __OpenBSD__
#define ifr_netmask ifr_addr
#endif

#include "ifnet.h"
#include "ipcalc.h"
#include "interfaces.h"
#include "defs.h"
#include "olsr.h"
#include "net_os.h"
#include "net_olsr.h"
#include "parser.h"
#include "scheduler.h"
#include "generate_msg.h"
#include "mantissa.h"
#include "lq_packet.h"
#include "log.h"
#include "link_set.h"
#include "../common/string.h"

#include <signal.h>
#include <sys/types.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define BUFSPACE  (127*1024)	/* max. input buffer size to request */

#if 0
int
set_flag(char *ifname, short flag __attribute__((unused)))
{
  struct ifreq ifr;

  /* Get flags */
  strscpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
  if (ioctl(olsr_cnf->ioctl_s, SIOCGIFFLAGS, &ifr) < 0) {
    fprintf(stderr,"ioctl (get interface flags)");
    return -1;
  }

  strscpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
  //printf("Setting flags for if \"%s\"\n", ifr.ifr_name);
  if ((ifr.ifr_flags & (IFF_UP | IFF_RUNNING)) == 0) {
    /* Add UP */
    ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
    /* Set flags + UP */
    if (ioctl(olsr_cnf->ioctl_s, SIOCSIFFLAGS, &ifr) < 0) {
      fprintf(stderr, "ERROR(%s): %s\n", ifr.ifr_name, strerror(errno));
      return -1;
    }
  }
  return 1;
}
#endif

void
check_interface_updates(void *foo __attribute__((unused)))
{
  struct olsr_if *tmp_if;

#ifdef DEBUG
  OLSR_PRINTF(3, "Checking for updates in the interface set\n");
#endif

  for (tmp_if = olsr_cnf->interfaces; tmp_if != NULL; tmp_if = tmp_if->next) {
    if (tmp_if->host_emul) {
	continue;
    }
    if (olsr_cnf->host_emul) { /* XXX: TEMPORARY! */
      continue;
    }
    if (!tmp_if->cnf->autodetect_chg) {
#ifdef DEBUG
      /* Don't check this interface */
      OLSR_PRINTF(3, "Not checking interface %s\n", tmp_if->name);
#endif
      continue;
    }

    if (tmp_if->configured) {
      chk_if_changed(tmp_if);
    } else {
      chk_if_up(tmp_if, 3);
    }
  }
}

/**
 * Checks if an initialized interface is changed
 * that is if it has been set down or the address
 * has been changed.
 *
 *@param iface the olsr_if struct describing the interface
 */
int
chk_if_changed(struct olsr_if *iface)
{
  struct ipaddr_str buf;
  struct interface *ifp;
  struct ifreq ifr;
  int if_changes = 0;

#ifdef DEBUG
  OLSR_PRINTF(3, "Checking if %s is set down or changed\n", iface->name);
#endif

  if (iface->host_emul) {
    return -1;
  }
  ifp = iface->interf;
  if (ifp == NULL) {
    /* Should not happen */
    iface->configured = 0;
    return 0;
  }

  memset(&ifr, 0, sizeof(ifr));
  strscpy(ifr.ifr_name, iface->name, sizeof(ifr.ifr_name));
  /* Get flags (and check if interface exists) */
  if (ioctl(olsr_cnf->ioctl_s, SIOCGIFFLAGS, &ifr) < 0) {
    OLSR_PRINTF(3, "No such interface: %s\n", iface->name);
    goto remove_interface;
  }
  ifp->int_flags = ifr.ifr_flags;

  /*
   * First check if the interface is set DOWN
   */
  if ((ifp->int_flags & IFF_UP) == 0) {
    OLSR_PRINTF(1, "\tInterface %s not up - removing it...\n", iface->name);
    goto remove_interface;
  }

  /*
   * We do all the interface type checks over.
   * This is because the interface might be a PCMCIA card. Therefore
   * it might not be the same physical interface as we configured earlier.
   */

  /* Check broadcast */
  if (olsr_cnf->ip_version == AF_INET &&
      !iface->cnf->ipv4_broadcast.v4.s_addr && /* Skip if fixed bcast */
      ((ifp->int_flags & IFF_BROADCAST)) == 0) {
    OLSR_PRINTF(3, "\tNo broadcast - removing\n");
    goto remove_interface;
  }

  if (ifp->int_flags & IFF_LOOPBACK) {
    OLSR_PRINTF(3, "\tThis is a loopback interface - removing it...\n");
    goto remove_interface;
  }

  ifp->is_hcif = OLSR_FALSE;

  /* trying to detect if interface is wireless. */
  ifp->is_wireless = check_wireless_interface(ifr.ifr_name);

  /* Set interface metric */
  ifp->int_metric = iface->cnf->weight.fixed ? iface->cnf->weight.value : calculate_if_metric(ifr.ifr_name);

  /* Get MTU */
  if (ioctl(olsr_cnf->ioctl_s, SIOCGIFMTU, &ifr) < 0) {
    ifp->int_mtu = 0;
  } else {
    ifr.ifr_mtu -= olsr_cnf->ip_version == AF_INET6 ? UDP_IPV6_HDRSIZE : UDP_IPV4_HDRSIZE;
    if (ifp->int_mtu != ifr.ifr_mtu) {
      ifp->int_mtu = ifr.ifr_mtu;
      /* Create new outputbuffer */
      net_remove_buffer(ifp); /* Remove old */
      net_add_buffer(ifp);
    }
  }

  /* Get interface index */
  ifp->if_index = if_nametoindex(ifr.ifr_name);

  /*
   * Now check if the IP has changed
   */
  
  /* IP version 6 */
  if (olsr_cnf->ip_version == AF_INET6) {
    struct sockaddr_in6 tmp_saddr6;
    /* Get interface address */
    if (get_ipv6_address(ifr.ifr_name, &tmp_saddr6, iface->cnf->ipv6_addrtype) <= 0) {
      OLSR_PRINTF(3, "\tCould not find %s IPv6 address for %s\n", iface->cnf->ipv6_addrtype == IPV6_ADDR_SITELOCAL ? "site-local" : "global", ifr.ifr_name);
      goto remove_interface;
    }
      
#ifdef DEBUG
    OLSR_PRINTF(3, "\tAddress: %s\n", ip6_to_string(&buf, &tmp_saddr6.sin6_addr));
#endif

    if (ip6equal(&tmp_saddr6.sin6_addr, &ifp->int6_addr.sin6_addr)) {
      OLSR_PRINTF(1, "New IP address for %s:\n", ifr.ifr_name);
      OLSR_PRINTF(1, "\tOld: %s\n", ip6_to_string(&buf, &ifp->int6_addr.sin6_addr));
      OLSR_PRINTF(1, "\tNew: %s\n", ip6_to_string(&buf, &tmp_saddr6.sin6_addr));

      /* Check main addr */
      if (ip6equal(&olsr_cnf->main_addr.v6, &tmp_saddr6.sin6_addr)) {
	/* Update main addr */
	olsr_cnf->main_addr.v6 = tmp_saddr6.sin6_addr;
      }

      /* Update address */
      ifp->int6_addr.sin6_addr = tmp_saddr6.sin6_addr;
      ifp->ip_addr.v6 = tmp_saddr6.sin6_addr;

      if_changes = 1;
    }
  } else {
    /* IP version 4 */
    const struct sockaddr_in *tmp_saddr4  = (struct sockaddr_in *)&ifr.ifr_addr;

    /* Check interface address (IPv4)*/
    if (ioctl(olsr_cnf->ioctl_s, SIOCGIFADDR, &ifr) < 0) {
      OLSR_PRINTF(1, "\tCould not get address of interface - removing it\n");
      goto remove_interface;
    }

#ifdef DEBUG
      OLSR_PRINTF(3, "\tAddress:%s\n", ip4_to_string(&buf, ifp->int_addr.sin_addr));
#endif

    if (ip4equal(&ifp->int_addr.sin_addr, &tmp_saddr4->sin_addr)) {
      /* New address */
      OLSR_PRINTF(1, "IPv4 address changed for %s\n", ifr.ifr_name);
      OLSR_PRINTF(1, "\tOld:%s\n", ip4_to_string(&buf, ifp->int_addr.sin_addr));
      OLSR_PRINTF(1, "\tNew:%s\n", ip4_to_string(&buf, tmp_saddr4->sin_addr));

      ifp->int_addr = *(struct sockaddr_in *)&ifr.ifr_addr;
      if (ip4equal(&olsr_cnf->main_addr.v4, &ifp->ip_addr.v4)) {
	OLSR_PRINTF(1, "New main address: %s\n", ip4_to_string(&buf, tmp_saddr4->sin_addr));
	olsr_syslog(OLSR_LOG_INFO, "New main address: %s\n", ip4_to_string(&buf, tmp_saddr4->sin_addr));
	olsr_cnf->main_addr.v4 = tmp_saddr4->sin_addr;
      }

      ifp->ip_addr.v4 = tmp_saddr4->sin_addr;

      if_changes = 1;
    }

    /* Check netmask */
    if (ioctl(olsr_cnf->ioctl_s, SIOCGIFNETMASK, &ifr) < 0) {
      olsr_syslog(OLSR_LOG_ERR, "%s: ioctl (get broadaddr)", ifr.ifr_name);
      goto remove_interface;
    }

#ifdef DEBUG
    OLSR_PRINTF(3, "\tNetmask:%s\n", ip4_to_string(&buf, ((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr));
#endif

    if (ip4equal(&ifp->int_netmask.sin_addr, &((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr)) {
      /* New address */
      OLSR_PRINTF(1, "IPv4 netmask changed for %s\n", ifr.ifr_name);
      OLSR_PRINTF(1, "\tOld:%s\n", ip4_to_string(&buf, ifp->int_netmask.sin_addr));
      OLSR_PRINTF(1, "\tNew:%s\n", ip4_to_string(&buf, ((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr));

      ifp->int_netmask = *(struct sockaddr_in *)&ifr.ifr_netmask;

      if_changes = 1;
    }

    if (!iface->cnf->ipv4_broadcast.v4.s_addr) {
      /* Check broadcast address */
      if (ioctl(olsr_cnf->ioctl_s, SIOCGIFBRDADDR, &ifr) < 0) {
	olsr_syslog(OLSR_LOG_ERR, "%s: ioctl (get broadaddr)", ifr.ifr_name);
 	goto remove_interface;
      }

#ifdef DEBUG
      OLSR_PRINTF(3, "\tBroadcast address:%s\n", ip4_to_string(&buf, ((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr));
#endif

      if (ifp->int_broadaddr.sin_addr.s_addr != ((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr.s_addr) {
	/* New address */
	OLSR_PRINTF(1, "IPv4 broadcast changed for %s\n", ifr.ifr_name);
	OLSR_PRINTF(1, "\tOld:%s\n", ip4_to_string(&buf, ifp->int_broadaddr.sin_addr));
	OLSR_PRINTF(1, "\tNew:%s\n", ip4_to_string(&buf, ((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr));
	
	ifp->int_broadaddr = *(struct sockaddr_in *)&ifr.ifr_broadaddr;
	if_changes = 1;
      }
    }
  }
  if (if_changes) {
    run_ifchg_cbs(ifp, IFCHG_IF_UPDATE);
  }
  return if_changes;


 remove_interface:
  OLSR_PRINTF(1, "Removing interface %s\n", iface->name);
  olsr_syslog(OLSR_LOG_INFO, "Removing interface %s\n", iface->name);

  olsr_delete_link_entry_by_ip(&ifp->ip_addr);

  /*
   * Call possible ifchange functions registered by plugins
   */
  run_ifchg_cbs(ifp, IFCHG_IF_REMOVE);

  /* Dequeue */
  if (ifp == ifnet) {
    ifnet = ifp->int_next;
  } else {
    struct interface *tmp_ifp = ifnet;
    while (tmp_ifp->int_next != ifp) {
      tmp_ifp = tmp_ifp->int_next;
    }
    tmp_ifp->int_next = ifp->int_next;
  }

  /* Remove output buffer */
  net_remove_buffer(ifp);

  /* Check main addr */
  if (ipequal(&olsr_cnf->main_addr, &ifp->ip_addr)) {
    if (ifnet == NULL) {
      /* No more interfaces */
      memset(&olsr_cnf->main_addr, 0, olsr_cnf->ipsize);
      OLSR_PRINTF(1, "No more interfaces...\n");
    } else {
      olsr_cnf->main_addr = ifnet->ip_addr;
      olsr_ip_to_string(&buf, &olsr_cnf->main_addr);
      OLSR_PRINTF(1, "New main address: %s\n", buf.buf);
      olsr_syslog(OLSR_LOG_INFO, "New main address: %s\n", buf.buf);
    }
  }

  /*
   * Deregister functions for periodic message generation
   */
  olsr_stop_timer(ifp->hello_gen_timer);
  olsr_stop_timer(ifp->tc_gen_timer);
  olsr_stop_timer(ifp->mid_gen_timer);
  olsr_stop_timer(ifp->hna_gen_timer);

  iface->configured = 0;
  iface->interf = NULL;

  /* Close olsr socket */
  remove_olsr_socket(ifp->olsr_socket, &olsr_input, NULL);
  close(ifp->olsr_socket);
  ifp->olsr_socket = -1;

  /* Free memory */
  free(ifp->int_name);
  free(ifp);

  if (ifnet == NULL && !olsr_cnf->allow_no_interfaces) {
    OLSR_PRINTF(1, "No more active interfaces - exiting.\n");
    olsr_syslog(OLSR_LOG_INFO, "No more active interfaces - exiting.\n");
    olsr_cnf->exit_value = EXIT_FAILURE;
    kill(getpid(), SIGINT);
  }
  return 0;
}

/**
 * Initializes the special interface used in
 * host-client emulation
 */
int add_hemu_if (struct olsr_if *iface)
{
  struct interface *ifp;
  olsr_u32_t addr[4];
  struct ipaddr_str buf;
  size_t name_size;

  if (!iface->host_emul)
    return -1;

  ifp = olsr_malloc(sizeof(struct interface), "Interface update 2");

  memset(ifp, 0, sizeof(struct interface));

  iface->configured = OLSR_TRUE;
  iface->interf = ifp;

  name_size = strlen("hcif01") + 1;
  ifp->is_hcif = OLSR_TRUE;
  ifp->int_name = olsr_malloc(name_size, "Interface update 3");
  ifp->int_metric = 0;

  strscpy(ifp->int_name, "hcif01", name_size);

  OLSR_PRINTF(1, "Adding %s(host emulation):\n", ifp->int_name);

  OLSR_PRINTF(1, "       Address:%s\n", olsr_ip_to_string(&buf, &iface->hemu_ip));

  OLSR_PRINTF(1, "       NB! This is a emulated interface\n       that does not exist in the kernel!\n");

  ifp->int_next = ifnet;
  ifnet = ifp;

  if (ipequal(&all_zero, &olsr_cnf->main_addr)) {
    olsr_cnf->main_addr = iface->hemu_ip;
    OLSR_PRINTF(1, "New main address: %s\n", olsr_ip_to_string(&buf, &olsr_cnf->main_addr));
    olsr_syslog(OLSR_LOG_INFO, "New main address: %s\n", olsr_ip_to_string(&buf, &olsr_cnf->main_addr));
  }

  ifp->int_mtu = OLSR_DEFAULT_MTU;

  ifp->int_mtu -= (olsr_cnf->ip_version == AF_INET6) ? UDP_IPV6_HDRSIZE : UDP_IPV4_HDRSIZE;

  /* Set up buffer */
  net_add_buffer(ifp);


  if (olsr_cnf->ip_version == AF_INET) {
    struct sockaddr_in sin;
 
    memset(&sin, 0, sizeof(sin));

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sin.sin_port = htons(10150);
 
    /* IP version 4 */
    ifp->ip_addr.v4 = iface->hemu_ip.v4;
 
    memcpy(&ifp->int_addr.sin_addr, &iface->hemu_ip, olsr_cnf->ipsize);

    /*
     * We create one socket for each interface and bind
     * the socket to it. This to ensure that we can control
     * on what interface the message is transmitted
     */

    ifp->olsr_socket = gethemusocket(&sin);
    if (ifp->olsr_socket < 0) {
      fprintf(stderr, "Could not initialize socket... exiting!\n\n");
      olsr_syslog(OLSR_LOG_ERR, "Could not initialize socket... exiting!\n\n");
      olsr_cnf->exit_value = EXIT_FAILURE;
      kill(getpid(), SIGINT);
    }
  } else {
    /* IP version 6 */
    memcpy(&ifp->ip_addr, &iface->hemu_ip, olsr_cnf->ipsize);

#if 0
    /*
     * We create one socket for each interface and bind
     * the socket to it. This to ensure that we can control
     * on what interface the message is transmitted
     */

    ifp->olsr_socket = gethcsocket6(&addrsock6, BUFSPACE, ifp->int_name);
    if (ifp->olsr_socket < 0)	{
      fprintf(stderr, "Could not initialize socket... exiting!\n\n");
      olsr_syslog(OLSR_LOG_ERR, "Could not initialize socket... exiting!\n\n");
      olsr_cnf->exit_value = EXIT_FAILURE;
      kill(getpid(), SIGINT);
    }
    join_mcast(ifp, ifp->olsr_socket);
#endif
    }

  /* Send IP as first 4/16 bytes on socket */
  memcpy(addr, iface->hemu_ip.v6.s6_addr, olsr_cnf->ipsize);
  addr[0] = htonl(addr[0]);
  addr[1] = htonl(addr[1]);
  addr[2] = htonl(addr[2]);
  addr[3] = htonl(addr[3]);

  if (send(ifp->olsr_socket, addr , olsr_cnf->ipsize, 0) != (int)olsr_cnf->ipsize) {
    fprintf(stderr, "Error sending IP: %m");
  }

  /* Register socket */
  add_olsr_socket(ifp->olsr_socket, &olsr_input_hostemu, NULL, NULL, SP_PR_READ);

  /*
   * Register functions for periodic message generation
   */

  ifp->hello_gen_timer =
    olsr_start_timer(iface->cnf->hello_params.emission_interval * MSEC_PER_SEC,
                     HELLO_JITTER, OLSR_TIMER_PERIODIC,
                     olsr_cnf->lq_level == 0 ? &generate_hello : &olsr_output_lq_hello,
                     ifp, hello_gen_timer_cookie->ci_id);
  ifp->tc_gen_timer =
    olsr_start_timer(iface->cnf->tc_params.emission_interval * MSEC_PER_SEC,
                     TC_JITTER, OLSR_TIMER_PERIODIC,
                     olsr_cnf->lq_level == 0 ? &generate_tc : &olsr_output_lq_tc,
                     ifp, tc_gen_timer_cookie->ci_id);
  ifp->mid_gen_timer =
    olsr_start_timer(iface->cnf->mid_params.emission_interval * MSEC_PER_SEC,
                     MID_JITTER, OLSR_TIMER_PERIODIC, &generate_mid, ifp,
                     mid_gen_timer_cookie->ci_id);
  ifp->hna_gen_timer =
    olsr_start_timer(iface->cnf->hna_params.emission_interval * MSEC_PER_SEC,
                     HNA_JITTER, OLSR_TIMER_PERIODIC, &generate_hna, ifp,
                     hna_gen_timer_cookie->ci_id);

  /* Recalculate max topology hold time */
  if (olsr_cnf->max_tc_vtime < iface->cnf->tc_params.emission_interval) {
    olsr_cnf->max_tc_vtime = iface->cnf->tc_params.emission_interval;
  }
  ifp->hello_etime = (olsr_reltime)(iface->cnf->hello_params.emission_interval * MSEC_PER_SEC);
  ifp->valtimes.hello = reltime_to_me(iface->cnf->hello_params.validity_time * MSEC_PER_SEC);
  ifp->valtimes.tc = reltime_to_me(iface->cnf->tc_params.validity_time * MSEC_PER_SEC);
  ifp->valtimes.mid = reltime_to_me(iface->cnf->mid_params.validity_time * MSEC_PER_SEC);
  ifp->valtimes.hna = reltime_to_me(iface->cnf->hna_params.validity_time * MSEC_PER_SEC);

  return 1;
}

static char basenamestr[32];
static const char* if_basename(const char* name);
static const char* if_basename(const char* name)
{
  const char *p = strchr(name, ':');
  if (NULL == p || p - name >= (int)(ARRAYSIZE(basenamestr) - 1)) {
    return name;
  }
  memcpy(basenamestr, name, p - name);
  basenamestr[p - name] = 0;
  return basenamestr;
}

/**
 * Initializes a interface described by iface,
 * if it is set up and is of the correct type.
 *
 *@param iface the olsr_if struct describing the interface
 *@param so the socket to use for ioctls
 *
 */
int
chk_if_up(struct olsr_if *iface, int debuglvl __attribute__((unused)))
{
  struct interface ifs, *ifp;
  struct ifreq ifr;
  const char *ifr_basename;

  if (iface->host_emul) {
    return -1;
  }
  memset(&ifr, 0, sizeof(ifr));
  memset(&ifs, 0, sizeof(ifs));

  strscpy(ifr.ifr_name, iface->name, sizeof(ifr.ifr_name));

  OLSR_PRINTF(debuglvl, "Checking %s:\n", ifr.ifr_name);

  /* Get flags (and check if interface exists) */
  if (ioctl(olsr_cnf->ioctl_s, SIOCGIFFLAGS, &ifr) < 0) {
    OLSR_PRINTF(debuglvl, "\tNo such interface!\n");
    return 0;
  }

  ifs.int_flags = ifr.ifr_flags;
  if ((ifs.int_flags & IFF_UP) == 0) {
    OLSR_PRINTF(debuglvl, "\tInterface not up - skipping it...\n");
    return 0;
  }

  /* Check broadcast */
  if (olsr_cnf->ip_version == AF_INET &&
      !iface->cnf->ipv4_broadcast.v4.s_addr && /* Skip if fixed bcast */
      (ifs.int_flags & IFF_BROADCAST) == 0) {
    OLSR_PRINTF(debuglvl, "\tNo broadcast - skipping\n");
    return 0;
  }

  if (ifs.int_flags & IFF_LOOPBACK) {
    OLSR_PRINTF(debuglvl, "\tThis is a loopback interface - skipping it...\n");
    return 0;
  }

  ifs.is_hcif = OLSR_FALSE;

  /* trying to detect if interface is wireless. */
  ifs.is_wireless = check_wireless_interface(ifr.ifr_name);
  OLSR_PRINTF(debuglvl, ifs.is_wireless ? "\tWireless interface detected\n" : "\tNot a wireless interface\n");

  ifr_basename = if_basename(ifr.ifr_name);

  /* IP version 6 */
  if (olsr_cnf->ip_version == AF_INET6) {
    /* Get interface address */
    struct ipaddr_str buf;
    if (get_ipv6_address(ifr.ifr_name, &ifs.int6_addr, iface->cnf->ipv6_addrtype) <= 0) {
      OLSR_PRINTF(debuglvl, "\tCould not find %s IPv6 address for %s\n", iface->cnf->ipv6_addrtype == IPV6_ADDR_SITELOCAL ? "site-local" : "global", ifr.ifr_name);
      return 0;
    }

    OLSR_PRINTF(debuglvl, "\tAddress: %s\n", ip6_to_string(&buf, &ifs.int6_addr.sin6_addr));

    /* Multicast */
    memset (&ifs.int6_multaddr, 0, sizeof(ifs.int6_multaddr));
    ifs.int6_multaddr.sin6_family   = AF_INET6;
    ifs.int6_multaddr.sin6_flowinfo = htonl(0);
    ifs.int6_multaddr.sin6_scope_id = if_nametoindex(ifr.ifr_name);
    ifs.int6_multaddr.sin6_port     = htons(OLSRPORT);
    ifs.int6_multaddr.sin6_addr = iface->cnf->ipv6_addrtype == IPV6_ADDR_SITELOCAL
        ? iface->cnf->ipv6_multi_site.v6
        : iface->cnf->ipv6_multi_glbl.v6;

#ifdef __MacOSX__
    ifs.int6_multaddr.sin6_scope_id = 0;
#endif

    OLSR_PRINTF(debuglvl, "\tMulticast: %s\n", ip6_to_string(&buf, &ifs.int6_multaddr.sin6_addr));  
  } else {
    /* IP version 4 */

    /* Get interface address (IPv4)*/
    if(ioctl(olsr_cnf->ioctl_s, SIOCGIFADDR, &ifr) < 0) {
      OLSR_PRINTF(debuglvl, "\tCould not get address of interface - skipping it\n");
      return 0;
    }

    ifs.int_addr = *(struct sockaddr_in *)&ifr.ifr_addr;

    /* Find netmask */
    if (ioctl(olsr_cnf->ioctl_s, SIOCGIFNETMASK, &ifr) < 0) {
      olsr_syslog(OLSR_LOG_ERR, "%s: ioctl (get netmask)", ifr.ifr_name);
      return 0;
    }
    ifs.int_netmask = *(struct sockaddr_in *)&ifr.ifr_netmask;

    /* Find broadcast address */
    if (iface->cnf->ipv4_broadcast.v4.s_addr) {
      /* Specified broadcast */
      ifs.int_broadaddr.sin_addr = iface->cnf->ipv4_broadcast.v4;
    } else {
      /* Autodetect */
      if (ioctl(olsr_cnf->ioctl_s, SIOCGIFBRDADDR, &ifr) < 0) {
	olsr_syslog(OLSR_LOG_ERR, "%s: ioctl (get broadaddr)", ifr.ifr_name);
	return 0;
      }
	
      ifs.int_broadaddr = *(struct sockaddr_in *)&ifr.ifr_broadaddr;
    }

    /* Deactivate IP spoof filter */
    deactivate_spoof(ifr_basename, &ifs, olsr_cnf->ip_version);

    /* Disable ICMP redirects */
    disable_redirects(ifr_basename, &ifs, olsr_cnf->ip_version);
  }
  
  /* Get interface index */
  ifs.if_index = if_nametoindex(ifr.ifr_name);
  
  /* Set interface metric */
  ifs.int_metric = iface->cnf->weight.fixed ? iface->cnf->weight.value : calculate_if_metric(ifr.ifr_name);
  OLSR_PRINTF(1, "\tMetric: %d\n", ifs.int_metric);

  /* Get MTU */
  if (ioctl(olsr_cnf->ioctl_s, SIOCGIFMTU, &ifr) < 0) {
    ifs.int_mtu = OLSR_DEFAULT_MTU;
  } else {
    ifs.int_mtu = ifr.ifr_mtu;
  }
  ifs.int_mtu -= olsr_cnf->ip_version == AF_INET6 ? UDP_IPV6_HDRSIZE : UDP_IPV4_HDRSIZE;

  ifs.ttl_index = -32; /* For the first 32 TC's, fish-eye is disabled */

  /* Set up buffer */
  net_add_buffer(&ifs);
	       
  OLSR_PRINTF(1, "\tMTU - IPhdr: %d\n", ifs.int_mtu);

  olsr_syslog(OLSR_LOG_INFO, "Adding interface %s\n", iface->name);
  OLSR_PRINTF(1, "\tIndex %d\n", ifs.if_index);

  if(olsr_cnf->ip_version == AF_INET) {
    struct ipaddr_str buf;
    OLSR_PRINTF(1, "\tAddress:%s\n", ip4_to_string(&buf, ifs.int_addr.sin_addr));
    OLSR_PRINTF(1, "\tNetmask:%s\n", ip4_to_string(&buf, ifs.int_netmask.sin_addr));
    OLSR_PRINTF(1, "\tBroadcast address:%s\n", ip4_to_string(&buf, ifs.int_broadaddr.sin_addr));
  } else {
    struct ipaddr_str buf;
    OLSR_PRINTF(1, "\tAddress: %s\n", ip6_to_string(&buf, &ifs.int6_addr.sin6_addr));
    OLSR_PRINTF(1, "\tMulticast: %s\n", ip6_to_string(&buf, &ifs.int6_multaddr.sin6_addr));
  }
  
  ifp = olsr_malloc(sizeof(*ifp), "Interface update 2");
  
  iface->configured = 1;
  iface->interf = ifp;

  //memcpy(ifp, &ifs, sizeof(*ifp));
  *ifp = ifs;
  
  ifp->immediate_send_tc = iface->cnf->tc_params.emission_interval < iface->cnf->hello_params.emission_interval;
  ifp->int_name = olsr_malloc(strlen(ifr_basename) + 1, "Interface update 3");
  strcpy(ifp->int_name, ifr_basename);
  ifp->gen_properties = NULL;
  ifp->int_next = ifnet;
  ifnet = ifp;

  if (olsr_cnf->ip_version == AF_INET) {
    /* IP version 4 */
    ifp->ip_addr.v4 = ifp->int_addr.sin_addr;
    /*
     * We create one socket for each interface and bind
     * the socket to it. This to ensure that we can control
     * on what interface the message is transmitted
     */
    ifp->olsr_socket = getsocket(BUFSPACE, ifp->int_name);
    if (ifp->olsr_socket < 0) {
      fprintf(stderr, "Could not initialize socket... exiting!\n\n");
      olsr_syslog(OLSR_LOG_ERR, "Could not initialize socket... exiting!\n\n");
      olsr_cnf->exit_value = EXIT_FAILURE;
      kill(getpid(), SIGINT);
    }
  } else {
    /* IP version 6 */
    ifp->ip_addr.v6 =  ifp->int6_addr.sin6_addr;

    /*
     * We create one socket for each interface and bind
     * the socket to it. This to ensure that we can control
     * on what interface the message is transmitted
     */
    ifp->olsr_socket = getsocket6(BUFSPACE, ifp->int_name);
    if (ifp->olsr_socket < 0)	{
      fprintf(stderr, "Could not initialize socket... exiting!\n\n");
      olsr_syslog(OLSR_LOG_ERR, "Could not initialize socket... exiting!\n\n");
      olsr_cnf->exit_value = EXIT_FAILURE;
      kill(getpid(), SIGINT);
    }
    join_mcast(ifp, ifp->olsr_socket);
  }

  set_buffer_timer(ifp);

  /* Register socket */
  add_olsr_socket(ifp->olsr_socket, &olsr_input, NULL, NULL, SP_PR_READ);
  
#ifdef linux
  {
    /* Set TOS */
    int data = IPTOS_PREC(olsr_cnf->tos);
    if (setsockopt(ifp->olsr_socket, SOL_SOCKET, SO_PRIORITY, (char*)&data, sizeof(data)) < 0) {
      perror("setsockopt(SO_PRIORITY)");
      olsr_syslog(OLSR_LOG_ERR, "OLSRD: setsockopt(SO_PRIORITY) error %m");
    }
    data = IPTOS_TOS(olsr_cnf->tos);
    if (setsockopt(ifp->olsr_socket, SOL_IP, IP_TOS, (char*)&data, sizeof(data)) < 0) {
      perror("setsockopt(IP_TOS)");
      olsr_syslog(OLSR_LOG_ERR, "setsockopt(IP_TOS) error %m");
    }
  }
#endif

  /*
   *Initialize sequencenumber as a random 16bit value
   */
  ifp->olsr_seqnum = random() & 0xFFFF;

  /*
   * Set main address if this is the only interface
   */
  if (ipequal(&all_zero, &olsr_cnf->main_addr)) {
    struct ipaddr_str buf;
    olsr_cnf->main_addr = ifp->ip_addr;
    OLSR_PRINTF(1, "New main address: %s\n", olsr_ip_to_string(&buf, &olsr_cnf->main_addr));
    olsr_syslog(OLSR_LOG_INFO, "New main address: %s\n", olsr_ip_to_string(&buf, &olsr_cnf->main_addr));
  }

  /*
   * Register functions for periodic message generation
   */
  ifp->hello_gen_timer =
    olsr_start_timer(iface->cnf->hello_params.emission_interval * MSEC_PER_SEC,
                     HELLO_JITTER, OLSR_TIMER_PERIODIC,
                     olsr_cnf->lq_level == 0 ? &generate_hello : &olsr_output_lq_hello,
                     ifp, hello_gen_timer_cookie->ci_id);
  ifp->tc_gen_timer =
    olsr_start_timer(iface->cnf->tc_params.emission_interval * MSEC_PER_SEC,
                     TC_JITTER, OLSR_TIMER_PERIODIC,
                     olsr_cnf->lq_level == 0 ? &generate_tc : &olsr_output_lq_tc,
                     ifp, tc_gen_timer_cookie->ci_id);
  ifp->mid_gen_timer =
    olsr_start_timer(iface->cnf->mid_params.emission_interval * MSEC_PER_SEC,
                     MID_JITTER, OLSR_TIMER_PERIODIC, &generate_mid, ifp,
                     mid_gen_timer_cookie->ci_id);
  ifp->hna_gen_timer =
    olsr_start_timer(iface->cnf->hna_params.emission_interval * MSEC_PER_SEC,
                     HNA_JITTER, OLSR_TIMER_PERIODIC, &generate_hna, ifp,
                     hna_gen_timer_cookie->ci_id);

  /* Recalculate max topology hold time */
  if (olsr_cnf->max_tc_vtime < iface->cnf->tc_params.emission_interval) {
    olsr_cnf->max_tc_vtime = iface->cnf->tc_params.emission_interval;
  }
  ifp->hello_etime = (olsr_reltime)(iface->cnf->hello_params.emission_interval * MSEC_PER_SEC);
  ifp->valtimes.hello = reltime_to_me(iface->cnf->hello_params.validity_time * MSEC_PER_SEC);
  ifp->valtimes.tc = reltime_to_me(iface->cnf->tc_params.validity_time * MSEC_PER_SEC);
  ifp->valtimes.mid = reltime_to_me(iface->cnf->mid_params.validity_time * MSEC_PER_SEC);
  ifp->valtimes.hna = reltime_to_me(iface->cnf->hna_params.validity_time * MSEC_PER_SEC);

  /*
   * Call possible ifchange functions registered by plugins
   */
  run_ifchg_cbs(ifp, IFCHG_IF_ADD);

  return 1;
}

/*
 * Local Variables:
 * c-basic-offset: 2
 * End:
 */
