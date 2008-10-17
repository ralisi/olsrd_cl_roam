/*
 * The olsr.org Optimized Link-State Routing daemon(olsrd)
 * Copyright (c) 2004, Andreas Tønnesen(andreto@olsr.org)
 *                     includes code by Bruno Randolf
 *                     includes code by Andreas Lopatic
 *                     includes code by Sven-Ola Tuecke
 *                     includes code by Lorenz Schori
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

/*
 * Dynamic linked library for the olsr.org olsr daemon
 */
 
#include "olsrd_txtinfo.h"
#include "olsr.h"
#include "ipcalc.h"
#include "neighbor_table.h"
#include "two_hop_neighbor_table.h"
#include "mpr_selector_set.h"
#include "tc_set.h"
#include "hna_set.h"
#include "mid_set.h"
#include "routing_table.h"
#include "log.h"

#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/uio.h>

#ifdef WIN32
#define close(x) closesocket(x)
#endif


#define RESPCHUNK	4096
struct ipc_conn {
    int resplen;
    int respsize;
    int respstart;
    char *resp;
    int requlen;
    char requ[256];
};

static int ipc_socket = -1;


static void conn_destroy(struct ipc_conn *conn);

static int set_nonblocking(int);

static void ipc_action(int, void *, unsigned int);

static void ipc_http(int, void *, unsigned int);

static void ipc_http_read(int, struct ipc_conn *);

static void ipc_http_read_dummy(int, struct ipc_conn *);

static void ipc_http_write(int, struct ipc_conn *);

static int send_info(struct ipc_conn *, int);

static int ipc_print_neigh(struct ipc_conn *);

static int ipc_print_link(struct ipc_conn *);

static int ipc_print_routes(struct ipc_conn *);

static int ipc_print_topology(struct ipc_conn *);

static int ipc_print_hna(struct ipc_conn *);

static int ipc_print_mid(struct ipc_conn *);

static int ipc_sendf(struct ipc_conn *, const char *, ...) __attribute__((format(printf, 2, 3)));

#define isprefix(str, pre) (strncmp((str), (pre), strlen(pre)) == 0)

#define SIW_NEIGH	(1 << 0)
#define SIW_LINK	(1 << 1)
#define SIW_ROUTE	(1 << 2)
#define SIW_HNA		(1 << 3)
#define SIW_MID		(1 << 4)
#define SIW_TOPO	(1 << 5)
#define SIW_NEIGHLINK	(SIW_LINK|SIW_NEIGH)
#define SIW_ALL		(SIW_NEIGH|SIW_LINK|SIW_ROUTE|SIW_HNA|SIW_MID|SIW_TOPO)

/**
 * destructor - called at unload
 */
void olsr_plugin_exit(void)
{
    CLOSE(ipc_socket);
}

/**
 *Do initialization here
 *
 *This function is called by the my_init
 *function in uolsrd_plugin.c
 */
int
olsrd_plugin_init(void)
{
    struct sockaddr_storage sst;
    olsr_u32_t yes = 1;
    socklen_t addrlen;

    /* Init ipc socket */
    ipc_socket = socket(olsr_cnf->ip_version, SOCK_STREAM, 0);
    if (ipc_socket == -1) {
#ifndef NODEBUG
        olsr_printf(1, "(TXTINFO) socket()=%s\n", strerror(errno));
#endif
        return 0;
    }
    if (setsockopt(ipc_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes)) < 0) {
#ifndef NODEBUG
        olsr_printf(1, "(TXTINFO) setsockopt()=%s\n", strerror(errno));
#endif
        CLOSE(ipc_socket);
        return 0;
    }

#if defined __FreeBSD__ && defined SO_NOSIGPIPE
    if (setsockopt(ipc_socket, SOL_SOCKET, SO_NOSIGPIPE, (char *)&yes, sizeof(yes)) < 0) {
        perror("SO_REUSEADDR failed");
        CLOSE(ipc_socket);
        return 0;
    }
