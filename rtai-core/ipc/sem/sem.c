/** 
 * @file
 * Semaphore functions.
 * @author Paolo Mantegazza
 *
 * @note Copyright (C) 1999-2003 Paolo Mantegazza
 * <mantegazza@aero.polimi.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * @ingroup sem
 */

/**
 * @ingroup sched
 * @defgroup sem Semaphore functions
 *
 *@{*/

#include <rtai_schedcore.h>
#include <rtai_sem.h>
#include <rtai_rwl.h>
#include <rtai_spl.h>

MODULE_LICENSE("GPL");

/* +++++++++++++++++++++ ALL SEMAPHORES TYPES SUPPORT +++++++++++++++++++++++ */

/**
 * @anchor rt_typed_sem_init
 * @brief Initialize a specifically typed (counting, binary, resource)
 *	  semaphore
 *
 * rt_typed_sem_init initializes a semaphore @e sem of type @e type. A
 * semaphore can be used for communication and synchronization among
 * real time tasks. Negative value of a semaphore shows how many tasks
 * are blocked on the semaphore queue, waiting to be awaken by calls
 * to rt_sem_signal.
 *
 * @param sem must point to an allocated SEM structure.
 *
 * @param value is the initial value of the semaphore, always set to 1
 *	  for a resource semaphore.
 *
 * @param type is the semaphore type and can be: CNT_SEM for counting
 *	  semaphores, BIN_SEM for binary semaphores, RES_SEM for
 *	  resource semaphores.
 * 
 * Counting semaphores can register up to 0xFFFE events. Binary
 * semaphores do not count signalled events, their count will never
 * exceed 1 whatever number of events is signaled to them. Resource
 * semaphores are special binary semaphores suitable for managing
 * resources. The task that acquires a resource semaphore becomes its
 * owner, also called resource owner, since it is the only one capable
 * of manipulating the resource the semaphore is protecting. The owner
 * has its priority increased to that of any task blocking on a wait
 * to the semaphore. Such a feature, called priority inheritance,
 * ensures that a high priority task is never slaved to a lower
 * priority one, thus allowing to avoid any deadlock due to priority
 * inversion. Resource semaphores can be recursed, i.e. their task
 * owner is not blocked by nested waits placed on an owned
 * resource. The owner must insure that it will signal the semaphore,
 * in reversed order, as many times as he waited on it. Note that that
 * full priority inheritance is supported both for resource semaphores
 * and inter task messages, for a singly owned resource. Instead it
 * becomes an adaptive priority ceiling when a task owns multiple
 * resources, including messages sent to him. In such a case in fact
 * its priority is returned to its base one only when all such
 * resources are released and no message is waiting for being
 * received. This is a compromise design choice aimed at avoiding
 * extensive searches for the new priority to be inherited across
 * multiply owned resources and blocked tasks sending messages to
 * him. Such a solution will be implemented only if it proves
 * necessary. Note also that, to avoid @e deadlocks, a task owning a
 * resource semaphore cannot be suspended. Any @ref rt_task_suspend()
 * posed on it is just registered. An owner task will go into suspend
 * state only when it releases all the owned resources.
 *
 * @note RTAI counting semaphores assume that their counter will never
 *	 exceed 0xFFFF, such a number being used to signal returns in
 *	 error. Thus also the initial count value cannot be greater
 *	 than 0xFFFF. To be used only with RTAI24.x.xx (FIXME).
 */
void rt_typed_sem_init(SEM *sem, int value, int type)
{
	sem->magic = RT_SEM_MAGIC;
	sem->count = value;
	sem->qtype = type != RES_SEM && (type & FIFO_Q) ? 1 : 0;
	type = (type & 3) - 2;
	if ((sem->type = type) < 0 && value > 1) {
		sem->count = 1;
	} else if (type > 0) {
		sem->type = sem->count = 1;
	}
	sem->queue.prev = &(sem->queue);
	sem->queue.next = &(sem->queue);
	sem->queue.task = sem->owndby = 0;
}


/**
 * @anchor rt_sem_init
 * @brief Initialize a counting semaphore.
 *
 * rt_sem_init initializes a semaphore @e sem.
 *
 * A semaphore can be used for communication and synchronization among
 * real time tasks.
 *
 * @param sem must point to an allocated @e SEM structure.
 *
 * @param value is the initial value of the semaphore.
 * 
 * Positive values of the semaphore variable show how many tasks can
 * do a @ref rt_sem_wait() call without blocking. Negative value of a
 * semaphore shows how many tasks are blocked on the semaphore queue,
 * waiting to be awaken by calls to @ref rt_sem_signal().
 *
 * @note RTAI counting semaphores assume that their counter will never
 *	 exceed 0xFFFF, such a number being used to signal returns in
 *	 error. Thus also the initial count value cannot be greater
 *	 than 0xFFFF.
 *	 RTAI 24.1.xx has also @ref rt_typed_sem_init(), allowing to
 *	 choose among counting, binary and resource
 *	 semaphores. Resource semaphores have priority inherithance. 
 */
void rt_sem_init(SEM *sem, int value)
{
	rt_typed_sem_init(sem, value, CNT_SEM);
}


