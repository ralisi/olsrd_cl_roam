/*
 * HTTP Info plugin for the olsr.org OLSR daemon
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
 * $Id: olsrd_httpinfo.c,v 1.30 2005/01/02 14:26:14 kattemat Exp $
 */

/*
 * Dynamic linked library for the olsr.org olsr daemon
 */

#include "olsrd_httpinfo.h"
#include "olsr_cfg.h"
#include "gfx.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#ifdef WIN32
#define close(x) closesocket(x)
#endif

#define MAX_CLIENTS 3

#define MAX_HTTPREQ_SIZE 1024 * 10

#define DEFAULT_TCP_PORT 8080

#define HTML_BUFSIZE 1024*50

#define FRAMEWIDTH 800

#define ACTIVE_TAB "class=\"active\""


#define FILENREQ_MATCH(req, filename) \
        !strcmp(req, filename) || \
        (strlen(req) && !strcmp(&req[1], filename))

struct tab_entry
{
  char *tab_label;
  char *filename;
  int(*build_body_cb)(char *, olsr_u32_t);
};

struct static_bin_file_entry
{
  char *filename;
  unsigned char *data;
  unsigned int data_size;
};

struct static_txt_file_entry
{
  char *filename;
  const char **data;
};

static int
get_http_socket(int);

int
build_tabs(char *, int);

void
parse_http_request(int);

int
build_http_header(http_header_type, olsr_bool, olsr_u32_t, char *, olsr_u32_t);

static int
build_frame(char *, char *, int, char *, olsr_u32_t, int(*frame_body_cb)(char *, olsr_u32_t));

int
build_routes_body(char *, olsr_u32_t);

int
build_status_body(char *, olsr_u32_t);

int
build_neigh_body(char *, olsr_u32_t);

int
build_topo_body(char *, olsr_u32_t);

int
build_hna_body(char *, olsr_u32_t);

int
build_mid_body(char *, olsr_u32_t);

int
build_nodes_body(char *, olsr_u32_t);

int
build_all_body(char *, olsr_u32_t);

int
build_about_body(char *, olsr_u32_t);

char *
sockaddr_to_string(struct sockaddr *);

olsr_bool
check_allowed_ip(union olsr_ip_addr *);

static struct timeval start_time;
static struct http_stats stats;
static int client_sockets[MAX_CLIENTS];
static int curr_clients;
static int http_socket;



struct tab_entry tab_entries[] =
  {
    {"Status", "status", build_status_body},
    {"Routes", "routes", build_routes_body},
    {"Links/Topology", "nodes", build_nodes_body},
    {"All", "all", build_all_body},
    {"About", "about", build_about_body},
    {NULL, NULL, NULL}
  };

struct static_bin_file_entry static_bin_files[] =
  {
    {"favicon.ico", favicon_ico, 1406/*favicon_ico_len*/},
    {"logo.gif", logo_gif, 2801/*logo_gif_len*/},
    {"grayline.gif", grayline_gif, 43/*grayline_gif_len*/},
    {NULL, NULL, 0}
  };

struct static_txt_file_entry static_txt_files[] =
  {
    {"httpinfo.css", httpinfo_css},
    {NULL, NULL}
  };

/**
 *Do initialization here
 *
 *This function is called by the my_init
 *function in uolsrd_plugin.c
 */
int
olsr_plugin_init()
{
  /* Get start time */
  gettimeofday(&start_time, NULL);

  curr_clients = 0;
  /* set up HTTP socket */
  http_socket = get_http_socket(http_port != 0 ? http_port :  DEFAULT_TCP_PORT);

  if(http_socket < 0)
    {
      fprintf(stderr, "(HTTPINFO) could not initialize HTTP socket\n");
      exit(0);
    }

  /* Register socket */
  add_olsr_socket(http_socket, &parse_http_request);

  return 1;
}

static int
get_http_socket(int port)
{
  struct sockaddr_in sin;
  olsr_u32_t yes = 1;
  int s;

  /* Init ipc socket */
  if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
      olsr_printf(1, "(HTTPINFO)socket %s\n", strerror(errno));
      return -1;
    }

  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes)) < 0) 
    {
      olsr_printf(1, "(HTTPINFO)SO_REUSEADDR failed %s\n", strerror(errno));
      close(s);
      return -1;
    }

  /* Bind the socket */
  
  /* complete the socket structure */
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);
  
  /* bind the socket to the port number */
  if (bind(s, (struct sockaddr *) &sin, sizeof(sin)) == -1) 
    {
      olsr_printf(1, "(HTTPINFO) bind failed %s\n", strerror(errno));
      return -1;
    }
      
  /* show that we are willing to listen */
  if (listen(s, 1) == -1) 
    {
      olsr_printf(1, "(HTTPINFO) listen failed %s\n", strerror(errno));
      return -1;
    }

  return s;
}


