# The olsr.org Optimized Link-State Routing daemon(olsrd)
# Copyright (c) 2004, Andreas T�nnesen(andreto@olsr.org)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions 
# are met:
#
# * Redistributions of source code must retain the above copyright 
#   notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright 
#   notice, this list of conditions and the following disclaimer in 
#   the documentation and/or other materials provided with the 
#   distribution.
# * Neither the name of olsr.org, olsrd nor the names of its 
#   contributors may be used to endorse or promote products derived 
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
# POSSIBILITY OF SUCH DAMAGE.
#
# Visit http://www.olsr.org for more information.
#
# If you find this software useful feel free to make a donation
# to the project. For more information see the website or contact
# the copyright holders.
#
# $Id: Makefile,v 1.25 2004/11/21 10:51:09 kattemat Exp $

#OS ?=		linux
#OS =		fbsd
#OS =		win32
#OS =		osx

CC ?= 		gcc

PREFIX ?=

STRIP ?=	strip

BISON ?=	bison
FLEX ?=		flex
CFGDIR =	src/cfgparser

DEPFILE =	.depend
DEPBAK =	.depend.bak
DEPEND =	makedepend -f $(DEPFILE)

DEFINES = 	-DUSE_LINK_QUALITY
INCLUDES =	-I src

all:		olsrd

