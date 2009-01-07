
/*
 * The olsr.org Optimized Link-State Routing daemon(olsrd)
 * Copyright (c) 2004, Andreas Tonnesen(andreto@olsr.org)
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

#include "olsr_cfg.h"

#include "olsr.h"
// #include "ipcalc.h"
#include "parser.h"
#include "net_olsr.h"
#include "ifnet.h"
#include "log.h"
#include "olsr_ip_prefix_list.h"

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <errno.h>

#ifdef DEBUG
#define PARSER_DEBUG_PRINTF(x, args...)   printf(x, ##args)
#else
#define PARSER_DEBUG_PRINTF(x, ...)   do { } while (0)
#endif

#define NOEXIT_ON_DEPRECATED_OPTIONS 1

/*
 * Special strcat for reading the config file and
 * joining a longer section { ... } to one string
 */
static void
read_cfg_cat(char **pdest, const char *src)
{
  char *tmp = *pdest;
  if (*src) {
    *pdest = olsr_malloc(1 + (tmp ? strlen(tmp) : 0) + strlen(src), "read_cfg_cat");
    if (tmp) {
      strcpy(*pdest, tmp);
      free(tmp);
    }
    strcat(*pdest, src);
  }
}

/*
 * Read the olsrd.conf file and replace/insert found
 * options into the argv[] array at the position of
 * the original -f filename parameters. Note, that
 * longer sections { ... } are joined to one string
 */
static int
read_cfg(const char *filename, int *pargc, char ***pargv, int **pline)
{
  FILE *f = fopen(filename, "r");
  if (f) {
    int bopen = 0, optind_tmp = optind, line_lst = 0, line = 0;
    char sbuf[512], *pbuf = NULL, *p;

    while ((p = fgets(sbuf, sizeof(sbuf), f)) || pbuf) {
      line++;
      if (!p) {
        *sbuf = 0;
        goto eof;
      }
      while (*p && '#' != *p) {
        if ('"' == *p || '\'' == *p) {
          char sep = *p;
          while (*++p && sep != *p);
        }
        p++;
      }
      *p = 0;
      while (sbuf <= --p && ' ' >= *p);
      *(p + 1) = 0;
      if (*sbuf) {
        if (bopen) {
          read_cfg_cat(&pbuf, sbuf);
          if ('}' == *p) {
            bopen = 0;
          }
        } else if (p == sbuf && '{' == *p) {
          read_cfg_cat(&pbuf, " ");
          read_cfg_cat(&pbuf, sbuf);
          bopen = 1;
        } else {
          if ('{' == *p) {
            bopen = 1;
          }
        eof:
          if (pbuf) {
            int i, *line_tmp;
            char **argv_tmp;
            int argc_tmp = *pargc + 2;
            char *q = pbuf, *n;

            while (*q && ' ' >= *q)
              q++;
            p = q;
            while (' ' < *p)
              p++;
            n = olsr_malloc(p - q + 3, "config arg0");
            strcpy(n, "--");
            strncat(n, q, p - q);
            while (*q && ' ' >= *p)
              p++;

            line_tmp = olsr_malloc(argc_tmp * sizeof(line_tmp[0]), "config line");
            argv_tmp = olsr_malloc(argc_tmp * sizeof(argv_tmp[0]), "config args");
            for (i = 0; i < argc_tmp; i++) {
              if (i < optind_tmp) {
                line_tmp[i] = (*pline)[i];
                argv_tmp[i] = (*pargv)[i];
              } else if (i == optind_tmp) {
                line_tmp[i] = line_lst;
                argv_tmp[i] = n;
              } else if (i == 1 + optind_tmp) {
                line_tmp[i] = line_lst;
                argv_tmp[i] = olsr_strdup(p);
              } else {
                line_tmp[i] = (*pline)[i - 2];
                argv_tmp[i] = (*pargv)[i - 2];
              }
            }
            optind_tmp += 2;
            *pargc = argc_tmp;
            free(*pargv);
            *pargv = argv_tmp;
            free(*pline);
            *pline = line_tmp;
            line_lst = line;
            free(pbuf);
            pbuf = NULL;
          }
          read_cfg_cat(&pbuf, sbuf);
        }
      }
    }
    fclose(f);
    return 0;
  }
  return -1;
}

/*
 * Free an array of string tokens
 */
static void
parse_tok_free(char **s)
{
  if (s) {
    char **p = s;
    while (*p) {
      free(*p);
      p++;
    }
    free(s);
  }
}

/*
 * Test for end-of-string or { ... } section
 */
static inline bool
parse_tok_delim(const char *p)
{
  switch (*p) {
  case 0:
  case '{':
  case '}':
    return true;
  }
  return false;
}

/*
 * Slit the src string into tokens and return
 * an array of token strings. May return NULL
 * if no token found, otherwise you need to
 * free the strings using parse_tok_free()
 */
static char **
parse_tok(const char *s, const char **snext)
{
  char **tmp, **ret = NULL;
  int i, count = 0;
  const char *p = s;

  while (!parse_tok_delim(p)) {
    while (!parse_tok_delim(p) && ' ' >= *p)
      p++;
    if (!parse_tok_delim(p)) {
      char c = 0;
      const char *q = p;
      if ('"' == *p || '\'' == *p) {
        c = *q++;
        while (!parse_tok_delim(++p) && c != *p);
      } else {
        while (!parse_tok_delim(p) && ' ' < *p)
          p++;
      }
      tmp = ret;
      ret = olsr_malloc((2 + count) * sizeof(ret[0]), "parse_tok");
      for (i = 0; i < count; i++) {
        ret[i] = tmp[i];
      }
      if (tmp)
        free(tmp);
      ret[count++] = olsr_strndup(q, p - q);
      ret[count] = NULL;
      if (c)
        p++;
    }
  }
  if (snext)
    *snext = p;
  return ret;
}

/*
 * Returns default interface options
 */
