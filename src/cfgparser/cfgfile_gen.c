/*
 * The olsr.org Optimized Link-State Routing daemon(olsrd)
 * Copyright (c) 2005, Andreas T�nnesen(andreto@olsr.org)
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
 * $Id: cfgfile_gen.c,v 1.1 2005/03/14 21:24:22 kattemat Exp $
 */


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "olsrd_conf.h"


int
olsrd_write_cnf(struct olsrd_config *cnf, const char *fname)
{
  struct hna4_entry        *h4 = cnf->hna4_entries;
  struct hna6_entry        *h6 = cnf->hna6_entries;
  struct olsr_if           *in = cnf->interfaces;
  struct plugin_entry      *pe = cnf->plugins;
  struct plugin_param      *pp;
  struct ipc_host          *ih = cnf->ipc_hosts;
  struct ipc_net           *ie = cnf->ipc_nets;
  struct olsr_lq_mult      *mult;

  char ipv6_buf[100];             /* buffer for IPv6 inet_htop */
  struct in_addr in4;

  FILE *fd;

  fd = fopen(fname, "w");

  if(fd == NULL)
    {
      fprintf(stderr, "Could not open file %s for writing\n%s\n", fname, strerror(errno));
      return -1;
    }

  printf("Writing config to file \"%s\".... ", fname);

  fprintf(fd, "#\n# Configuration file for olsr.org olsrd\n# automatically generated by olsrd-cnf %s\n#\n\n\n", PARSER_VERSION);

  /* Debug level */
  fprintf(fd, "# Debug level(0-9)\n# If set to 0 the daemon runs in the background\n\nDebugLevel\t%d\n\n", cnf->debug_level);

  /* IP version */
  if(cnf->ip_version == AF_INET6)
    fprintf(fd, "# IP version to use (4 or 6)\n\nIpVersion\t6\n\n");
  else
    fprintf(fd, "# IP version to use (4 or 6)\n\nIpVersion\t4\n\n");


  /* HNA IPv4 */
  fprintf(fd, "# HNA IPv4 routes\n# syntax: netaddr netmask\n\nHna4\n{\n");
  while(h4)
    {
      in4.s_addr = h4->net.v4;
      fprintf(fd, "    %s ", inet_ntoa(in4));
      in4.s_addr = h4->netmask.v4;
      fprintf(fd, "%s\n", inet_ntoa(in4));
      h4 = h4->next;
    }
  fprintf(fd, "}\n\n");


  /* HNA IPv6 */
  fprintf(fd, "# HNA IPv6 routes\n# syntax: netaddr prefix\n\nHna6\n{\n");
  while(h6)
    {
      fprintf(fd, "    %s/%d\n", (char *)inet_ntop(AF_INET6, &h6->net.v6, ipv6_buf, sizeof(ipv6_buf)), h6->prefix_len);
      h6 = h6->next;
    }

  fprintf(fd, "}\n\n");

  /* No interfaces */
  fprintf(fd, "# Should olsrd keep on running even if there are\n# no interfaces available? This is a good idea\n# for a PCMCIA/USB hotswap environment.\n# \"yes\" OR \"no\"\n\nAllowNoInt\t");
  if(cnf->allow_no_interfaces)
    fprintf(fd, "yes\n\n");
  else
    fprintf(fd, "no\n\n");

  /* TOS */
  fprintf(fd, "# TOS(type of service) to use. Default is 16\n\n");
  fprintf(fd, "TosValue\t%d\n\n", cnf->tos);

  /* Willingness */
  fprintf(fd, "# The fixed willingness to use(0-7)\n# If not set willingness will be calculated\n# dynammically based on battery/power status\n\n");
  if(cnf->willingness_auto)
    fprintf(fd, "#Willingness\t4\n\n");
  else
    fprintf(fd, "Willingness%d\n\n", cnf->willingness);

  /* IPC */
  fprintf(fd, "# Allow processes like the GUI front-end\n# to connect to the daemon.\n\n");
  fprintf(fd, "IpcConnect\n{\n");
  fprintf(fd, "   MaxConnections  %d\n", cnf->ipc_connections);

  while(ih)
    {
      in4.s_addr = ih->host.v4;
      fprintf(fd, "   Host          %s\n", inet_ntoa(in4));
      ih = ih->next;
    }
  while(ie)
    {
      in4.s_addr = ie->net.v4;
      fprintf(fd, "   Net           %s ", inet_ntoa(in4));
      in4.s_addr = ie->mask.v4;
      fprintf(fd, "%s\n", inet_ntoa(in4));
      ie = ie->next;
    }

  fprintf(fd, "}\n\n");



  /* Hysteresis */
  fprintf(fd, "# Hysteresis adds more robustness to the\n# link sensing.\n# Used by default. 'yes' or 'no'\n\n");

  if(cnf->use_hysteresis)
    {
      fprintf(fd, "UseHysteresis\tyes\n\n");
      fprintf(fd, "# Hysteresis parameters\n# Do not alter these unless you know \n# what you are doing!\n# Set to auto by default. Allowed\n# values are floating point values\n# in the interval 0,1\n# THR_LOW must always be lower than\n# THR_HIGH!!\n\n");
      fprintf(fd, "HystScaling\t%0.2f\n", cnf->hysteresis_param.scaling);
      fprintf(fd, "HystThrHigh\t%0.2f\n", cnf->hysteresis_param.thr_high);
      fprintf(fd, "HystThrLow\t%0.2f\n\n", cnf->hysteresis_param.thr_low);
    }
  else
    {
      fprintf(fd, "UseHysteresis\tno\n\n");
      fprintf(fd, "# Hysteresis parameters\n# Do not alter these unless you know \n# what you are doing!\n# Set to auto by default. Allowed\n# values are floating point values\n# in the interval 0,1\n# THR_LOW must always be lower than\n# THR_HIGH!!\n\n");
      fprintf(fd, "#HystScaling\t%0.2f\n", cnf->hysteresis_param.scaling);
      fprintf(fd, "#HystThrHigh\t%0.2f\n", cnf->hysteresis_param.thr_high);
      fprintf(fd, "#HystThrLow\t%0.2f\n\n", cnf->hysteresis_param.thr_low);
    }

  /* Pollrate */
  fprintf(fd, "# Polling rate in seconds(float).\n# Auto uses default value 0.05 sec\n\n");
  fprintf(fd, "Pollrate\t%0.2f\n", cnf->pollrate);

  /* TC redundancy */
  fprintf(fd, "# TC redundancy\n# Specifies how much neighbor info should\n# be sent in TC messages\n# Possible values are:\n# 0 - only send MPR selectors\n# 1 - send MPR selectors and MPRs\n# 2 - send all neighbors\n#\n# defaults to 0\n\n");
  fprintf(fd, "TcRedundancy\t%d\n\n", cnf->tc_redundancy);

  /* MPR coverage */
  fprintf(fd, "# MPR coverage\n# Specifies how many MPRs a node should\n# try select to reach every 2 hop neighbor\n# Can be set to any integer >0\n# defaults to 1\n\n");

  fprintf(fd, "MprCoverage\t%d\n\n", cnf->mpr_coverage);

  fprintf(fd, "# Link quality level\n# 0 = do not use link quality\n# 1 = use link quality for MPR selection\n# 2 = use link quality for MPR selection and routing\n\n");
  fprintf(fd, "LinkQualityLevel\t%d\n\n", cnf->lq_level);

  fprintf(fd, "# Link quality window size\n\n");
  fprintf(fd, "LinkQualityWinSize\t%d\n\n", cnf->lq_wsize);

  fprintf(fd, "# Clear screen when printing debug output?\n\n");
  fprintf(fd, "ClearScreen\t%s\n\n", cnf->clear_screen ? "yes" : "no");

  /* Plugins */
  fprintf(fd, "# Olsrd plugins to load\n# This must be the absolute path to the file\n# or the loader will use the following scheme:\n# - Try the paths in the LD_LIBRARY_PATH \n#   environment variable.\n# - The list of libraries cached in /etc/ld.so.cache\n# - /lib, followed by /usr/lib\n\n");
  if(pe)
    {
      while(pe)
	{
	  fprintf(fd, "LoadPlugin \"%s\"\n{\n", pe->name);
          pp = pe->params;
          while(pp)
            {
              fprintf(fd, "    PlParam \"%s\" \"%s\"\n", pp->key, pp->value);
              pp = pp->next;
            }
	  fprintf(fd, "}\n");
	  pe = pe->next;
	}
    }
  fprintf(fd, "\n");

  
  

  /* Interfaces */
  fprintf(fd, "# Interfaces\n# Multiple interfaces with the same configuration\n# can shar the same config block. Just list the\n# interfaces(e.g. Interface \"eth0\" \"eth2\"\n\n");
  /* Interfaces */
  if(in)
    {
      while(in)
	{
	  fprintf(fd, "Interface \"%s\"\n{\n", in->name);
	  fprintf(fd, "\n");
      
	  fprintf(fd, "    # IPv4 broadcast address to use. The\n    # one usefull example would be 255.255.255.255\n    # If not defined the broadcastaddress\n    # every card is configured with is used\n\n");


	  if(in->cnf->ipv4_broadcast.v4)
	    {
	      in4.s_addr = in->cnf->ipv4_broadcast.v4;
	      fprintf(fd, "    Ip4Broadcast\t %s\n\n", inet_ntoa(in4));
	    }
	  else
	    {
	      fprintf(fd, "    #Ip4Broadcast\t255.255.255.255\n\n");
	    }
	  
	  
	  fprintf(fd, "    # IPv6 address scope to use.\n    # Must be 'site-local' or 'global'\n\n");
	  if(in->cnf->ipv6_addrtype)
	    fprintf(fd, "    Ip6AddrType \tsite-local\n\n");
	  else
	    fprintf(fd, "    Ip6AddrType \tglobal\n\n");
	  
	  fprintf(fd, "    # IPv6 multicast address to use when\n    # using site-local addresses.\n    # If not defined, ff05::15 is used\n");
	  fprintf(fd, "    Ip6MulticastSite\t%s\n\n", (char *)inet_ntop(AF_INET6, &in->cnf->ipv6_multi_site.v6, ipv6_buf, sizeof(ipv6_buf)));
	  fprintf(fd, "    # IPv6 multicast address to use when\n    # using global addresses\n    # If not defined, ff0e::1 is used\n");
	  fprintf(fd, "    Ip6MulticastGlobal\t%s\n\n", (char *)inet_ntop(AF_INET6, &in->cnf->ipv6_multi_glbl.v6, ipv6_buf, sizeof(ipv6_buf)));
	  
	  
	  
	  fprintf(fd, "    # Emission and validity intervals.\n    # If not defined, RFC proposed values will\n    # in most cases be used.\n\n");
	  
	  
	  if(in->cnf->hello_params.emission_interval != HELLO_INTERVAL)
	    fprintf(fd, "    HelloInterval\t%0.2f\n", in->cnf->hello_params.emission_interval);
	  else
	    fprintf(fd, "    #HelloInterval\t%0.2f\n", in->cnf->hello_params.emission_interval);
	  if(in->cnf->hello_params.validity_time != NEIGHB_HOLD_TIME)
	    fprintf(fd, "    HelloValidityTime\t%0.2f\n", in->cnf->hello_params.validity_time);
	  else
	    fprintf(fd, "    #HelloValidityTime\t%0.2f\n", in->cnf->hello_params.validity_time);
	  if(in->cnf->tc_params.emission_interval != TC_INTERVAL)
	    fprintf(fd, "    TcInterval\t\t%0.2f\n", in->cnf->tc_params.emission_interval);
	  else
	    fprintf(fd, "    #TcInterval\t\t%0.2f\n", in->cnf->tc_params.emission_interval);
	  if(in->cnf->tc_params.validity_time != TOP_HOLD_TIME)
	    fprintf(fd, "    TcValidityTime\t%0.2f\n", in->cnf->tc_params.validity_time);
	  else
	    fprintf(fd, "    #TcValidityTime\t%0.2f\n", in->cnf->tc_params.validity_time);
	  if(in->cnf->mid_params.emission_interval != MID_INTERVAL)
	    fprintf(fd, "    MidInterval\t\t%0.2f\n", in->cnf->mid_params.emission_interval);
	  else
	    fprintf(fd, "    #MidInterval\t%0.2f\n", in->cnf->mid_params.emission_interval);
	  if(in->cnf->mid_params.validity_time != MID_HOLD_TIME)
	    fprintf(fd, "    MidValidityTime\t%0.2f\n", in->cnf->mid_params.validity_time);
	  else
	    fprintf(fd, "    #MidValidityTime\t%0.2f\n", in->cnf->mid_params.validity_time);
	  if(in->cnf->hna_params.emission_interval != HNA_INTERVAL)
	    fprintf(fd, "    HnaInterval\t\t%0.2f\n", in->cnf->hna_params.emission_interval);
	  else
	    fprintf(fd, "    #HnaInterval\t%0.2f\n", in->cnf->hna_params.emission_interval);
	  if(in->cnf->hna_params.validity_time != HNA_HOLD_TIME)
	    fprintf(fd, "    HnaValidityTime\t%0.2f\n", in->cnf->hna_params.validity_time);	  
	  else
	    fprintf(fd, "    #HnaValidityTime\t%0.2f\n", in->cnf->hna_params.validity_time);	  
	  
          mult = in->cnf->lq_mult;

          if (mult == NULL)
	    {
	      fprintf(fd, "    #LinkQualityMult\tdefault 1.0\n");
	    }
          else
	    {
	      while (mult != NULL)
		{
		  inet_ntop(cnf->ip_version, &mult->addr, ipv6_buf,
			    sizeof (ipv6_buf));
		  
		  fprintf(fd, "    LinkQualityMult\t%s %0.2f\n",
			  ipv6_buf, mult->val);
		  
		  mult = mult->next;
		}
	    }

	  fprintf(fd, "    # When multiple links exist between hosts\n    # the weight of interface is used to determine\n    # the link to use. Normally the weight is\n    # automatically calculated by olsrd based\n    # on the characteristics of the interface,\n    # but here you can specify a fixed value.\n    # Olsrd will choose links with the lowest value.\n");
	  if(in->cnf->weight.fixed)
	    {
	      fprintf(fd, "    Weight\t %d\n\n", in->cnf->weight.value);
	    }
	  else
	    {
	      fprintf(fd, "    #Weight\t 0\n\n");
	    }

	  
	  fprintf(fd, "}\n\n");
	  in = in->next;
	}

    }


  fprintf(fd, "\n# END AUTOGENERATED CONFIG\n");

  fclose(fd);
  printf("DONE\n");

  return 1;
}