#endif
    /* Bind the socket */

    /* complete the socket structure */
    memset(&sst, 0, sizeof(sst));
    if (olsr_cnf->ip_version == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)&sst;
        addr4->sin_family = AF_INET;
        addrlen = sizeof(struct sockaddr_in);
#ifdef SIN6_LEN
        addr4->sin_len = addrlen;
#endif
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = htons(ipc_port);
    } else {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&sst;
        addr6->sin6_family = AF_INET6;
        addrlen = sizeof(struct sockaddr_in6);
#ifdef SIN6_LEN
        addr6->sin6_len = addrlen;
#endif
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = htons(ipc_port);
    }
      
    /* bind the socket to the port number */
    if (bind(ipc_socket, (struct sockaddr *) &sst, addrlen) == -1) {
#ifndef NODEBUG
        olsr_printf(1, "(TXTINFO) bind()=%s\n", strerror(errno));
#endif
        CLOSE(ipc_socket);
        return 0;
    }

    /* show that we are willing to listen */
    if (listen(ipc_socket, 1) == -1) {
#ifndef NODEBUG
        olsr_printf(1, "(TXTINFO) listen()=%s\n", strerror(errno));
#endif
        CLOSE(ipc_socket);
        return 0;
    }

    /* Register with olsrd */
    add_olsr_socket(ipc_socket, NULL, &ipc_action, NULL, SP_IMM_READ);
  
#ifndef NODEBUG
    olsr_printf(2, "(TXTINFO) listening on port %d\n",ipc_port);
#endif
    return 1;
}

/* destroy the connection */
static void conn_destroy(struct ipc_conn *conn)
{
    free(conn->resp);
    free(conn);
}

static void kill_connection(int fd, struct ipc_conn *conn)
{
    remove_olsr_socket(fd, NULL, &ipc_http);
    close(fd);
    conn_destroy(conn);
}

static int set_nonblocking(int fd)
{
    /* make the fd non-blocking */
    int socket_flags = fcntl(fd, F_GETFL);
    if (socket_flags < 0) {
        olsr_syslog(OLSR_LOG_ERR, "(TXTINFO) Cannot get the socket flags: %s", strerror(errno));
        return -1;
    }
    if (fcntl(fd, F_SETFL, socket_flags|O_NONBLOCK) < 0) {
        olsr_syslog(OLSR_LOG_ERR, "(TXTINFO) Cannot set the socket flags: %s", strerror(errno));
        return -1;
    }
    return 0;

}
static void ipc_action(int fd, void *data __attribute__((unused)), unsigned int flags __attribute__((unused)))
{
    struct ipc_conn *conn;
    struct sockaddr_storage pin;
    char addr[INET6_ADDRSTRLEN];
    socklen_t addrlen = sizeof(pin);
    int ipc_connection = accept(fd, (struct sockaddr *)&pin, &addrlen);

    if (ipc_connection == -1) {
        /* this may well happen if the other side immediately closes the connection. */
#ifndef NODEBUG
        olsr_printf(1, "(TXTINFO) accept()=%s\n", strerror(errno));
#endif
        return;
    }
    /* check if we want ot speak with it */
    if (olsr_cnf->ip_version == AF_INET) {
        const struct sockaddr_in *addr4 = (struct sockaddr_in *)&pin;
        if (inet_ntop(olsr_cnf->ip_version, &addr4->sin_addr, addr, sizeof(addr)) == NULL) {
             addr[0] = '\0';
        }
        if (!ip4equal(&addr4->sin_addr, &ipc_accept_ip.v4)) {
            olsr_printf(1, "(TXTINFO) From host(%s) not allowed!\n", addr);
            close(ipc_connection);
            return;
        }
    } else {
        const struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&pin;
        if (inet_ntop(olsr_cnf->ip_version, &addr6->sin6_addr, addr, sizeof(addr)) == NULL) {
             addr[0] = '\0';
        }
       /* Use in6addr_any (::) in olsr.conf to allow anybody. */
        if (!ip6equal(&in6addr_any, &ipc_accept_ip.v6) &&
           !ip6equal(&addr6->sin6_addr, &ipc_accept_ip.v6)) {
            olsr_printf(1, "(TXTINFO) From host(%s) not allowed!\n", addr);
            close(ipc_connection);
            return;
        }
    }
#ifndef NODEBUG
    olsr_printf(2, "(TXTINFO) Connect from %s\n",addr);
#endif
    
    /* make the fd non-blocking */
    if (set_nonblocking(fd) < 0) {
        close(ipc_connection);
        return;
    }

    conn = malloc(sizeof(*conn));
    if (conn == NULL) {
        olsr_syslog(OLSR_LOG_ERR, "(TXTINFO) Out of memory!");
        close(ipc_connection);
        return;
    }
    conn->requlen = 0;
    *conn->requ = '\0';
    conn->resplen = 0;
    conn->respstart = 0;
    conn->respsize = RESPCHUNK;
    conn->resp = malloc(conn->respsize);
    if (conn->resp == NULL) {
        olsr_syslog(OLSR_LOG_ERR, "(TXTINFO) Out of memory!");
        close(ipc_connection);
        free(conn);
        return;
    }
    *conn->resp = '\0';
    add_olsr_socket(ipc_connection, NULL, &ipc_http, conn, SP_IMM_READ);
}