static struct olsr_if_options *
get_default_olsr_if_options(void)
{
  struct olsr_if_options *new_io = olsr_malloc(sizeof(*new_io), "default_if_config");

  /* No memset because olsr_malloc uses calloc() */
  /* memset(&new_io->ipv4_broadcast, 0, sizeof(new_io->ipv4_broadcast)); */
  new_io->ipv6_addrtype = OLSR_IP6T_AUTO;
  inet_pton(AF_INET6, OLSR_IPV6_MCAST_SITE_LOCAL, &new_io->ipv6_multi_site.v6);
  inet_pton(AF_INET6, OLSR_IPV6_MCAST_GLOBAL, &new_io->ipv6_multi_glbl.v6);
  /* new_io->weight.fixed = false; */
  /* new_io->weight.value = 0; */
  new_io->hello_params.emission_interval = HELLO_INTERVAL;
  new_io->hello_params.validity_time = NEIGHB_HOLD_TIME;
  new_io->tc_params.emission_interval = TC_INTERVAL;
  new_io->tc_params.validity_time = TOP_HOLD_TIME;
  new_io->mid_params.emission_interval = MID_INTERVAL;
  new_io->mid_params.validity_time = MID_HOLD_TIME;
  new_io->hna_params.emission_interval = HNA_INTERVAL;
  new_io->hna_params.validity_time = HNA_HOLD_TIME;
  /* new_io->lq_mult = NULL; */
  new_io->autodetect_chg = true;

  return new_io;
}

/**
 * Create a new interf_name struct using a given
 * name and insert it into the interface list.
 */
static struct olsr_if_config *
queue_if(const char *name, struct olsr_config *cfg)
{
  struct olsr_if_config *new_if;

  /* check if the interface already exists */
  for (new_if = cfg->if_configs; new_if != NULL; new_if = new_if->next) {
    if (0 == strcmp(new_if->name, name)) {
      fprintf(stderr, "Duplicate interfaces defined... not adding %s\n", name);
      return NULL;
    }
  }

  new_if = olsr_malloc(sizeof(*new_if), "queue interface");
  new_if->name = olsr_strdup(name);
  /* new_if->config = NULL; */
  /* memset(&new_if->hemu_ip, 0, sizeof(new_if->hemu_ip)); */
  /* new_if->interf = NULL; */
  new_if->cnf = get_default_olsr_if_options();
  new_if->next = cfg->if_configs;
  cfg->if_configs = new_if;

  return new_if;
}

/*
 * Parses a single hna option block
 */
static void
parse_cfg_hna(char *argstr, const int ip_version, struct olsr_config *cfg)
{
  char **tok;
#ifdef DEBUG
  struct ipaddr_str buf;
#endif
  if ('{' != *argstr) {
    fprintf(stderr, "No {}\n");
    exit(EXIT_FAILURE);
  }
  if (NULL != (tok = parse_tok(argstr + 1, NULL))) {
    char **p = tok;
    if (ip_version != cfg->ip_version) {
      fprintf(stderr, "IPv%d addresses can only be used if \"IpVersion\" == %d\n",
              AF_INET == ip_version ? 4 : 6, AF_INET == ip_version ? 4 : 6);
      exit(EXIT_FAILURE);
    }
    while (p[0]) {
      union olsr_ip_addr ipaddr;
      if (!p[1]) {
        fprintf(stderr, "Odd args in %s\n", argstr);
        exit(EXIT_FAILURE);
      }
      if (inet_pton(ip_version, p[0], &ipaddr) <= 0) {
        fprintf(stderr, "Failed converting IP address %s\n", p[0]);
        exit(EXIT_FAILURE);
      }
      if (AF_INET == ip_version) {
        union olsr_ip_addr netmask;
        if (inet_pton(AF_INET, p[1], &netmask) <= 0) {
          fprintf(stderr, "Failed converting IP address %s\n", p[1]);
          exit(EXIT_FAILURE);
        }
        if ((ipaddr.v4.s_addr & ~netmask.v4.s_addr) != 0) {
          fprintf(stderr, "The IP address %s/%s is not a network address!\n", p[0], p[1]);
          exit(EXIT_FAILURE);
        }
        ip_prefix_list_add(&cfg->hna_entries, &ipaddr, netmask_to_prefix((uint8_t *)&netmask, cfg->ipsize));
        PARSER_DEBUG_PRINTF("Hna4 %s/%d\n", ip_to_string(cfg->ip_version, &buf, &ipaddr), netmask_to_prefix((uint8_t *)&netmask, cfg->ipsize));
      } else {
        int prefix = -1;
        sscanf('/' == *p[1] ? p[1] + 1 : p[1], "%d", &prefix);
        if (0 > prefix || 128 < prefix) {
          fprintf(stderr, "Illegal IPv6 prefix %s\n", p[1]);
          exit(EXIT_FAILURE);
        }
        ip_prefix_list_add(&cfg->hna_entries, &ipaddr, prefix);
        PARSER_DEBUG_PRINTF("Hna6 %s/%d\n", ip_to_string(cfg->ip_version, &buf, &ipaddr), prefix);
      }
      p += 2;
    }
    parse_tok_free(tok);
  }
}

/*
 * Parses a single interface option block
 */
