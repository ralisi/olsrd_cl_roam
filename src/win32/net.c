
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

#if defined WINCE
#include <sys/types.h>          // for time_t
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#undef interface

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "defs.h"
#include "net_os.h"
#include "net_olsr.h"
#include "ipcalc.h"
#include "olsr_logging.h"

#if defined WINCE
#define WIDE_STRING(s) L##s
#else
#define WIDE_STRING(s) s
#endif

void WinSockPError(const char *Str);
void PError(const char *);

void DisableIcmpRedirects(void);
int disable_ip_forwarding(int Ver);


int
getsocket(int BuffSize, struct interface *ifp __attribute__ ((unused)))
{
  struct sockaddr_in Addr;
  int On = 1;
  unsigned long Len;
  int Sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (Sock < 0) {
    OLSR_ERROR(LOG_NETWORKING, "Cannot open socket for OLSR PDUs (%s)\n", strerror(errno));
    olsr_exit(EXIT_FAILURE);
  }

  if (setsockopt(Sock, SOL_SOCKET, SO_BROADCAST, (char *)&On, sizeof(On)) < 0) {
    OLSR_ERROR(LOG_NETWORKING, "Cannot set socket for OLSR PDUs to broadcast mode (%s)\n", strerror(errno));
    CLOSESOCKET(Sock);
    olsr_exit(EXIT_FAILURE);
  }

  if (setsockopt(Sock, SOL_SOCKET, SO_REUSEADDR, (char *)&On, sizeof(On)) < 0) {
    OLSR_ERROR(LOG_NETWORKING, "Cannot set socket for OLSR PDUs to broadcast mode (%s)\n", strerror(errno));
    CLOSESOCKET(Sock);
    olsr_exit(EXIT_FAILURE);
  }

  while (BuffSize > 8192) {
    if (setsockopt(Sock, SOL_SOCKET, SO_RCVBUF, (char *)&BuffSize, sizeof(BuffSize)) == 0)
      break;

    BuffSize -= 1024;
  }

  if (BuffSize <= 8192)
    OLSR_WARN(LOG_NETWORKING, "Cannot set IPv4 socket receive buffer.\n");

  memset(&Addr, 0, sizeof(Addr));
  Addr.sin_family = AF_INET;
  Addr.sin_port = htons(olsr_cnf->olsrport);

  if(bufspace <= 0) {
    Addr.sin_addr.s_addr = ifp->int_addr.sin_addr.s_addr;
  }
  else {
    Addr.sin_addr.s_addr = INADDR_ANY;
  }

  if (bind(Sock, (struct sockaddr *)&Addr, sizeof(Addr)) < 0) {
    OLSR_ERROR(LOG_NETWORKING, "Could not bind socket for OLSR PDUs to device (%s)\n", strerror(errno));
    CLOSESOCKET(Sock);
    olsr_exit(EXIT_FAILURE);
  }

  if (WSAIoctl(Sock, FIONBIO, &On, sizeof(On), NULL, 0, &Len, NULL, NULL) < 0) {
    OLSR_ERROR(LOG_NETWORKING, "WSAIoctl");
    CLOSESOCKET(Sock);
    olsr_exit(EXIT_FAILURE);
  }

  return Sock;
}