static void ipc_http_read_dummy(int fd, struct ipc_conn *conn)
{
    /* just read dummy stuff */
    ssize_t bytes_read;
    char dummybuf[128];
    do {
        bytes_read = read(fd, dummybuf, sizeof(dummybuf));
    } while (bytes_read > 0);
    if (bytes_read == 0) {
        /* EOF */
        if (conn->respstart < conn->resplen && conn->resplen > 0) {
            disable_olsr_socket(fd, NULL, &ipc_http, SP_IMM_READ);
            conn->requlen = -1;
            return;
        }
    } else if (errno == EINTR || errno == EAGAIN) {
        /* ignore interrupted sys-calls */
        return;
    }
    /* we had either an error or EOF and we are completely done */
    kill_connection(fd, conn);
}

static void ipc_http_read(int fd, struct ipc_conn *conn)
{
    int send_what;
    const char *p;
    ssize_t bytes_read = read(fd, conn->requ+conn->requlen, sizeof(conn->requ)-conn->requlen-1); /* leave space for a terminating '\0' */
    if (bytes_read < 0) {
        if (errno == EINTR || errno == EAGAIN) {
            return;
        }
        olsr_syslog(OLSR_LOG_ERR, "(TXTINFO) read error: %s", strerror(errno));
        kill_connection(fd, conn);
        return;
    }
    conn->requlen += bytes_read;
    conn->requ[conn->requlen] = '\0';

    /* look if we have the necessary info. We get here somethign like "GET /path HTTP/1.0" */
    p = strchr(conn->requ, '/');
    if (p == NULL) {
        /* we didn't get all. Wait for more data. */
        return;
    }
    if (isprefix(p, "/neighbours")) send_what=SIW_NEIGHLINK;
    else if (isprefix(p, "/neigh")) send_what=SIW_NEIGH;
    else if (isprefix(p, "/link")) send_what=SIW_LINK;
    else if (isprefix(p, "/route")) send_what=SIW_ROUTE;
    else if (isprefix(p, "/hna")) send_what=SIW_HNA;
    else if (isprefix(p, "/mid")) send_what=SIW_MID;
    else if (isprefix(p, "/topo")) send_what=SIW_TOPO;    
    else send_what = SIW_ALL;

    if (send_info(conn, send_what) < 0) {
        kill_connection(fd, conn);
        return;
    }
    enable_olsr_socket(fd, NULL, &ipc_http, SP_IMM_WRITE);
}