static void
parse_cfg_interface(char *argstr, struct olsr_config *cfg)
{
  char **tok;
  const char *nxt;
#ifdef DEBUG
  struct ipaddr_str buf;
#endif
  if (NULL != (tok = parse_tok(argstr, &nxt))) {
    if ('{' != *nxt) {
      fprintf(stderr, "No {}\n");
      exit(EXIT_FAILURE);
    } else {
      char **tok_next = parse_tok(nxt + 1, NULL);
      char **p = tok;
      while (p[0]) {
        char **p_next = tok_next;
        struct olsr_if_config *new_if = queue_if(p[0], cfg);
        PARSER_DEBUG_PRINTF("Interface %s\n", p[0]);
        while (new_if && p_next[0]) {
          if (!p_next[1]) {
            fprintf(stderr, "Odd args in %s\n", nxt);
            exit(EXIT_FAILURE);
          }
          if (0 == strcmp("AutoDetectChanges", p_next[0])) {
            new_if->cnf->autodetect_chg = (0 == strcmp("yes", p_next[1]));
            PARSER_DEBUG_PRINTF("\tAutodetect changes: %d\n", new_if->cnf->autodetect_chg);
          } else if (0 == strcmp("Ip4Broadcast", p_next[0])) {
            union olsr_ip_addr ipaddr;
            if (inet_pton(AF_INET, p_next[1], &ipaddr) <= 0) {
              fprintf(stderr, "Failed converting IP address %s\n", p_next[1]);
              exit(EXIT_FAILURE);
            }
            new_if->cnf->ipv4_broadcast = ipaddr;
            PARSER_DEBUG_PRINTF("\tIPv4 broadcast: %s\n", ip4_to_string(&buf, new_if->cnf->ipv4_broadcast.v4));
          } else if (0 == strcmp("Ip6AddrType", p_next[0])) {
            if (0 == strcmp("site-local", p_next[1])) {
              new_if->cnf->ipv6_addrtype = OLSR_IP6T_SITELOCAL;
            } else if (0 == strcmp("unique-local", p_next[1])) {
              new_if->cnf->ipv6_addrtype = OLSR_IP6T_UNIQUELOCAL;
            } else if (0 == strcmp("global", p_next[1])) {
              new_if->cnf->ipv6_addrtype = OLSR_IP6T_GLOBAL;
            } else {
              new_if->cnf->ipv6_addrtype = OLSR_IP6T_AUTO;
            }
            PARSER_DEBUG_PRINTF("\tIPv6 addrtype: %d\n", new_if->cnf->ipv6_addrtype);
          } else if (0 == strcmp("Ip6MulticastSite", p_next[0])) {
            union olsr_ip_addr ipaddr;
            if (inet_pton(AF_INET6, p_next[1], &ipaddr) <= 0) {
              fprintf(stderr, "Failed converting IP address %s\n", p_next[1]);
              exit(EXIT_FAILURE);
            }
            new_if->cnf->ipv6_multi_site = ipaddr;
            PARSER_DEBUG_PRINTF("\tIPv6 site-local multicast: %s\n", ip6_to_string(&buf, &new_if->cnf->ipv6_multi_site.v6));
          } else if (0 == strcmp("Ip6MulticastGlobal", p_next[0])) {
            union olsr_ip_addr ipaddr;
            if (inet_pton(AF_INET6, p_next[1], &ipaddr) <= 0) {
              fprintf(stderr, "Failed converting IP address %s\n", p_next[1]);
              exit(EXIT_FAILURE);
            }
            new_if->cnf->ipv6_multi_glbl = ipaddr;
            PARSER_DEBUG_PRINTF("\tIPv6 global multicast: %s\n", ip6_to_string(&buf, &new_if->cnf->ipv6_multi_glbl.v6));
          } else if (0 == strcmp("HelloInterval", p_next[0])) {
            new_if->cnf->hello_params.emission_interval = 0;
            sscanf(p_next[1], "%f", &new_if->cnf->hello_params.emission_interval);
            PARSER_DEBUG_PRINTF("\tHELLO interval: %0.2f\n", new_if->cnf->hello_params.emission_interval);
          } else if (0 == strcmp("HelloValidityTime", p_next[0])) {
            new_if->cnf->hello_params.validity_time = 0;
            sscanf(p_next[1], "%f", &new_if->cnf->hello_params.validity_time);
            PARSER_DEBUG_PRINTF("\tHELLO validity: %0.2f\n", new_if->cnf->hello_params.validity_time);
          } else if (0 == strcmp("TcInterval", p_next[0])) {
            new_if->cnf->tc_params.emission_interval = 0;
            sscanf(p_next[1], "%f", &new_if->cnf->tc_params.emission_interval);
            PARSER_DEBUG_PRINTF("\tTC interval: %0.2f\n", new_if->cnf->tc_params.emission_interval);
          } else if (0 == strcmp("TcValidityTime", p_next[0])) {
            new_if->cnf->tc_params.validity_time = 0;
            sscanf(p_next[1], "%f", &new_if->cnf->tc_params.validity_time);
            PARSER_DEBUG_PRINTF("\tTC validity: %0.2f\n", new_if->cnf->tc_params.validity_time);
          } else if (0 == strcmp("MidInterval", p_next[0])) {
            new_if->cnf->mid_params.emission_interval = 0;
            sscanf(p_next[1], "%f", &new_if->cnf->mid_params.emission_interval);
            PARSER_DEBUG_PRINTF("\tMID interval: %0.2f\n", new_if->cnf->mid_params.emission_interval);
          } else if (0 == strcmp("MidValidityTime", p_next[0])) {
            new_if->cnf->mid_params.validity_time = 0;
            sscanf(p_next[1], "%f", &new_if->cnf->mid_params.validity_time);
            PARSER_DEBUG_PRINTF("\tMID validity: %0.2f\n", new_if->cnf->mid_params.validity_time);
          } else if (0 == strcmp("HnaInterval", p_next[0])) {
            new_if->cnf->hna_params.emission_interval = 0;
            sscanf(p_next[1], "%f", &new_if->cnf->hna_params.emission_interval);
            PARSER_DEBUG_PRINTF("\tHNA interval: %0.2f\n", new_if->cnf->hna_params.emission_interval);
          } else if (0 == strcmp("HnaValidityTime", p_next[0])) {
            new_if->cnf->hna_params.validity_time = 0;
            sscanf(p_next[1], "%f", &new_if->cnf->hna_params.validity_time);
            PARSER_DEBUG_PRINTF("\tHNA validity: %0.2f\n", new_if->cnf->hna_params.validity_time);
          } else if (0 == strcmp("Weight", p_next[0])) {
            new_if->cnf->weight.fixed = true;
            PARSER_DEBUG_PRINTF("\tFixed willingness: %d\n", new_if->cnf->weight.value);
          } else if (0 == strcmp("LinkQualityMult", p_next[0])) {
            float f;
            struct olsr_lq_mult *mult = olsr_malloc(sizeof(*mult), "lqmult");
            if (!p_next[2]) {
              fprintf(stderr, "Odd args in %s\n", nxt);
              exit(EXIT_FAILURE);
            }
            memset(&mult->addr, 0, sizeof(mult->addr));
            if (0 != strcmp("default", p_next[1])) {
              if (inet_pton(cfg->ip_version, p_next[1], &mult->addr) <= 0) {
                fprintf(stderr, "Failed converting IP address %s\n", p_next[1]);
                exit(EXIT_FAILURE);
              }
            }
            f = 0;
            sscanf(p_next[2], "%f", &f);
            mult->value = (uint32_t) (f * LINK_LOSS_MULTIPLIER);
            mult->next = new_if->cnf->lq_mult;
            new_if->cnf->lq_mult = mult;
            PARSER_DEBUG_PRINTF("\tLinkQualityMult %s %0.2f\n", ip_to_string(cfg->ip_version, &buf, &mult->addr),
                                (float)mult->value / LINK_LOSS_MULTIPLIER);
            p_next++;
          } else {
            fprintf(stderr, "Unknown arg: %s %s\n", p_next[0], p_next[1]);
            exit(EXIT_FAILURE);
          }
          p_next += 2;
        }
        p++;
      }
      if (tok_next)
        parse_tok_free(tok_next);
    }
    parse_tok_free(tok);
  } else {
    fprintf(stderr, "Error in %s\n", argstr);
    exit(EXIT_FAILURE);
  }
}