/* Non reentrant - but we are not multithreaded anyway */
void
parse_http_request(int fd)
{
  struct sockaddr_in pin;
  socklen_t addrlen;
  char *addr;  
  char req[MAX_HTTPREQ_SIZE];
  static char body[HTML_BUFSIZE];
  char req_type[11];
  char filename[251];
  char http_version[11];
  int c = 0, r = 1, size = 0;

  addrlen = sizeof(struct sockaddr_in);

  if(curr_clients >= MAX_CLIENTS)
    return;

  curr_clients++;

  if ((client_sockets[curr_clients] = accept(fd, (struct sockaddr *)  &pin, &addrlen)) == -1)
    {
      olsr_printf(1, "(HTTPINFO) accept: %s\n", strerror(errno));
      goto close_connection;
    }

  if(!check_allowed_ip((union olsr_ip_addr *)&pin.sin_addr.s_addr))
    {
      olsr_printf(1, "HTTP request from non-allowed host %s!\n", 
		  olsr_ip_to_string((union olsr_ip_addr *)&pin.sin_addr.s_addr));
      close(client_sockets[curr_clients]);
    }

  addr = inet_ntoa(pin.sin_addr);


  memset(req, 0, MAX_HTTPREQ_SIZE);
  memset(body, 0, 1024*10);

  while((r = recv(client_sockets[curr_clients], &req[c], 1, 0)) > 0 && (c < (MAX_HTTPREQ_SIZE-1)))
    {
      c++;

      if((c > 3 && !strcmp(&req[c-4], "\r\n\r\n")) ||
	 (c > 1 && !strcmp(&req[c-2], "\n\n")))
	break;
    }
  
  if(r < 0)
    {
      olsr_printf(1, "(HTTPINFO) Failed to recieve data from client!\n");
      stats.err_hits++;
      goto close_connection;
    }
  
  /* Get the request */
  if(sscanf(req, "%10s %250s %10s\n", req_type, filename, http_version) != 3)
    {
      /* Try without HTTP version */
      if(sscanf(req, "%10s %250s\n", req_type, filename) != 2)
	{
	  olsr_printf(1, "(HTTPINFO) Error parsing request %s!\n", req);
	  stats.err_hits++;
	  goto close_connection;
	}
    }
  
  
  olsr_printf(1, "Request: %s\nfile: %s\nVersion: %s\n\n", req_type, filename, http_version);

  if(strcmp(req_type, "GET"))
    {
      /* We only support GET */
      strcpy(body, HTTP_400_MSG);
      stats.ill_hits++;
      c = build_http_header(HTTP_BAD_REQ, OLSR_TRUE, strlen(body), req, MAX_HTTPREQ_SIZE);
    }
  else
    {
      int i = 0;
      int y = 0;

      while(static_bin_files[i].filename)
	{
	  if(FILENREQ_MATCH(filename, static_bin_files[i].filename))
	    break;
	  i++;
	}
      
      if(static_bin_files[i].filename)
	{
	  stats.ok_hits++;
	  memcpy(body, static_bin_files[i].data, static_bin_files[i].data_size);
	  size = static_bin_files[i].data_size;
	  c = build_http_header(HTTP_OK, OLSR_FALSE, size, req, MAX_HTTPREQ_SIZE);  
	  goto send_http_data;
	}

      i = 0;

      while(static_txt_files[i].filename)
	{
	  if(FILENREQ_MATCH(filename, static_txt_files[i].filename))
	    break;
	  i++;
	}
      
      if(static_txt_files[i].filename)
	{
	  stats.ok_hits++;
	  y = 0;
	  while(static_txt_files[i].data[y])
	    {
	      size += sprintf(&body[size], static_txt_files[i].data[y]);
	      y++;
	    }

	  c = build_http_header(HTTP_OK, OLSR_FALSE, size, req, MAX_HTTPREQ_SIZE);  
	  goto send_http_data;
	}

      i = 0;

      if(strlen(filename) > 1)
	{
	  while(tab_entries[i].filename)
	    {
	      if(FILENREQ_MATCH(filename, tab_entries[i].filename))
		break;
	      i++;
	    }
	}

      if(tab_entries[i].filename)
	{
	  y = 0;
	  while(http_ok_head[y])
	    {
	      size += sprintf(&body[size], http_ok_head[y]);
	      y++;
	    }
	  
	  size += build_tabs(&body[size], i);
	  size += build_frame("Current Routes", 
			      "routes", 
			      FRAMEWIDTH, 
			      &body[size], 
			      HTML_BUFSIZE - size, 
			      tab_entries[i].build_body_cb);
	  
	  stats.ok_hits++;
	  
	  y = 0;
	  while(http_ok_tail[y])
	    {
	      size += sprintf(&body[size], http_ok_tail[y]);
	      y++;
	    }  
	  
	  c = build_http_header(HTTP_OK, OLSR_TRUE, size, req, MAX_HTTPREQ_SIZE);
	  
	  goto send_http_data;
	}
      
      
      stats.ill_hits++;
      strcpy(body, HTTP_404_MSG);
      c = build_http_header(HTTP_BAD_FILE, OLSR_TRUE, strlen(body), req, MAX_HTTPREQ_SIZE);
    }

 send_http_data:
  
  r = send(client_sockets[curr_clients], req, c, 0);   
  if(r < 0)
    {
      olsr_printf(1, "(HTTPINFO) Failed sending data to client!\n");
      goto close_connection;
    }

  r = send(client_sockets[curr_clients], body, size, 0);
  if(r < 0)
    {
      olsr_printf(1, "(HTTPINFO) Failed sending data to client!\n");
      goto close_connection;
    }

 close_connection:
  close(client_sockets[curr_clients]);
  curr_clients--;

}