/**
 * @anchor rt_sem_delete
 * @brief Delete a semaphore
 *
 * rt_sem_delete deletes a semaphore previously created with 
 * @ref rt_sem_init(). 
 *
 * @param sem points to the structure used in the corresponding
 * call to rt_sem_init. 
 *
 * Any tasks blocked on this semaphore is returned in error and
 * allowed to run when semaphore is destroyed. 
 *
 * @return 0 is returned upon success. A negative value is returned on
 * failure as described below: 
 * - @b 0xFFFF: @e sem does not refer to a valid semaphore.
 *
 * @note In principle 0xFFFF could theoretically be a usable
 *	 semaphores events count, so it could be returned also under
 *	 normal circumstances. It is unlikely you are going to count
 *	 up to such number of events, in any case avoid counting up 
 *	 to 0xFFFF. 
 */
int rt_sem_delete(SEM *sem)
{
	unsigned long flags;
	RT_TASK *task;
	unsigned long schedmap;
	QUEUE *q;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}

	schedmap = 0;
	q = &(sem->queue);
	flags = rt_global_save_flags_and_cli();
	sem->magic = 0;
	while ((q = q->next) != &(sem->queue) && (task = q->task)) {
		rem_timed_task(task);
		if (task->state != RT_SCHED_READY && (task->state &= ~(RT_SCHED_SEMAPHORE | RT_SCHED_DELAYED)) == RT_SCHED_READY) {
			enq_ready_task(task);
			set_bit(task->runnable_on_cpus & 0x1F, &schedmap);
		}
	}
	clear_bit(hard_cpu_id(), &schedmap);
	if ((task = sem->owndby) && sem->type > 0) {
		int sched;
		if (task->owndres & SEMHLF) {
			--task->owndres;
		}
		if (!task->owndres) {
			sched = renq_ready_task(task, task->base_priority);
		} else if (!(task->owndres & SEMHLF)) {
			int priority;
                        sched = renq_ready_task(task, task->base_priority > (priority = ((task->msg_queue.next)->task)->priority) ? priority : task->base_priority);
		} else {
			sched = 0;
		}
		if (task->suspdepth) {
			if (task->suspdepth > 0) {
				task->state |= RT_SCHED_SUSPENDED;
				rem_ready_task(task);
				sched = 1;
			} else {
				rt_task_delete(task);
			}
		}
		if (sched) {
			if (schedmap) {
				RT_SCHEDULE_MAP_BOTH(schedmap);
			} else {
				rt_schedule();
			}
		}
	} else {
		RT_SCHEDULE_MAP(schedmap);
	}
	rt_global_restore_flags(flags);
	return 0;
}


int rt_sem_count(SEM *sem)
{
	return sem->count;
}


/**
 * @anchor rt_sem_signal
 * @brief Signaling a semaphore.
 *
 * rt_sem_signal signal an event to a semaphore. It is typically
 * called when the task leaves a critical region. The semaphore value
 * is incremented and tested. If the value is not positive, the first
 * task in semaphore's waiting queue is allowed to run.  rt_sem_signal
 * never blocks the caller task.
 *
 * @param sem points to the structure used in the call to @ref
 * rt_sem_init().
 * 
 * @return 0 is returned upon success. A negative value is returned on
 * failure as described below: 
 * - @b 0xFFFF: @e sem does not refer to a valid semaphore.
 *
 * @note In principle 0xFFFF could theoretically be a usable
 *	 semaphores events count, so it could be returned also under
 *	 normal circumstances. It is unlikely you are going to count
 *	 up to such number of events, in any case avoid counting up to
 * 	 0xFFFF.
 *	 See @ref rt_sem_wait() notes for some curiosities.
 */
int rt_sem_signal(SEM *sem)
{
	unsigned long flags;
	RT_TASK *task;
	int tosched;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	if (sem->type) {
		if (sem->type > 1) {
			sem->type--;
			rt_global_restore_flags(flags);
			return 0;
		}
		if (++sem->count > 1) {
			sem->count = 1;
		}
	} else {
		sem->count++;
	}
	if ((task = (sem->queue.next)->task)) {
		dequeue_blocked(task);
		rem_timed_task(task);
		if (task->state != RT_SCHED_READY && (task->state &= ~(RT_SCHED_SEMAPHORE | RT_SCHED_DELAYED)) == RT_SCHED_READY) {
			enq_ready_task(task);
			if (sem->type <= 0) {
				RT_SCHEDULE(task, hard_cpu_id());
				rt_global_restore_flags(flags);
				return 0;
			}
			tosched = 1;
			goto res;
		}
	}
	tosched = 0;
res:	if (sem->type > 0) {
		DECLARE_RT_CURRENT;
		int sched;
		ASSIGN_RT_CURRENT;
		sem->owndby = 0;
		if (rt_current->owndres & SEMHLF) {
			--rt_current->owndres;
		}
		if (!rt_current->owndres) {
			sched = renq_current(rt_current, rt_current->base_priority);
		} else if (!(rt_current->owndres & SEMHLF)) {
			int priority;
			sched = renq_current(rt_current, rt_current->base_priority > (priority = ((rt_current->msg_queue.next)->task)->priority) ? priority : rt_current->base_priority);
		} else {
			sched = 0;
		}
		if (rt_current->suspdepth) {
			if (rt_current->suspdepth > 0) {
				rt_current->state |= RT_SCHED_SUSPENDED;
				rem_ready_current(rt_current);
                        	sched = 1;
			} else {
				rt_task_delete(rt_current);
			}
		}
		if (sched) {
			if (tosched) {
				RT_SCHEDULE_BOTH(task, cpuid);
			} else {
				rt_schedule();
			}
		} else if (tosched) {
			RT_SCHEDULE(task, cpuid);
		}
	}
	rt_global_restore_flags(flags);
	return 0;
}