/*
 * Parses a single ipc option block
 */
static void
parse_cfg_ipc(char *argstr, struct olsr_config *cfg)
{
  char **tok;
  const char *nxt;
#ifdef DEBUG
  struct ipaddr_str buf;
#endif
  if ('{' != *argstr) {
    fprintf(stderr, "No {}\n");
    exit(EXIT_FAILURE);
  }
  if (NULL != (tok = parse_tok(argstr + 1, &nxt))) {
    char **p = tok;
    while (p[0]) {
      if (!p[1]) {
        fprintf(stderr, "Odd args in %s\n", argstr);
        exit(EXIT_FAILURE);
      }
      if (0 == strcmp("MaxConnections", p[0])) {
        int arg = -1;
        sscanf(argstr, "%d", &arg);
        if (0 <= arg && arg < 1 << (8 * sizeof(cfg->ipc_connections)))
          cfg->ipc_connections = arg;
        PARSER_DEBUG_PRINTF("\tIPC connections: %d\n", cfg->ipc_connections);
      } else if (0 == strcmp("Host", p[0])) {
        union olsr_ip_addr ipaddr;
        if (inet_pton(cfg->ip_version, p[1], &ipaddr) <= 0) {
          fprintf(stderr, "Failed converting IP address %s\n", p[0]);
          exit(EXIT_FAILURE);
        }

        ip_acl_add(&cfg->ipc_nets, &ipaddr, cfg->maxplen, false);
        PARSER_DEBUG_PRINTF("\tIPC host: %s\n", ip_to_string(cfg->ip_version, &buf, &ipaddr));
      } else if (0 == strcmp("Net", p[0])) {
        union olsr_ip_addr ipaddr;
        if (!p[2]) {
          fprintf(stderr, "Odd args in %s\n", nxt);
          exit(EXIT_FAILURE);
        }
        if (inet_pton(cfg->ip_version, p[1], &ipaddr) <= 0) {
          fprintf(stderr, "Failed converting IP address %s\n", p[0]);
          exit(EXIT_FAILURE);
        }
        if (AF_INET == cfg->ip_version) {
          union olsr_ip_addr netmask;
          if (inet_pton(AF_INET, p[2], &netmask) <= 0) {
            fprintf(stderr, "Failed converting IP address %s\n", p[2]);
            exit(EXIT_FAILURE);
          }
          if ((ipaddr.v4.s_addr & ~netmask.v4.s_addr) != 0) {
            fprintf(stderr, "The IP address %s/%s is not a network address!\n", p[1], p[2]);
            exit(EXIT_FAILURE);
          }
          ip_acl_add(&cfg->ipc_nets, &ipaddr, olsr_netmask_to_prefix(&netmask), false);
          PARSER_DEBUG_PRINTF("\tIPC net: %s/%d\n", ip_to_string(cfg->ip_version, &buf, &ipaddr), olsr_netmask_to_prefix(&netmask));
        } else {
          int prefix = -1;
          sscanf('/' == *p[2] ? p[2] + 1 : p[2], "%d", &prefix);
          if (0 > prefix || 128 < prefix) {
            fprintf(stderr, "Illegal IPv6 prefix %s\n", p[2]);
            exit(EXIT_FAILURE);
          }
          ip_acl_add(&cfg->ipc_nets, &ipaddr, prefix, false);
          PARSER_DEBUG_PRINTF("\tIPC net: %s/%d\n", ip_to_string(cfg->ip_version, &buf, &ipaddr), prefix);
        }
        p++;
      } else {
        fprintf(stderr, "Unknown arg: %s %s\n", p[0], p[1]);
        exit(EXIT_FAILURE);
      }
      p += 2;
    }
    parse_tok_free(tok);
  }
}

/*
 * Parses a single loadplugin option block
 */
static void
parse_cfg_loadplugin(char *argstr, struct olsr_config *cfg)
{
  char **tok;
  const char *nxt;
  if (NULL != (tok = parse_tok(argstr, &nxt))) {
    if ('{' != *nxt) {
      fprintf(stderr, "No {}\n");
      exit(EXIT_FAILURE);
    } else {
      char **tok_next = parse_tok(nxt + 1, NULL);
      struct plugin_entry *pe = olsr_malloc(sizeof(*pe), "plugin");
      pe->name = olsr_strdup(*tok);
      pe->params = NULL;
      pe->next = cfg->plugins;
      cfg->plugins = pe;
      PARSER_DEBUG_PRINTF("Plugin: %s\n", pe->name);
      if (tok_next) {
        char **p_next = tok_next;
        while (p_next[0]) {
          struct plugin_param *pp = olsr_malloc(sizeof(*pp), "plparam");
          if (0 != strcmp("PlParam", p_next[0]) || !p_next[1] || !p_next[2]) {
            fprintf(stderr, "Odd args in %s\n", nxt);
            exit(EXIT_FAILURE);
          }
          pp->key = olsr_strdup(p_next[1]);
          pp->value = olsr_strdup(p_next[2]);
          pp->next = pe->params;
          pe->params = pp;
          PARSER_DEBUG_PRINTF("\tPlParam: %s %s\n", pp->key, pp->value);
          p_next += 3;
        }
        parse_tok_free(tok_next);
      }
    }
    parse_tok_free(tok);
  } else {
    fprintf(stderr, "Error in %s\n", argstr);
    exit(EXIT_FAILURE);
  }
}

/*
 * Parses a single loadplugin option block
 */
static void
parse_cfg_log(char *argstr , struct olsr_config *cfg)
{
  char *p = (char *) argstr, *nextEquals, *nextColon, *nextSlash;
  bool hasErrorOption = false;
  int i,j;

  while (p != NULL && *p != 0) {
    /* split at ',' */
    nextColon = strchr(p, ',');
    if (nextColon) {
      *nextColon++ = 0;
    }

    nextEquals = strchr(p, '=');
    if (nextEquals) {
      *nextEquals++ = 0;
    }

    for (j=0; j<LOG_SEVERITY_COUNT; j++) {
      if (strcasecmp(p, LOG_SEVERITY_NAMES[j]) == 0) {
        break;
      }
    }

    if (j < LOG_SEVERITY_COUNT) {
      p = nextEquals;
      while (p != NULL && *p != 0) {
        /* split at '/' */
        nextSlash = strchr(p, '/');
        if (nextSlash) {
          *nextSlash++ = 0;
        }

        for (i=0; i<LOG_SOURCE_COUNT; i++) {
          if (strcasecmp(p, LOG_SOURCE_NAMES[i]) == 0) {
            break;
          }
        }

        if (i < LOG_SOURCE_COUNT) {
          cfg->log_event[j][i] = true;
        }
        p = nextSlash;
      }
    }
    p = nextColon;
  }

  /* process results to clean them up */
  for (i=0; i<LOG_SOURCE_COUNT; i++) {
    if (cfg->log_event[INFO][i]) {
      cfg->log_event[WARN][i] = true;
    }
    if (cfg->log_event[WARN][i]) {
      cfg->log_event[ERROR][i] = true;
    }
    if (!hasErrorOption) {
      cfg->log_event[ERROR][i] = true;
    }
  }
}

