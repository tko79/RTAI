/**
 * @file
 * Real-Time Driver Model for RTAI, driver library
 *
 * @note Copyright (C) 2005-2007 Jan Kiszka <jan.kiszka@web.de>
 * @note Copyright (C) 2005 Joerg Langenberg <joerg.langenberg@gmx.net>
 * @note Copyright (C) 2008 Gilles Chanteperdrix <gilles.chanteperdrix@gmail.com>
 *
 * with adaptions for RTAI by Paolo Mantegazza <mantegazza@aero.polimi.it>
 *
 * RTAI is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * RTAI is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RTAI; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*!
 * @ingroup rtdm
 * @defgroup driverapi Driver Development API
 *
 * This is the lower interface of RTDM provided to device drivers, currently
 * limited to kernel-space. Real-time drivers should only use functions of
 * this interface in order to remain portable.
 */

#include <linux/bitops.h>
#include <asm/page.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <linux/delay.h>
#include <linux/mman.h>
#include <linux/highmem.h>

#include <rtdm/rtdm_driver.h>

/*!
 * @ingroup driverapi
 * @defgroup clock Clock Services
 * @{
 */

#ifdef DOXYGEN_CPP /* Only used for doxygen doc generation */
/**
 * @brief Get system time
 *
 * @return The system time in nanoseconds is returned
 *
 * @note The resolution of this service depends on the system timer. In
 * particular, if the system timer is running in periodic mode, the return
 * value will be limited to multiples of the timer tick period.
 *
 * @note The system timer may have to be started to obtain valid results.
 * Whether this happens automatically or is controlled by the
 * application (as with RTAI) depends on the RTDM host environment.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
nanosecs_abs_t rtdm_clock_read(void);

/**
 * @brief Get monotonic time
 *
 * @return The monotonic time in nanoseconds is returned
 *
 * @note The resolution of this service depends on the system timer. In
 * particular, if the system timer is running in periodic mode, the return
 * value will be limited to multiples of the timer tick period.
 *
 * @note The system timer may have to be started to obtain valid results.
 * Whether this happens automatically or is controlled by the
 * application (as with RTAI) depends on the RTDM host environment.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
nanosecs_abs_t rtdm_clock_read_monotonic(void);
#endif /* DOXYGEN_CPP */
/** @} */

/*!
 * @ingroup driverapi
 * @defgroup rtdmtask Task Services
 * @{
 */

/**
 * @brief Intialise and start a real-time task
 *
 * After initialising a task, the task handle remains valid and can be passed
 * to RTDM services until either rtdm_task_destroy() or rtdm_task_join_nrt()
 * was invoked.
 *
 * @param[in,out] task Task handle
 * @param[in] name Optional task name
 * @param[in] task_proc Procedure to be executed by the task
 * @param[in] arg Custom argument passed to @c task_proc() on entry
 * @param[in] priority Priority of the task, see also
 * @ref taskprio "Task Priority Range"
 * @param[in] period Period in nanoseconds of a cyclic task, 0 for non-cyclic
 * mode
 *
 * @return 0 on success, otherwise negative error code
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: possible.
 */
int rtdm_task_init_cpuid(rtdm_task_t *task, const char *name,
			 rtdm_task_proc_t task_proc, void *arg,
			 int priority, nanosecs_rel_t period, int cpuid)
{
	char *lname;
	if (rt_task_init_cpuid(task, (void *)task_proc, (long)arg, PAGE_SIZE, priority, 0, 0, cpuid)) {
        	return -ENOMEM;
	}
	if (period > 0) {
		rt_task_make_periodic_relative_ns(task, 0, period);
	} else {
		rt_task_resume(task);
	}
	lname = (char *)((unsigned long)name + strlen(name) - 6);
	rt_register(nam2num(lname > name ? lname : name), task, IS_TASK, 0);
	return 0;
}











EXPORT_SYMBOL(rtdm_task_init_cpuid);

#ifdef DOXYGEN_CPP /* Only used for doxygen doc generation */
/**
 * @brief Destroy a real-time task
 *
 * @param[in,out] task Task handle as returned by rtdm_task_init()
 *
 * @note Passing the same task handle to RTDM services after the completion of
 * this function is not allowed.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
void rtdm_task_destroy(rtdm_task_t *task);

/**
 * @brief Adjust real-time task priority
 *
 * @param[in,out] task Task handle as returned by rtdm_task_init()
 * @param[in] priority New priority of the task, see also
 * @ref taskprio "Task Priority Range"
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: possible.
 */
void rtdm_task_set_priority(rtdm_task_t *task, int priority);

/**
 * @brief Adjust real-time task period
 *
 * @param[in,out] task Task handle as returned by rtdm_task_init()
 * @param[in] period New period in nanoseconds of a cyclic task, 0 for
 * non-cyclic mode
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: possible.
 */
int rtdm_task_set_period(rtdm_task_t *task, nanosecs_rel_t period);

/**
 * @brief Wait on next real-time task period
 *
 * @return 0 on success, otherwise:
 *
 * - -EINVAL is returned if calling task is not in periodic mode.
 *
 * - -ETIMEDOUT is returned if a timer overrun occurred, which indicates
 * that a previous release point has been missed by the calling task.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel-based task
 * - User-space task (RT)
 *
 * Rescheduling: always, unless a timer overrun occured.
 */
int rtdm_task_wait_period(void);

/**
 * @brief Activate a blocked real-time task
 *
 * @return Non-zero is returned if the task was actually unblocked from a
 * pending wait state, 0 otherwise.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: possible.
 */
int rtdm_task_unblock(rtdm_task_t *task);

/**
 * @brief Get current real-time task
 *
 * @return Pointer to task handle
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
rtdm_task_t *rtdm_task_current(void);

/**
 * @brief Sleep a specified amount of time
 *
 * @param[in] delay Delay in nanoseconds, see @ref RTDM_TIMEOUT_xxx for
 * special values.
 *
 * @return 0 on success, otherwise:
 *
 * - -EINTR is returned if calling task has been unblock by a signal or
 * explicitly via rtdm_task_unblock().
 *
 * - -EPERM @e may be returned if an illegal invocation environment is
 * detected.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel-based task
 * - User-space task (RT)
 *
 * Rescheduling: always.
 */
int rtdm_task_sleep(nanosecs_rel_t delay);

/**
 * @brief Sleep until a specified absolute time
 *
 * @deprecated Use rtdm_task_sleep_abs instead!
 *
 * @param[in] wakeup_time Absolute timeout in nanoseconds
 *
 * @return 0 on success, otherwise:
 *
 * - -EINTR is returned if calling task has been unblock by a signal or
 * explicitly via rtdm_task_unblock().
 *
 * - -EPERM @e may be returned if an illegal invocation environment is
 * detected.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel-based task
 * - User-space task (RT)
 *
 * Rescheduling: always, unless the specified time already passed.
 */
