/*
 * OLSR ad-hoc routing table management protocol
 * Copyright (C) 2004 Andreas T�nnesen (andreto@ifi.uio.no)
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
 * 
 * $ Id $
 *
 */

/*
 * Private functions
 */

void
scheduler();

#include "defs.h"
#include "scheduler.h"
#include "tc_set.h"
#include "link_set.h"
#include "duplicate_set.h"
#include "mpr_selector_set.h"
#include "mid_set.h"
#include "mpr.h"
#include "olsr.h"
#include "build_msg.h"


int
init_scheduler(float poll_interval)
{

  sched_poll_interval = poll_interval;

  timeout_functions = NULL;
  event_functions = NULL;

  return 1;
}


/*
 * The mutex "mutex" is used to protect memory
 * between the scheduler, which runs in a thread
 * of its own, and the main thread which mainly
 * is packet processing and route calculation.
 */
void
start_scheduler(pthread_t *thread_id)
{
  /* Initialize the mutex */
  pthread_mutex_init(&mutex, NULL);
  /* Create the scheduler thread */
  pthread_create(thread_id, NULL, (void *)&scheduler, NULL);

  return;
}

/**
 *Main scheduler event loop. Polls at every
 *sched_poll_interval and calls all functions
 *that are timed out or that are triggered.
 *Also calls the olsr_process_changes()
 *function at every poll.
 *
 *
 *@return nada
 */

void
scheduler()
{
  struct timespec remainder_spec;
  struct timespec sleeptime_spec;

  /*
   *Used to calculate sleep time
   */
  struct timeval start_of_loop;
  struct timeval end_of_loop;
  struct timeval time_used;
  struct timeval interval;
  struct timeval sleeptime_val;

  olsr_u32_t interval_usec;

  struct event_entry *entry;
  struct timeout_entry *time_out_entry;
 
  interval_usec = (olsr_u32_t)(sched_poll_interval * 1000000);

  interval.tv_sec = interval_usec / 1000000;
  interval.tv_usec = interval_usec % 1000000;

  olsr_printf(1, "Scheduler started - polling every %0.2f seconds\n", sched_poll_interval);

  olsr_printf(3, "Max jitter is %f\n\n", max_jitter);



  /* Main scheduler event loop */

  for(;;)
    {

      gettimeofday(&start_of_loop, NULL);



      /* C R I T I C A L - S E C T I O N - S T A R T */
      pthread_mutex_lock(&mutex);

      /* Update the global timestamp */
      gettimeofday(&now, NULL);
      nowtm = gmtime(&now.tv_sec);

      while (nowtm == NULL)
	{
	  nowtm = gmtime(&now.tv_sec);
	}


      /* Run timout functions (before packet generation) */

      time_out_entry = timeout_functions;
      
      while(time_out_entry)
	{
	  time_out_entry->function();
	  time_out_entry = time_out_entry->next;
	}

      /* Update */
      
      olsr_process_changes();


      /* Check for changes in topology */

      if(changes)
	{
	  olsr_printf(3, "ANSN UPDATED\n\n");
	  ansn++;
	}


      /* Check scheduled events */

      entry = event_functions;

      /* UPDATED - resets timer upon triggered execution */
      while(entry)
	{
	  entry->since_last += sched_poll_interval;

	  /* Timed out */
	  if((entry->since_last > entry->interval) ||
	     /* Triggered */
	     ((entry->trigger != NULL) &&
	      (*(entry->trigger) == 1)))
	    {
	      /* Run scheduled function */
	      entry->function();

	      /* Set jitter */
	      entry->since_last = (float) random()/RAND_MAX;
	      entry->since_last *= max_jitter;
	      
	      /* Reset trigger */
	      if(entry->trigger != NULL)
		*(entry->trigger) = 0;
	      
	      //olsr_printf(3, "Since_last jitter: %0.2f\n", entry->since_last);

	    }

	  entry = entry->next;
	}




      /* Forward message(s) OUTSIDE INTERFACE LOOP */
      if(fwdsize && TIMED_OUT(&fwdtimer)) 
	{ 
#ifdef DEBUG
	  olsr_printf(3, "Forwarding message - size %d\n", fwdsize);
#endif	  
	  net_forward();
	  fwdsize = 0;
	  
	}

      /* C R I T I C A L - S E C T I O N - E N D */
      pthread_mutex_unlock(&mutex);


      gettimeofday(&end_of_loop, NULL);

      timersub(&end_of_loop, &start_of_loop, &time_used);


      //printf("Time to sleep: %ld\n", sleeptime.tv_nsec);
      //printf("Time used: %ld\n", time_used.tv_usec/1000);

      if(timercmp(&time_used, &interval, <))
	{
	  timersub(&interval, &time_used, &sleeptime_val);
	  
	  // printf("sleeptime_val = %u.%06u\n",
	  //        sleeptime_val.tv_sec, sleeptime_val.tv_usec);
	  
	  sleeptime_spec.tv_sec = sleeptime_val.tv_sec;
	  sleeptime_spec.tv_nsec = sleeptime_val.tv_usec * 1000;
	  
	  while(nanosleep(&sleeptime_spec, &remainder_spec) < 0)
	    sleeptime_spec = remainder_spec;
	}
      
    }//end for
}


