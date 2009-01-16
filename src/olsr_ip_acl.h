
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

#ifndef OLSR_IP_PREFIX_ACL_H_
#define OLSR_IP_PREFIX_ACL_H_

#include "defs.h"
#include "olsr_types.h"
#include "plugin.h"
#include "common/list.h"

#define IP_ACL_ACCEPT_PARAP         "accept"
#define IP_ACL_REJECT_PARAM         "reject"
#define IP_ACL_CHECKFIRST_PARAM     "checkFirst"
#define IP_ACL_DEFAULTPOLICY_PARAM "defaultPolicy"

struct ip_acl {
  struct list_node accept;
  struct list_node reject;
  bool first_accept;
  bool default_accept;
};

void EXPORT(ip_acl_init) (struct ip_acl *);
void EXPORT(ip_acl_flush) (struct ip_acl *);
void EXPORT(ip_acl_add) (struct ip_acl *, const union olsr_ip_addr *, uint8_t, bool);
void EXPORT(ip_acl_remove) (struct ip_acl *, const union olsr_ip_addr *, uint8_t, bool);
bool EXPORT(ip_acl_acceptable) (struct ip_acl *, const union olsr_ip_addr *);

/**
 *  accessor methods for plugins
 *
 *  Add the following lines into your plugins parameter list to allow acl initalization
 *  (this assumes that the acl is called allowed_nets, rename if neccesary)
 *
 *  { .name = IP_ACL_ACCEPT_PARAP,        .set_plugin_parameter = &ip_acl_add_plugin_accept,  .data = &allowed_nets },
 *  { .name = IP_ACL_REJECT_PARAM,        .set_plugin_parameter = &ip_acl_add_plugin_reject,  .data = &allowed_nets },
 *  { .name = IP_ACL_CHECKFIRST_PARAM,    .set_plugin_parameter = &ip_acl_add_plugin_checkFirst, .data = &allowed_nets },
 *  { .name = IP_ACL_DEFAULTPOLICY_PARAM, .set_plugin_parameter = &ip_acl_add_plugin_defaultPolicy, .data = &allowed_nets },
 */
int EXPORT(ip_acl_add_plugin_accept) (const char *value, void *data, set_plugin_parameter_addon addon __attribute__ ((unused)));
int EXPORT(ip_acl_add_plugin_reject) (const char *value, void *data, set_plugin_parameter_addon addon __attribute__ ((unused)));
int EXPORT(ip_acl_add_plugin_checkFirst) (const char *value, void *data, set_plugin_parameter_addon addon __attribute__ ((unused)));
int EXPORT(ip_acl_add_plugin_defaultPolicy) (const char *value, void *data, set_plugin_parameter_addon addon
                                             __attribute__ ((unused)));

#endif /* OLSR_IP_PREFIX_ACL_H_ */