int rtdm_task_sleep_until(nanosecs_abs_t wakeup_time);

/**
 * @brief Sleep until a specified absolute time
 *
 * @param[in] wakeup_time Absolute timeout in nanoseconds
 * @param[in] mode Selects the timer mode, see RTDM_TIMERMODE_xxx for details
 *
 * @return 0 on success, otherwise:
 *
 * - -EINTR is returned if calling task has been unblock by a signal or
 * explicitly via rtdm_task_unblock().
 *
 * - -EPERM @e may be returned if an illegal invocation environment is
 * detected.
 *
 * - -EINVAL is returned if an invalid parameter was passed.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel-based task
 * - User-space task (RT)
 *
 * Rescheduling: always, unless the specified time already passed.
 */
int rtdm_task_sleep_abs(nanosecs_abs_t wakeup_time, enum rtdm_timer_mode mode);

#endif /* DOXYGEN_CPP */
















/**
 * @brief Wait on a real-time task to terminate
 *
 * @param[in,out] task Task handle as returned by rtdm_task_init()
 * @param[in] poll_delay Delay in milliseconds between periodic tests for the
 * state of the real-time task. This parameter is ignored if the termination
 * is internally realised without polling.
 *
 * @note Passing the same task handle to RTDM services after the completion of
 * this function is not allowed.
 *
 * @note This service does not trigger the termination of the targeted task.
 * The user has to take of this, otherwise rtdm_task_join_nrt() will never
 * return.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - User-space task (non-RT)
 *
 * Rescheduling: possible.
 */
void rtdm_task_join_nrt(rtdm_task_t *task, unsigned int poll_delay)
{
#define JOIN_TIMEOUT 1000 // in millisecs
	int t;


	trace_mark(xn_rtdm_task_joinnrt, "thread %p poll_delay %u",
		   task, poll_delay);

	for (t = 0; task->magic && t < JOIN_TIMEOUT; t += poll_delay) {
		msleep(poll_delay);
	}
	rtdm_task_destroy(task);
}










EXPORT_SYMBOL(rtdm_task_join_nrt);

/**
 * @brief Busy-wait a specified amount of time
 *
 * @param[in] delay Delay in nanoseconds. Note that a zero delay does @b not
 * have the meaning of @c RTDM_TIMEOUT_INFINITE here.
 *
 * @note The caller must not be migratable to different CPUs while executing
 * this service. Otherwise, the actual delay will be undefined.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine (should be avoided or kept short)
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never (except due to external interruptions).
 */
void rtdm_task_busy_sleep(nanosecs_rel_t delay)
{
        xnticks_t wakeup = rtai_rdtsc() + llimd(delay, tuned.cpu_freq, 1000000000);
        while ((xnticks_t)(rtai_rdtsc() - wakeup) < 0)
                cpu_relax();
}

EXPORT_SYMBOL(rtdm_task_busy_sleep);
/** @} */

/*!
 * @ingroup driverapi
 * @defgroup rtdmtimer Timer Services
 * @{
 */

#ifdef DOXYGEN_CPP /* Only used for doxygen doc generation */
/**
 * @brief Initialise a timer
 *
 * @param[in,out] timer Timer handle
 * @param[in] handler Handler to be called on timer expiry
 * @param[in] name Optional timer name
 *
 * @return 0 on success, otherwise negative error code
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
int rtdm_timer_init(rtdm_timer_t *timer, rtdm_timer_handler_t handler,
		    const char *name);
#endif /* DOXYGEN_CPP */

/**
 * @brief Destroy a timer
 *
 * @param[in,out] timer Timer handle as returned by rtdm_timer_init()
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
void rtdm_timer_destroy(rtdm_timer_t *timer)
{



	xntimer_destroy(timer);

}

EXPORT_SYMBOL(rtdm_timer_destroy);

/**
 * @brief Start a timer
 *
 * @param[in,out] timer Timer handle as returned by rtdm_timer_init()
 * @param[in] expiry Firing time of the timer, @c mode defines if relative or
 * absolute
 * @param[in] interval Relative reload value, > 0 if the timer shall work in
 * periodic mode with the specific interval, 0 for one-shot timers
 * @param[in] mode Defines the operation mode, see @ref RTDM_TIMERMODE_xxx for
 * possible values
 *
 * @return 0 on success, otherwise:
 *
 * - -ETIMEDOUT is returned if @c expiry describes an absolute date in the
 * past.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
int rtdm_timer_start(rtdm_timer_t *timer, nanosecs_abs_t expiry,
		     nanosecs_rel_t interval, enum rtdm_timer_mode mode)
{
 
	int err;



	err = xntimer_start(timer, xntbase_ns2ticks(rtdm_tbase, expiry),
			    xntbase_ns2ticks(rtdm_tbase, interval),
			    (xntmode_t)mode);

	return err;
}

EXPORT_SYMBOL(rtdm_timer_start);

/**
 * @brief Stop a timer
 *
 * @param[in,out] timer Timer handle as returned by rtdm_timer_init()
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
void rtdm_timer_stop(rtdm_timer_t *timer)
{
 


	xntimer_stop(timer);

}

EXPORT_SYMBOL(rtdm_timer_stop);

#ifdef DOXYGEN_CPP /* Only used for doxygen doc generation */
/**
 * @brief Start a timer from inside a timer handler
 *
 * @param[in,out] timer Timer handle as returned by rtdm_timer_init()
 * @param[in] expiry Firing time of the timer, @c mode defines if relative or
 * absolute
 * @param[in] interval Relative reload value, > 0 if the timer shall work in
 * periodic mode with the specific interval, 0 for one-shot timers
 * @param[in] mode Defines the operation mode, see @ref RTDM_TIMERMODE_xxx for
 * possible values
 *
 * @return 0 on success, otherwise:
 *
 * - -ETIMEDOUT is returned if @c expiry describes an absolute date in the
 * past.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Timer handler
 *
 * Rescheduling: never.
 */
int rtdm_timer_start_in_handler(rtdm_timer_t *timer, nanosecs_abs_t expiry,
				nanosecs_rel_t interval,
				enum rtdm_timer_mode mode);

/**
 * @brief Stop a timer from inside a timer handler
 *
 * @param[in,out] timer Timer handle as returned by rtdm_timer_init()
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Timer handler
 *
 * Rescheduling: never.
 */
void rtdm_timer_stop_in_handler(rtdm_timer_t *timer);
#endif /* DOXYGEN_CPP */
/** @} */

/* --- RTAI proper common to events, sems and mtxes --- */