/*
 *
 *@param initial how long utnil the first generation
 *@param trigger pointer to a boolean indicating that
 *this function should be triggered immediatley
 */
int
olsr_register_scheduler_event(void (*event_function)(), float interval, float initial, olsr_u8_t *trigger)
{
  struct event_entry *new_entry;

  olsr_printf(3, "Scheduler event registered int: %0.2f\n", interval);

  /* check that this entry is not added already */
  new_entry = event_functions;
  while(new_entry)
    {
      if((new_entry->function == event_function) &&
	 (new_entry->trigger == trigger) &&
	 (new_entry->interval == interval))
	{
	  fprintf(stderr, "Register scheduler event: Event alread registered!\n");
	  olsr_syslog(OLSR_LOG_ERR, "Register scheduler event: Event alread registered!\n");
	  return 0;
	}
      new_entry = new_entry->next;
    }

  new_entry = olsr_malloc(sizeof(struct event_entry), "add scheduler event");

  new_entry->function = event_function;
  new_entry->interval = interval;
  new_entry->since_last = interval - initial;
  new_entry->next = event_functions;
  new_entry->trigger = trigger;

  event_functions = new_entry;

  return 1;
}



/*
 *
 *@param initial how long utnil the first generation
 *@param trigger pointer to a boolean indicating that
 *this function should be triggered immediatley
 */
int
olsr_remove_scheduler_event(void (*event_function)(), float interval, float initial, olsr_u8_t *trigger)
{
  struct event_entry *entry, *prev;

  prev = NULL;
  entry = event_functions;

  while(entry)
    {
      if((entry->function == event_function) &&
	 (entry->trigger == trigger) &&
	 (entry->interval == interval))
	{
	  if(entry == event_functions)
	    {
	      event_functions = entry->next;
	    }
	  else
	    {
	      prev->next = entry->next;
	    }
	  return 1;
	}

      prev = entry;
      entry = entry->next;
    }

  return 0;
}


int
olsr_register_timeout_function(void (*time_out_function)())
{
  struct timeout_entry *new_entry;

  /* check that this entry is not added already */
  new_entry = timeout_functions;
  while(new_entry)
    {
      if(new_entry->function == time_out_function)
	{
	  fprintf(stderr, "Register scheduler timeout: Event alread registered!\n");
	  olsr_syslog(OLSR_LOG_ERR, "Register scheduler timeout: Event alread registered!\n");
	  return 0;
	}
      new_entry = new_entry->next;
    }

  new_entry = olsr_malloc(sizeof(struct timeout_entry), "scheduler add timeout");

  new_entry->function = time_out_function;
  new_entry->next = timeout_functions;

  timeout_functions = new_entry;

  return 1;
}



int
olsr_remove_timeout_function(void (*time_out_function)())
{
  struct timeout_entry *entry, *prev;

  /* check that this entry is not added already */
  entry = timeout_functions;
  prev = NULL;

  while(entry)
    {
      if(entry->function == time_out_function)
	{
	  if(entry == timeout_functions)
	    {
	      timeout_functions = entry->next;
	    }
	  else
	    {
	      prev->next = entry->next;
	    }
	  free(entry);
	  return 1;
	}
      prev = entry;
      entry = entry->next;
    }

  return 0;
}