static void ipc_http_write(int fd, struct ipc_conn *conn)
{
    ssize_t bytes_written = write(fd, conn->resp+conn->respstart, conn->resplen-conn->respstart);
    if (bytes_written < 0) {
        if (errno == EINTR || errno == EAGAIN) {
            return;
        }
        olsr_syslog(OLSR_LOG_ERR, "(TXTINFO) write error: %s", strerror(errno));
        kill_connection(fd, conn);
        return;
    }
    conn->respstart += bytes_written;
    if (conn->respstart >= conn->resplen) {
        /* we are done. */
        if (conn->requlen < 0) {
            /* we are completely done. */
            kill_connection(fd, conn);
        } else if (shutdown(fd, SHUT_WR) < 0) {
            kill_connection(fd, conn);
        } else {
            disable_olsr_socket(fd, NULL, &ipc_http, SP_IMM_WRITE);
        }
    }
}

static void ipc_http(int fd, void *data, unsigned int flags)
{
    struct ipc_conn *conn = data;
    if ((flags & SP_IMM_READ) != 0) {
        if (conn->resplen > 0) {
            ipc_http_read_dummy(fd, conn);
        } else {
            ipc_http_read(fd, conn);
        }
    }
    if ((flags & SP_IMM_WRITE) != 0) {
        ipc_http_write(fd, conn);
    }
}
    

static int ipc_print_neigh(struct ipc_conn *conn)
{
    struct neighbor_entry *neigh;

    if (ipc_sendf(conn, "Table: Neighbors\nIP address\tSYM\tMPR\tMPRS\tWill.\t2 Hop Neighbors\n") < 0) {
        return -1;
    }

    /* Neighbors */
    OLSR_FOR_ALL_NBR_ENTRIES(neigh) {
        struct neighbor_2_list_entry *list_2;
        struct ipaddr_str buf1;
        int thop_cnt = 0;
        for (list_2 = neigh->neighbor_2_list.next;
             list_2 != &neigh->neighbor_2_list;
             list_2 = list_2->next) {
            thop_cnt++;
        }
        if (ipc_sendf(conn,
                      "%s\t%s\t%s\t%s\t%d\t%d\n",
                      olsr_ip_to_string(&buf1, &neigh->neighbor_main_addr),
                      (neigh->status == SYM) ? "YES" : "NO",
                      neigh->is_mpr ? "YES" : "NO",
                      olsr_lookup_mprs_set(&neigh->neighbor_main_addr) ? "YES" : "NO",
                      neigh->willingness,
                      thop_cnt) < 0) {
            return -1;
        }
    } OLSR_FOR_ALL_NBR_ENTRIES_END(neigh);
    if (ipc_sendf(conn, "\n") < 0) {
        return -1;
    }
    return 0;
}

static int ipc_print_link(struct ipc_conn *conn)
{
    struct link_entry *lnk;

    if (ipc_sendf(conn, "Table: Links\nLocal IP\tRemote IP\tHyst.\tLQ\tNLQ\tCost\n") < 0) {
        return -1;
    }

    /* Link set */
    OLSR_FOR_ALL_LINK_ENTRIES(lnk) {
        struct ipaddr_str buf1, buf2;
        struct lqtextbuffer lqbuffer1, lqbuffer2;
	if (ipc_sendf(conn,
                      "%s\t%s\t%0.2f\t%s\t%s\t\n",
                      olsr_ip_to_string(&buf1, &lnk->local_iface_addr),
                      olsr_ip_to_string(&buf2, &lnk->neighbor_iface_addr),
                      lnk->L_link_quality, 
                      get_link_entry_text(lnk, '\t', &lqbuffer1),
                      get_linkcost_text(lnk->linkcost, OLSR_FALSE, &lqbuffer2)) < 0) {
            return -1;
        }
    } OLSR_FOR_ALL_LINK_ENTRIES_END(lnk);

    if (ipc_sendf(conn, "\n") < 0) {
        return -1;
    }
    return 0;
}