static inline int _sem_wait(void *sem)
{
	if (rt_sem_wait(sem) < SEM_TIMOUT) {
		return 0;
	}
	return _rt_whoami()->unblocked ? -EINTR : -EIDRM;
}

static inline int _sem_wait_timed(void *sem, nanosecs_rel_t timeout, rtdm_toseq_t *timeout_seq)
{
	int ret;

	if (timeout < 0) {
		return (ret = rt_sem_wait_if(sem)) > 0 ? 0 : ret != RTE_OBJINV ? -EWOULDBLOCK : -EIDRM;
	}
	if (!timeout) {
		/* infinite timeout */
		ret = rt_sem_wait(sem);
	} else {
		/* timeout sequence, i.e. abs timeout, or relative timeout */
		ret = timeout_seq ? rt_sem_wait_until(sem, *timeout_seq) : rt_sem_wait_timed(sem, nano2count(timeout)); 
	}
	if (ret < SEM_TIMOUT) {
		return 0;
	}
	if (ret == SEM_TIMOUT) {
		return -ETIMEDOUT;
	}
	return _rt_whoami()->unblocked ? -EINTR : -EIDRM;
}

/*!
 * @ingroup driverapi
 * @defgroup rtdmsync Synchronisation Services
 * @{
 */

/*!
 * @name Timeout Sequence Management
 * @{
 */

/**
 * @brief Initialise a timeout sequence
 *
 * This service initialises a timeout sequence handle according to the given
 * timeout value. Timeout sequences allow to maintain a continuous @a timeout
 * across multiple calls of blocking synchronisation services. A typical
 * application scenario is given below.
 *
 * @param[in,out] timeout_seq Timeout sequence handle
 * @param[in] timeout Relative timeout in nanoseconds, see
 * @ref RTDM_TIMEOUT_xxx for special values
 *
 * Application Scenario:
 * @code
int device_service_routine(...)
{
	rtdm_toseq_t timeout_seq;
	...

	rtdm_toseq_init(&timeout_seq, timeout);
	...
	while (received < requested) {
		ret = rtdm_event_timedwait(&data_available, timeout, &timeout_seq);
		if (ret < 0) // including -ETIMEDOUT
			break;

		// receive some data
		...
	}
	...
}
 * @endcode
 * Using a timeout sequence in such a scenario avoids that the user-provided
 * relative @c timeout is restarted on every call to rtdm_event_timedwait(),
 * potentially causing an overall delay that is larger than specified by
 * @c timeout. Moreover, all functions supporting timeout sequences also
 * interpret special timeout values (infinite and non-blocking),
 * disburdening the driver developer from handling them separately.
 *
 * RTAI REMARK: 
 * This is just a confusing set of words to patch missing APIs with absolute
 * deadlines in xenomai. In short a timeout_seq is nothing but the absolute 
 * deadline and so should be used in any related RTAI API that does it so
 * natively already. Thus the same code in RTAI would simply be: 
 * @code
int device_service_routine(...)
{
 	...
	while (received < requested) {
		ret = rtdm_event_timedwait(&event->synch_base, rt_get_time()+timeout);
		if (ret < 0) // including -ETIMEDOUT
			break;
		...
		// receive some data
		...
	}
	...
}
 * @endcode
 * though it is not so because of an easier porting. Nonetheless the RTAI 
 * implementation takes care of using its simpler native way anyhow.
 * END OF RTAI REMARK.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel-based task
 * - User-space task (RT)
 *
 * Rescheduling: never.
 */
void rtdm_toseq_init(rtdm_toseq_t *timeout_seq, nanosecs_rel_t timeout)
{





	*timeout_seq = rt_get_time() + nano2count(timeout);
}

EXPORT_SYMBOL(rtdm_toseq_init);
/** @} */

/*!
 * @name Event Services
 * @{
 */

/**
 * @brief Initialise an event
 *
 * @param[in,out] event Event handle
 * @param[in] pending Non-zero if event shall be initialised as set, 0 otherwise
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
void rtdm_event_init(rtdm_event_t *event, unsigned long pending)
{
	spl_t s;

	trace_mark(xn_rtdm_event_init, "event %p pending %lu", event, pending);



	if (pending)
		event->pending = pending;
	rt_typed_sem_init(&event->synch_base, 0, BIN_SEM | PRIO_Q);
	xnlock_get_irqsave(&nklock, s);
	xnselect_init(&event->select_block);

	xnlock_put_irqrestore(&nklock, s);
}

EXPORT_SYMBOL(rtdm_event_init);

#ifdef DOXYGEN_CPP /* Only used for doxygen doc generation */
/**
 * @brief Destroy an event
 *
 * @param[in,out] event Event handle as returned by rtdm_event_init()
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: possible.
 */
void rtdm_event_destroy(rtdm_event_t *event);

/**
 * @brief Signal an event occurrence to currently listening waiters
 *
 * This function wakes up all current waiters of the given event, but it does
 * not change the event state. Subsequently callers of rtdm_event_wait() or
 * rtdm_event_timedwait() will therefore be blocked first.
 *
 * @param[in,out] event Event handle as returned by rtdm_event_init()
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: possible.
 */
void rtdm_event_pulse(rtdm_event_t *event);
#endif /* DOXYGEN_CPP */

/**
 * @brief Signal an event occurrence
 *
 * This function sets the given event and wakes up all current waiters. If no
 * waiter is presently registered, the next call to rtdm_event_wait() or
 * rtdm_event_timedwait() will return immediately.
 *
 * @param[in,out] event Event handle as returned by rtdm_event_init()
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: possible.
 */
void rtdm_event_signal(rtdm_event_t *event)
{
	unsigned long flags;


	trace_mark(xn_rtdm_event_signal, "event %p", event);

	flags = rt_global_save_flags_and_cli();
	__set_bit(0, &event->pending);
	rt_sem_broadcast(&event->synch_base);
	rt_global_restore_flags(flags);
	SELECT_SIGNAL(&event->select_block, 1);
}







EXPORT_SYMBOL(rtdm_event_signal);

/**
 * @brief Wait on event occurrence
 *
 * This is the light-weight version of rtdm_event_timedwait(), implying an
 * infinite timeout.
 *
 * @param[in,out] event Event handle as returned by rtdm_event_init()
 *
 * @return 0 on success, otherwise:
 *
 * - -EINTR is returned if calling task has been unblock by a signal or
 * explicitly via rtdm_task_unblock().
 *
 * - -EIDRM is returned if @a event has been destroyed.
 *
 * - -EPERM @e may be returned if an illegal invocation environment is
 * detected.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel-based task
 * - User-space task (RT)
 *
 * Rescheduling: possible.
 */