/*
 * Parses a single option found on the command line
 */
static void
parse_cfg_option(const int optint, char *argstr, const int line, struct olsr_config *cfg)
{
  switch (optint) {
  case 'D':                    /* delgw */
    cfg->del_gws = true;
    PARSER_DEBUG_PRINTF("del_gws set to %d\n", cfg->del_gws);
    break;
  case 'X':                    /* dispin */
    PARSER_DEBUG_PRINTF("Calling parser_set_disp_pack_in(true)\n");
    parser_set_disp_pack_in(true);
    break;
  case 'O':                    /* dispout */
    PARSER_DEBUG_PRINTF("Calling net_set_disp_pack_out(true)\n");
    net_set_disp_pack_out(true);
    break;
  case 'i':                    /* iface */
    /* Ignored */
    break;
#ifdef WIN32
  case 'l':                    /* int */
    ListInterfaces();
    exit(0);
#endif
  case 'P':                    /* ipc */
    cfg->ipc_connections = 1;
    PARSER_DEBUG_PRINTF("IPC connections: %d\n", cfg->ipc_connections);
    break;
  case 'n':                    /* nofork */
    cfg->no_fork = true;
    PARSER_DEBUG_PRINTF("no_fork set to %d\n", cfg->no_fork);
    break;
  case 'v':                    /* version */
    /* Version string already printed */
    exit(0);
  case 'A':                    /* AllowNoInt (yes/no) */
    cfg->allow_no_interfaces = (0 == strcmp("yes", argstr));
    PARSER_DEBUG_PRINTF("Noint set to %d\n", cfg->allow_no_interfaces);
    break;
  case 'C':                    /* ClearScreen (yes/no) */
    cfg->clear_screen = (0 == strcmp("yes", argstr));
    PARSER_DEBUG_PRINTF("Clear screen %s\n", cfg->clear_screen ? "enabled" : "disabled");
    break;
  case 'd':                    /* DebugLevel (i) */
    {
      int arg = -1;
      sscanf(argstr, "%d", &arg);
      if (0 <= arg && arg < 128)
        cfg->debug_level = arg;
      PARSER_DEBUG_PRINTF("Debug level: %d\n", cfg->debug_level);
    }
    break;
  case 'F':                    /* FIBMetric (str) */
    {
      char **tok;
      if (NULL != (tok = parse_tok(argstr, NULL))) {
        if (strcmp(*tok, CFG_FIBM_FLAT) == 0) {
          cfg->fib_metric = FIBM_FLAT;
        } else if (strcmp(*tok, CFG_FIBM_CORRECT) == 0) {
          cfg->fib_metric = FIBM_CORRECT;
        } else if (strcmp(*tok, CFG_FIBM_APPROX) == 0) {
          cfg->fib_metric = FIBM_APPROX;
        } else {
          fprintf(stderr, "FIBMetric must be \"%s\", \"%s\", or \"%s\"!\n", CFG_FIBM_FLAT, CFG_FIBM_CORRECT, CFG_FIBM_APPROX);
          exit(EXIT_FAILURE);
        }
        parse_tok_free(tok);
      } else {
        fprintf(stderr, "Error in %s\n", argstr);
        exit(EXIT_FAILURE);
      }
      PARSER_DEBUG_PRINTF("FIBMetric: %d=%s\n", cfg->fib_metric, argstr);
    }
    break;
  case '4':                    /* Hna4 (4body) */
    parse_cfg_hna(argstr, AF_INET, cfg);
    break;
  case '6':                    /* Hna6 (6body) */
    parse_cfg_hna(argstr, AF_INET6, cfg);
    break;
  case 'I':                    /* Interface if1 if2 { ifbody } */
    parse_cfg_interface(argstr, cfg);
    break;
  case 'Q':                    /* IpcConnect (Host,Net,MaxConnections) */
    parse_cfg_ipc(argstr, cfg);
    break;
  case 'V':                    /* IpVersion (i) */
    {
      int ver = -1;
      sscanf(argstr, "%d", &ver);
      if (ver == 4) {
        cfg->ip_version = AF_INET;
        cfg->ipsize = sizeof(struct in_addr);
        cfg->maxplen = 32;
      } else if (ver == 6) {
        cfg->ip_version = AF_INET6;
        cfg->ipsize = sizeof(struct in6_addr);
        cfg->maxplen = 128;
      } else {
        fprintf(stderr, "IpVersion must be 4 or 6!\n");
        exit(EXIT_FAILURE);
      }
    }
    PARSER_DEBUG_PRINTF("IpVersion: %d\n", cfg->ip_version);
    break;
  case 'J':                    /* LinkQualityDijkstraLimit (i,f) */
    {
      int arg = -1;
      sscanf(argstr, "%d %f", &arg, &cfg->lq_dinter);
      if (0 <= arg && arg < (1 << (8 * sizeof(cfg->lq_dlimit))))
        cfg->lq_dlimit = arg;
      PARSER_DEBUG_PRINTF("Link quality dijkstra limit %d, %0.2f\n", cfg->lq_dlimit, cfg->lq_dinter);
    }
    break;
  case 'E':                    /* LinkQualityFishEye (i) */
    {
      int arg = -1;
      sscanf(argstr, "%d", &arg);
      if (0 <= arg && arg < (1 << (8 * sizeof(cfg->lq_fish))))
        cfg->lq_fish = arg;
      PARSER_DEBUG_PRINTF("Link quality fish eye %d\n", cfg->lq_fish);
    }
    break;
  case 'p':                    /* LoadPlugin (soname {PlParams}) */
    parse_cfg_loadplugin(argstr, cfg);
    break;
  case 'M':                    /* MprCoverage (i) */
    {
      int arg = -1;
      sscanf(argstr, "%d", &arg);
      if (0 <= arg && arg < (1 << (8 * sizeof(cfg->mpr_coverage))))
        cfg->mpr_coverage = arg;
      PARSER_DEBUG_PRINTF("MPR coverage %d\n", cfg->mpr_coverage);
    }
    break;
  case 'N':                    /* NatThreshold (f) */
    sscanf(argstr, "%f", &cfg->lq_nat_thresh);
    PARSER_DEBUG_PRINTF("NAT threshold %0.2f\n", cfg->lq_nat_thresh);
    break;
  case 'Y':                    /* NicChgsPollInt (f) */
    sscanf(argstr, "%f", &cfg->nic_chgs_pollrate);
    PARSER_DEBUG_PRINTF("NIC Changes Pollrate %0.2f\n", cfg->nic_chgs_pollrate);
    break;
  case 'T':                    /* Pollrate (f) */
    {
      float arg = -1;
      sscanf(argstr, "%f", &arg);
      if (0 <= arg)
        cfg->pollrate = conv_pollrate_to_microsecs(arg);
      PARSER_DEBUG_PRINTF("Pollrate %ud\n", cfg->pollrate);
    }
    break;
  case 'q':                    /* RtProto (i) */
    {
      int arg = -1;
      sscanf(argstr, "%d", &arg);
      if (0 <= arg && arg < (1 << (8 * sizeof(cfg->rtproto))))
        cfg->rtproto = arg;
      PARSER_DEBUG_PRINTF("RtProto: %d\n", cfg->rtproto);
    }
    break;
  case 'R':                    /* RtTableDefault (i) */
    {
      int arg = -1;
      sscanf(argstr, "%d", &arg);
      if (0 <= arg && arg < (1 << (8 * sizeof(cfg->rttable_default))))
        cfg->rttable_default = arg;
      PARSER_DEBUG_PRINTF("RtTableDefault: %d\n", cfg->rttable_default);
    }
    break;
  case 'r':                    /* RtTable (i) */
    {
      int arg = -1;
      sscanf(argstr, "%d", &arg);
      if (0 <= arg && arg < (1 << (8 * sizeof(cfg->rttable))))
        cfg->rttable = arg;
      PARSER_DEBUG_PRINTF("RtTable: %d\n", cfg->rttable);
    }
    break;
  case 't':                    /* TcRedundancy (i) */
    {
      int arg = -1;
      sscanf(argstr, "%d", &arg);
      if (0 <= arg && arg < (1 << (8 * sizeof(cfg->tc_redundancy))))
        cfg->tc_redundancy = arg;
      PARSER_DEBUG_PRINTF("TC redundancy %d\n", cfg->tc_redundancy);
    }
    break;
  case 'Z':                    /* TosValue (i) */
    {
      int arg = -1;
      sscanf(argstr, "%d", &arg);
      if (0 <= arg && arg < (1 << (8 * sizeof(cfg->tos))))
        cfg->tos = arg;
      PARSER_DEBUG_PRINTF("TOS: %d\n", cfg->tos);
    }
    break;
  case 'w':                    /* Willingness (i) */
    {
      int arg = -1;
      sscanf(argstr, "%d", &arg);
      if (0 <= arg && arg < (1 << (8 * sizeof(cfg->willingness))))
        cfg->willingness = arg;
      cfg->willingness_auto = false;
      PARSER_DEBUG_PRINTF("Willingness: %d (no auto)\n", cfg->willingness);
    }
    break;
  case 'L':                    /* Log (string) */
    parse_cfg_log(argstr, cfg);
    break;
  default:
    fprintf(stderr, "Unknown arg in line %d.\n", line);
#if NOEXIT_ON_DEPRECATED_OPTIONS
    if (0 < line) {

      olsr_syslog(OLSR_LOG_ERR, "Unknown arg in line %d.\n", line);
      break;
    }
#endif
    exit(EXIT_FAILURE);
  }                             /* switch */
}