int rt_sem_broadcast(SEM *sem)
{
	unsigned long flags, schedmap;
	RT_TASK *task;
	QUEUE *q;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}
	schedmap = 0;
	q = &(sem->queue);
	flags = rt_global_save_flags_and_cli();
	while ((q = q->next) != &(sem->queue)) {
		dequeue_blocked(task = q->task);
		rem_timed_task(task);
		if (task->state != RT_SCHED_READY && (task->state &= ~(RT_SCHED_SEMAPHORE | RT_SCHED_DELAYED)) == RT_SCHED_READY) {
			enq_ready_task(task);
			set_bit(task->runnable_on_cpus & 0x1F, &schedmap);
		}
		flags = rt_global_save_flags_and_cli();
		rt_global_restore_flags(flags);
	}
	sem->count = 0;
	if (schedmap) {
		if (test_and_clear_bit(hard_cpu_id(), &schedmap)) {
			RT_SCHEDULE_MAP_BOTH(schedmap);
		} else {
			RT_SCHEDULE_MAP(schedmap);
		}
	}
	rt_global_restore_flags(flags);
	return 0;
}


/**
 * @anchor rt_sem_wait
 * @brief Take a semaphore.
 *
 * rt_sem_wait waits for a event to be signaled to a semaphore. It is
 * typically called when a task enters a critical region. The
 * semaphore value is decremented and tested. If it is still
 * non-negative rt_sem_wait returns immediately. Otherwise the caller
 * task is blocked and queued up. Queuing may happen in priority order
 * or on FIFO base. This is determined by the compile time option @e
 * SEM_PRIORD. In this case rt_sem_wait returns if:
 *	       - The caller task is in the first place of the waiting
 *		 queue and another task issues a @ref rt_sem_signal()
 *		 call;
 *	       - An error occurs (e.g. the semaphore is destroyed);
 *
 * @param sem points to the structure used in the call to @ref
 *	  rt_sem_init().
 *
 * @return the number of events already signaled upon success.
 * A special value" as described below in case of a failure :
 * - @b 0xFFFF: @e sem does not refer to a valid semaphore.
 *
 * @note In principle 0xFFFF could theoretically be a usable
 *	 semaphores events count, so it could be returned also under
 *	 normal circumstances. It is unlikely you are going to count
 *	 up to such number of events, in any case avoid counting up to
 *	 0xFFFF.<br>
 *	 Just for curiosity: the original Dijkstra notation for
 *	 rt_sem_wait was a "P" operation, and rt_sem_signal was a "V"
 *	 operation. The name for P comes from the Dutch "prolagen", a
 *	 combination of "proberen" (to probe) and "verlagen" (to
 *	 decrement). Also from the word "passeren" (to pass).<br>
 *	 The name for V comes from the Dutch "verhogen" (to increase)
 *	 or "vrygeven" (to release).  (Source: Daniel Tabak -
 *	 Multiprocessors, Prentice Hall, 1990).<br>
 *	 It should be also remarked that real time programming
 *	 practitioners were using semaphores a long time before
 *	 Dijkstra formalized P and V. "In Italian semaforo" means a
 *	 traffic light, so that semaphores have an intuitive appeal
 * 	 and their use and meaning is easily understood.
 */
int rt_sem_wait(SEM *sem)
{
	RT_TASK *rt_current;
	unsigned long flags;
	int count;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	rt_current = RT_CURRENT;
	if ((count = sem->count) <= 0) {
		unsigned long schedmap;
		if (sem->type > 0) {
			if (sem->owndby == rt_current) {
				sem->type++;
				rt_global_restore_flags(flags);
				return count;
			}
			schedmap = pass_prio(sem->owndby, rt_current);
		} else {
			schedmap = 0;
		}
		sem->count--;
		rt_current->state |= RT_SCHED_SEMAPHORE;
		rem_ready_current(rt_current);
		enqueue_blocked(rt_current, &sem->queue, sem->qtype);
		RT_SCHEDULE_MAP_BOTH(schedmap);
		if (rt_current->blocked_on || sem->magic != RT_SEM_MAGIC) {
			rt_current->prio_passed_to = NOTHING;
			rt_global_restore_flags(flags);
			return SEM_ERR;
		} else { 
			count = sem->count;
		}
	} else {
		sem->count--;
	}
	if (sem->type > 0) {
		(sem->owndby = rt_current)->owndres++;
	}
	rt_global_restore_flags(flags);
	return count;
}