int rtdm_event_wait(rtdm_event_t *event)
{
	unsigned long flags;
	int ret;

	flags = rt_global_save_flags_and_cli();
	if (!__test_and_clear_bit(0, &event->pending)) {
		if (!(ret = _sem_wait(&event->synch_base))) {
			__clear_bit(0, &event->pending);
		}
	} else {
		ret = 0;
	}
	rt_global_restore_flags(flags);

	return ret;
}

EXPORT_SYMBOL(rtdm_event_wait);

/**
 * @brief Wait on event occurrence with timeout
 *
 * This function waits or tests for the occurence of the given event, taking
 * the provided timeout into account. On successful return, the event is
 * reset.
 *
 * @param[in,out] event Event handle as returned by rtdm_event_init()
 * @param[in] timeout Relative timeout in nanoseconds, see
 * @ref RTDM_TIMEOUT_xxx for special values
 * @param[in,out] timeout_seq Handle of a timeout sequence as returned by
 * rtdm_toseq_init() or NULL
 *
 * @return 0 on success, otherwise:
 *
 * - -ETIMEDOUT is returned if the if the request has not been satisfied
 * within the specified amount of time.
 *
 * - -EINTR is returned if calling task has been unblock by a signal or
 * explicitly via rtdm_task_unblock().
 *
 * - -EIDRM is returned if @a event has been destroyed.
 *
 * - -EPERM @e may be returned if an illegal invocation environment is
 * detected.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel-based task
 * - User-space task (RT)
 *
 * Rescheduling: possible.
 */
int rtdm_event_timedwait(rtdm_event_t *event, nanosecs_rel_t timeout,
			 rtdm_toseq_t *timeout_seq)
{
	unsigned long flags;
	int ret;




	trace_mark(xn_rtdm_event_timedwait,
		   "event %p timeout %Lu timeout_seq %p timeout_seq_value %Lu",
		   event, (long long)timeout, timeout_seq, (long long)(timeout_seq ? *timeout_seq : 0));

	flags = rt_global_save_flags_and_cli();
	if (!__test_and_clear_bit(0, &event->pending)) {
		if (!(ret = _sem_wait_timed(&event->synch_base, timeout, timeout_seq))) {
			__clear_bit(0, &event->pending);
			rt_global_restore_flags(flags);
			SELECT_SIGNAL(&event->select_block, 0);
		} else {
			rt_global_restore_flags(flags);
		}
	} else {
		rt_global_restore_flags(flags);
		SELECT_SIGNAL(&event->select_block, 0);
		ret = 0;
	}

	return ret;
}






























EXPORT_SYMBOL(rtdm_event_timedwait);

/**
 * @brief Clear event state
 *
 * @param[in,out] event Event handle as returned by rtdm_event_init()
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
void rtdm_event_clear(rtdm_event_t *event)
{

        trace_mark(xn_rtdm_event_clear, "event %p", event);


	event->pending = 0;
	SELECT_SIGNAL(&event->select_block, 0);
}





EXPORT_SYMBOL(rtdm_event_clear);

#ifdef CONFIG_RTAI_RTDM_SELECT
/**
 * @brief Bind a selector to an event
 *
 * This functions binds the given selector to an event so that the former is
 * notified when the event state changes. Typically the select binding handler
 * will invoke this service.
 *
 * @param[in,out] event Event handle as returned by rtdm_event_init()
 * @param[in,out] selector Selector as passed to the select binding handler
 * @param[in] type Type of the bound event as passed to the select binding handler
 * @param[in] fd_index File descriptor index as passed to the select binding
 * handler
 *
 * @return 0 on success, otherwise:
 *
 * - -EIDRM is returned if @a event has been destroyed.
 *
 * - -ENOMEM is returned if there is insufficient memory to establish the
 * dynamic binding.
 *
 * - -EINVAL is returned if @a type or @a fd_index are invalid.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
int rtdm_event_select_bind(rtdm_event_t *event, rtdm_selector_t *selector,
			   enum rtdm_selecttype type, unsigned fd_index)
{
	struct xnselect_binding *binding;
	int err;
	spl_t s;

	binding = xnmalloc(sizeof(*binding));
	if (!binding)
		return -ENOMEM;

	xnlock_get_irqsave(&nklock, s);
	if (unlikely(event->synch_base.magic != RT_SEM_MAGIC))
		err = -EIDRM;
	else
		err = xnselect_bind(&event->select_block,
				    binding, selector, type, fd_index,
				    event->pending);

	xnlock_put_irqrestore(&nklock, s);

	return err;
}
EXPORT_SYMBOL(rtdm_event_select_bind);
#endif /* CONFIG_RTAI_RTDM_SELECT */
/** @} */

/*!
 * @name Semaphore Services
 * @{
 */

/**
 * @brief Initialise a semaphore
 *
 * @param[in,out] sem Semaphore handle
 * @param[in] value Initial value of the semaphore
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
void rtdm_sem_init(rtdm_sem_t *sem, unsigned long value)
{
	spl_t s;

	trace_mark(xn_rtdm_sem_init, "sem %p value %lu", sem, value);




	rt_typed_sem_init(&sem->sem, value, CNT_SEM | PRIO_Q);

	xnlock_get_irqsave(&nklock, s);
	xnselect_init(&sem->select_block);
	xnlock_put_irqrestore(&nklock, s);
}

EXPORT_SYMBOL(rtdm_sem_init);

#ifdef DOXYGEN_CPP /* Only used for doxygen doc generation */
/**
 * @brief Destroy a semaphore
 *
 * @param[in,out] sem Semaphore handle as returned by rtdm_sem_init()
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: possible.
 */
void rtdm_sem_destroy(rtdm_sem_t *sem);
#endif /* DOXYGEN_CPP */

/**
 * @brief Decrement a semaphore
 *
 * This is the light-weight version of rtdm_sem_timeddown(), implying an
 * infinite timeout.
 *
 * @param[in,out] sem Semaphore handle as returned by rtdm_sem_init()
 *
 * @return 0 on success, otherwise:
 *
 * - -EINTR is returned if calling task has been unblock by a signal or
 * explicitly via rtdm_task_unblock().
 *
 * - -EIDRM is returned if @a sem has been destroyed.
 *
 * - -EPERM @e may be returned if an illegal invocation environment is
 * detected.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel-based task
 * - User-space task (RT)
 *
 * Rescheduling: possible.
 */
int rtdm_sem_down(rtdm_sem_t *sem)
{
	return _sem_wait(&sem->sem);
}

EXPORT_SYMBOL(rtdm_sem_down);