/*
 * Parses command line options using the getopt() runtime
 * function. May also replace a "-f filename" combination
 * with options read in from a config file.
 */
struct olsr_config *
olsr_parse_cfg(int argc, char *argv[], const char *conf_file_name)
{
  int opt;
  int opt_idx = 0;
  char *opt_str = 0;
  int opt_argc = 0;
  char **opt_argv = olsr_malloc(argc * sizeof(opt_argv[0]), "opt_argv");
  int *opt_line = olsr_malloc(argc * sizeof(opt_line[0]), "opt_line");
  struct olsr_config *cfg = olsr_get_default_cfg();

  /*
   * Original cmd line params
   *
   * -bcast                   (removed)
   * -delgw                   (-> --delgw)
   * -dispin                  (-> --dispin)
   * -dispout                 (-> --dispout)
   * -d (level)               (preserved)
   * -f (config)              (preserved)
   * -hemu (queue_if(hcif01)) (-> --hemu)
   * -hint                    (removed)
   * -hnaint                  (removed)
   * -i if0 if1...            (see comment below)
   * -int (WIN32)             (preserved)
   * -ipc (ipc conn = 1)      (-> --ipc)
   * -ipv6                    (-> -V or --IpVersion)
   * -lqa (lq aging)          (removed)
   * -lql (lq lev)            (removed)
   * -lqnt (lq nat)           (removed)
   * -midint                  (removed)
   * -multi (ipv6 mcast)      (removed)
   * -nofork                  (preserved)
   * -tcint                   (removed)
   * -T (pollrate)            (preserved)
   *
   * Note: Remaining args are interpreted as list of
   * interfaces. For this reason, '-i' is ignored which
   * adds the following non-minus args to the list.
   */

  /* *INDENT-OFF* */
  static struct option long_options[] = {
    {"config",                   required_argument, 0, 'f'}, /* (filename) */
    {"delgw",                    no_argument,       0, 'D'},
    {"dispin",                   no_argument,       0, 'X'},
    {"dispout",                  no_argument,       0, 'O'},
    {"help",                     no_argument,       0, 'h'},
    {"iface",                    no_argument,       0, 'i'}, /* if0 if1... */
#ifdef WIN32
    {"int",                      no_argument,       0, 'l'},
#endif
    {"ipc",                      no_argument,       0, 'P'},
    {"log",                      required_argument, 0, 'L'}, /* info=src1/...,warn=src3/... */
    {"nofork",                   no_argument,       0, 'n'},
    {"version",                  no_argument,       0, 'v'},
    {"AllowNoInt",               required_argument, 0, 'A'}, /* (yes/no) */
    {"ClearScreen",              required_argument, 0, 'C'}, /* (yes/no) */
    {"DebugLevel",               required_argument, 0, 'd'}, /* (i) */
    {"FIBMetric",                required_argument, 0, 'F'}, /* (str) */
    {"Hna4",                     required_argument, 0, '4'}, /* (4body) */
    {"Hna6",                     required_argument, 0, '6'}, /* (6body) */
    {"Interface",                required_argument, 0, 'I'}, /* (if1 if2 {ifbody}) */
    {"IpcConnect",               required_argument, 0, 'Q'}, /* (Host,Net,MaxConnections) */
    {"IpVersion",                required_argument, 0, 'V'}, /* (i) */
    {"LinkQualityDijkstraLimit", required_argument, 0, 'J'}, /* (i,f) */
    {"LinkQualityFishEye",       required_argument, 0, 'E'}, /* (i) */
    {"LoadPlugin",               required_argument, 0, 'p'}, /* (soname {PlParams}) */
    {"MprCoverage",              required_argument, 0, 'M'}, /* (i) */
    {"NatThreshold",             required_argument, 0, 'N'}, /* (f) */
    {"NicChgsPollInt",           required_argument, 0, 'Y'}, /* (f) */
    {"Pollrate",                 required_argument, 0, 'T'}, /* (f) */
    {"RtProto",                  required_argument, 0, 'q'}, /* (i) */
    {"RtTableDefault",           required_argument, 0, 'R'}, /* (i) */
    {"RtTable",                  required_argument, 0, 'r'}, /* (i) */
    {"TcRedundancy",             required_argument, 0, 't'}, /* (i) */
    {"TosValue",                 required_argument, 0, 'Z'}, /* (i) */
    {"Willingness",              required_argument, 0, 'w'}, /* (i) */
    {0, 0, 0, 0}
  }, *popt = long_options;
  /* *INDENT-ON* */

  /*
   * olsr_malloc() uses calloc, so opt_line is already filled
   * memset(opt_line, 0, argc * sizeof(opt_line[0]));
   */

  /* Copy argv array for safe free'ing later on */
  while (opt_argc < argc) {
    const char *p = argv[opt_argc];
    if (0 == strcmp(p, "-nofork"))
      p = "-n";
#ifdef WIN32
    else if (0 == strcmp(p, "-int"))
      p = "-l";
#endif
    opt_argv[opt_argc] = olsr_strdup(p);
    opt_argc++;
  }

  /* get option count */
  for (opt_idx = 0; long_options[opt_idx].name; opt_idx++);

  /* Calculate short option string */
  opt_str = olsr_malloc(opt_idx * 3, "create short opt_string");
  opt_idx = 0;
  while (popt->name != NULL && popt->val != 0) {
    opt_str[opt_idx++] = popt->val;

    switch (popt->has_arg) {
    case optional_argument:
      opt_str[opt_idx++] = ':';
      /* Fall through */
    case required_argument:
      opt_str[opt_idx++] = ':';
      break;
    }
    popt++;
  }

  /* If no arguments, revert to default behaviour */
  if (1 == opt_argc) {
    char *argv0_tmp = opt_argv[0];
    free(opt_argv);
    opt_argv = olsr_malloc(3 * sizeof(opt_argv[0]), "default argv");
    opt_argv[0] = argv0_tmp;
    opt_argv[1] = olsr_strdup("-f");
    opt_argv[2] = olsr_strdup(conf_file_name);
    opt_argc = 3;
  }

  while (0 <= (opt = getopt_long(opt_argc, opt_argv, opt_str, long_options, &opt_idx))) {
    switch (opt) {
    case 'f':                  /* config (filename) */
      PARSER_DEBUG_PRINTF("Read config from %s\n", optarg);
      if (0 > read_cfg(optarg, &opt_argc, &opt_argv, &opt_line)) {
        fprintf(stderr, "Could not find specified config file %s!\n%s\n\n", optarg, strerror(errno));
        exit(EXIT_FAILURE);
      }
      break;
    case 'h':                  /* help */
      popt = long_options;
      printf("Usage: olsrd [OPTIONS]... [interfaces]...\n");
      while (popt->name) {
        if (popt->val)
          printf("-%c or ", popt->val);
        else
          printf("       ");
        printf("--%s ", popt->name);
        switch (popt->has_arg) {
        case required_argument:
          printf("arg");
          break;
        case optional_argument:
          printf("[arg]");
          break;
        }
        printf("\n");
        popt++;
      }
      exit(0);
    default:
      parse_cfg_option(opt, optarg, opt_line[optind], cfg);
    }                           /* switch(opt) */
  }                             /* while getopt_long() */

  while (optind < opt_argc) {
    PARSER_DEBUG_PRINTF("new interface %s\n", opt_argv[optind]);
    queue_if(opt_argv[optind++], cfg);
  }

  /* Some cleanup */
  while (0 < opt_argc) {
    free(opt_argv[--opt_argc]);
  }
  free(opt_argv);
  free(opt_line);
  free(opt_str);

  return cfg;
}

