/* 
 * OLSR ad-hoc routing table management protocol
 * Copyright (C) 2004 Thomas Lopatic (thomas@lopatic.de)
 *
 * Derived from its Linux counterpart.
 * Copyright (C) 2003 Andreas T�nnesen (andreto@ifi.uio.no)
 *
 * This file is part of the olsr.org OLSR daemon.
 *
 * olsr.org is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * olsr.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with olsr.org; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: apm.c,v 1.1 2004/11/05 02:06:14 tlopatic Exp $
 *
 */

#include "../apm.h"
#include <stdio.h>
#include <string.h>

extern int olsr_printf(int, char *, ...);

int apm_init()
{
  return -1;
}

void apm_printinfo(struct olsr_apm_info *ApmInfo)
{
}

int apm_read(struct olsr_apm_info *ApmInfo)
{
  return -1;
}