#define MAX_LINESIZE 250

#define WRITE_TO_BUF(s,args...) \
        { \
        if((bufsize - size) < MAX_LINESIZE) \
            return -1; \
        size += snprintf(&buf[size], MAX_LINESIZE, s, ##args);\
        }

int
olsrd_write_cnf_buf(struct olsrd_config *cnf, char *buf, olsr_u32_t bufsize)
{
  struct hna4_entry        *h4 = cnf->hna4_entries;
  struct hna6_entry        *h6 = cnf->hna6_entries;
  struct olsr_if           *in = cnf->interfaces;
  struct plugin_entry      *pe = cnf->plugins;
  struct plugin_param      *pp;
  struct ipc_host          *ih = cnf->ipc_hosts;
  struct ipc_net           *ie = cnf->ipc_nets;
  struct olsr_lq_mult      *mult;

  int size = 0;

  char ipv6_buf[100];             /* buffer for IPv6 inet_htop */
  struct in_addr in4;

  if(buf == NULL || bufsize < MAX_LINESIZE)
    {
      return -1;
    }

  WRITE_TO_BUF("#\n# Configuration file for olsr.org olsrd\n# automatically generated by olsrd-cnf %s\n#\n\n\n", PARSER_VERSION)

  /* Debug level */
  WRITE_TO_BUF("# Debug level(0-9)\n# If set to 0 the daemon runs in the background\n\nDebugLevel\t%d\n\n", cnf->debug_level)

  /* IP version */
  if(cnf->ip_version == AF_INET6)
      WRITE_TO_BUF("# IP version to use (4 or 6)\n\nIpVersion\t6\n\n")
  else
      WRITE_TO_BUF("# IP version to use (4 or 6)\n\nIpVersion\t4\n\n")

  /* HNA IPv4 */
  WRITE_TO_BUF("# HNA IPv4 routes\n# syntax: netaddr netmask\n\nHna4\n{\n")
  while(h4)
    {
      in4.s_addr = h4->net.v4;
      WRITE_TO_BUF("    %s ", inet_ntoa(in4))
      in4.s_addr = h4->netmask.v4;
      WRITE_TO_BUF("%s\n", inet_ntoa(in4))
      h4 = h4->next;
    }
  WRITE_TO_BUF("}\n\n")


  /* HNA IPv6 */
  WRITE_TO_BUF("# HNA IPv6 routes\n# syntax: netaddr prefix\n\nHna6\n{\n")
  while(h6)
    {
      WRITE_TO_BUF("    %s/%d\n", (char *)inet_ntop(AF_INET6, &h6->net.v6, ipv6_buf, sizeof(ipv6_buf)), h6->prefix_len)
      h6 = h6->next;
    }

  WRITE_TO_BUF("}\n\n")

  /* No interfaces */
  WRITE_TO_BUF("# Should olsrd keep on running even if there are\n# no interfaces available? This is a good idea\n# for a PCMCIA/USB hotswap environment.\n# \"yes\" OR \"no\"\n\nAllowNoInt\t")
  if(cnf->allow_no_interfaces)
    WRITE_TO_BUF("yes\n\n")
  else
    WRITE_TO_BUF("no\n\n")

  /* TOS */
  WRITE_TO_BUF("# TOS(type of service) to use. Default is 16\n\n")
  WRITE_TO_BUF("TosValue\t%d\n\n", cnf->tos)

  /* Willingness */
  WRITE_TO_BUF("# The fixed willingness to use(0-7)\n# If not set willingness will be calculated\n# dynammically based on battery/power status\n\n")
  if(cnf->willingness_auto)
    WRITE_TO_BUF("#Willingness\t4\n\n")
  else
    WRITE_TO_BUF("Willingness%d\n\n", cnf->willingness)

  /* IPC */
  WRITE_TO_BUF("# Allow processes like the GUI front-end\n# to connect to the daemon.\n\n")
  WRITE_TO_BUF("IpcConnect\n{\n")
  WRITE_TO_BUF("   MaxConnections  %d\n", cnf->ipc_connections)

  while(ih)
    {
      in4.s_addr = ih->host.v4;
      WRITE_TO_BUF("   Host          %s\n", inet_ntoa(in4))
      ih = ih->next;
    }
  while(ie)
    {
      in4.s_addr = ie->net.v4;
      WRITE_TO_BUF("   Net           %s ", inet_ntoa(in4))
      in4.s_addr = ie->mask.v4;
      WRITE_TO_BUF("%s\n", inet_ntoa(in4))
      ie = ie->next;
    }

  WRITE_TO_BUF("}\n\n")



  /* Hysteresis */
  WRITE_TO_BUF("# Hysteresis adds more robustness to the\n# link sensing.\n# Used by default. 'yes' or 'no'\n\n")

  if(cnf->use_hysteresis)
    {
      WRITE_TO_BUF("UseHysteresis\tyes\n\n")
      WRITE_TO_BUF("# Hysteresis parameters\n# Do not alter these unless you know \n# what you are doing!\n# Set to auto by default. Allowed\n# values are floating point values\n# in the interval 0,1\n# THR_LOW must always be lower than\n# THR_HIGH!!\n\n")
      WRITE_TO_BUF("HystScaling\t%0.2f\n", cnf->hysteresis_param.scaling)
      WRITE_TO_BUF("HystThrHigh\t%0.2f\n", cnf->hysteresis_param.thr_high)
      WRITE_TO_BUF("HystThrLow\t%0.2f\n\n", cnf->hysteresis_param.thr_low)
    }
  else
    {
      WRITE_TO_BUF("UseHysteresis\tno\n\n")
      WRITE_TO_BUF("# Hysteresis parameters\n# Do not alter these unless you know \n# what you are doing!\n# Set to auto by default. Allowed\n# values are floating point values\n# in the interval 0,1\n# THR_LOW must always be lower than\n# THR_HIGH!!\n\n")
      WRITE_TO_BUF("#HystScaling\t%0.2f\n", cnf->hysteresis_param.scaling)
      WRITE_TO_BUF("#HystThrHigh\t%0.2f\n", cnf->hysteresis_param.thr_high)
      WRITE_TO_BUF("#HystThrLow\t%0.2f\n\n", cnf->hysteresis_param.thr_low)
    }

  /* Pollrate */
  WRITE_TO_BUF("# Polling rate in seconds(float).\n# Auto uses default value 0.05 sec\n\n")
  WRITE_TO_BUF("Pollrate\t%0.2f\n", cnf->pollrate)

  /* TC redundancy */
  WRITE_TO_BUF("# TC redundancy\n# Specifies how much neighbor info should\n# be sent in TC messages\n# Possible values are:\n# 0 - only send MPR selectors\n# 1 - send MPR selectors and MPRs\n# 2 - send all neighbors\n#\n# defaults to 0\n\n")
  WRITE_TO_BUF("TcRedundancy\t%d\n\n", cnf->tc_redundancy)

  /* MPR coverage */
  WRITE_TO_BUF("# MPR coverage\n# Specifies how many MPRs a node should\n# try select to reach every 2 hop neighbor\n# Can be set to any integer >0\n# defaults to 1\n\n")

  WRITE_TO_BUF("MprCoverage\t%d\n\n", cnf->mpr_coverage)

  WRITE_TO_BUF("# Link quality level\n# 0 = do not use link quality\n# 1 = use link quality for MPR selection\n# 2 = use link quality for MPR selection and routing\n\n")
  WRITE_TO_BUF("LinkQualityLevel\t%d\n\n", cnf->lq_level)

  WRITE_TO_BUF("# Link quality window size\n\n")
  WRITE_TO_BUF("LinkQualityWinSize\t%d\n\n", cnf->lq_wsize)

  WRITE_TO_BUF("# Clear screen when printing debug output?\n\n")
  WRITE_TO_BUF("ClearScreen\t%s\n\n", cnf->clear_screen ? "yes" : "no")

  /* Plugins */
  WRITE_TO_BUF("# Olsrd plugins to load\n# This must be the absolute path to the file\n# or the loader will use the following scheme:\n")
  WRITE_TO_BUF("# - Try the paths in the LD_LIBRARY_PATH \n#   environment variable.\n# - The list of libraries cached in /etc/ld.so.cache\n# - /lib, followed by /usr/lib\n\n")
  if(pe)
    {
      while(pe)
	{
	  WRITE_TO_BUF("LoadPlugin \"%s\"\n{\n", pe->name)
          pp = pe->params;
          while(pp)
            {
              WRITE_TO_BUF("    PlParam \"%s\" \"%s\"\n", pp->key, pp->value)
              pp = pp->next;
            }
	  WRITE_TO_BUF("}\n")
	  pe = pe->next;
	}
    }
  WRITE_TO_BUF("\n")

  
  

  /* Interfaces */
  WRITE_TO_BUF("# Interfaces\n# Multiple interfaces with the same configuration\n")
  WRITE_TO_BUF("# can shar the same config block. Just list the\n# interfaces(e.g. Interface \"eth0\" \"eth2\"\n\n")
  /* Interfaces */
  if(in)
    {
      olsr_bool first = OLSR_TRUE;
      while(in)
	{
	  WRITE_TO_BUF("Interface \"%s\"\n{\n", in->name)

          if(first)
	     WRITE_TO_BUF("    # IPv4 broadcast address to use. The\n    # one usefull example would be 255.255.255.255\n    # If not defined the broadcastaddress\n    # every card is configured with is used\n\n")


	  if(in->cnf->ipv4_broadcast.v4)
	    {
	      in4.s_addr = in->cnf->ipv4_broadcast.v4;
	      WRITE_TO_BUF("    Ip4Broadcast\t %s\n", inet_ntoa(in4))
	    }
	  else
	    {
	      if(first)
    	        WRITE_TO_BUF("    #Ip4Broadcast\t255.255.255.255\n")
	    }
	  
          if(first) WRITE_TO_BUF("\n")

          if(first)
	      WRITE_TO_BUF("    # IPv6 address scope to use.\n    # Must be 'site-local' or 'global'\n\n")
	  if(in->cnf->ipv6_addrtype)
	    WRITE_TO_BUF("    Ip6AddrType \tsite-local\n")
	  else
	    WRITE_TO_BUF("    Ip6AddrType \tglobal\n")

          if(first) WRITE_TO_BUF("\n")

          if(first)
	    WRITE_TO_BUF("    # IPv6 multicast address to use when\n    # using site-local addresses.\n    # If not defined, ff05::15 is used\n")
	  WRITE_TO_BUF("    Ip6MulticastSite\t%s\n", (char *)inet_ntop(AF_INET6, &in->cnf->ipv6_multi_site.v6, ipv6_buf, sizeof(ipv6_buf)))
          if(first) WRITE_TO_BUF("\n")
          if(first)
	    WRITE_TO_BUF("    # IPv6 multicast address to use when\n    # using global addresses\n    # If not defined, ff0e::1 is used\n")
	  WRITE_TO_BUF("    Ip6MulticastGlobal\t%s\n", (char *)inet_ntop(AF_INET6, &in->cnf->ipv6_multi_glbl.v6, ipv6_buf, sizeof(ipv6_buf)))
          if(first) WRITE_TO_BUF("\n")
	  
	  
          if(first)
            WRITE_TO_BUF("    # Emission and validity intervals.\n    # If not defined, RFC proposed values will\n    # in most cases be used.\n\n")
	  
	  
	  if(in->cnf->hello_params.emission_interval != HELLO_INTERVAL)
	    WRITE_TO_BUF("    HelloInterval\t%0.2f\n", in->cnf->hello_params.emission_interval)
          else if(first)
	    WRITE_TO_BUF("    #HelloInterval\t%0.2f\n", in->cnf->hello_params.emission_interval)
	  if(in->cnf->hello_params.validity_time != NEIGHB_HOLD_TIME)
	    WRITE_TO_BUF("    HelloValidityTime\t%0.2f\n", in->cnf->hello_params.validity_time)
          else if(first)
	    WRITE_TO_BUF("    #HelloValidityTime\t%0.2f\n", in->cnf->hello_params.validity_time)
	  if(in->cnf->tc_params.emission_interval != TC_INTERVAL)
	    WRITE_TO_BUF("    TcInterval\t\t%0.2f\n", in->cnf->tc_params.emission_interval)
          else if(first)
	    WRITE_TO_BUF("    #TcInterval\t\t%0.2f\n", in->cnf->tc_params.emission_interval)
	  if(in->cnf->tc_params.validity_time != TOP_HOLD_TIME)
	    WRITE_TO_BUF("    TcValidityTime\t%0.2f\n", in->cnf->tc_params.validity_time)
          else if(first)
	    WRITE_TO_BUF("    #TcValidityTime\t%0.2f\n", in->cnf->tc_params.validity_time)
	  if(in->cnf->mid_params.emission_interval != MID_INTERVAL)
	    WRITE_TO_BUF("    MidInterval\t\t%0.2f\n", in->cnf->mid_params.emission_interval)
          else if(first)
	    WRITE_TO_BUF("    #MidInterval\t%0.2f\n", in->cnf->mid_params.emission_interval)
	  if(in->cnf->mid_params.validity_time != MID_HOLD_TIME)
	    WRITE_TO_BUF("    MidValidityTime\t%0.2f\n", in->cnf->mid_params.validity_time)
          else if(first)
	    WRITE_TO_BUF("    #MidValidityTime\t%0.2f\n", in->cnf->mid_params.validity_time)
	  if(in->cnf->hna_params.emission_interval != HNA_INTERVAL)
	    WRITE_TO_BUF("    HnaInterval\t\t%0.2f\n", in->cnf->hna_params.emission_interval)
          else if(first)
	    WRITE_TO_BUF("    #HnaInterval\t%0.2f\n", in->cnf->hna_params.emission_interval)
	  if(in->cnf->hna_params.validity_time != HNA_HOLD_TIME)
	    WRITE_TO_BUF("    HnaValidityTime\t%0.2f\n", in->cnf->hna_params.validity_time)	  
          else if(first)
	    WRITE_TO_BUF("    #HnaValidityTime\t%0.2f\n", in->cnf->hna_params.validity_time)	  
	  
          mult = in->cnf->lq_mult;

          if (mult == NULL)
	    {
              if(first)
	        WRITE_TO_BUF("    #LinkQualityMult\tdefault 1.0\n")
	    }
          else
	    {
	      while (mult != NULL)
		{
		  inet_ntop(cnf->ip_version, &mult->addr, ipv6_buf,
			    sizeof (ipv6_buf));
		  
		  WRITE_TO_BUF("    LinkQualityMult\t%s %0.2f\n",
			  ipv6_buf, mult->val)
		  
		  mult = mult->next;
		}
	    }

	  if(first)
    	    WRITE_TO_BUF("    # When multiple links exist between hosts\n    # the weight of interface is used to determine\n    # the link to use. Normally the weight is\n")
          if(first)
            WRITE_TO_BUF("    # automatically calculated by olsrd based\n    # on the characteristics of the interface,\n    # but here you can specify a fixed value.\n    # Olsrd will choose links with the lowest value.\n")
	  if(in->cnf->weight.fixed)
	    {
	      WRITE_TO_BUF("    Weight\t %d\n\n", in->cnf->weight.value)
	    }
	  else
	    {
              if(first)
       	        WRITE_TO_BUF("    #Weight\t 0\n\n")
	    }

	  
	  WRITE_TO_BUF("}\n\n")
	  in = in->next;
	  first = OLSR_FALSE;
	}

    }


  WRITE_TO_BUF("\n# END AUTOGENERATED CONFIG\n")

  return size;
}