/*
 * Checks a given config for illegal values
 */
int
olsr_sanity_check_cfg(struct olsr_config *cfg)
{
  struct olsr_if_config *in = cfg->if_configs;
  struct olsr_if_options *io;

  /* Debug level */
  if (cfg->debug_level < MIN_DEBUGLVL || cfg->debug_level > MAX_DEBUGLVL) {
    fprintf(stderr, "Debuglevel %d is not allowed\n", cfg->debug_level);
    return -1;
  }

  /* IP version */
  if (cfg->ip_version != AF_INET && cfg->ip_version != AF_INET6) {
    fprintf(stderr, "Ipversion %d not allowed!\n", cfg->ip_version);
    return -1;
  }

  /* TOS */
  if (cfg->tos > MAX_TOS) {
    fprintf(stderr, "TOS %d is not allowed\n", cfg->tos);
    return -1;
  }

  /* Willingness */
  if (cfg->willingness_auto == false && (cfg->willingness > MAX_WILLINGNESS)) {
    fprintf(stderr, "Willingness %d is not allowed\n", cfg->willingness);
    return -1;
  }

  /* Check Link quality dijkstra limit */
  if (cfg->lq_dinter < conv_pollrate_to_secs(cfg->pollrate) && cfg->lq_dlimit != 255) {
    fprintf(stderr, "Link quality dijkstra limit must be higher than pollrate\n");
    return -1;
  }

  /* NIC Changes Pollrate */
  if (cfg->nic_chgs_pollrate < MIN_NICCHGPOLLRT || cfg->nic_chgs_pollrate > MAX_NICCHGPOLLRT) {
    fprintf(stderr, "NIC Changes Pollrate %0.2f is not allowed\n", cfg->nic_chgs_pollrate);
    return -1;
  }

  /* TC redundancy */
  if (cfg->tc_redundancy > MAX_TC_REDUNDANCY) {
    fprintf(stderr, "TC redundancy %d is not allowed\n", cfg->tc_redundancy);
    return -1;
  }

  /* MPR coverage */
  if (cfg->mpr_coverage < MIN_MPR_COVERAGE || cfg->mpr_coverage > MAX_MPR_COVERAGE) {
    fprintf(stderr, "MPR coverage %d is not allowed\n", cfg->mpr_coverage);
    return -1;
  }

  /* NAT threshold value */
  if (cfg->lq_nat_thresh < 0.1 || cfg->lq_nat_thresh > 1.0) {
    fprintf(stderr, "NAT threshold %f is not allowed\n", cfg->lq_nat_thresh);
    return -1;
  }

  if (in == NULL) {
    fprintf(stderr, "No interfaces configured!\n");
    return -1;
  }

  /* Interfaces */
  while (in) {
    io = in->cnf;

    if (in->name == NULL || !strlen(in->name)) {
      fprintf(stderr, "Interface has no name!\n");
      return -1;
    }

    if (io == NULL) {
      fprintf(stderr, "Interface %s has no configuration!\n", in->name);
      return -1;
    }

    /* HELLO interval */

    if (io->hello_params.emission_interval < conv_pollrate_to_secs(cfg->pollrate) ||
        io->hello_params.emission_interval > io->hello_params.validity_time) {
      fprintf(stderr, "Bad HELLO parameters! (em: %0.2f, vt: %0.2f)\n", io->hello_params.emission_interval,
              io->hello_params.validity_time);
      return -1;
    }

    /* TC interval */
    if (io->tc_params.emission_interval < conv_pollrate_to_secs(cfg->pollrate) ||
        io->tc_params.emission_interval > io->tc_params.validity_time) {
      fprintf(stderr, "Bad TC parameters! (em: %0.2f, vt: %0.2f)\n", io->tc_params.emission_interval, io->tc_params.validity_time);
      return -1;
    }

    /* MID interval */
    if (io->mid_params.emission_interval < conv_pollrate_to_secs(cfg->pollrate) ||
        io->mid_params.emission_interval > io->mid_params.validity_time) {
      fprintf(stderr, "Bad MID parameters! (em: %0.2f, vt: %0.2f)\n", io->mid_params.emission_interval,
              io->mid_params.validity_time);
      return -1;
    }

    /* HNA interval */
    if (io->hna_params.emission_interval < conv_pollrate_to_secs(cfg->pollrate) ||
        io->hna_params.emission_interval > io->hna_params.validity_time) {
      fprintf(stderr, "Bad HNA parameters! (em: %0.2f, vt: %0.2f)\n", io->hna_params.emission_interval,
              io->hna_params.validity_time);
      return -1;
    }

    in = in->next;
  }

  return 0;
}