int
build_http_header(http_header_type type, 
		  olsr_bool is_html, 
		  olsr_u32_t size, 
		  char *buf, 
		  olsr_u32_t bufsize)
{
  time_t currtime;
  char timestr[45];
  char tmp[30];

  memset(buf, 0, bufsize);

  switch(type)
    {
    case(HTTP_BAD_REQ):
      memcpy(buf, HTTP_400, strlen(HTTP_400));
      break;
    case(HTTP_BAD_FILE):
      memcpy(buf, HTTP_404, strlen(HTTP_404));
      break;
    default:
      /* Defaults to OK */
      memcpy(buf, HTTP_200, strlen(HTTP_200));
      break;
    }


  /* Date */
  if(time(&currtime))
    {
      strftime(timestr, 45, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", gmtime(&currtime));
      strcat(buf, timestr);
    }
  
  /* Server version */
  strcat(buf, "Server: " PLUGIN_NAME " " PLUGIN_VERSION " " HTTP_VERSION "\r\n");

  /* connection-type */
  strcat(buf,"Connection: closed\r\n");

  /* MIME type */
  if(is_html)
    strcat(buf, "Content-type: text/html\r\n");
  else
    strcat(buf, "Content-type: text/plain\r\n");

  /* Content length */
  if(size > 0)
    {
      sprintf(tmp, "Content-length: %i\r\n", size);
      strcat(buf, tmp);
    }


  /* Cache-control 
   * No caching dynamic pages
   */
  strcat(buf, "Cache-Control: no-cache\r\n");

  if(!is_html)
    strcat(buf, "Accept-Ranges: bytes\r\n");

  /* End header */
  strcat(buf, "\r\n");
  
  olsr_printf(1, "HEADER:\n%s", buf);

  return strlen(buf);

}



int 
build_tabs(char *buf, int active)
{
  int size = 0, i = 0, tabs = 0;

  while(strcmp(html_tabs[i], "<!-- TAB ELEMENTS -->"))
    {
      size += sprintf(&buf[size], html_tabs[i]);
      i++;
    }

  i++;

  while(tab_entries[tabs].tab_label)
    {
      if(tabs == active)
	size += sprintf(&buf[size], 
			html_tabs[i], 
			tab_entries[tabs].filename, 
			ACTIVE_TAB, 
			tab_entries[tabs].tab_label);
      else
	size += sprintf(&buf[size], 
			html_tabs[i], 
			tab_entries[tabs].filename, 
			" ", 
			tab_entries[tabs].tab_label);
      tabs++;
    }
  
  i++;      
  while(html_tabs[i])
    {
      size += sprintf(&buf[size], html_tabs[i]);
      i++;
    }
  
  return size;
}


/*
 * destructor - called at unload
 */
void
olsr_plugin_exit()
{
  if(http_socket)
    close(http_socket);
}


/* Mulitpurpose funtion */
int
plugin_io(int cmd, void *data, size_t size)
{

  switch(cmd)
    {
    default:
      return 0;
    }
  
  return 1;

}


static int
build_frame(char *title, 
	    char *link, 
	    int width,
	    char *buf,
	    olsr_u32_t bufsize, 
	    int(*frame_body_cb)(char *, olsr_u32_t))
{
  int i = 0, size = 0;

  while(http_frame[i])
    {
      if(!strcmp(http_frame[i], "<!-- BODY -->"))
	size += frame_body_cb(&buf[size], bufsize - size);
      else
	size += sprintf(&buf[size], http_frame[i]);      

      i++;
    }

  return size;
}



int
build_routes_body(char *buf, olsr_u32_t bufsize)
{
  int size = 0, index;
  struct rt_entry *routes;

  size += sprintf(&buf[size], "<div id=\"hdr\">OLSR routes in kernel</div>\n");

  size += sprintf(&buf[size], "<table width=\"100%%\" BORDER=0 CELLSPACING=0 CELLPADDING=0 ALIGN=center><tr><th>Destination</th><th>Gateway</th><th>Metric</th><th>Interface</th><th>Type</th></tr>\n");

  /* Neighbors */
  for(index = 0;index < HASHSIZE;index++)
    {
      for(routes = host_routes[index].next;
	  routes != &host_routes[index];
	  routes = routes->next)
	{
	  size += sprintf(&buf[size], "<tr><td>%s</td><td>%s</td><td>%d</td><td>%s</td><td>HOST</td></tr>\n",
			  olsr_ip_to_string(&routes->rt_dst),
			  olsr_ip_to_string(&routes->rt_router),
			  routes->rt_metric,
			  routes->rt_if->int_name);
	}
    }

  /* HNA */
  for(index = 0;index < HASHSIZE;index++)
    {
      for(routes = hna_routes[index].next;
	  routes != &hna_routes[index];
	  routes = routes->next)
	{
	  size += sprintf(&buf[size], "<tr><td>%s</td><td>%s</td><td>%d</td><td>%s</td><td>HNA</td></tr>\n",
			  olsr_ip_to_string(&routes->rt_dst),
			  olsr_ip_to_string(&routes->rt_router),
			  routes->rt_metric,
			  routes->rt_if->int_name);
	}
    }

  size += sprintf(&buf[size], "</table>\n");

  return size;
}

int
build_status_body(char *buf, olsr_u32_t bufsize)
{
    char systime[100];
    time_t currtime;
    int size = 0;
    struct olsr_if *ifs;
    struct plugin_entry *pentry;
    struct plugin_param *pparam;
    struct timeval now, uptime;
    int hours, mins, days;

    gettimeofday(&now, NULL);
    timersub(&now, &start_time, &uptime);
    days = uptime.tv_sec/86400;
    uptime.tv_sec -= days*86400;
    hours = uptime.tv_sec/3600;
    uptime.tv_sec -= hours*3600;
    mins = uptime.tv_sec/60;
    uptime.tv_sec -= mins*60;

    time(&currtime);
    strftime(systime, 100, "System time: <i>%a, %d %b %Y %H:%M:%S</i><br>", gmtime(&currtime));


    size += sprintf(&buf[size], "%s\n", systime);

    if(days)
      size += sprintf(&buf[size], "Olsrd uptime: <i>%d day(s) %02d hours %02d minutes %02d seconds</i><br>\n", days, hours, mins, (int)uptime.tv_sec);
    else
      size += sprintf(&buf[size], "Olsrd uptime: <i>%02d hours %02d minutes %02d seconds</i><br>\n", hours, mins, (int)uptime.tv_sec);

      size += sprintf(&buf[size], "HTTP stats(ok/error/illegal): <i>%d/%d/%d</i>\n", stats.ok_hits, stats.err_hits, stats.ill_hits);

    size += sprintf(&buf[size], "<div id=\"hdr\">Variables</div>\n");

    size += sprintf(&buf[size], "<table width=\"100%%\" border=0>\n<tr>");

    size += sprintf(&buf[size], "<td>Main address: <b>%s</b></td>\n", olsr_ip_to_string(main_addr));
    
    size += sprintf(&buf[size], "<td>IP version: %d</td>\n", cfg->ip_version == AF_INET ? 4 : 6);

    size += sprintf(&buf[size], "<td>Debug level: %d</td>\n", cfg->debug_level);

    size += sprintf(&buf[size], "</tr>\n<tr>\n");

    size += sprintf(&buf[size], "<td>Pollrate: %0.2f</td>\n", cfg->pollrate);
    size += sprintf(&buf[size], "<td>TC redundancy: %d</td>\n", cfg->tc_redundancy);
    size += sprintf(&buf[size], "<td>MPR coverage: %d</td>\n", cfg->mpr_coverage);


    size += sprintf(&buf[size], "</tr>\n<tr>\n");

    size += sprintf(&buf[size], "<td>TOS: 0x%04x</td>\n", cfg->tos);

    size += sprintf(&buf[size], "<td>Willingness: %d %s</td>\n", cfg->willingness, cfg->willingness_auto ? "(auto)" : "");
    
    size += sprintf(&buf[size], "</tr>\n<tr>\n");

    size += sprintf(&buf[size], "<td>Hysteresis: %s</td>\n", cfg->use_hysteresis ? "Enabled" : "Disabled");
	
    size += sprintf(&buf[size], "<td>Hyst scaling: %0.2f</td>\n", cfg->hysteresis_param.scaling);
    size += sprintf(&buf[size], "<td>Hyst lower/upper: %0.2f/%0.2f</td>\n", cfg->hysteresis_param.thr_low, cfg->hysteresis_param.thr_high);

    size += sprintf(&buf[size], "</tr>\n<tr>\n");

    size += sprintf(&buf[size], "<td>LQ extention: %s</td>\n", cfg->lq_level ? "Enabled" : "Disabled");
    size += sprintf(&buf[size], "<td>LQ level: %d</td>\n", cfg->lq_level);
    size += sprintf(&buf[size], "<td>LQ winsize: %d</td>\n", cfg->lq_wsize);
    size += sprintf(&buf[size], "<td></td>\n");

    size += sprintf(&buf[size], "</tr></table>\n");

    size += sprintf(&buf[size], "<div id=\"hdr\">Interfaces</div>\n");


    size += sprintf(&buf[size], "<table width=\"100%%\" border=0>\n");


    for(ifs = cfg->interfaces; ifs; ifs = ifs->next)
      {
	struct interface *rifs = ifs->interf;

	size += sprintf(&buf[size], "<tr><th colspan=3>%s</th>\n", ifs->name);
	if(!rifs)
	  {
	    size += sprintf(&buf[size], "<tr><td colspan=3>Status: DOWN</td></tr>\n");
	    continue;
	  }
	
	if(cfg->ip_version == AF_INET)
	  {
	    size += sprintf(&buf[size], "<tr><td>IP: %s</td>\n", 
			    sockaddr_to_string(&rifs->int_addr));
	    size += sprintf(&buf[size], "<td>MASK: %s</td>\n", 
			    sockaddr_to_string(&rifs->int_netmask));
	    size += sprintf(&buf[size], "<td>BCAST: %s</td></tr>\n",
			    sockaddr_to_string(&rifs->int_broadaddr));
	    size += sprintf(&buf[size], "<tr><td>MTU: %d</td>\n", rifs->int_mtu);
	    size += sprintf(&buf[size], "<td>WLAN: %s</td>\n", rifs->is_wireless ? "Yes" : "No");
	    size += sprintf(&buf[size], "<td>STATUS: UP</td></tr>\n");
	  }
	else
	  {
	    size += sprintf(&buf[size], "<tr><td>IP: TBD</td>\n");
	    size += sprintf(&buf[size], "<td>MASK: TBD</td>\n");
	    size += sprintf(&buf[size], "<td>BCAST: TBD</td></tr>\n");
	    size += sprintf(&buf[size], "<tr><td>MTU: TBD</td>\n");
	    size += sprintf(&buf[size], "<td>WLAN: TBD</td>\n");
	    size += sprintf(&buf[size], "<td>STATUS: TBD</td></tr>\n");
	  }	    

      }

    size += sprintf(&buf[size], "</table>\n");

    if(cfg->allow_no_interfaces)
      size += sprintf(&buf[size], "<i>Olsrd is configured to run even if no interfaces are available</i><br>\n");
    else
      size += sprintf(&buf[size], "<i>Olsrd is configured to halt if no interfaces are available</i><br>\n");

    size += sprintf(&buf[size], "<div id=\"hdr\">Plugins</div>\n");

    size += sprintf(&buf[size], "<table width=\"100%%\" border=0><tr><th>Name</th><th>Parameters</th></tr>\n");

    for(pentry = cfg->plugins; pentry; pentry = pentry->next)
      {
	size += sprintf(&buf[size], "<tr><td>%s</td>\n", pentry->name);

	size += sprintf(&buf[size], "<td><select>\n");
	size += sprintf(&buf[size], "<option>KEY, VALUE</option>\n");

	for(pparam = pentry->params; pparam; pparam = pparam->next)
	  {
	    size += sprintf(&buf[size], "<option>\"%s\", \"%s\"</option>\n",
			    pparam->key,
			    pparam->value);
	  }
	size += sprintf(&buf[size], "</select></td></tr>\n");

      }

    size += sprintf(&buf[size], "</table>\n");


    if(cfg->hna4_entries)
      {
	struct hna4_entry *hna4;
	
	size += sprintf(&buf[size], "<div id=\"hdr\">Announced HNA entries</div>\n");
	size += sprintf(&buf[size], "<table width=\"100%%\" BORDER=0 CELLSPACING=0 CELLPADDING=0 ALIGN=center><tr><th>Network</th><th>Netmask</th></tr>\n");
	
	for(hna4 = cfg->hna4_entries; hna4; hna4 = hna4->next)
	  {
	    size += sprintf(&buf[size], "<tr><td>%s</td><td>%s</td></tr>\n", 
			    olsr_ip_to_string((union olsr_ip_addr *)&hna4->net),
			    olsr_ip_to_string((union olsr_ip_addr *)&hna4->netmask));
	  }
	
	size += sprintf(&buf[size], "</table>\n");
      }
    

    return size;
}



int
build_neigh_body(char *buf, olsr_u32_t bufsize)
{
  struct neighbor_entry *neigh;
  struct neighbor_2_list_entry *list_2;
  struct link_entry *link = NULL;
  int size = 0, index, thop_cnt;

  size += sprintf(&buf[size], "<div id=\"hdr\">Links</div>\n");
  size += sprintf(&buf[size], "<table width=\"100%%\" BORDER=0 CELLSPACING=0 CELLPADDING=0 ALIGN=center><tr><th>Local IP</th><th>remote IP</th><th>Hysteresis</th><th>LinkQuality</th><th>lost</th><th>total</th><th>NLQ</th><th>ETX</th></tr>\n");

  /* Link set */
  if(olsr_plugin_io(GETD__LINK_SET, &link, sizeof(link)) && link)
  {
    while(link)
      {
	size += sprintf(&buf[size], "<tr><td>%s</td><td>%s</td><td>%0.2f</td><td>%0.2f</td><td>%d</td><td>%d</td><td>%0.2f</td><td>%0.2f</td></tr>\n",
			olsr_ip_to_string(&link->local_iface_addr),
			olsr_ip_to_string(&link->neighbor_iface_addr),
			link->L_link_quality, 
			link->loss_link_quality,
			link->lost_packets, 
			link->total_packets,
			link->neigh_link_quality, 
			(link->loss_link_quality * link->neigh_link_quality) ? 1.0 / (link->loss_link_quality * link->neigh_link_quality) : 0.0);

	link = link->next;
      }
  }

  size += sprintf(&buf[size], "</table>\n");

  size += sprintf(&buf[size], "<div id=\"hdr\">Neighbors</div>\n");
  size += sprintf(&buf[size], "<table width=\"100%%\" BORDER=0 CELLSPACING=0 CELLPADDING=0 ALIGN=center><tr><th>IP address</th><th>SYM</th><th>MPR</th><th>MPRS</th><th>Willingness</th><th>2 Hop Neighbors</th></tr>\n");
  /* Neighbors */
  for(index=0;index<HASHSIZE;index++)
    {
      for(neigh = neighbortable[index].next;
	  neigh != &neighbortable[index];
	  neigh = neigh->next)
	{
	  olsr_printf(1, "Size: %d IP: %s\n", size, olsr_ip_to_string(&neigh->neighbor_main_addr));

	  size += sprintf(&buf[size], 
			  "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%d</td>", 
			  olsr_ip_to_string(&neigh->neighbor_main_addr),
			  (neigh->status == SYM) ? "YES" : "NO",
			  neigh->is_mpr ? "YES" : "NO",
			  olsr_lookup_mprs_set ? 
			  (olsr_lookup_mprs_set(&neigh->neighbor_main_addr) ? "YES" : "NO") 
			  : "UNAVAILABLE",
			  neigh->willingness);

	  size += sprintf(&buf[size], "<td><select>\n");
	  size += sprintf(&buf[size], "<option>IP ADDRESS</option>\n");

	  thop_cnt = 0;

	  for(list_2 = neigh->neighbor_2_list.next;
	      list_2 != &neigh->neighbor_2_list;
	      list_2 = list_2->next)
	    {
	      size += sprintf(&buf[size], "<option>%s</option>\n", olsr_ip_to_string(&list_2->neighbor_2->neighbor_2_addr));
	      thop_cnt ++;
	    }
	  size += sprintf(&buf[size], "</select> (%d)</td></tr>\n", thop_cnt);

	}
    }

  size += sprintf(&buf[size], "</table>\n");

  return size;
}



int
build_topo_body(char *buf, olsr_u32_t bufsize)
{
  int size = 0;
  olsr_u8_t index;
  struct tc_entry *entry;
  struct topo_dst *dst_entry;


  size += sprintf(&buf[size], "<div id=\"hdr\">Topology entries</div>\n<table width=\"100%%\" BORDER=0 CELLSPACING=0 CELLPADDING=0 ALIGN=center><tr><th>Source IP addr</th><th>Dest IP addr</th><th>LQ</th><th>ILQ</th><th>ETX</th></tr>\n");


  /* Topology */  
  for(index=0;index<HASHSIZE;index++)
    {
      /* For all TC entries */
      entry = tc_table[index].next;
      while(entry != &tc_table[index])
	{
	  /* For all destination entries of that TC entry */
	  dst_entry = entry->destinations.next;
	  while(dst_entry != &entry->destinations)
	    {
	      size += sprintf(&buf[size], "<tr><td>%s</td><td>%s</td><td>%0.2f</td><td>%0.2f</td><td>%0.2f</td></tr>\n", 
			      olsr_ip_to_string(&entry->T_last_addr), 
			      olsr_ip_to_string(&dst_entry->T_dest_addr),
			      dst_entry->link_quality,
			      dst_entry->inverse_link_quality,
			      (dst_entry->link_quality * dst_entry->inverse_link_quality) ? 1.0 / (dst_entry->link_quality * dst_entry->inverse_link_quality) : 0.0);

	      dst_entry = dst_entry->next;
	    }
	  entry = entry->next;
	}
    }

  size += sprintf(&buf[size], "</table>\n");

  return size;
}



int
build_hna_body(char *buf, olsr_u32_t bufsize)
{
  int size;
  olsr_u8_t index;
  struct hna_entry *tmp_hna;
  struct hna_net *tmp_net;

  size = 0;

  size += sprintf(&buf[size], "<div id=\"hdr\">HNA entries</div>\n<table width=\"100%%\" BORDER=0 CELLSPACING=0 CELLPADDING=0 ALIGN=center><tr><th>Network</th><th>Netmask</th><th>Gateway</th></tr>\n");

  /* HNA entries */
  for(index=0;index<HASHSIZE;index++)
    {
      tmp_hna = hna_set[index].next;
      /* Check all entrys */
      while(tmp_hna != &hna_set[index])
	{
	  /* Check all networks */
	  tmp_net = tmp_hna->networks.next;
	      
	  while(tmp_net != &tmp_hna->networks)
	    {
	      size += sprintf(&buf[size], "<tr><td>%s</td><td>%s</td><td>%s</td></tr>\n", 
                              olsr_ip_to_string(&tmp_net->A_network_addr),
                              olsr_netmask_to_string(&tmp_net->A_netmask),
                              olsr_ip_to_string(&tmp_hna->A_gateway_addr));
	      tmp_net = tmp_net->next;
	    }
	      
	  tmp_hna = tmp_hna->next;
	}
    }

  size += sprintf(&buf[size], "</table>\n");

  return size;
}


int
build_mid_body(char *buf, olsr_u32_t bufsize)
{
  int size = 0;
  olsr_u8_t index;
  struct mid_entry *entry;
  struct addresses *alias;

  size += sprintf(&buf[size], "<div id=\"hdr\">MID entries</div>\n<table width=\"100%%\" BORDER=0 CELLSPACING=0 CELLPADDING=0 ALIGN=center><tr><th>Main Address</th><th>Aliases</th></tr>\n");
  
  /* MID */  
  for(index=0;index<HASHSIZE;index++)
    {
      entry = mid_set[index].next;
      while(entry != &mid_set[index])
	{
	  size += sprintf(&buf[size], "<tr><td>%s</td>\n", olsr_ip_to_string(&entry->main_addr));
	  size += sprintf(&buf[size], "<td><select>\n<option>IP ADDRESS</option>\n");

	  alias = entry->aliases;
	  while(alias)
	    {
	      size += sprintf(&buf[size], "<option>%s</option>\n", olsr_ip_to_string(&alias->address));
	      alias = alias->next;
	    }
	  size += sprintf(&buf[size], "</select>\n");

	  size += sprintf(&buf[size], "</tr>\n");
	  entry = entry->next;
	}
    }

  size += sprintf(&buf[size], "</table>\n");



  return size;
}


int
build_nodes_body(char *buf, olsr_u32_t bufsize)
{
  int size = 0;

  size += build_neigh_body(buf, bufsize);
  size += build_topo_body(&buf[size], bufsize - size);
  size += build_hna_body(&buf[size], bufsize - size);
  size += build_mid_body(&buf[size], bufsize - size);

  return size;
}

int
build_all_body(char *buf, olsr_u32_t bufsize)
{
  int size = 0;

  size += build_status_body(&buf[size], bufsize);
  size += build_routes_body(&buf[size], bufsize - size);
  size += build_neigh_body(&buf[size], bufsize);
  size += build_topo_body(&buf[size], bufsize - size);
  size += build_hna_body(&buf[size], bufsize - size);
  size += build_mid_body(&buf[size], bufsize - size);

  return size;
}


int
build_about_body(char *buf, olsr_u32_t bufsize)
{
  int size = 0, i = 0;

  while(about_frame[i])
    {
      size += sprintf(&buf[size], about_frame[i]);
      i++;
    }
  return size;
}

olsr_bool
check_allowed_ip(union olsr_ip_addr *addr)
{
  struct allowed_host *allh = allowed_hosts;
  struct allowed_net *alln = allowed_nets;

  if(addr->v4 == ntohl(INADDR_LOOPBACK))
    return OLSR_TRUE;

  /* check hosts */
  while(allh)
    {
      if(addr->v4 == allh->host.v4)
	return OLSR_TRUE;
      allh = allh->next;
    }

  /* check nets */
  while(alln)
    {
      if((addr->v4 & alln->mask.v4) == (alln->net.v4 & alln->mask.v4))
	return OLSR_TRUE;
      alln = alln->next;
    }

  return OLSR_FALSE;
}


/**
 *Converts a olsr_ip_addr to a string
 *Goes for both IPv4 and IPv6
 *
 *@param the IP to convert
 *@return a pointer to a static string buffer
 *representing the address in "dots and numbers"
 *
 */
char *
olsr_ip_to_string(union olsr_ip_addr *addr)
{
  static int index = 0;
  static char buff[4][100];
  char *ret;
  struct in_addr in;
 
  if(ipversion == AF_INET)
    {
      in.s_addr=addr->v4;
      ret = inet_ntoa(in);
    }
  else
    {
      /* IPv6 */
      ret = (char *)inet_ntop(AF_INET6, &addr->v6, ipv6_buf, sizeof(ipv6_buf));
    }

  strncpy(buff[index], ret, 100);

  ret = buff[index];

  index = (index + 1) & 3;

  return ret;
}




/**
 *This function is just as bad as the previous one :-(
 */
char *
olsr_netmask_to_string(union hna_netmask *mask)
{
  char *ret;
  struct in_addr in;
  static char netmask[5];

  if(ipversion == AF_INET)
    {
      in.s_addr = mask->v4;
      ret = inet_ntoa(in);
      return ret;

    }
  else
    {
      /* IPv6 */
      sprintf(netmask, "%d", mask->v6);
      return netmask;
    }

  return ret;
}




char *
sockaddr_to_string(struct sockaddr *address_to_convert)
{
  struct sockaddr_in           *address;
  
  address=(struct sockaddr_in *)address_to_convert; 
  return(inet_ntoa(address->sin_addr));
  
}