/**
 * @brief Decrement a semaphore with timeout
 *
 * This function tries to decrement the given semphore's value if it is
 * positive on entry. If not, the caller is blocked unless non-blocking
 * operation was selected.
 *
 * @param[in,out] sem Semaphore handle as returned by rtdm_sem_init()
 * @param[in] timeout Relative timeout in nanoseconds, see
 * @ref RTDM_TIMEOUT_xxx for special values
 * @param[in,out] timeout_seq Handle of a timeout sequence as returned by
 * rtdm_toseq_init() or NULL
 *
 * @return 0 on success, otherwise:
 *
 * - -ETIMEDOUT is returned if the if the request has not been satisfied
 * within the specified amount of time.
 *
 * - -EWOULDBLOCK is returned if @a timeout is negative and the semaphore
 * value is currently not positive.
 *
 * - -EINTR is returned if calling task has been unblock by a signal or
 * explicitly via rtdm_task_unblock().
 *
 * - -EIDRM is returned if @a sem has been destroyed.
 *
 * - -EPERM @e may be returned if an illegal invocation environment is
 * detected.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel-based task
 * - User-space task (RT)
 *
 * Rescheduling: possible.
 */
int rtdm_sem_timeddown(rtdm_sem_t *sem, nanosecs_rel_t timeout,
		       rtdm_toseq_t *timeout_seq)
{

	int retval;



	trace_mark(xn_rtdm_sem_timedwait,
		   "sem %p timeout %Lu timeout_seq %p timeout_seq_value %Lu",
		   sem, (long long)timeout, timeout_seq, (long long)(timeout_seq ? *timeout_seq : 0));



































	if ((retval = _sem_wait_timed(&sem->sem, timeout, timeout_seq)) == 1) {	
		SELECT_SIGNAL(&sem->select_block, 0);
	}
	return retval;
}

EXPORT_SYMBOL(rtdm_sem_timeddown);

/**
 * @brief Increment a semaphore
 *
 * This function increments the given semphore's value, waking up a potential
 * waiter which was blocked upon rtdm_sem_down().
 *
 * @param[in,out] sem Semaphore handle as returned by rtdm_sem_init()
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: possible.
 */
void rtdm_sem_up(rtdm_sem_t *sem)
{


	trace_mark(xn_rtdm_sem_up, "sem %p", sem);


	rt_sem_signal(&sem->sem);
	if (sem->sem.count > 0) {
		SELECT_SIGNAL(&sem->select_block, 1);
	}
}






EXPORT_SYMBOL(rtdm_sem_up);

#ifdef CONFIG_RTAI_RTDM_SELECT
/**
 * @brief Bind a selector to a semaphore
 *
 * This functions binds the given selector to the semaphore so that the former
 * is notified when the semaphore state changes. Typically the select binding
 * handler will invoke this service.
 *
 * @param[in,out] sem Semaphore handle as returned by rtdm_sem_init()
 * @param[in,out] selector Selector as passed to the select binding handler
 * @param[in] type Type of the bound event as passed to the select binding handler
 * @param[in] fd_index File descriptor index as passed to the select binding
 * handler
 *
 * @return 0 on success, otherwise:
 *
 * - -EIDRM is returned if @a sem has been destroyed.
 *
 * - -ENOMEM is returned if there is insufficient memory to establish the
 * dynamic binding.
 *
 * - -EINVAL is returned if @a type or @a fd_index are invalid.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
int rtdm_sem_select_bind(rtdm_sem_t *sem, rtdm_selector_t *selector,
			 enum rtdm_selecttype type, unsigned fd_index)
{
	struct xnselect_binding *binding;
	int err;
	spl_t s;

	binding = xnmalloc(sizeof(*binding));
	if (!binding)
		return -ENOMEM;

	xnlock_get_irqsave(&nklock, s);
	if (unlikely(sem->sem.magic != RT_SEM_MAGIC))
		err = -EIDRM;
	else
		err = xnselect_bind(&sem->select_block, binding, selector,
				    type, fd_index, (sem->sem.count > 0));
	xnlock_put_irqrestore(&nklock, s);

	return err;
}
EXPORT_SYMBOL(rtdm_sem_select_bind);
#endif /* CONFIG_RTAI_RTDM_SELECT */
/** @} */

/*!
 * @name Mutex Services
 * @{
 */

/**
 * @brief Initialise a mutex
 *
 * This function initalises a basic mutex with priority inversion protection.
 * "Basic", as it does not allow a mutex owner to recursively lock the same
 * mutex again.
 *
 * @param[in,out] mutex Mutex handle
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
void rtdm_mutex_init(rtdm_mutex_t *mutex)
{







	rt_typed_sem_init(mutex, 1, RES_SEM | PRIO_Q);
}

EXPORT_SYMBOL(rtdm_mutex_init);

#ifdef DOXYGEN_CPP /* Only used for doxygen doc generation */
/**
 * @brief Destroy a mutex
 *
 * @param[in,out] mutex Mutex handle as returned by rtdm_mutex_init()
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: possible.
 */
void rtdm_mutex_destroy(rtdm_mutex_t *mutex);

/**
 * @brief Release a mutex
 *
 * This function releases the given mutex, waking up a potential waiter which
 * was blocked upon rtdm_mutex_lock() or rtdm_mutex_timedlock().
 *
 * @param[in,out] mutex Mutex handle as returned by rtdm_mutex_init()
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel-based task
 * - User-space task (RT)
 *
 * Rescheduling: possible.
 */
void rtdm_mutex_unlock(rtdm_mutex_t *mutex);
#endif /* DOXYGEN_CPP */

/**
 * @brief Request a mutex
 *
 * This is the light-weight version of rtdm_mutex_timedlock(), implying an
 * infinite timeout.
 *
 * @param[in,out] mutex Mutex handle as returned by rtdm_mutex_init()
 *
 * @return 0 on success, otherwise:
 *
 * - -EIDRM is returned if @a mutex has been destroyed.
 *
 * - -EPERM @e may be returned if an illegal invocation environment is
 * detected.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel-based task
 * - User-space task (RT)
 *
 * Rescheduling: possible.
 */
int rtdm_mutex_lock(rtdm_mutex_t *mutex)
{
	int ret; while ((ret = _sem_wait(mutex)) == -EINTR); return ret;
}

EXPORT_SYMBOL(rtdm_mutex_lock);

/**
 * @brief Request a mutex with timeout
 *
 * This function tries to acquire the given mutex. If it is not available, the
 * caller is blocked unless non-blocking operation was selected.
 *
 * @param[in,out] mutex Mutex handle as returned by rtdm_mutex_init()
 * @param[in] timeout Relative timeout in nanoseconds, see
 * @ref RTDM_TIMEOUT_xxx for special values
 * @param[in,out] timeout_seq Handle of a timeout sequence as returned by
 * rtdm_toseq_init() or NULL
 *
 * @return 0 on success, otherwise:
 *
 * - -ETIMEDOUT is returned if the if the request has not been satisfied
 * within the specified amount of time.
 *
 * - -EWOULDBLOCK is returned if @a timeout is negative and the semaphore
 * value is currently not positive.
 *
 * - -EIDRM is returned if @a mutex has been destroyed.
 *
 * - -EPERM @e may be returned if an illegal invocation environment is
 * detected.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel-based task
 * - User-space task (RT)
 *
 * Rescheduling: possible.
 */