/**
 * @anchor rt_sem_wait_if
 * @brief Take a semaphore, only if the calling task is not blocked.
 *
 * rt_sem_wait_if is a version of the semaphore wait operation is
 * similar to @ref rt_sem_wait() but it is never blocks the caller. If
 * the semaphore is not free, rt_sem_wait_if returns immediately and
 * the semaphore value remains unchanged.
 *
 * @param sem points to the structure used in the call to @ref
 * rt_sem_init().
 *
 * @return the number of events already signaled upon success.
 * A special value as described below in case of a failure:
 * - @b 0xFFFF: @e sem does not refer to a valid semaphore.
 *
 * @note In principle 0xFFFF could theoretically be a usable
 *	 semaphores events count so it could be returned also under
 *	 normal circumstances. It is unlikely you are going to count
 *	 up to such number  of events, in any case avoid counting up
 *	 to 0xFFFF.
 */
int rt_sem_wait_if(SEM *sem)
{
	int count;
	unsigned long flags;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	if ((count = sem->count) <= 0) {
		if (sem->type > 0 && sem->owndby == RT_CURRENT) {
			sem->type++;
			rt_global_restore_flags(flags);
			return 0;
		}
	} else {
		sem->count--;
		if (sem->type > 0) {
			(sem->owndby = RT_CURRENT)->owndres++;
		}
	}
	rt_global_restore_flags(flags);
	return count;
}


/**
 * @anchor rt_sem_wait_until
 * @brief Wait a semaphore with timeout.
 *
 * rt_sem_wait_until, like @ref rt_sem_wait_timed() is a timed version
 * of the standard semaphore wait call. The semaphore value is
 * decremented and tested. If it is still non-negative these functions
 * return immediately. Otherwise the caller task is blocked and queued
 * up. Queuing may happen in priority order or on FIFO base. This is
 * determined by the compile time option @e SEM_PRIORD. In this case
 * the function returns if:
 *	- The caller task is in the first place of the waiting queue
 *	  and an other task issues a @ref rt_sem_signal call();
 *	- a timeout occurs;
 *	- an error occurs (e.g. the semaphore is destroyed);
 *
 * In case of a timeout, the semaphore value is incremented before 
 * return.  
 *
 * @param sem points to the structure used in the call to @ref
 *	  rt_sem_init().
 *
 * @param time is an absolute value to the current time.
 *
 * @return the number of events already signaled upon success.
 * Aa special value" as described below in case of a failure:
 * - @b 0xFFFF: @e sem does not refer to a valid semaphore.
 * 
 * @note In principle 0xFFFF could theoretically be a usable
 *	 semaphores events count so it could be returned also under
 *	 normal circumstances. It is unlikely you are going to count
 *	 up to such number of events, in any case avoid counting up to
 *	 0xFFFF.
 */
int rt_sem_wait_until(SEM *sem, RTIME time)
{
	DECLARE_RT_CURRENT;
	int count;
	unsigned long flags;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if ((count = sem->count) <= 0) {
		rt_current->blocked_on = &sem->queue;
		if ((rt_current->resume_time = time) > rt_time_h) {
			unsigned long schedmap;
			if (sem->type > 0) {
				if (sem->owndby == rt_current) {
					sem->type++;
					rt_global_restore_flags(flags);
					return 0;
				}
				schedmap = pass_prio(sem->owndby, rt_current);
			} else {
				schedmap = 0;
			}	
			sem->count--;
			rt_current->state |= (RT_SCHED_SEMAPHORE | RT_SCHED_DELAYED);
			rem_ready_current(rt_current);
			enqueue_blocked(rt_current, &sem->queue, sem->qtype);
			enq_timed_task(rt_current);
			RT_SCHEDULE_MAP_BOTH(schedmap);
		} else {
			sem->count--;
			rt_current->queue.prev = rt_current->queue.next = &rt_current->queue;
		}
		if (sem->magic != RT_SEM_MAGIC) {
			rt_current->prio_passed_to = NOTHING;
			rt_global_restore_flags(flags);
			return SEM_ERR;
		} else {
			if (rt_current->blocked_on) {
				dequeue_blocked(rt_current);
				if(++sem->count > 1 && sem->type) {
					sem->count = 1;
				}
				rt_global_restore_flags(flags);
				return SEM_TIMOUT;
			} else {
				count = sem->count;
			}
		}
	} else {
		sem->count--;
	}
	if (sem->type > 0) {
		(sem->owndby = rt_current)->owndres++;
	}
	rt_global_restore_flags(flags);
	return count;
}