static int ipc_print_routes(struct ipc_conn *conn)
{
    struct rt_entry *rt;
    
    if (ipc_sendf(conn, "Table: Routes\nDestination\tGateway IP\tMetric\tETX\tInterface\n") < 0) {
        return -1;
    }

    /* Walk the route table */
    OLSR_FOR_ALL_RT_ENTRIES(rt) {
        struct ipaddr_str buf1, buf2;
        struct lqtextbuffer lqbuffer;
        if (ipc_sendf(conn,
                      "%s/%d\t%s\t%d\t%s\t%s\t\n",
                      olsr_ip_to_string(&buf1, &rt->rt_dst.prefix),
                      rt->rt_dst.prefix_len,
                      olsr_ip_to_string(&buf2, &rt->rt_best->rtp_nexthop.gateway),
                      rt->rt_best->rtp_metric.hops,
                      get_linkcost_text(rt->rt_best->rtp_metric.cost, OLSR_TRUE, &lqbuffer),
                      if_ifwithindex_name(rt->rt_best->rtp_nexthop.iif_index)) < 0) {
            return -1;
        }
    } OLSR_FOR_ALL_RT_ENTRIES_END(rt);

    if (ipc_sendf(conn, "\n") < 0) {
        return -1;
    }
    return 0;
}

static int ipc_print_topology(struct ipc_conn *conn)
{
    struct tc_entry *tc;
    
    if (ipc_sendf(conn, "Table: Topology\nDest. IP\tLast hop IP\tLQ\tNLQ\tCost\n") < 0) {
        return -1;
    }

    /* Topology */  
    OLSR_FOR_ALL_TC_ENTRIES(tc) {
        struct tc_edge_entry *tc_edge;
        OLSR_FOR_ALL_TC_EDGE_ENTRIES(tc, tc_edge) {
        	if (tc_edge->edge_inv)  {
            struct ipaddr_str dstbuf, addrbuf;
            struct lqtextbuffer lqbuffer1, lqbuffer2;
            if (ipc_sendf(conn,
                          "%s\t%s\t%s\t%s\n", 
                          olsr_ip_to_string(&dstbuf, &tc_edge->T_dest_addr),
                          olsr_ip_to_string(&addrbuf, &tc->addr), 
                          get_tc_edge_entry_text(tc_edge, '\t', &lqbuffer1),
                          get_linkcost_text(tc_edge->cost, OLSR_FALSE, &lqbuffer2)) < 0) {
                return -1;
            }
        	}
        } OLSR_FOR_ALL_TC_EDGE_ENTRIES_END(tc, tc_edge);
    } OLSR_FOR_ALL_TC_ENTRIES_END(tc);

    if (ipc_sendf(conn, "\n") < 0) {
        return -1;
    }
    return 0;
}

static int ipc_print_hna(struct ipc_conn *conn)
{
    const struct ip_prefix_list *hna;
    struct tc_entry *tc;
    struct ipaddr_str addrbuf, mainaddrbuf;

    if (ipc_sendf(conn, "Table: HNA\nDestination\tGateway\n") < 0) {
        return -1;
    }

    /* Announced HNA entries */
    for (hna = olsr_cnf->hna_entries; hna != NULL; hna = hna->next) {
        if (ipc_sendf(conn,
                      "%s/%d\t%s\n",
                      olsr_ip_to_string(&addrbuf, &hna->net.prefix),
                      hna->net.prefix_len,
                      olsr_ip_to_string(&mainaddrbuf, &olsr_cnf->main_addr)) < 0) {
            return -1;
        }
    }

    /* HNA entries */
    OLSR_FOR_ALL_TC_ENTRIES(tc) {
        struct hna_net *tmp_net;
        /* Check all networks */
        OLSR_FOR_ALL_TC_HNA_ENTRIES(tc, tmp_net) {
            if (ipc_sendf(conn,
                          "%s/%d\t%s\n",
                          olsr_ip_to_string(&addrbuf, &tmp_net->hna_prefix.prefix),
                          tmp_net->hna_prefix.prefix_len,
                          olsr_ip_to_string(&mainaddrbuf, &tc->addr)) < 0) {
                return -1;
            }
        } OLSR_FOR_ALL_TC_HNA_ENTRIES_END(tc, tmp_net);
    } OLSR_FOR_ALL_TC_ENTRIES_END(tc);

    if (ipc_sendf(conn, "\n") < 0) {
        return -1;
    }
    return 0;
}

