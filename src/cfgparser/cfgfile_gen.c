/*
 * The olsr.org Optimized Link-State Routing daemon(olsrd)
 * Copyright (c) 2005, Andreas Tønnesen(andreto@olsr.org)
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

#include "olsrd_conf.h"
#include "../ipcalc.h"
#include "../net_olsr.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int
olsrd_write_cnf(const struct olsrd_config *cnf, const char *fname)
{
  char buf[10000];
  FILE *fd = fopen(fname, "w");
  if(fd == NULL) {
    fprintf(stderr, "Could not open file %s for writing\n%s\n", fname, strerror(errno));
    return -1;
  }

  printf("Writing config to file \"%s\".... ", fname);

  olsrd_write_cnf_buf(cnf, OLSR_FALSE, buf, sizeof(buf));
  fputs(buf, fd);

  fclose(fd);
  printf("DONE\n");

  return 1;
}

#define MAX_LINESIZE 250

#define WRITE_TO_BUF(fmt, args...)                                      \
    do {                                                                \
        if((bufsize - size) < MAX_LINESIZE) {                           \
            return -1;                                                  \
        }                                                               \
        size += snprintf(&buf[size], MAX_LINESIZE, fmt, ##args);        \
    } while (0)

int
olsrd_write_cnf_buf(const struct olsrd_config *cnf, olsr_bool write_more_comments, char *buf, olsr_u32_t bufsize)
{
  struct ip_prefix_list   *h  = cnf->hna_entries;
  struct olsr_if           *in = cnf->interfaces;
  struct plugin_entry      *pe = cnf->plugins;
  struct plugin_param      *pp;
  struct ip_prefix_list    *ie = cnf->ipc_nets;
  struct olsr_lq_mult      *mult;

  int size = 0;

  char ipv6_buf[100];             /* buffer for IPv6 inet_htop */

  if (buf == NULL || bufsize < MAX_LINESIZE) {
      return -1;
  }

  WRITE_TO_BUF("#\n# Configuration file for %s\n# automatically generated by olsrd-cnf parser v.  %s\n#\n\n", olsrd_version, PARSER_VERSION);

  /* Debug level */
  WRITE_TO_BUF("# Debug level(0-9)\n# If set to 0 the daemon runs in the background\n\nDebugLevel\t%d\n\n", cnf->debug_level);

  /* IP version */
  WRITE_TO_BUF("# IP version to use (4 or 6)\n\nIpVersion\t%d\n\n", cnf->ip_version == AF_INET ? 4 : 6);

  /* FIB Metric */
  WRITE_TO_BUF("# FIBMetric (\"%s\", \"%s\", or \"%s\")\n\nFIBMetric\t\"%s\"\n\n", CFG_FIBM_FLAT, CFG_FIBM_CORRECT, CFG_FIBM_APPROX,
    FIBM_FLAT == cnf->fib_metric ? CFG_FIBM_FLAT : FIBM_CORRECT == cnf->fib_metric ? CFG_FIBM_CORRECT : CFG_FIBM_APPROX);

  /* HNA IPv4/IPv6 */
  WRITE_TO_BUF("# HNA IPv%d routes\n# syntax: netaddr/prefix\n\nHna%d {\n", cnf->ip_version == AF_INET ? 4 : 6, cnf->ip_version == AF_INET ? 4 : 6);
  while(h) {
    struct ipprefix_str strbuf;
    WRITE_TO_BUF("    %s\n", olsr_ip_prefix_to_string(&strbuf, &h->net));
    h = h->next;
  }
  WRITE_TO_BUF("}\n\n");

  /* No interfaces */
  WRITE_TO_BUF("# Should olsrd keep on running even if there are\n# no interfaces available? This is a good idea\n# for a PCMCIA/USB hotswap environment.\n# \"yes\" OR \"no\"\n\nAllowNoInt\t");
  if(cnf->allow_no_interfaces)
    WRITE_TO_BUF("yes\n\n");
  else
    WRITE_TO_BUF("no\n\n");

  /* TOS */
  WRITE_TO_BUF("# TOS(type of service) to use. Default is 16\n\n");
  WRITE_TO_BUF("TosValue\t%d\n\n", cnf->tos);

  /* RtTable */
  WRITE_TO_BUF("# Policy Routing Table to use. Default is 254\n\n");
  WRITE_TO_BUF("RtTable\t\t%d\n\n", cnf->rttable);

  /* RtTableDefault */
  WRITE_TO_BUF("# Policy Routing Table to use for the default Route. Default is 0 (Take the same table as specified by RtTable)\n\n");
  WRITE_TO_BUF("RtTableDefault\t\t%d\n\n", cnf->rttable_default);

  /* Willingness */
  WRITE_TO_BUF("# The fixed willingness to use(0-7)\n# If not set willingness will be calculated\n# dynammically based on battery/power status\n\n");
  if(cnf->willingness_auto)
    WRITE_TO_BUF("#Willingness\t4\n\n");
  else
    WRITE_TO_BUF("Willingness\t%d\n\n", cnf->willingness);

  /* IPC */
  WRITE_TO_BUF("# Allow processes like the GUI front-end\n# to connect to the daemon.\n\n");
  WRITE_TO_BUF("IpcConnect {\n");
  WRITE_TO_BUF("    MaxConnections\t%d\n", cnf->ipc_connections);

  while (ie) {
    if (ie->net.prefix_len == olsr_cnf->maxplen) {
      struct ipaddr_str strbuf;
      WRITE_TO_BUF("    Host\t\t%s\n", olsr_ip_to_string(&strbuf, &ie->net.prefix));
    } else {
      struct ipprefix_str strbuf;
      WRITE_TO_BUF("    Net\t\t\t%s\n", olsr_ip_prefix_to_string(&strbuf, &ie->net));
    }
    ie = ie->next;
  }

  WRITE_TO_BUF("}\n\n");

  /* Hysteresis */
  WRITE_TO_BUF("# Hysteresis adds more robustness to the\n# link sensing.\n# Used by default. 'yes' or 'no'\n\n");

  if(cnf->use_hysteresis)
    {
      WRITE_TO_BUF("UseHysteresis\tyes\n\n");
      WRITE_TO_BUF("# Hysteresis parameters\n# Do not alter these unless you know \n# what you are doing!\n# Set to auto by default. Allowed\n# values are floating point values\n# in the interval 0,1\n# THR_LOW must always be lower than\n# THR_HIGH!!\n\n");
      WRITE_TO_BUF("HystScaling\t%0.2f\n", cnf->hysteresis_param.scaling);
      WRITE_TO_BUF("HystThrHigh\t%0.2f\n", cnf->hysteresis_param.thr_high);
      WRITE_TO_BUF("HystThrLow\t%0.2f\n\n", cnf->hysteresis_param.thr_low);
    }
  else
    {
      WRITE_TO_BUF("UseHysteresis\tno\n\n");
      WRITE_TO_BUF("# Hysteresis parameters\n# Do not alter these unless you know \n# what you are doing!\n# Set to auto by default. Allowed\n# values are floating point values\n# in the interval 0,1\n# THR_LOW must always be lower than\n# THR_HIGH!!\n\n");
      WRITE_TO_BUF("#HystScaling\t%0.2f\n", cnf->hysteresis_param.scaling);
      WRITE_TO_BUF("#HystThrHigh\t%0.2f\n", cnf->hysteresis_param.thr_high);
      WRITE_TO_BUF("#HystThrLow\t%0.2f\n\n", cnf->hysteresis_param.thr_low);
    }

  /* Pollrate */
  WRITE_TO_BUF("# Polling rate in seconds(float).\n# Auto uses default value 0.05 sec\n\n");
  WRITE_TO_BUF("Pollrate\t%0.2f\n", cnf->pollrate);

  /* NIC Changes Pollrate */
  WRITE_TO_BUF("# Interval to poll network interfaces for configuration\n# changes. Defaults to 2.5 seconds\n");
  WRITE_TO_BUF("NicChgsPollInt\t%0.2f\n", cnf->nic_chgs_pollrate);

  /* TC redundancy */
  WRITE_TO_BUF("# TC redundancy\n# Specifies how much neighbor info should\n# be sent in TC messages\n# Possible values are:\n# 0 - only send MPR selectors\n# 1 - send MPR selectors and MPRs\n# 2 - send all neighbors\n#\n# defaults to 0\n\n");
  WRITE_TO_BUF("TcRedundancy\t%d\n\n", cnf->tc_redundancy);

  /* MPR coverage */
  WRITE_TO_BUF("# MPR coverage\n# Specifies how many MPRs a node should\n# try select to reach every 2 hop neighbor\n# Can be set to any integer >0\n# defaults to 1\n\n");

  WRITE_TO_BUF("MprCoverage\t%d\n\n", cnf->mpr_coverage);

  WRITE_TO_BUF("# Link quality level\n# 0 = do not use link quality\n# 1 = use link quality for MPR selection\n# 2 = use link quality for MPR selection and routing\n\n");
  WRITE_TO_BUF("LinkQualityLevel\t%d\n\n", cnf->lq_level);

  WRITE_TO_BUF("# Fish Eye algorithm\n# 0 = do not use fish eye\n# 1 = use fish eye\n\n");
  WRITE_TO_BUF("LinkQualityFishEye\t%d\n\n", cnf->lq_fish);

  if (NULL != cnf->lq_algorithm)
  {
    WRITE_TO_BUF("# Link quality algorithm (if LinkQualityLevel > 0)\n# etx_fpm (hello loss, fixed point math)\n# etx_float (hello loss, floating point)\n# etx_ff (packet loss for freifunk compat)\n\n");
    WRITE_TO_BUF("LinkQualityAlgorithm\t\"%s\"\n\n", cnf->lq_algorithm);
  }

  WRITE_TO_BUF("# Link quality aging factor\n\n");
  WRITE_TO_BUF("LinkQualityAging\t%f\n\n", cnf->lq_aging);

  WRITE_TO_BUF("# NAT threshold\n\n");
  WRITE_TO_BUF("NatThreshold\t%f\n\n", cnf->lq_nat_thresh);

  WRITE_TO_BUF("# Clear screen when printing debug output?\n\n");
  WRITE_TO_BUF("ClearScreen\t%s\n\n", cnf->clear_screen ? "yes" : "no");

  /* Plugins */
  WRITE_TO_BUF("# Olsrd plugins to load\n# This must be the absolute path to the file\n# or the loader will use the following scheme:\n");
  WRITE_TO_BUF("# - Try the paths in the LD_LIBRARY_PATH \n#   environment variable.\n# - The list of libraries cached in /etc/ld.so.cache\n# - /lib, followed by /usr/lib\n\n");
  if(pe)
    {
      while(pe)
	{
	  WRITE_TO_BUF("LoadPlugin \"%s\" {\n", pe->name);
          pp = pe->params;
          while(pp)
            {
              WRITE_TO_BUF("    PlParam \"%s\"\t\"%s\"\n", pp->key, pp->value);
              pp = pp->next;
            }
	  WRITE_TO_BUF("}\n");
	  pe = pe->next;
	}
    }
  WRITE_TO_BUF("\n");

  
  

  /* Interfaces */
  WRITE_TO_BUF("# Interfaces\n# Multiple interfaces with the same configuration\n# can shar the same config block. Just list the\n# interfaces(e.g. Interface \"eth0\" \"eth2\"\n\n");
  /* Interfaces */
  if(in)
    {
      olsr_bool first = write_more_comments;
      while(in)
	{
	  WRITE_TO_BUF("Interface \"%s\" {\n", in->name);

          if(first)
	     WRITE_TO_BUF("    # IPv4 broadcast address to use. The\n    # one usefull example would be 255.255.255.255\n    # If not defined the broadcastaddress\n    # every card is configured with is used\n\n");


	  if(in->cnf->ipv4_broadcast.v4.s_addr)
	    {
	      WRITE_TO_BUF("    Ip4Broadcast\t%s\n", inet_ntoa(in->cnf->ipv4_broadcast.v4));
	    }
	  else
	    {
	      if(first)
    	        WRITE_TO_BUF("    #Ip4Broadcast\t255.255.255.255\n");
	    }
	  
          if(first) WRITE_TO_BUF("\n");

          if(first)
	      WRITE_TO_BUF("    # IPv6 address scope to use.\n    # Must be 'site-local' or 'global'\n\n");
	  if(in->cnf->ipv6_addrtype)
	    WRITE_TO_BUF("    Ip6AddrType \tsite-local\n");
	  else
	    WRITE_TO_BUF("    Ip6AddrType \tglobal\n");

          if(first) WRITE_TO_BUF("\n");

          if(first)
	    WRITE_TO_BUF("    # IPv6 multicast address to use when\n    # using site-local addresses.\n    # If not defined, ff05::15 is used\n");
	  WRITE_TO_BUF("    Ip6MulticastSite\t%s\n", inet_ntop(AF_INET6, &in->cnf->ipv6_multi_site.v6, ipv6_buf, sizeof(ipv6_buf)));
          if(first) WRITE_TO_BUF("\n");
          if(first)
	    WRITE_TO_BUF("    # IPv6 multicast address to use when\n    # using global addresses\n    # If not defined, ff0e::1 is used\n");
	  WRITE_TO_BUF("    Ip6MulticastGlobal\t%s\n", inet_ntop(AF_INET6, &in->cnf->ipv6_multi_glbl.v6, ipv6_buf, sizeof(ipv6_buf)));
          if(first) WRITE_TO_BUF("\n");
  
	  
          WRITE_TO_BUF("    # Olsrd can autodetect changes in\n    # interface configurations. Enabled by default\n    # turn off to save CPU.\n    AutoDetectChanges: %s\n", in->cnf->autodetect_chg ? "yes" : "no");

          if(first)
            WRITE_TO_BUF("    # Emission and validity intervals.\n    # If not defined, RFC proposed values will\n    # in most cases be used.\n\n");
	  
	  
	  if(in->cnf->hello_params.emission_interval != HELLO_INTERVAL)
	    WRITE_TO_BUF("    HelloInterval\t%0.2f\n", in->cnf->hello_params.emission_interval);
          else if(first)
	    WRITE_TO_BUF("    #HelloInterval\t%0.2f\n", in->cnf->hello_params.emission_interval);
	  if(in->cnf->hello_params.validity_time != NEIGHB_HOLD_TIME)
	    WRITE_TO_BUF("    HelloValidityTime\t%0.2f\n", in->cnf->hello_params.validity_time);
          else if(first)
	    WRITE_TO_BUF("    #HelloValidityTime\t%0.2f\n", in->cnf->hello_params.validity_time);
	  if(in->cnf->tc_params.emission_interval != TC_INTERVAL)
	    WRITE_TO_BUF("    TcInterval\t\t%0.2f\n", in->cnf->tc_params.emission_interval);
          else if(first)
	    WRITE_TO_BUF("    #TcInterval\t\t%0.2f\n", in->cnf->tc_params.emission_interval);
	  if(in->cnf->tc_params.validity_time != TOP_HOLD_TIME)
	    WRITE_TO_BUF("    TcValidityTime\t%0.2f\n", in->cnf->tc_params.validity_time);
          else if(first)
	    WRITE_TO_BUF("    #TcValidityTime\t%0.2f\n", in->cnf->tc_params.validity_time);
	  if(in->cnf->mid_params.emission_interval != MID_INTERVAL)
	    WRITE_TO_BUF("    MidInterval\t\t%0.2f\n", in->cnf->mid_params.emission_interval);
          else if(first)
	    WRITE_TO_BUF("    #MidInterval\t%0.2f\n", in->cnf->mid_params.emission_interval);
	  if(in->cnf->mid_params.validity_time != MID_HOLD_TIME)
	    WRITE_TO_BUF("    MidValidityTime\t%0.2f\n", in->cnf->mid_params.validity_time);
          else if(first)
	    WRITE_TO_BUF("    #MidValidityTime\t%0.2f\n", in->cnf->mid_params.validity_time);
	  if(in->cnf->hna_params.emission_interval != HNA_INTERVAL)
	    WRITE_TO_BUF("    HnaInterval\t\t%0.2f\n", in->cnf->hna_params.emission_interval);
          else if(first)
	    WRITE_TO_BUF("    #HnaInterval\t%0.2f\n", in->cnf->hna_params.emission_interval);
	  if(in->cnf->hna_params.validity_time != HNA_HOLD_TIME)
	    WRITE_TO_BUF("    HnaValidityTime\t%0.2f\n", in->cnf->hna_params.validity_time);	  
          else if(first)
	    WRITE_TO_BUF("    #HnaValidityTime\t%0.2f\n", in->cnf->hna_params.validity_time);	  
	  
          mult = in->cnf->lq_mult;

          if (mult == NULL)
	    {
              if(first)
	        WRITE_TO_BUF("    #LinkQualityMult\tdefault 1.0\n");
	    }
          else
	    {
	      while (mult != NULL)
		{
		  WRITE_TO_BUF("    LinkQualityMult\t%s %0.2f\n", inet_ntop(cnf->ip_version, &mult->addr, ipv6_buf, sizeof(ipv6_buf)), (float)(mult->value) / 65536.0);
		  mult = mult->next;
		}
	    }

	  if(first)
	    {
             WRITE_TO_BUF("    # When multiple links exist between hosts\n");
             WRITE_TO_BUF("    # the weight of interface is used to determine\n");
             WRITE_TO_BUF("    # the link to use. Normally the weight is\n");
             WRITE_TO_BUF("    # automatically calculated by olsrd based\n");
             WRITE_TO_BUF("    # on the characteristics of the interface,\n");
             WRITE_TO_BUF("    # but here you can specify a fixed value.\n");
             WRITE_TO_BUF("    # Olsrd will choose links with the lowest value.\n");
             WRITE_TO_BUF("    # Note:\n");
             WRITE_TO_BUF("    # Interface weight is used only when LinkQualityLevel is 0.\n");
             WRITE_TO_BUF("    # For any other value of LinkQualityLevel, the interface ETX\n");
             WRITE_TO_BUF("    # value is used instead.\n\n");
            }
	  if(in->cnf->weight.fixed)
	    {
	      WRITE_TO_BUF("    Weight\t %d\n\n", in->cnf->weight.value);
	    }
	  else
	    {
              if(first)
       	        WRITE_TO_BUF("    #Weight\t 0\n\n");
	    }

	  
	  WRITE_TO_BUF("}\n\n");
	  in = in->next;
	  first = OLSR_FALSE;
	}

    }


  WRITE_TO_BUF("\n# END AUTOGENERATED CONFIG\n");

  return size;
}

/*
 * Local Variables:
 * c-basic-offset: 2
 * End:
 */