/**
 * @anchor rt_sem_wait_timed
 * @brief Wait a semaphore with timeout.
 *
 * rt_sem_wait_timed, like @ref rt_sem_wait_until(), is a timed version
 * of the standard semaphore wait call. The semaphore value is
 * decremented and tested. If it is still non-negative these functions
 * return immediately. Otherwise the caller task is blocked and queued
 * up. Queuing may happen in priority order or on FIFO base. This is
 * determined by the compile time option @e SEM_PRIORD. In this case
 * the function returns if:
 *	- The caller task is in the first place of the waiting queue
 *	  and an other task issues a @ref rt_sem_signal() call;
 *	- a timeout occurs;
 *	- an error occurs (e.g. the semaphore is destroyed);
 *
 * In case of a timeout, the semaphore value is incremented before 
 * return.  
 *
 * @param sem points to the structure used in the call to @ref
 *	  rt_sem_init().
 *
 * @param delay is an absolute value to the current time.
 *
 * @return the number of events already signaled upon success.
 * A special value as described below in case of a failure:
 * - @b 0xFFFF: @e sem does not refer to a valid semaphore.
 * 
 * @note In principle 0xFFFF could theoretically be a usable
 *	 semaphores events count so it could be returned also under
 *	 normal circumstances. It is unlikely you are going to count
 *	 up to such number of events, in any case avoid counting up to
 *	 0xFFFF.
 */
int rt_sem_wait_timed(SEM *sem, RTIME delay)
{
	return rt_sem_wait_until(sem, get_time() + delay);
}


/* ++++++++++++++++++++++++++ BARRIER SUPPORT +++++++++++++++++++++++++++++++ */