int
getsocket6(int BuffSize, struct interface *ifp __attribute__ ((unused)))
{
  struct sockaddr_in6 Addr6;
  int On = 1;
  int Sock = socket(AF_INET6, SOCK_DGRAM, 0);
  if (Sock < 0) {
    OLSR_ERROR(LOG_NETWORKING, "Cannot open socket for OLSR PDUs (%s)\n", strerror(errno));
    olsr_exit(EXIT_FAILURE);
  }

  if (setsockopt(Sock, SOL_SOCKET, SO_BROADCAST, (char *)&On, sizeof(On)) < 0) {
    OLSR_ERROR(LOG_NETWORKING, "Cannot set socket for OLSR PDUs to broadcast mode (%s)\n", strerror(errno));
    CLOSESOCKET(Sock);
    olsr_exit(EXIT_FAILURE);
  }

  if (setsockopt(Sock, SOL_SOCKET, SO_REUSEADDR, (char *)&On, sizeof(On)) < 0) {
    OLSR_ERROR(LOG_NETWORKING, "Cannot set socket for OLSR PDUs to broadcast mode (%s)\n", strerror(errno));
    CLOSESOCKET(Sock);
    olsr_exit(EXIT_FAILURE);
  }

  while (BuffSize > 8192) {
    if (setsockopt(Sock, SOL_SOCKET, SO_RCVBUF, (char *)&BuffSize, sizeof(BuffSize)) == 0)
      break;

    BuffSize -= 1024;
  }

  if (BuffSize <= 8192)
    OLSR_WARN(LOG_NETWORKING, "Cannot set IPv6 socket receive buffer.\n");

  memset(&Addr6, 0, sizeof(Addr6));
  Addr6.sin6_family = AF_INET6;
  Addr6.sin6_port = htons(olsr_cnf->olsrport);

  if(bufspace <= 0) {
    memcpy(&Addr6.sin6_addr, &ifp->int6_addr.sin6_addr, sizeof(struct in6_addr));
  }

  if (bind(Sock, (struct sockaddr *)&Addr6, sizeof(Addr6)) < 0) {
    OLSR_ERROR(LOG_NETWORKING, "Could not bind socket for OLSR PDUs to device (%s)\n", strerror(errno));
    CLOSESOCKET(Sock);
    olsr_exit(EXIT_FAILURE);
  }

  return Sock;
}

static OVERLAPPED RouterOver;

int
enable_ip_forwarding(int Ver)
{
  HMODULE Lib;
  unsigned int __stdcall(*EnableRouterFunc) (HANDLE * Hand, OVERLAPPED * Over);
  HANDLE Hand;

  Ver = Ver;

  Lib = LoadLibrary(WIDE_STRING("iphlpapi.dll"));

  if (Lib == NULL)
    return 0;

  EnableRouterFunc = (unsigned int __stdcall(*)(HANDLE *, OVERLAPPED *))
    GetProcAddress(Lib, WIDE_STRING("EnableRouter"));

  if (EnableRouterFunc == NULL)
    return 0;

  memset(&RouterOver, 0, sizeof(OVERLAPPED));

  RouterOver.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

  if (RouterOver.hEvent == NULL) {
    OLSR_WARN(LOG_NETWORKING, "CreateEvent()");
    return -1;
  }

  if (EnableRouterFunc(&Hand, &RouterOver) != ERROR_IO_PENDING) {
    OLSR_WARN(LOG_NETWORKING, "EnableRouter()");
    return -1;
  }

  OLSR_DEBUG(LOG_NETWORKING, "Routing enabled.\n");

  return 0;
}

int
disable_ip_forwarding(int Ver)
{
  HMODULE Lib;
  unsigned int __stdcall(*UnenableRouterFunc) (OVERLAPPED * Over, unsigned int *Count);
  unsigned int Count;

  Ver = Ver;

  Lib = LoadLibrary(WIDE_STRING("iphlpapi.dll"));

  if (Lib == NULL)
    return 0;

  UnenableRouterFunc = (unsigned int __stdcall(*)(OVERLAPPED *, unsigned int *))
    GetProcAddress(Lib, WIDE_STRING("UnenableRouter"));

  if (UnenableRouterFunc == NULL)
    return 0;

  if (UnenableRouterFunc(&RouterOver, &Count) != NO_ERROR) {
    OLSR_WARN(LOG_NETWORKING, "UnenableRouter()");
    return -1;
  }

  OLSR_DEBUG(LOG_NETWORKING, "Routing disabled, count = %u.\n", Count);

  return 0;
}

int
restore_settings(int Ver)
{
  disable_ip_forwarding(Ver);

  return 0;
}