int rtdm_mutex_timedlock(rtdm_mutex_t *mutex, nanosecs_rel_t timeout,
			 rtdm_toseq_t *timeout_seq)
{
	int retval;



	trace_mark(xn_rtdm_mutex_timedlock,
		   "mutex %p timeout %Lu timeout_seq %p timeout_seq_value %Lu",
		   mutex, (long long)timeout, timeout_seq, (long long)(timeout_seq ? *timeout_seq : 0));

	if (timeout_seq) {
		while((retval = _sem_wait_timed(mutex, timeout, timeout_seq)) == -EINTR);
	} else {
		rtdm_toseq_t until = rt_get_time() + nano2count(timeout);
		while((retval = _sem_wait_timed(mutex, timeout, &until)) == -EINTR);
	}
	return retval;
}








































EXPORT_SYMBOL(rtdm_mutex_timedlock);
/** @} */

/** @} Synchronisation services */

/*!
 * @ingroup driverapi
 * @defgroup rtdmirq Interrupt Management Services
 * @{
 */

/**
 * @brief Register an interrupt handler
 *
 * This function registers the provided handler with an IRQ line and enables
 * the line.
 *
 * @param[in,out] irq_handle IRQ handle
 * @param[in] irq_no Line number of the addressed IRQ
 * @param[in] handler Interrupt handler
 * @param[in] flags Registration flags, see @ref RTDM_IRQTYPE_xxx for details
 * @param[in] device_name Device name to show up in real-time IRQ lists
 * @param[in] arg Pointer to be passed to the interrupt handler on invocation
 *
 * @return 0 on success, otherwise:
 *
 * - -EINVAL is returned if an invalid parameter was passed.
 *
 * - -EBUSY is returned if the specified IRQ line is already in use.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
int rtdm_irq_request(rtdm_irq_t *irq_handle, unsigned int irq_no,
		     rtdm_irq_handler_t handler, unsigned long flags,
		     const char *device_name, void *arg)
{
        int err;

        xnintr_init(irq_handle, device_name, irq_no, handler, NULL, flags);

        err = xnintr_attach(irq_handle, arg);
        if (err)
                return err;

        err = xnintr_enable(irq_handle);
        if (err)
                xnintr_detach(irq_handle);

        return err;
}

EXPORT_SYMBOL(rtdm_irq_request);

#ifdef DOXYGEN_CPP /* Only used for doxygen doc generation */
/**
 * @brief Release an interrupt handler
 *
 * @param[in,out] irq_handle IRQ handle as returned by rtdm_irq_request()
 *
 * @return 0 on success, otherwise negative error code
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
int rtdm_irq_free(rtdm_irq_t *irq_handle);

/**
 * @brief Enable interrupt line
 *
 * @param[in,out] irq_handle IRQ handle as returned by rtdm_irq_request()
 *
 * @return 0 on success, otherwise negative error code
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: possible.
 */
int rtdm_irq_enable(rtdm_irq_t *irq_handle);

/**
 * @brief Disable interrupt line
 *
 * @param[in,out] irq_handle IRQ handle as returned by rtdm_irq_request()
 *
 * @return 0 on success, otherwise negative error code
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
int rtdm_irq_disable(rtdm_irq_t *irq_handle);
#endif /* DOXYGEN_CPP */

/** @} Interrupt Management Services */

#ifdef DOXYGEN_CPP /* Only used for doxygen doc generation */

/*!
 * @ingroup driverapi
 * @defgroup nrtsignal Non-Real-Time Signalling Services
 *
 * These services provide a mechanism to request the execution of a specified
 * handler in non-real-time context. The triggering can safely be performed in
 * real-time context without suffering from unknown delays. The handler
 * execution will be deferred until the next time the real-time subsystem
 * releases the CPU to the non-real-time part.
 * @{
 */

/**
 * @brief Register a non-real-time signal handler
 *
 * @param[in,out] nrt_sig Signal handle
 * @param[in] handler Non-real-time signal handler
 * @param[in] arg Custom argument passed to @c handler() on each invocation
 *
 * @return 0 on success, otherwise:
 *
 * - -EAGAIN is returned if no free signal slot is available.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
int rtdm_nrtsig_init(rtdm_nrtsig_t *nrt_sig, rtdm_nrtsig_handler_t handler,
		     void *arg);

/**
 * @brief Release a non-realtime signal handler
 *
 * @param[in,out] nrt_sig Signal handle
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
void rtdm_nrtsig_destroy(rtdm_nrtsig_t *nrt_sig);

/**
 * Trigger non-real-time signal
 *
 * @param[in,out] nrt_sig Signal handle
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never in real-time context, possible in non-real-time
 * environments.
 */
void rtdm_nrtsig_pend(rtdm_nrtsig_t *nrt_sig);
/** @} Non-Real-Time Signalling Services */

#endif /* DOXYGEN_CPP */

/*!
 * @ingroup driverapi
 * @defgroup util Utility Services
 * @{
 */


struct rtdm_mmap_data {
	void *src_vaddr;
	unsigned long src_paddr;
	struct vm_operations_struct *vm_ops;
	void *vm_private_data;
};

static int rtdm_mmap_buffer(struct file *filp, struct vm_area_struct *vma)
{
	struct rtdm_mmap_data *mmap_data = filp->private_data;
	unsigned long vaddr, paddr, maddr, size;

	vma->vm_ops = mmap_data->vm_ops;
	vma->vm_private_data = mmap_data->vm_private_data;

	vaddr = (unsigned long)mmap_data->src_vaddr;
	paddr = (unsigned long)mmap_data->src_paddr;
	if (!paddr)
		/* kmalloc memory */
		paddr = virt_to_phys((void *)vaddr);

	maddr = vma->vm_start;
	size = vma->vm_end - vma->vm_start;

#ifdef CONFIG_MMU
	/* Catch vmalloc memory (vaddr is 0 for I/O mapping) */
	if ((vaddr >= VMALLOC_START) && (vaddr < VMALLOC_END)) {
		unsigned long mapped_size = 0;

		RTAI_ASSERT(RTDM, vaddr == PAGE_ALIGN(vaddr), return -EINVAL);
		RTAI_ASSERT(RTDM, (size % PAGE_SIZE) == 0, return -EINVAL);

		while (mapped_size < size) {
			if (xnarch_remap_vm_page(vma, maddr, vaddr))
				return -EAGAIN;

			maddr += PAGE_SIZE;
			vaddr += PAGE_SIZE;
			mapped_size += PAGE_SIZE;
		}
		return 0;
	} else
#endif /* CONFIG_MMU */
	if (mmap_data->src_paddr)
		return xnarch_remap_io_page_range(vma, maddr, paddr,
						  size, PAGE_SHARED);
	else
		return 0; /*xnarch_remap_kmem_page_range(vma, maddr, paddr,
						    size, PAGE_SHARED);*/
}