static int ipc_print_mid(struct ipc_conn *conn)
{
    struct tc_entry *tc;

    if (ipc_sendf(conn, "Table: MID\nIP address\tAliases\n") < 0) {
        return -1;
    }

    /* MID root is the TC entry */
    OLSR_FOR_ALL_TC_ENTRIES(tc) {
        struct ipaddr_str buf;
        struct mid_entry *alias;
        char sep = '\t';
        if (ipc_sendf(conn, "%s",  olsr_ip_to_string(&buf,  &tc->addr)) < 0) {
            return -1;
        }

        OLSR_FOR_ALL_TC_MID_ENTRIES(tc, alias) {
            if (ipc_sendf(conn, "%c%s", sep, olsr_ip_to_string(&buf, &alias->mid_alias_addr)) < 0) {
                return -1;
            }
            sep = ';';
        }  OLSR_FOR_ALL_TC_MID_ENTRIES_END(tc, alias);
        if (ipc_sendf(conn, "\n") < 0) {
            return -1;
        }
    } OLSR_FOR_ALL_TC_ENTRIES_END(tc);
    if (ipc_sendf(conn, "\n") < 0) {
        return -1;
    }
    return 0;
}

static int send_info(struct ipc_conn *conn, int send_what)
{
    int rv;

    /* Print minimal http header */
    if (ipc_sendf(conn,
                  "HTTP/1.0 200 OK\n"
                  "Content-type: text/plain\n\n") < 0) {
        return -1;
    }
 
     /* Print tables to IPC socket */
	
    rv = 0;
     /* links + Neighbors */
    if ((send_what & (SIW_NEIGHLINK|SIW_LINK)) != 0) {
        if (ipc_print_link(conn) < 0) {
            rv = -1;
        }
    }
    if ((send_what & (SIW_NEIGHLINK|SIW_NEIGH)) != 0) {
        if (ipc_print_neigh(conn) < 0) {
            rv = -1;
        }
    }
     /* topology */
    if ((send_what & SIW_TOPO) != 0) {
        rv = ipc_print_topology(conn);
    }
     /* hna */
    if ((send_what & SIW_HNA) != 0) {
        rv = ipc_print_hna(conn);
    }
     /* mid */
    if ((send_what & SIW_MID) != 0) {
        rv = ipc_print_mid(conn);
    }
     /* routes */
    if ((send_what & SIW_ROUTE) != 0) {
        rv = ipc_print_routes(conn);
    }
    return rv;
}

/*
 * In a bigger mesh, there are probs with the fixed
 * bufsize. Because the Content-Length header is
 * optional, the sprintf() is changed to a more
 * scalable solution here.
 */ 
static int ipc_sendf(struct ipc_conn *conn, const char* format, ...)
{
    va_list arg, arg2;
    int rv;
#if defined __FreeBSD__ || defined __NetBSD__ || defined __OpenBSD__ || defined __MacOSX__
#define flags 0
#else
#define flags MSG_NOSIGNAL
#endif
    va_start(arg, format);
    va_copy(arg2, arg);
    rv = vsnprintf(conn->resp+conn->resplen, conn->respsize-conn->resplen, format, arg);
    va_end(arg);
    if (conn->resplen + rv >= conn->respsize) {
        char *p;
        conn->respsize += RESPCHUNK;
        p = realloc(conn->resp, conn->respsize);
        if (p == NULL) {
            olsr_syslog(OLSR_LOG_ERR, "(TXTINFO) Out of memory!");
            return -1;
        }
        conn->resp = p;
        vsnprintf(conn->resp+conn->resplen, conn->respsize-conn->resplen, format, arg2);
    }
    va_end(arg2);
    conn->resplen += rv;
    return 0;
}

/*
 * Local Variables:
 * mode: c
 * style: linux
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