static int
SetEnableRedirKey(unsigned long New)
{
#if !defined WINCE
  HKEY Key;
  unsigned long Type;
  unsigned long Len;
  unsigned long Old;

  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                   "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", 0, KEY_READ | KEY_WRITE, &Key) != ERROR_SUCCESS)
    return -1;

  Len = sizeof(Old);

  if (RegQueryValueEx(Key, "EnableICMPRedirect", NULL, &Type, (unsigned char *)&Old, &Len) != ERROR_SUCCESS || Type != REG_DWORD)
    Old = 1;

  if (RegSetValueEx(Key, "EnableICMPRedirect", 0, REG_DWORD, (unsigned char *)&New, sizeof(New))) {
    RegCloseKey(Key);
    return -1;
  }

  RegCloseKey(Key);
  return Old;
#else
  return 0;
#endif
}

void
DisableIcmpRedirects(void)
{
  int Res;

  Res = SetEnableRedirKey(0);

  if (Res != 1)
    return;

  OLSR_ERROR(LOG_NETWORKING, "\n*** IMPORTANT *** IMPORTANT *** IMPORTANT *** IMPORTANT *** IMPORTANT ***\n\n");

#if 0
  if (Res < 0) {
    OLSR_ERROR(LOG_NETWORKING, "Cannot disable ICMP redirect processing in the registry.\n");
    OLSR_ERROR(LOG_NETWORKING, "Please disable it manually. Continuing in 3 seconds...\n");
    Sleep(3000);

    return;
  }
#endif

  OLSR_ERROR(LOG_NETWORKING, "I have disabled ICMP redirect processing in the registry for you.\n");
  OLSR_ERROR(LOG_NETWORKING, "REBOOT NOW, so that these changes take effect. Exiting...\n\n");

  exit(0);
}


int
join_mcast(struct interface *Nic, int Sock)
{
  /* See linux/in6.h */
  struct ipaddr_str buf;
  struct ipv6_mreq McastReq;

  McastReq.ipv6mr_multiaddr = Nic->int6_multaddr.sin6_addr;
  McastReq.ipv6mr_interface = Nic->if_index;

  OLSR_DEBUG(LOG_NETWORKING, "Interface %s joining multicast %s...", Nic->int_name,
             olsr_ip_to_string(&buf, (union olsr_ip_addr *)&Nic->int6_multaddr.sin6_addr));
  /* Send multicast */
  if (setsockopt(Sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *)&McastReq, sizeof(struct ipv6_mreq))
      < 0) {
    OLSR_WARN(LOG_NETWORKING, "Join multicast: %s\n", strerror(errno));
    return -1;
  }

  /* Old libc fix */
#ifdef IPV6_JOIN_GROUP
  /* Join reciever group */
  if (setsockopt(Sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char *)&McastReq, sizeof(struct ipv6_mreq))
      < 0)
#else
  /* Join reciever group */
  if (setsockopt(Sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *)&McastReq, sizeof(struct ipv6_mreq))
      < 0)
#endif
  {
    OLSR_WARN(LOG_NETWORKING, "Join multicast send: %s\n", strerror(errno));
    return -1;
  }


  if (setsockopt(Sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char *)&McastReq.ipv6mr_interface, sizeof(McastReq.ipv6mr_interface))
      < 0) {
    OLSR_WARN(LOG_NETWORKING, "Join multicast if: %s\n", strerror(errno));
    return -1;
  }

  return 0;
}


/**
 * Wrapper for sendto(2)
 */

ssize_t
olsr_sendto(int s, const void *buf, size_t len, int flags, const struct sockaddr * to, socklen_t tolen)
{
  return sendto(s, buf, len, flags, to, tolen);
}


/**
 * Wrapper for recvfrom(2)
 */

ssize_t
olsr_recvfrom(int s, void *buf, size_t len, int flags __attribute__ ((unused)), struct sockaddr * from, socklen_t * fromlen)
{
  return recvfrom(s, buf, len, 0, from, fromlen);
}

/**
 * Wrapper for select(2)
 */

int
olsr_select(int nfds, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, struct timeval *timeout)
{
  return select(nfds, readfds, writefds, exceptfds, timeout);
}

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