static struct file_operations rtdm_mmap_fops = {
	.mmap = rtdm_mmap_buffer,
};

static int rtdm_do_mmap(rtdm_user_info_t *user_info,
			struct rtdm_mmap_data *mmap_data,
			size_t len, int prot, void **pptr)
{
	struct file *filp;
	const struct file_operations *old_fops;
	void *old_priv_data;
	void *user_ptr;

	RTAI_ASSERT(RTDM, xnpod_root_p(), return -EPERM;);

	filp = filp_open("/dev/zero", O_RDWR, 0);
	if (IS_ERR(filp))
		return PTR_ERR(filp);

	old_fops = filp->f_op;
	filp->f_op = &rtdm_mmap_fops;

	old_priv_data = filp->private_data;
	filp->private_data = mmap_data;

	down_write(&user_info->mm->mmap_sem);
	user_ptr = (void *)do_mmap(filp, (unsigned long)*pptr, len, prot,
				   MAP_SHARED, 0);
	up_write(&user_info->mm->mmap_sem);

	filp->f_op = (typeof(filp->f_op))old_fops;
	filp->private_data = old_priv_data;

	filp_close(filp, user_info->files);

	if (IS_ERR(user_ptr))
		return PTR_ERR(user_ptr);

	*pptr = user_ptr;
	return 0;
}

/**
 * Map a kernel memory range into the address space of the user.
 *
 * @param[in] user_info User information pointer as passed to the invoked
 * device operation handler
 * @param[in] src_addr Kernel virtual address to be mapped
 * @param[in] len Length of the memory range
 * @param[in] prot Protection flags for the user's memory range, typically
 * either PROT_READ or PROT_READ|PROT_WRITE
 * @param[in,out] pptr Address of a pointer containing the desired user
 * address or NULL on entry and the finally assigned address on return
 * @param[in] vm_ops vm_operations to be executed on the vma_area of the
 * user memory range or NULL
 * @param[in] vm_private_data Private data to be stored in the vma_area,
 * primarily useful for vm_operation handlers
 *
 * @return 0 on success, otherwise (most common values):
 *
 * - -EINVAL is returned if an invalid start address, size, or destination
 * address was passed.
 *
 * - -ENOMEM is returned if there is insufficient free memory or the limit of
 * memory mapping for the user process was reached.
 *
 * - -EAGAIN is returned if too much memory has been already locked by the
 * user process.
 *
 * - -EPERM @e may be returned if an illegal invocation environment is
 * detected.
 *
 * @note This service only works on memory regions allocated via kmalloc() or
 * vmalloc(). To map physical I/O memory to user-space use
 * rtdm_iomap_to_user() instead.
 *
 * @note RTDM supports two models for unmapping the user memory range again.
 * One is explicit unmapping via rtdm_munmap(), either performed when the
 * user requests it via an IOCTL etc. or when the related device is closed.
 * The other is automatic unmapping, triggered by the user invoking standard
 * munmap() or by the termination of the related process. To track release of
 * the mapping and therefore relinquishment of the referenced physical memory,
 * the caller of rtdm_mmap_to_user() can pass a vm_operations_struct on
 * invocation, defining a close handler for the vm_area. See Linux
 * documentaion (e.g. Linux Device Drivers book) on virtual memory management
 * for details.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - User-space task (non-RT)
 *
 * Rescheduling: possible.
 */
int rtdm_mmap_to_user(rtdm_user_info_t *user_info,
		      void *src_addr, size_t len,
		      int prot, void **pptr,
		      struct vm_operations_struct *vm_ops,
		      void *vm_private_data)
{
	struct rtdm_mmap_data mmap_data =
		{ src_addr, 0, vm_ops, vm_private_data };

	return rtdm_do_mmap(user_info, &mmap_data, len, prot, pptr);
}

EXPORT_SYMBOL(rtdm_mmap_to_user);

/**
 * Map an I/O memory range into the address space of the user.
 *
 * @param[in] user_info User information pointer as passed to the invoked
 * device operation handler
 * @param[in] src_addr physical I/O address to be mapped
 * @param[in] len Length of the memory range
 * @param[in] prot Protection flags for the user's memory range, typically
 * either PROT_READ or PROT_READ|PROT_WRITE
 * @param[in,out] pptr Address of a pointer containing the desired user
 * address or NULL on entry and the finally assigned address on return
 * @param[in] vm_ops vm_operations to be executed on the vma_area of the
 * user memory range or NULL
 * @param[in] vm_private_data Private data to be stored in the vma_area,
 * primarily useful for vm_operation handlers
 *
 * @return 0 on success, otherwise (most common values):
 *
 * - -EINVAL is returned if an invalid start address, size, or destination
 * address was passed.
 *
 * - -ENOMEM is returned if there is insufficient free memory or the limit of
 * memory mapping for the user process was reached.
 *
 * - -EAGAIN is returned if too much memory has been already locked by the
 * user process.
 *
 * - -EPERM @e may be returned if an illegal invocation environment is
 * detected.
 *
 * @note RTDM supports two models for unmapping the user memory range again.
 * One is explicit unmapping via rtdm_munmap(), either performed when the
 * user requests it via an IOCTL etc. or when the related device is closed.
 * The other is automatic unmapping, triggered by the user invoking standard
 * munmap() or by the termination of the related process. To track release of
 * the mapping and therefore relinquishment of the referenced physical memory,
 * the caller of rtdm_iomap_to_user() can pass a vm_operations_struct on
 * invocation, defining a close handler for the vm_area. See Linux
 * documentaion (e.g. Linux Device Drivers book) on virtual memory management
 * for details.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - User-space task (non-RT)
 *
 * Rescheduling: possible.
 */
int rtdm_iomap_to_user(rtdm_user_info_t *user_info,
		       unsigned long src_addr, size_t len,
		       int prot, void **pptr,
		       struct vm_operations_struct *vm_ops,
		       void *vm_private_data)
{
	struct rtdm_mmap_data mmap_data =
		{ NULL, src_addr, vm_ops, vm_private_data };

	return rtdm_do_mmap(user_info, &mmap_data, len, prot, pptr);
}