/*
 * Free resources occupied by a configuration
 */
void
olsr_free_cfg(struct olsr_config *cfg)
{
  struct olsr_if_config *ind, *in = cfg->if_configs;
  struct plugin_entry *ped, *pe = cfg->plugins;
  struct olsr_lq_mult *mult, *next_mult;

  /*
   * Free HNAs.
   */
  ip_prefix_list_flush(&cfg->hna_entries);

  /*
   * Free IPCs.
   */
  ip_acl_flush(&cfg->ipc_nets);

  /*
   * Free Interfaces - remove_interface() already called
   */
  while (in) {
    for (mult = in->cnf->lq_mult; mult != NULL; mult = next_mult) {
      next_mult = mult->next;
      free(mult);
    }

    free(in->cnf);
    ind = in;
    in = in->next;
    free(ind->name);
    if (ind->config)
      free(ind->config);
    free(ind);
  }

  /*
   * Free Plugins - olsr_close_plugins() allready called
   */
  while (pe) {
    struct plugin_param *pard, *par = pe->params;
    while (par) {
      pard = par;
      par = par->next;
      free(pard->key);
      free(pard->value);
      free(pard);
    }
    ped = pe;
    pe = pe->next;
    free(ped->name);
    free(ped);
  }

  free(cfg);

  return;
}

/*
 * Get a default config
 */
struct olsr_config *
olsr_get_default_cfg(void)
{
  struct olsr_config *cfg = olsr_malloc(sizeof(struct olsr_config), "default config");

  /*
   * olsr_malloc calls calloc(), no memset necessary:
   * memset(cfg, 0, sizeof(*cfg));
   */

  cfg->debug_level = DEF_DEBUGLVL;
  cfg->no_fork = false;
  cfg->ip_version = AF_INET;
  cfg->ipsize = sizeof(struct in_addr);
  cfg->maxplen = 32;
  cfg->allow_no_interfaces = DEF_ALLOW_NO_INTS;
  cfg->tos = DEF_TOS;
  cfg->rtproto = 3;
  cfg->rttable = 254;
  cfg->rttable_default = 0;
  cfg->willingness_auto = DEF_WILL_AUTO;
  cfg->ipc_connections = DEF_IPC_CONNECTIONS;
  cfg->fib_metric = DEF_FIB_METRIC;

  cfg->pollrate = conv_pollrate_to_microsecs(DEF_POLLRATE);
  cfg->nic_chgs_pollrate = DEF_NICCHGPOLLRT;

  cfg->tc_redundancy = TC_REDUNDANCY;
  cfg->mpr_coverage = MPR_COVERAGE;
  cfg->lq_fish = DEF_LQ_FISH;
  cfg->lq_dlimit = DEF_LQ_DIJK_LIMIT;
  cfg->lq_dinter = DEF_LQ_DIJK_INTER;
  cfg->lq_nat_thresh = DEF_LQ_NAT_THRESH;
  cfg->clear_screen = DEF_CLEAR_SCREEN;

  cfg->del_gws = false;
  cfg->will_int = 10 * HELLO_INTERVAL;
  cfg->exit_value = EXIT_SUCCESS;
  cfg->max_tc_vtime = 0.0;
  cfg->ioctl_s = 0;

  list_head_init(&cfg->hna_entries);
  ip_acl_init(&cfg->ipc_nets);

#if defined linux
  cfg->rts_linux = 0;
#endif
#if defined __FreeBSD__ || defined __MacOSX__ || defined __NetBSD__ || defined __OpenBSD__
  cfg->rts_bsd = 0;
#endif
  return cfg;
}

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