int rt_sem_wait_barrier(SEM *sem)
{
	unsigned long flags;

	if (sem->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	if (!sem->owndby) {
		sem->owndby = (void *)(sem->count < 1 ? 1 : sem->count);
		sem->count = sem->type = 0;
	}
	if ((1 - sem->count) < (int)sem->owndby) {
		rt_sem_wait(sem);
	} else {
		rt_sem_broadcast(sem);
	}
	rt_global_restore_flags(flags);
	return 0;
}

/* +++++++++++++++++++++++++ COND VARIABLES SUPPORT +++++++++++++++++++++++++ */

int rt_cond_signal(CND *cnd)
{
	unsigned long flags;
	RT_TASK *task;

	if (cnd->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}
	flags = rt_global_save_flags_and_cli();
	if ((task = (cnd->queue.next)->task)) {
		dequeue_blocked(task);
		rem_timed_task(task);
		if (task->state != RT_SCHED_READY && (task->state &= ~(RT_SCHED_SEMAPHORE | RT_SCHED_DELAYED)) == RT_SCHED_READY) {
			enq_ready_task(task);
			RT_SCHEDULE(task, hard_cpu_id());
		}
	}
	rt_global_restore_flags(flags);
	return 0;
}

static inline void rt_cndmtx_signal(SEM *mtx, RT_TASK *rt_current)
{
	RT_TASK *task;

	if (mtx->type <= 1) {
		if (++mtx->count > 1) {
			mtx->count = 1;
		}
		if ((task = (mtx->queue.next)->task)) {
			dequeue_blocked(task);
			rem_timed_task(task);
			if (task->state != RT_SCHED_READY && (task->state &= ~(RT_SCHED_SEMAPHORE | RT_SCHED_DELAYED)) == RT_SCHED_READY) {
				enq_ready_task(task);
			}
		}
		if (mtx->type > 0) {
			mtx->owndby = 0;
			if (rt_current->owndres & SEMHLF) {
				--rt_current->owndres;
			}
			if (!rt_current->owndres) {
				renq_current(rt_current, rt_current->base_priority);
			} else if (!(rt_current->owndres & SEMHLF)) {
				int priority;
				renq_current(rt_current, rt_current->base_priority > (priority = ((rt_current->msg_queue.next)->task)->priority) ? priority : rt_current->base_priority);
			}
			if (rt_current->suspdepth > 0) {
				rt_current->state |= RT_SCHED_SUSPENDED;
				rem_ready_current(rt_current);
			} else if (rt_current->suspdepth < 0) {
				rt_task_delete(rt_current);
			}
		}
	} else {
		task = 0;
		mtx->type--;
	}
 	if (task) {
		 RT_SCHEDULE_BOTH(task, hard_cpu_id());
	} else {
		rt_schedule();
	}
}

int rt_cond_wait(CND *cnd, SEM *mtx)
{
	RT_TASK *rt_current;
	unsigned long flags;
	int retval;

	if (cnd->magic != RT_SEM_MAGIC || mtx->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}
	retval = 0;
	flags = rt_global_save_flags_and_cli();
	rt_current = RT_CURRENT;
	rt_current->state |= RT_SCHED_SEMAPHORE;
	rem_ready_current(rt_current);
	enqueue_blocked(rt_current, &cnd->queue, cnd->qtype);
	rt_cndmtx_signal(mtx, rt_current);
	if (rt_current->blocked_on || cnd->magic != RT_SEM_MAGIC) {
		retval = SEM_ERR;
	}
	rt_global_restore_flags(flags);
	rt_sem_wait(mtx);
	return retval;
}

int rt_cond_wait_until(CND *cnd, SEM *mtx, RTIME time)
{
	DECLARE_RT_CURRENT;
	unsigned long flags;
	int retval;

	if (cnd->magic != RT_SEM_MAGIC && mtx->magic != RT_SEM_MAGIC) {
		return SEM_ERR;
	}
	retval = SEM_TIMOUT;
	flags = rt_global_save_flags_and_cli();
	ASSIGN_RT_CURRENT;
	if ((rt_current->resume_time = time) > rt_time_h) {
		rt_current->state |= (RT_SCHED_SEMAPHORE | RT_SCHED_DELAYED);
		rem_ready_current(rt_current);
		enqueue_blocked(rt_current, &cnd->queue, cnd->qtype);
		enq_timed_task(rt_current);
		rt_cndmtx_signal(mtx, rt_current);
		if (cnd->magic != RT_SEM_MAGIC) {
			retval = SEM_ERR;
		} else {
			if (rt_current->blocked_on) {
				dequeue_blocked(rt_current);
			} else {
				retval = 0;
			}
		}
	} else {
		rt_global_restore_flags(flags);
		return retval;
	}
	rt_global_restore_flags(flags);
	rt_sem_wait(mtx);
	return retval;
}

int rt_cond_wait_timed(CND *cnd, SEM *mtx, RTIME delay)
{
	return rt_cond_wait_until(cnd, mtx, get_time() + delay);
}

/* ++++++++++++++++++++ READERS-WRITER LOCKS SUPPORT ++++++++++++++++++++++++ */

int rt_rwl_init(RWL *rwl)
{
	rt_typed_sem_init(&rwl->wrmtx, 1, RES_SEM);
	rt_typed_sem_init(&rwl->wrsem, 0, CNT_SEM);
	rt_typed_sem_init(&rwl->rdsem, 0, CNT_SEM);
	return 0;
}

int rt_rwl_delete(RWL *rwl)
{
	int ret;

	ret  =  rt_sem_delete(&rwl->rdsem);
	ret |= !rt_sem_delete(&rwl->wrsem);
	ret |= !rt_sem_delete(&rwl->wrmtx);
	return ret ? 0 : SEM_ERR;
}

int rt_rwl_rdlock(RWL *rwl)
{
	unsigned long flags;
	RT_TASK *wtask, *rt_current;

	flags = rt_global_save_flags_and_cli();
	rt_current = RT_CURRENT;
	while (rwl->wrmtx.owndby || ((wtask = (rwl->wrsem.queue.next)->task) && wtask->priority <= rt_current->priority)) {
		int ret;
		if (rwl->wrmtx.owndby == rt_current) {
			rt_global_restore_flags(flags);
			return SEM_ERR + 1;
		}
		if ((ret = rt_sem_wait(&rwl->rdsem)) >= SEM_TIMOUT) {
			rt_global_restore_flags(flags);
			return ret;
		}
	}
	((int *)&rwl->rdsem.owndby)[0]++;
	rt_global_restore_flags(flags);
	return 0;
}

int rt_rwl_rdlock_if(RWL *rwl)
{
	unsigned long flags;
	RT_TASK *wtask;

	flags = rt_global_save_flags_and_cli();
	if (!rwl->wrmtx.owndby && (!(wtask = (rwl->wrsem.queue.next)->task) || wtask->priority > RT_CURRENT->priority)) {
		((int *)&rwl->rdsem.owndby)[0]++;
		rt_global_restore_flags(flags);
		return 0;
	}
	rt_global_restore_flags(flags);
	return -1;
}

int rt_rwl_rdlock_until(RWL *rwl, RTIME time)
{
	unsigned long flags;
	RT_TASK *wtask, *rt_current;

	flags = rt_global_save_flags_and_cli();
	rt_current = RT_CURRENT;
	while (rwl->wrmtx.owndby || ((wtask = (rwl->wrsem.queue.next)->task) && wtask->priority <= rt_current->priority)) {
		int ret;
		if (rwl->wrmtx.owndby == rt_current) {
			rt_global_restore_flags(flags);
			return SEM_ERR + 1;
		}
		if ((ret = rt_sem_wait_until(&rwl->rdsem, time)) >= SEM_TIMOUT) {
			rt_global_restore_flags(flags);
			return ret;
		}
	}
	((int *)&rwl->rdsem.owndby)[0]++;
	rt_global_restore_flags(flags);
	return 0;
}

int rt_rwl_rdlock_timed(RWL *rwl, RTIME delay)
{
	return rt_rwl_rdlock_until(rwl, get_time() + delay);
}

int rt_rwl_wrlock(RWL *rwl)
{
	unsigned long flags;
	int ret;

	flags = rt_global_save_flags_and_cli();
	while (rwl->rdsem.owndby) {
		if ((ret = rt_sem_wait(&rwl->wrsem)) >= SEM_TIMOUT) {
			rt_global_restore_flags(flags);
			return ret;
		}
	}
	if ((ret = rt_sem_wait(&rwl->wrmtx)) >= SEM_TIMOUT) {
		rt_global_restore_flags(flags);
		return ret;
	}
	rt_global_restore_flags(flags);
	return 0;
}

int rt_rwl_wrlock_if(RWL *rwl)
{
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	if (!rwl->rdsem.owndby && rt_sem_wait_if(&rwl->wrmtx) >= 0) {
		rt_global_restore_flags(flags);
		return 0;
	}
	rt_global_restore_flags(flags);
	return -1;
}

int rt_rwl_wrlock_until(RWL *rwl, RTIME time)
{
	unsigned long flags;
	int ret;

	flags = rt_global_save_flags_and_cli();
	while (rwl->rdsem.owndby) {
		if ((ret = rt_sem_wait_until(&rwl->wrsem, time)) >= SEM_TIMOUT) {
			rt_global_restore_flags(flags);
			return ret;
		};
	}
	if ((ret = rt_sem_wait_until(&rwl->wrmtx, time)) >= SEM_TIMOUT) {
		rt_global_restore_flags(flags);
		return ret;
	};
	rt_global_restore_flags(flags);
	return 0;
}

int rt_rwl_wrlock_timed(RWL *rwl, RTIME delay)
{
	return rt_rwl_wrlock_until(rwl, get_time() + delay);
}

int rt_rwl_unlock(RWL *rwl)
{
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	if (rwl->wrmtx.owndby) {
		rt_sem_signal(&rwl->wrmtx);
	} else if (rwl->rdsem.owndby) {
    		((int)rwl->rdsem.owndby)--;
	}
	rt_global_restore_flags(flags);
	flags = rt_global_save_flags_and_cli();
	if (!rwl->wrmtx.owndby && !rwl->rdsem.owndby) {
		RT_TASK *wtask, *rtask;
		wtask = (rwl->wrsem.queue.next)->task;
		rtask = (rwl->rdsem.queue.next)->task;
		if (wtask && rtask) {
			if (wtask->priority < rtask->priority) {
				rt_sem_signal(&rwl->wrsem);
			} else {
				rt_sem_signal(&rwl->rdsem);
			}
		} else if (wtask) {
			rt_sem_signal(&rwl->wrsem);
		} else if (rtask) {
			rt_sem_signal(&rwl->rdsem);
		}
        }
	rt_global_restore_flags(flags);
	return 0;
}

/* +++++++++++++++++++++ RECURSIVE SPINLOCKS SUPPORT ++++++++++++++++++++++++ */

int rt_spl_init(SPL *spl)
{
	spl->owndby = 0;
	spl->count  = 0;
	return 0;
}

int rt_spl_delete(SPL *spl)
{
        return 0;
}

int rt_spl_lock(SPL *spl)
{
	unsigned long flags;
	RT_TASK *rt_current;

	hard_save_flags_and_cli(flags);
	if (spl->owndby == (rt_current = RT_CURRENT)) {
		spl->count++;
	} else {
		while (atomic_cmpxchg(&spl->owndby, 0, rt_current));
		spl->flags = flags;
	}
	return 0;
}

int rt_spl_lock_if(SPL *spl)
{
	unsigned long flags;
	RT_TASK *rt_current;

	hard_save_flags_and_cli(flags);
	if (spl->owndby == (rt_current = RT_CURRENT)) {
		spl->count++;
	} else {
		if (atomic_cmpxchg(&spl->owndby, 0, rt_current)) {
			hard_restore_flags(flags);
			return -1;
		}
		spl->flags = flags;
	}
	return 0;
}

int rt_spl_lock_timed(SPL *spl, unsigned long ns)
{
	unsigned long flags;
	RT_TASK *rt_current;

	hard_save_flags_and_cli(flags);
	if (spl->owndby == (rt_current = RT_CURRENT)) {
		spl->count++;
	} else {
		RTIME end_time;
		void *locked;
		end_time = rdtsc() + imuldiv(ns, tuned.cpu_freq, 1000000000);
		while ((locked = atomic_cmpxchg(&spl->owndby, 0, rt_current)) && rdtsc() < end_time);
		if (locked) {
			hard_restore_flags(flags);
			return -1;
		}
		spl->flags = flags;
	}
	return 0;
}

int rt_spl_unlock(SPL *spl)
{
	unsigned long flags;
	RT_TASK *rt_current;

	hard_save_flags_and_cli(flags);
	if (spl->owndby == (rt_current = RT_CURRENT)) {
		if (spl->count) {
			--spl->count;
		} else {
			spl->owndby = 0;
			spl->count  = 0;
			hard_restore_flags(spl->flags);
		}
		return 0;
	}
	hard_restore_flags(flags);
	return -1;
}

/* ++++++ NAMED SEMAPHORES, BARRIER, COND VARIABLES, RWLOCKS, SPINLOCKS +++++ */

#include <rtai_registry.h>

SEM *rt_typed_named_sem_init(const char *sem_name, int value, int type)
{
	SEM *sem;
	unsigned long name;

	if ((sem = rt_get_adr(name = nam2num(sem_name)))) {
		return sem;
	}
	if ((sem = rt_malloc(sizeof(SEM)))) {
		rt_typed_sem_init(sem, value, type);
		if (rt_register(name, sem, IS_SEM, 0)) {
			return sem;
		}
		rt_sem_delete(sem);
	}
	rt_free(sem);
	return (SEM *)0;
}

int rt_named_sem_delete(SEM *sem)
{
	if (!rt_sem_delete(sem)) {
		rt_free(sem);
	}
	return rt_drg_on_adr(sem);
}

RWL *rt_named_rwl_init(const char *rwl_name)
{
	RWL *rwl;
	unsigned long name;

	if ((rwl = rt_get_adr(name = nam2num(rwl_name)))) {
		return rwl;
	}
	if ((rwl = rt_malloc(sizeof(RWL)))) {
		rt_rwl_init(rwl);
		if (rt_register(name, rwl, IS_RWL, 0)) {
			return rwl;
		}
		rt_rwl_delete(rwl);
	}
	rt_free(rwl);
	return (RWL *)0;
}

int rt_named_rwl_delete(RWL *rwl)
{
	if (!rt_rwl_delete(rwl)) {
		rt_free(rwl);
	}
	return rt_drg_on_adr(rwl);
}

SPL *rt_named_spl_init(const char *spl_name)
{
	SPL *spl;
	unsigned long name;

	if ((spl = rt_get_adr(name = nam2num(spl_name)))) {
		return spl;
	}
	if ((spl = rt_malloc(sizeof(SPL)))) {
		rt_spl_init(spl);
		if (rt_register(name, spl, IS_SPL, 0)) {
			return spl;
		}
		rt_spl_delete(spl);
	}
	rt_free(spl);
	return (SPL *)0;
}

int rt_named_spl_delete(SPL *spl)
{
	if (!rt_spl_delete(spl)) {
		rt_free(spl);
	}
	return rt_drg_on_adr(spl);
}

/* +++++ SEMAPHORES, BARRIER, COND VARIABLES, RWLOCKS, SPINLOCKS ENTRIES ++++ */

struct rt_native_fun_entry rt_sem_entries[] = {
	{ { 0, rt_typed_sem_init },        TYPED_SEM_INIT },
	{ { 0, rt_sem_delete },            SEM_DELETE },
	{ { 0, rt_typed_named_sem_init },  NAMED_SEM_INIT },
	{ { 0, rt_named_sem_delete },      NAMED_SEM_DELETE },
	{ { 1, rt_sem_signal },            SEM_SIGNAL },
	{ { 1, rt_sem_wait },              SEM_WAIT },
	{ { 1, rt_sem_wait_if },           SEM_WAIT_IF },
	{ { 1, rt_sem_wait_until },        SEM_WAIT_UNTIL },
	{ { 1, rt_sem_wait_timed },        SEM_WAIT_TIMED },
	{ { 1, rt_sem_broadcast },         SEM_BROADCAST },
	{ { 1, rt_sem_wait_barrier },      SEM_WAIT_BARRIER },
	{ { 1, rt_sem_count },             SEM_COUNT },
        { { 1, rt_cond_wait },             COND_WAIT },
        { { 1, rt_cond_wait_until },       COND_WAIT_UNTIL },
        { { 1, rt_cond_wait_timed },       COND_WAIT_TIMED },
        { { 0, rt_rwl_init },              RWL_INIT },
        { { 0, rt_rwl_delete },            RWL_DELETE },
	{ { 0, rt_named_rwl_init },	   NAMED_RWL_INIT },
	{ { 0, rt_named_rwl_delete },      NAMED_RWL_DELETE },
        { { 1, rt_rwl_rdlock },            RWL_RDLOCK },
        { { 1, rt_rwl_rdlock_if },         RWL_RDLOCK_IF },
        { { 1, rt_rwl_rdlock_until },      RWL_RDLOCK_UNTIL },
        { { 1, rt_rwl_rdlock_timed },      RWL_RDLOCK_TIMED },
        { { 1, rt_rwl_wrlock },            RWL_WRLOCK },
        { { 1, rt_rwl_wrlock_if },         RWL_WRLOCK_IF },
        { { 1, rt_rwl_wrlock_until },      RWL_WRLOCK_UNTIL },
        { { 1, rt_rwl_wrlock_timed },      RWL_WRLOCK_TIMED },
        { { 1, rt_rwl_unlock },            RWL_UNLOCK },
        { { 0, rt_spl_init },              SPL_INIT },
        { { 0, rt_spl_delete },            SPL_DELETE },
	{ { 0, rt_named_spl_init },	   NAMED_SPL_INIT },
	{ { 0, rt_named_spl_delete },      NAMED_SPL_DELETE },
        { { 1, rt_spl_lock },              SPL_LOCK },
        { { 1, rt_spl_lock_if },           SPL_LOCK_IF },
        { { 1, rt_spl_lock_timed },        SPL_LOCK_TIMED },
        { { 1, rt_spl_unlock },            SPL_UNLOCK },
        { { 1, rt_cond_signal}, 	   COND_SIGNAL },
	{ { 0, 0 },  		           000 }
};

extern int set_rt_fun_entries(struct rt_native_fun_entry *entry);
extern void reset_rt_fun_entries(struct rt_native_fun_entry *entry);

int SEM_INIT_MODULE (void)
{
	return set_rt_fun_entries(rt_sem_entries);
}

void SEM_CLEANUP_MODULE (void)
{
	reset_rt_fun_entries(rt_sem_entries);
}

/* +++++++ END SEMAPHORES, BARRIER, COND VARIABLES, RWLOCKS, SPINLOCKS ++++++ */

/*@}*/