EXPORT_SYMBOL(rtdm_iomap_to_user);

/**
 * Unmap a user memory range.
 *
 * @param[in] user_info User information pointer as passed to
 * rtdm_mmap_to_user() when requesting to map the memory range
 * @param[in] ptr User address or the memory range
 * @param[in] len Length of the memory range
 *
 * @return 0 on success, otherwise:
 *
 * - -EINVAL is returned if an invalid address or size was passed.
 *
 * - -EPERM @e may be returned if an illegal invocation environment is
 * detected.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - User-space task (non-RT)
 *
 * Rescheduling: possible.
 */
int rtdm_munmap(rtdm_user_info_t *user_info, void *ptr, size_t len)
{
	int err;

	RTAI_ASSERT(RTDM, xnpod_root_p(), return -EPERM;);

	down_write(&user_info->mm->mmap_sem);
	err = do_munmap(user_info->mm, (unsigned long)ptr, len);
	up_write(&user_info->mm->mmap_sem);

	return err;
}

EXPORT_SYMBOL(rtdm_munmap);


#ifdef DOXYGEN_CPP /* Only used for doxygen doc generation */

/**
 * Real-time safe message printing on kernel console
 *
 * @param[in] format Format string (conforming standard @c printf())
 * @param ... Arguments referred by @a format
 *
 * @return On success, this service returns the number of characters printed.
 * Otherwise, a negative error code is returned.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine (consider the overhead!)
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never in real-time context, possible in non-real-time
 * environments.
 */
void rtdm_printk(const char *format, ...);

/**
 * Allocate memory block in real-time context
 *
 * @param[in] size Requested size of the memory block
 *
 * @return The pointer to the allocated block is returned on success, NULL
 * otherwise.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine (consider the overhead!)
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
void *rtdm_malloc(size_t size);

/**
 * Release real-time memory block
 *
 * @param[in] ptr Pointer to memory block as returned by rtdm_malloc()
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Interrupt service routine (consider the overhead!)
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
void rtdm_free(void *ptr);

/**
 * Check if read access to user-space memory block is safe
 *
 * @param[in] user_info User information pointer as passed to the invoked
 * device operation handler
 * @param[in] ptr Address of the user-provided memory block
 * @param[in] size Size of the memory block
 *
 * @return Non-zero is return when it is safe to read from the specified
 * memory block, 0 otherwise.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
int rtdm_read_user_ok(rtdm_user_info_t *user_info, const void __user *ptr,
		      size_t size);

/**
 * Check if read/write access to user-space memory block is safe
 *
 * @param[in] user_info User information pointer as passed to the invoked
 * device operation handler
 * @param[in] ptr Address of the user-provided memory block
 * @param[in] size Size of the memory block
 *
 * @return Non-zero is return when it is safe to read from or write to the
 * specified memory block, 0 otherwise.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
int rtdm_rw_user_ok(rtdm_user_info_t *user_info, const void __user *ptr,
		    size_t size);

/**
 * Copy user-space memory block to specified buffer
 *
 * @param[in] user_info User information pointer as passed to the invoked
 * device operation handler
 * @param[in] dst Destination buffer address
 * @param[in] src Address of the user-space memory block
 * @param[in] size Size of the memory block
 *
 * @return 0 on success, otherwise:
 *
 * - -EFAULT is returned if an invalid memory area was accessed.
 *
 * @note Before invoking this service, verify via rtdm_read_user_ok() that the
 * provided user-space address can securely be accessed.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
int rtdm_copy_from_user(rtdm_user_info_t *user_info, void *dst,
			const void __user *src, size_t size);

/**
 * Check if read access to user-space memory block and copy it to specified
 * buffer
 *
 * @param[in] user_info User information pointer as passed to the invoked
 * device operation handler
 * @param[in] dst Destination buffer address
 * @param[in] src Address of the user-space memory block
 * @param[in] size Size of the memory block
 *
 * @return 0 on success, otherwise:
 *
 * - -EFAULT is returned if an invalid memory area was accessed.
 *
 * @note This service is a combination of rtdm_read_user_ok and
 * rtdm_copy_from_user.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
int rtdm_safe_copy_from_user(rtdm_user_info_t *user_info, void *dst,
			     const void __user *src, size_t size);

/**
 * Copy specified buffer to user-space memory block
 *
 * @param[in] user_info User information pointer as passed to the invoked
 * device operation handler
 * @param[in] dst Address of the user-space memory block
 * @param[in] src Source buffer address
 * @param[in] size Size of the memory block
 *
 * @return 0 on success, otherwise:
 *
 * - -EFAULT is returned if an invalid memory area was accessed.
 *
 * @note Before invoking this service, verify via rtdm_rw_user_ok() that the
 * provided user-space address can securely be accessed.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
int rtdm_copy_to_user(rtdm_user_info_t *user_info, void __user *dst,
		      const void *src, size_t size);

/**
 * Check if read/write access to user-space memory block is safe and copy
 * specified buffer to it
 *
 * @param[in] user_info User information pointer as passed to the invoked
 * device operation handler
 * @param[in] dst Address of the user-space memory block
 * @param[in] src Source buffer address
 * @param[in] size Size of the memory block
 *
 * @return 0 on success, otherwise:
 *
 * - -EFAULT is returned if an invalid memory area was accessed.
 *
 * @note This service is a combination of rtdm_rw_user_ok and
 * rtdm_copy_to_user.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
int rtdm_safe_copy_to_user(rtdm_user_info_t *user_info, void __user *dst,
			   const void *src, size_t size);

/**
 * Copy user-space string to specified buffer
 *
 * @param[in] user_info User information pointer as passed to the invoked
 * device operation handler
 * @param[in] dst Destination buffer address
 * @param[in] src Address of the user-space string
 * @param[in] count Maximum number of bytes to copy, including the trailing
 * '0'
 *
 * @return Length of the string on success (not including the trailing '0'),
 * otherwise:
 *
 * - -EFAULT is returned if an invalid memory area was accessed.
 *
 * @note This services already includes a check of the source address,
 * calling rtdm_read_user_ok() for @a src explicitly is not required.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
int rtdm_strncpy_from_user(rtdm_user_info_t *user_info, char *dst,
			   const char __user *src, size_t count);

/**
 * Test if running in a real-time task
 *
 * @return Non-zero is returned if the caller resides in real-time context, 0
 * otherwise.
 *
 * Environments:
 *
 * This service can be called from:
 *
 * - Kernel module initialization/cleanup code
 * - Kernel-based task
 * - User-space task (RT, non-RT)
 *
 * Rescheduling: never.
 */
int rtdm_in_rt_context(void);

#endif /* DOXYGEN_CPP */

/** @} Utility Services */