SRCS =		$(wildcard src/*.c) \
		$(CFGDIR)/oparse.c $(CFGDIR)/oscan.c $(CFGDIR)/olsrd_conf.c

HDRS =		$(wildcard src/*.h) \
		$(CFGDIR)/oparse.h $(CFGDIR)/olsrd_conf.h

OBJS =		$(patsubst %.c,%.o,$(wildcard src/*.c)) \
		$(CFGDIR)/oparse.o $(CFGDIR)/oscan.o $(CFGDIR)/olsrd_conf.o

ifeq ($(OS), linux)

SRCS += 	$(wildcard src/linux/*.c) $(wildcard src/unix/*.c)

HDRS +=		$(wildcard src/linux/*.h) $(wildcard src/unix/*.h)

OBJS +=		$(patsubst %.c,%.o,$(wildcard src/linux/*.c)) \
		$(patsubst %.c,%.o,$(wildcard src/unix/*.c))

DEFINES += 	-Dlinux

CFLAGS ?=	-Wall -Wmissing-prototypes -Wstrict-prototypes \
		-O2 -g #-pg -DDEBUG #-march=i686

LIBS =		-lm -ldl


$(DEPFILE):	$(SRCS) $(HDRS)
		@echo '# olsrd dependency file. AUTOGENERATED' > $(DEPFILE)
		$(DEPEND) -Y $(INCLUDES) $(DEFINES) $(SRCS) >/dev/null 2>&1

olsrd:		$(OBJS)
		$(CC) -o bin/$@ $(OBJS) $(LIBS)

else
ifeq ($(OS), fbsd)

SRCS +=		$(wildcard src/bsd/*.c) $(wildcard src/unix/*.c)

HDRS +=		$(wildcard src/bsd/*.h) $(wildcard src/unix/*.h)

OBJS +=		$(patsubst %.c,%.o,$(wildcard src/bsd/*.c)) \
		$(patsubst %.c,%.o,$(wildcard src/unix/*.c))

CFLAGS ?=	-Wall -Wmissing-prototypes -Wstrict-prototypes \
		-O2 -g

LIBS =		-lm

$(DEPFILE):	$(SRCS) $(HDRS)
		@echo '# olsrd dependency file. AUTOGENERATED' > $(DEPFILE)
		$(DEPEND) $(INCLUDES) $(DEFINES) $(SRCS)

olsrd:		$(OBJS)
		$(CC) -o bin/$@ $(OBJS) $(LIBS)

else
ifeq ($(OS), win32)

SRCS +=		$(wildcard src/win32/*.c)

HDRS +=		$(wildcard src/win32/*.h)

OBJS +=		$(patsubst %.c,%.o,$(wildcard src/win32/*.c))

INCLUDES += 	-Isrc/win32

DEFINES +=	-DWIN32

CFLAGS ?=	-Wall -Wmissing-prototypes \
		-Wstrict-prototypes -mno-cygwin -O2 -g

LIBS =		-mno-cygwin -lws2_32 -liphlpapi

$(DEPFILE):	$(SRCS) $(HDRS)
		@echo '# olsrd dependency file. AUTOGENERATED' > $(DEPFILE)
		$(DEPEND) $(INCLUDES) $(DEFINES) $(SRCS) >/dev/null 2>&1

olsrd:		$(OBJS)
		$(CC) -o bin/$@ $(OBJS) $(LIBS)

else
ifeq ($(OS), osx)

SRCS +=		$(wildcard src/bsd/*.c) $(wildcard src/unix/*.c)

HDRS +=		$(wildcard src/bsd/*.h) $(wildcard src/unix/*.h)

OBJS +=		$(patsubst %.c,%.o,$(wildcard src/bsd/*.c)) \
		$(patsubst %.c,%.o,$(wildcard src/unix/*.c))

DEFINES +=	-D__MacOSX__

CFLAGS ?=	-Wall -Wmissing-prototypes -Wstrict-prototypes -O2 -g 

LIBS =		-lm -ldl

$(DEPFILE):	$(SRCS) $(HDRS)
		@echo '# olsrd dependency file. AUTOGENERATED' > $(DEPFILE)
		$(DEPEND) $(INCLUDES) $(DEFINES) $(SRCS)

olsrd:		$(OBJS)
		$(CC) -o bin/$@ $(OBJS) $(LIBS)

else

olsrd:
	@echo
	@echo '***** olsr.org olsr daemon Make ****'
	@echo ' You must provide a valid target OS '
	@echo ' by setting the OS variable! Valid  '
	@echo ' target OSes are:                   '
	@echo ' ---------------------------------  '
	@echo ' linux - GNU/Linux                  '
	@echo ' win32 - MS Windows                 '
	@echo ' fbsd  - FreeBSD                    '
	@echo ' osx   - Mac OS X                   '
	@echo ' ---------------------------------  '
	@echo ' Example - build for windows:       '
	@echo ' make OS=win32                      '
	@echo ' If you are developing olsrd code,  '
	@echo ' exporting the OS variable might    '
	@echo ' be a good idea :-) Have fun!       '
	@echo '************************************'
	@echo
endif
endif
endif
endif

override CFLAGS += $(INCLUDES) $(DEFINES)
export CFLAGS

depend:		$(DEPFILE)

$(CFGDIR)/oparse.c: \
		$(CFGDIR)/oparse.y $(CFGDIR)/olsrd_conf.h
		$(BISON) -d -o$(CFGDIR)/oparse.c $(CFGDIR)/oparse.y

$(CFGDIR)/oparse.h: \
		$(CFGDIR)/oparse.c

$(CFGDIR)/oscan.c: \
		$(CFGDIR)/oscan.lex $(CFGDIR)/oparse.h $(CFGDIR)/olsrd_conf.h
		$(FLEX) -o$(CFGDIR)/oscan.c $(CFGDIR)/oscan.lex

libs: 
		for i in lib/*; do \
			$(MAKE) -C $$i; \
		done; 

clean_libs: 
		for i in lib/*; do \
			$(MAKE) -C $$i clean; \
		done; 

.PHONY:		clean

clean:
		rm -f $(OBJS)

uberclean:
		rm -f $(OBJS) $(DEPFILE) $(DEPBAK)
		rm -f $(CFGDIR)/oscan.c $(CFGDIR)/oparse.h $(CFGDIR)/oparse.c
		rm -f bin/olsrd bin/olsrd.exe
		rm -f src/*~ src/linux/*~ src/unix/*~ src/win32/*~
		rm -f src/bsd/*~ src/cfgparser/*~

install_libs:
		for i in lib/*; do \
			$(MAKE) -C $$i LIBDIR=$(PREFIX)/usr/lib install; \
		done; 	

install_bin:
		$(STRIP) bin/olsrd
		install -D -m 755 bin/olsrd $(PREFIX)/usr/sbin/olsrd

install:	install_bin
		@echo olsrd uses the configfile $(PREFIX)/etc/olsr.conf
		@echo a default configfile. A sample configfile
		@echo can be installed
		mkdir -p $(PREFIX)/etc
		cp -i files/olsrd.conf.default $(PREFIX)/etc/olsrd.conf
		@echo -------------------------------------------
		@echo Edit $(PREFIX)/etc/olsrd.conf before running olsrd!!
		@echo -------------------------------------------
		mkdir -p $(PREFIX)/usr/share/man/man8/
		cp files/olsrd.8.gz $(PREFIX)/usr/share/man/man8/olsrd.8.gz

sinclude	$(DEPFILE)

