/* 
 * OLSR ad-hoc routing table management protocol
 * Copyright (C) 2004 Thomas Lopatic (thomas@lopatic.de)
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
 * $Id: misc.c,v 1.1 2004/11/20 15:40:52 tlopatic Exp $
 *
 */

#include <stdio.h>
#include <unistd.h>

void clear_console(void)
{
  FILE *pipe;
  static int first_time = 1;
  static char clear_buff[100];
  static int len = 0;
  int c;
  int i;

  if (first_time != 0)
    {
      first_time = 0;

      pipe = popen("clear", "r");

      for (len = 0; len < sizeof (clear_buff); len++)
        {
          c = fgetc(pipe);

          if (c == EOF)
            break;

          clear_buff[len] = (char)c;
        }

      pclose(pipe);
    }

  for (i = 0; i < len; i++)
    fputc(clear_buff[i], stdout);

  fflush(stdout);
}
