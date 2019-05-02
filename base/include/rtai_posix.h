/*
 * Copyright (C) 1999-2006 Paolo Mantegazza <mantegazza@aero.polimi.it>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 *
 */

#ifndef _RTAI_POSIX_H_
#define _RTAI_POSIX_H_

#define sem_open_rt                     sem_open
#define sem_close_rt                    sem_close
#define sem_init_rt                     sem_init
#define sem_destroy_rt                  sem_destroy
#define sem_wait_rt                     sem_wait
#define sem_trywait_rt                  sem_trywait
#define sem_timedwait_rt                sem_timedwait
#define sem_post_rt                     sem_post
#define sem_getvalue_rt                 sem_getvalue

#define pthread_mutex_open_rt           pthread_mutex_open
#define pthread_mutex_close_rt          pthread_mutex_close
#define pthread_mutex_init_rt           pthread_mutex_init
#define pthread_mutex_destroy_rt        pthread_mutex_destroy
#define pthread_mutex_lock_rt           pthread_mutex_lock
#define pthread_mutex_timedlock_rt      pthread_mutex_timedlock
#define pthread_mutex_trylock_rt        pthread_mutex_trylock
#define pthread_mutex_unlock_rt         pthread_mutex_unlock

#define pthread_cond_open_rt            pthread_cond_open
#define pthread_cond_close_rt           pthread_cond_close
#define pthread_cond_init_rt            pthread_cond_init
#define pthread_cond_destroy_rt         pthread_cond_destroy
#define pthread_cond_signal_rt          pthread_cond_signal
#define pthread_cond_broadcast_rt       pthread_cond_broadcast
#define pthread_cond_wait_rt            pthread_cond_wait
#define pthread_cond_timedwait_rt       pthread_cond_timedwait

#define pthread_barrier_open_rt         pthread_barrier_open
#define pthread_barrier_close_rt        pthread_barrier_close
#define pthread_barrier_init_rt         pthread_barrier_init
#define pthread_barrier_destroy_rt      pthread_barrier_destroy
#define pthread_barrier_wait_rt         pthread_barrier_wait

#define pthread_rwlock_open_rt          pthread_rwlock_open
#define pthread_rwlock_close_rt         pthread_rwlock_close
#define pthread_rwlock_init_rt          pthread_rwlock_init
#define pthread_rwlock_destroy_rt       pthread_rwlock_destroy
#define pthread_rwlock_rdlock_rt        pthread_rwlock_rdlock
#define pthread_rwlock_tryrdlock_rt     pthread_rwlock_tryrdlock
#define pthread_rwlock_timedrdlock_rt   pthread_rwlock_timedrdlock
#define pthread_rwlock_wrlock_rt        pthread_rwlock_wrlock
#define pthread_rwlock_trywrlock_rt     pthread_rwlock_trywrlock
#define pthread_rwlock_timedwrlock_rt   pthread_rwlock_timedwrlock
#define pthread_rwlock_unlock_rt        pthread_rwlock_unlock

#define pthread_spin_init_rt            pthread_spin_init
#define pthread_spin_destroy_rt         pthread_spin_destroy
#define pthread_spin_lock_rt            pthread_spin_lock
#define pthread_spin_trylock_rt         pthread_spin_trylock
#define pthread_spin_unlock_rt          pthread_spin_unlock

#define sched_get_max_priority_rt       sched_get_max_priority
#define sched_get_min_priority_rt       sched_get_min_priority

#define pthread_create_rt               pthread_create
#define pthread_yield_rt                pthread_yield
#define pthread_exit_rt                 pthread_exit
#define pthread_join_rt                 pthread_join
#define pthread_cancel_rt               pthread_cancel
#define pthread_equal_rt                pthread_equal
#define pthread_self_rt                 pthread_self
#define pthread_attr_init_rt            pthread_attr_init
#define pthread_attr_destroy_rt         pthread_attr_destroy
#define pthread_attr_setschedparam_rt   pthread_attr_setschedparam
#define pthread_attr_getschedparam_rt   pthread_attr_getschedparam
#define pthread_attr_setschedpolicy_rt  pthread_attr_setschedpolicy
#define pthread_attr_getschedpolicy_rt  pthread_attr_getschedpolicy
#define pthread_attr_setschedrr_rt      pthread_attr_setschedrr
#define pthread_attr_getschedrr_rt      pthread_attr_getschedrr
#define pthread_attr_setstacksize_rt    pthread_attr_setstacksize
#define pthread_attr_getstacksize_rt    pthread_attr_getstacksize
#define pthread_attr_setstack_rt        pthread_attr_setstack
#define pthread_attr_getstack_rt        pthread_attr_getstack
#define pthread_testcancel_rt           pthread_testcancel

#define clock_gettime_rt                clock_gettime
#define nanosleep_rt                    nanosleep

#define pthread_cleanup_push_rt         pthread_cleanup_push
#define pthread_cleanup_pop_rt          pthread_cleanup_pop

/*
 * _RT DO NOTHING FUNCTIONS 
 */

#define pthread_attr_setdetachstate_rt(attr, detachstate)
#define pthread_detach_rt(thread)
#define pthread_setconcurrency_rt(level)

#define pthread_mutexattr_init_rt(attr)
#define pthread_mutexattr_destroy_rt(attr)
#define pthread_mutexattr_getpshared_rt(attr, pshared)
#define pthread_mutexattr_setpshared_rt(attr, pshared)
#define pthread_mutexattr_settype_rt(attr, kind)
#define pthread_mutexattr_gettype_rt(attr, kind)

#define pthread_condattr_init_rt(attr)
#define pthread_condattr_destroy_rt(attr)
#define pthread_condattr_getpshared_rt(attr, pshared)
#define pthread_condattr_setpshared_rt(attr, pshared)

#ifdef __USE_XOPEN2
#define pthread_barrierattr_getpshared_rt(attr, pshared)
#define pthread_barrierattr_setpshared_rt(attr, pshared)
#endif

#define pthread_rwlockattr_init_rt(attr)
#define pthread_rwlockattr_destroy_rt(attr)
#define pthread_rwlockattr_getpshared_rt( ttr, pshared)
#define pthread_rwlockattr_setpshared_rt(attr, pshared)
#define pthread_rwlockattr_getkind_np_rt(attr, pref)
#define pthread_rwlockattr_setkind_np_rt(attr, pref)


#ifdef __KERNEL__

/*
 * KERNEL DO NOTHING FUNCTIONS (FOR RTAI HARD REAL TIME)
 */

#define pthread_mutexattr_init(attr)
#define pthread_mutexattr_destroy(attr)
#define pthread_mutexattr_getpshared(attr, pshared)
#define pthread_mutexattr_setpshared(attr, pshared)
#define pthread_mutexattr_settype(attr, kind)
#define pthread_mutexattr_gettype(attr, kind)

#define pthread_condattr_init(attr)
#define pthread_condattr_destroy(attr)
#define pthread_condattr_getpshared(attr, pshared)
#define pthread_condattr_setpshared(attr, pshared)

#ifdef __USE_XOPEN2
#define pthread_barrierattr_getpshared(attr, pshared)
#define pthread_barrierattr_setpshared(attr, pshared)
#endif

#define pthread_rwlockattr_init(attr)
#define pthread_rwlockattr_destroy(attr)
#define pthread_rwlockattr_getpshared( ttr, pshared)
#define pthread_rwlockattr_setpshared(attr, pshared)
#define pthread_rwlockattr_getkind_np(attr, pref)
#define pthread_rwlockattr_setkind_np(attr, pref)

#define pthread_setcanceltype_rt(type, oldtype)
#define pthread_setcancelstate_rt(state, oldstate)
#define pthread_attr_getstackaddr_rt(attr, stackaddr) 
#define pthread_attr_setstackaddr_rt(attr, stackaddr)
#define pthread_attr_setguardsize_rt(attr, guardsize) 
#define pthread_attr_getguardsize_rt(attr, guardsize)
#define pthread_attr_setscope_rt(attr, scope)
#define pthread_attr_getscope_rt(attr, scope)
#define pthread_attr_getdetachstate_rt(attr, detachstate)
#define pthread_attr_getdetachstate(attr, detachstate)
#define pthread_attr_setinheritsched_rt(attr, inherit)
#define pthread_attr_getinheritsched_rt(attr, inherit)
#define pthread_attr_setinheritsched(attr, inherit)
#define pthread_attr_getinheritsched(attr, inherit)

#include <rtai_malloc.h>
#include <rtai_rwl.h>
#include <rtai_spl.h>
#include <rtai_sem.h>

#ifndef MAX_PRIO
#define MAX_PRIO  99
#endif
#ifndef MIN_PRIO
#define MIN_PRIO  1
#endif
#define STACK_SIZE     8192
#define RR_QUANTUM_NS  1000000

typedef struct rt_semaphore sem_t;

typedef struct rt_semaphore pthread_mutex_t;

typedef unsigned long pthread_mutexattr_t;

typedef struct rt_semaphore pthread_cond_t;

typedef unsigned long pthread_condattr_t;

typedef struct rt_semaphore pthread_barrier_t;

typedef int pthread_barrierattr_t;

typedef RWL pthread_rwlock_t;

typedef int pthread_rwlockattr_t;

typedef struct rt_spl_t pthread_spinlock_t;

typedef struct rt_task_struct *pthread_t;

typedef struct pthread_attr {
	int stacksize;
	int policy;
	int rr_quantum_ns;
	int priority;
} pthread_attr_t;

typedef struct pthread_cookie {
	RT_TASK task;
	SEM sem;
	void (*task_fun)(int);
	int arg;
} pthread_cookie_t;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static inline int sem_init(sem_t *sem, int pshared, unsigned int value)
{
	if (value < SEM_TIMOUT) {
		rt_typed_sem_init(sem, value, CNT_SEM | PRIO_Q);
		return 0;
	}
	return -EINVAL;
}

static inline int sem_destroy(sem_t *sem)
{
	if (rt_sem_wait_if(sem) >= 0) {
		rt_sem_signal(sem);
		return rt_sem_delete(sem);
	}
	return -EBUSY;
}

static inline int sem_wait(sem_t *sem)
{
	return rt_sem_wait(sem) < SEM_TIMOUT ? 0 : -1;
}

static inline int sem_trywait(sem_t *sem)
{
	return rt_sem_wait_if(sem) > 0 ? 0 : -EAGAIN;
}

static inline int sem_timedwait(sem_t *sem, const struct timespec *abstime)
{
	return rt_sem_wait_until(sem, timespec2count(abstime)) < SEM_TIMOUT ? 0 : -1;
}

static inline int sem_post(sem_t *sem)
{
	return rt_sem_signal(sem) < SEM_TIMOUT ? 0 : -ERANGE;
}

static inline int sem_getvalue(sem_t *sem, int *sval)
{
	if ((*sval = rt_sem_wait_if(sem)) > 0) {
		rt_sem_signal(sem);
	}
	return 0;
}

static inline int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
	rt_typed_sem_init(mutex, 1, RES_SEM);
	return 0;
}

static inline int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	if (rt_sem_wait_if(mutex) > 0) {
		rt_sem_signal(mutex);
		return rt_sem_delete(mutex);
	}
	return -EBUSY;
}

static inline int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	return rt_sem_wait(mutex) < SEM_TIMOUT ? 0 : -EINVAL;
}

static inline int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abstime)
{
	return rt_sem_wait_until(mutex, timespec2count(abstime)) < SEM_TIMOUT ? 0 : -1;
}

static inline int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	return rt_sem_wait_if(mutex) > 0 ? 0 : -EBUSY;
}

static inline int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	return rt_sem_signal(mutex) > 0 ? 0 : -EINVAL;
}

static inline int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *cond_attr)
{
        return sem_init(cond, BIN_SEM, 0);
}

static inline int pthread_cond_destroy(pthread_cond_t *cond)
{
        return sem_destroy((sem_t *)cond);
}

static inline int pthread_cond_signal(pthread_cond_t *cond)
{
	return rt_cond_signal((sem_t *)cond);
}

static inline int pthread_cond_broadcast(pthread_cond_t *cond)
{
	return rt_sem_broadcast(cond);
}

static inline int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	return rt_cond_wait(cond, mutex);
}

static inline int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
	return rt_cond_wait_until(cond, mutex, timespec2count(abstime)) < SEM_TIMOUT ? 0 : -ETIMEDOUT;
}

static inline int pthread_barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned int count)
{
	return sem_init(barrier, CNT_SEM, count);
}

static inline int pthread_barrier_destroy(pthread_barrier_t *barrier)
{
	return sem_destroy(barrier);
}

static inline int pthread_barrier_wait(pthread_barrier_t *barrier)
{
	return rt_sem_wait_barrier(barrier);
}

static inline int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr)
{
	return rt_rwl_init((RWL *)rwlock);
}

static inline int pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
	return rt_rwl_delete((RWL *)rwlock);
}

static inline int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
	return rt_rwl_rdlock((RWL *)rwlock);
}

static inline int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
	return rt_rwl_rdlock_if((RWL *)rwlock);
}

static inline int pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock, struct timespec *abstime)
{
	return rt_rwl_rdlock_until((RWL *)rwlock, timespec2count(abstime));
}

static inline int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
	return rt_rwl_wrlock((RWL *)rwlock);
}

static inline int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
	return rt_rwl_wrlock_if((RWL *)rwlock);
}

static inline int pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock, struct timespec *abstime)
{
	return rt_rwl_wrlock_until((RWL *)rwlock, timespec2count(abstime));
}

static inline int pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
	return rt_rwl_unlock((RWL *)rwlock);
}

static inline int pthread_spin_init(pthread_spinlock_t *lock)
{
	return rt_spl_init((SPL *)lock);
}

static inline int pthread_spin_destroy(pthread_spinlock_t *lock)
{
	return rt_spl_delete((SPL *)lock);
}

static inline int pthread_spin_lock(pthread_spinlock_t *lock)
{
	return rt_spl_lock((SPL *)lock);
}

static inline int pthread_spin_trylock(pthread_spinlock_t *lock)
{
	return rt_spl_lock_if((SPL *)lock);
}

static inline int pthread_spin_unlock(pthread_spinlock_t *lock)
{
	return rt_spl_unlock((SPL *)lock);
}

static inline int get_max_priority(int policy)
{
	return MAX_PRIO;
}

static inline int get_min_priority(int policy)
{
	return MIN_PRIO;
}

static void posix_wrapper_fun(pthread_cookie_t *cookie)
{
	cookie->task_fun(cookie->arg);
	rt_sem_broadcast(&cookie->sem);
	rt_sem_delete(&cookie->sem);
	rt_task_suspend(&cookie->task);
} 

static inline int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
	pthread_cookie_t *cookie;
	cookie = (void *)rt_malloc(sizeof(pthread_cookie_t));
	if (cookie) {
		(cookie->task).magic = 0;
		cookie->task_fun = (void *)start_routine;
		cookie->arg = (int)arg;
		if (!rt_task_init(&cookie->task, (void *)posix_wrapper_fun, (int)cookie,
				(attr) ? attr->stacksize : STACK_SIZE, (attr) ? attr->priority : RT_SCHED_LOWEST_PRIORITY, 1, 0)) {
			*thread = &cookie->task;
			rt_typed_sem_init(&cookie->sem, 0, BIN_SEM | FIFO_Q);
			rt_task_resume(&cookie->task);
        		return 0;
		}
	}
	rt_free(cookie);
       	return ENOMEM;
}

static inline int pthread_yield(void)
{
	rt_task_yield();
	return 0;
}

static inline void pthread_exit(void *retval)
{
	RT_TASK *rt_task;
	rt_task = rt_whoami();
	rt_sem_broadcast((SEM *)(rt_task + 1));
	rt_sem_delete((SEM *)(rt_task + 1));
	rt_task_suspend(rt_task);
}

static inline int pthread_join(pthread_t thread, void **thread_return)
{
	int retval1, retval2;
	if (rt_whoami()->priority != RT_SCHED_LINUX_PRIORITY)
		retval1 = rt_sem_wait((SEM *)(thread + 1));
	else {
		while ((retval1 = rt_sem_wait_if((SEM *)(thread + 1))) <= 0) {
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(HZ/10);
		}
	}
	if (retval1 != 0xFFFF)
		retval1 = 0;
	retval2 = rt_task_delete(thread);
	rt_free(thread);
	return (retval1) ? retval1 : retval2;
}

static inline int pthread_cancel(pthread_t thread)
{
	int retval;
	if (!thread) {
		thread = rt_whoami();
	}
	retval = rt_task_delete(thread);
	rt_free(thread);
	return retval;
}

static inline int pthread_equal(pthread_t thread1,pthread_t thread2)
{
	return thread1 == thread2;
}

static inline pthread_t pthread_self(void)
{
	return rt_whoami();
}

static inline int pthread_attr_init(pthread_attr_t *attr)
{
	attr->stacksize     = STACK_SIZE;
	attr->policy        = SCHED_FIFO;
	attr->rr_quantum_ns = RR_QUANTUM_NS;
	attr->priority      = 1;
	return 0;
}

static inline int pthread_attr_destroy(pthread_attr_t *attr)
{
	return 0;
}

static inline int pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param)
{
	if(param->sched_priority < MIN_PRIO || param->sched_priority > MAX_PRIO) {
		return(EINVAL);
	}
	attr->priority = MAX_PRIO - param->sched_priority;
	return 0;
}

static inline int pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *param)
{
	param->sched_priority = MAX_PRIO - attr->priority;
	return 0;
}

static inline int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy)
{
	if(policy != SCHED_FIFO && policy != SCHED_RR) {
		return EINVAL;
	}
	if ((attr->policy = policy) == SCHED_RR) {
		rt_set_sched_policy(rt_whoami(), SCHED_RR, attr->rr_quantum_ns);
	}
	return 0;
}


static inline int pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *policy)
{
	*policy = attr->policy;
	return 0;
}

static inline int pthread_attr_setschedrr(pthread_attr_t *attr, int rr_quantum_ns)
{
	attr->rr_quantum_ns = rr_quantum_ns;
	return 0;
}


static inline int pthread_attr_getschedrr(const pthread_attr_t *attr, int *rr_quantum_ns)
{
	*rr_quantum_ns = attr->rr_quantum_ns;
	return 0;
}

static inline int pthread_attr_setstacksize(pthread_attr_t *attr, int stacksize)
{
	attr->stacksize = stacksize;
	return 0;
}

static inline int pthread_attr_getstacksize(const pthread_attr_t *attr, int *stacksize)
{
	*stacksize = attr->stacksize;
	return 0;
}

static inline int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, int stacksize)
{
	attr->stacksize = stacksize;
	return 0;
}

static inline int pthread_attr_getstack(const pthread_attr_t *attr, void **stackaddr, int *stacksize)
{
	*stacksize = attr->stacksize;
	return 0;
}

static inline void pthread_testcancel(void)
{
	rt_task_delete(rt_whoami());
	pthread_exit(NULL);
}

static inline int clock_getres(int clockid, struct timespec *res)
{
	res->tv_sec = 0;
	if (!(res->tv_nsec = count2nano(1))) {
		res->tv_nsec = 1;
	}
	return 0;
}

static inline int clock_gettime(int clockid, struct timespec *tp)
{
	count2timespec(rt_get_time(), tp);
	return 0;
}

static inline int clock_settime(int clockid, const struct timespec *tp)
{
	return 0;
}

static inline int clock_nanosleep(int clockid, int flags, const struct timespec *rqtp, struct timespec *rmtp)
{
	RTIME expire;
	if (rqtp->tv_nsec >= 1000000000L || rqtp->tv_nsec < 0 || rqtp->tv_sec < 0) {
		return -EINVAL;
	}
	rt_sleep_until(expire = flags ? timespec2count(rqtp) : rt_get_time() + timespec2count(rqtp));
	if ((expire -= rt_get_time()) > 0) {
		if (rmtp) {
			count2timespec(expire, rmtp);
		}
		return -EINTR;
	}
        return 0;
}

static inline int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
        RTIME expire;
        if (rqtp->tv_nsec >= 1000000000L || rqtp->tv_nsec < 0 || rqtp->tv_sec <
0) {
                return -EINVAL;
        }
        rt_sleep_until(expire = rt_get_time() + timespec2count(rqtp));
        if ((expire -= rt_get_time()) > 0) {
                if (rmtp) {
                        count2timespec(expire, rmtp);
                }
                return -EINTR;
        }
        return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#else  /* !__KERNEL__ */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <ctype.h>

#include <semaphore.h>
#include <limits.h>
#include <pthread.h>

struct task_struct;

#undef  SEM_VALUE_MAX 
#define SEM_VALUE_MAX  (SEM_TIMOUT - 1)
#define SEM_BINARY     (0x7FFFFFFF)

#define RTAI_PNAME_MAXSZ  6
#define SET_ADR(s)     (((void **)s)[0])
#define SET_VAL(s)     (((void **)s)[1])
#define INC_VAL(s)     atomic_inc((atomic_t *)&(((void **)s)[1]))
#define DEC_VAL(s)     atomic_dec_and_test((atomic_t *)&(((void **)s)[1]))
#define TST_VAL(s)     (((void **)s)[1])

#include <asm/rtai_atomic.h>
#include <rtai_sem.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * SUPPORT STUFF
 */

static inline int MAKE_SOFT(void)
{
	if (rt_is_hard_real_time(rt_buddy())) {
		rt_make_soft_real_time();
		return 1;
	}
	return 0;
}

#define MAKE_HARD(hs)  do { if (hs) rt_make_hard_real_time(); } while (0)

RTAI_PROTO(void, count2timespec, (RTIME rt, struct timespec *t))
{
	t->tv_sec = (rt = count2nano(rt))/1000000000;
	t->tv_nsec = rt - t->tv_sec*1000000000LL;
}

RTAI_PROTO(void, nanos2timespec, (RTIME rt, struct timespec *t))
{
	t->tv_sec = rt/1000000000;
	t->tv_nsec = rt - t->tv_sec*1000000000LL;
}

RTAI_PROTO(RTIME, timespec2count, (const struct timespec *t))
{
	return nano2count(t->tv_sec*1000000000LL + t->tv_nsec);
}

RTAI_PROTO(RTIME, timespec2nanos,(const struct timespec *t))
{
	return t->tv_sec*1000000000LL + t->tv_nsec;
}

RTAI_PROTO(int, pthread_get_name_np, (void *adr, unsigned long *nameid))
{
	return (*nameid = rt_get_name(SET_ADR(adr))) ? 0 : EINVAL;
}

RTAI_PROTO(int, pthread_get_adr_np, (unsigned long nameid, void *adr))
{
	return (SET_ADR(adr) = rt_get_adr(nameid)) ? 0 : EINVAL;
}

/*
 * SEMAPHORES
 */

#define str2upr(si, so) \
do { int i; for (i = 0; i <= RTAI_PNAME_MAXSZ; i++) so[i] = toupper(si[i]); } while (0) 

RTAI_PROTO(sem_t *, __wrap_sem_open, (const char *namein, int oflags, int value, int type))
{
	char name[RTAI_PNAME_MAXSZ + 1];
	if (strlen(namein) > RTAI_PNAME_MAXSZ) {
		errno = ENAMETOOLONG;
		return SEM_FAILED;
	}
	str2upr(namein, name);
	if (!oflags || value <= SEM_VALUE_MAX) {
		void *tsem;
		unsigned long handle = 0UL;
		struct { unsigned long name; long value, type; unsigned long *handle; } arg = { nam2num(name), value, type, &handle };
		if ((tsem = rtai_lxrt(BIDX, SIZARG, NAMED_SEM_INIT, &arg).v[LOW])) {
			int fd;
			void *psem;
			if (handle == (unsigned long)tsem) {
				if (oflags == (O_CREAT | O_EXCL)) {
					errno = EEXIST;
					return SEM_FAILED;
				}
				while ((fd = open(name, O_RDONLY)) <= 0 || read(fd, &psem, sizeof(psem)) != sizeof(psem));
				close(fd);
			} else {
				rtai_lxrt(BIDX, SIZARG, NAMED_SEM_INIT, &arg);
				psem = malloc(sizeof(void *));
				((void **)psem)[0] = tsem;
				fd = open(name, O_CREAT | O_WRONLY);
				write(fd, &psem, sizeof(psem));
				close(fd);
			}
			return psem;
		}
		errno = ENOSPC;
		return SEM_FAILED;
	}
	errno = EINVAL;
	return SEM_FAILED;
}

RTAI_PROTO(int, __wrap_sem_close, (sem_t *sem))
{
	struct { void *sem; } arg = { SET_ADR(sem) };
	if (arg.sem) {
		char name[RTAI_PNAME_MAXSZ + 1];
		num2nam(rt_get_name(SET_ADR(sem)), name);
		if (rtai_lxrt(BIDX, SIZARG, SEM_WAIT_IF, &arg).i[LOW] < 0) {
			errno = EBUSY;
			return -1;
		}
		if (!rtai_lxrt(BIDX, SIZARG, NAMED_SEM_DELETE, &arg).i[LOW]) {
			while (!unlink(name));
			free(sem);
		}
		return 0;
	}
	errno =  EINVAL;
	return -1;
}

RTAI_PROTO(int, __wrap_sem_unlink, (const char *namein))
{
	char name[RTAI_PNAME_MAXSZ + 1];
	int fd;
	void *psem;
	if (strlen(namein) > RTAI_PNAME_MAXSZ) {
		errno = ENAMETOOLONG;
		return -1;
	}
	str2upr(namein, name);
	if ((fd = open(name, O_RDONLY)) > 0 && read(fd, &psem, sizeof(psem)) == sizeof(psem)) {
		return __wrap_sem_close(psem);
	}
	errno = ENOENT;
	return -1;
}

RTAI_PROTO(int, __wrap_sem_init, (sem_t *sem, int pshared, unsigned int value))
{
	if (value <= SEM_VALUE_MAX) {
		struct { unsigned long name; long value, type; unsigned long *handle; } arg = { rt_get_name(0), value, CNT_SEM | PRIO_Q, NULL };
		if (!(SET_ADR(sem) = rtai_lxrt(BIDX, SIZARG, NAMED_SEM_INIT, &arg).v[LOW])) {
			errno = ENOSPC;
			return -1;
		}
		return 0;
	}
	errno = EINVAL;
	return -1;
}

RTAI_PROTO(int, __wrap_sem_destroy, (sem_t *sem))
{
	struct { void *sem; } arg = { SET_ADR(sem) };
	if (arg.sem) {
		if (rtai_lxrt(BIDX, SIZARG, SEM_WAIT_IF, &arg).i[LOW] < 0) {
			errno = EBUSY;
			return -1;
		}
		SET_ADR(sem) = NULL;
		while (rtai_lxrt(BIDX, SIZARG, NAMED_SEM_DELETE, &arg).i[LOW]);
		return 0;
	}
	errno =  EINVAL;
	return -1;
}

RTAI_PROTO(int, __wrap_sem_wait, (sem_t *sem))
{
	int oldtype, retval = -1;
	struct { void *sem; } arg = { SET_ADR(sem) };
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);
	pthread_testcancel();
	if (arg.sem) {
		if (abs(rtai_lxrt(BIDX, SIZARG, SEM_WAIT, &arg).i[LOW]) >= RTE_BASE) {
			errno =  EINTR;
		} else {
			retval = 0;
		}
	} else {
		errno =  EINVAL;
	}
	pthread_testcancel();
	pthread_setcanceltype(oldtype, NULL);
	return retval;
}

RTAI_PROTO(int, __wrap_sem_trywait, (sem_t *sem))
{
	struct { void *sem; } arg = { SET_ADR(sem) };
	if (arg.sem) {
		int retval;
		if (abs(retval = rtai_lxrt(BIDX, SIZARG, SEM_WAIT_IF, &arg).i[LOW]) >= RTE_BASE) {
			errno =  EINTR;
			return -1;
		}
		if (retval <= 0) {
			errno = EAGAIN;
			return -1;
		}
		return 0;
	}
	errno = EINVAL;
	return -1;
}

RTAI_PROTO(int, __wrap_sem_timedwait, (sem_t *sem, const struct timespec *abstime))
{
	int oldtype, retval = -1;
	struct { void *sem; RTIME time; } arg = { SET_ADR(sem), timespec2count(abstime) };
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);
	pthread_testcancel();
	if (arg.sem) {
		int ret;
		if (abs(ret = rtai_lxrt(BIDX, SIZARG, SEM_WAIT_UNTIL, &arg).i[LOW]) == RTE_TIMOUT) {
			errno =  ETIMEDOUT;
		} else if (ret >= RTE_BASE) {
			errno = EINTR;
		} else {
			retval = 0;
		}
	} else {
		errno =  EINVAL;
	}
	pthread_testcancel();
	pthread_setcanceltype(oldtype, NULL);
	return retval;
}

RTAI_PROTO(int, __wrap_sem_post, (sem_t *sem))
{
	struct { void *sem; } arg = { SET_ADR(sem) };
	if (arg.sem) {
		rtai_lxrt(BIDX, SIZARG, SEM_SIGNAL, &arg);
		return 0;
	}
	errno =  EINVAL;
	return -1;
}

RTAI_PROTO(int, __wrap_sem_getvalue, (sem_t *sem, int *sval))
{
	struct { void *sem; } arg = { SET_ADR(sem) };
	if (arg.sem) {
		*sval = rtai_lxrt(BIDX, SIZARG, SEM_COUNT, &arg).i[LOW];
		return 0;
	}
	errno =  EINVAL;
	return -1;
}

/*
 * MUTEXES
 */

#define RTAI_MUTEX_DEFAULT    (1 << 0)
#define RTAI_MUTEX_ERRCHECK   (1 << 1)
#define RTAI_MUTEX_RECURSIVE  (1 << 2)
#define RTAI_MUTEX_PSHARED    (1 << 3)

RTAI_PROTO(int, __wrap_pthread_mutex_init, (pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr))
{
	struct { unsigned long name; long value, type; unsigned long *handle; } arg = { rt_get_name(0), !mutexattr || (((long *)mutexattr)[0] & RTAI_MUTEX_DEFAULT) ? RESEM_BINSEM : (((long *)mutexattr)[0] & RTAI_MUTEX_ERRCHECK) ? RESEM_CHEKWT : RESEM_RECURS, RES_SEM, NULL };
	SET_VAL(mutex) = 0;
	if (!(SET_ADR(mutex) = rtai_lxrt(BIDX, SIZARG, NAMED_SEM_INIT, &arg).v[LOW])) {
		return ENOMEM;
	}
	return 0;
}

RTAI_PROTO(int, __wrap_pthread_mutex_destroy, (pthread_mutex_t *mutex))
{
	struct { void *mutex; } arg = { SET_ADR(mutex) };
	if (arg.mutex) {
		int count;
		if (TST_VAL(mutex)) {
			return EBUSY;
		}
		if ((count = rtai_lxrt(BIDX, SIZARG, SEM_WAIT_IF, &arg).i[LOW]) <= 0 || count > 1) {
			if (count > 1 && count != RTE_DEADLOK) {
				rtai_lxrt(BIDX, SIZARG, SEM_SIGNAL, &arg);
			}
			return EBUSY;
		}
		SET_ADR(mutex) = NULL;
		while (rtai_lxrt(BIDX, SIZARG, NAMED_SEM_DELETE, &arg).i[LOW]);
		return 0;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_mutex_lock, (pthread_mutex_t *mutex))
{
	struct { void *mutex; } arg = { SET_ADR(mutex) };
	if (arg.mutex) {
		int retval;
		while ((retval = rtai_lxrt(BIDX, SIZARG, SEM_WAIT, &arg).i[LOW]) == RTE_UNBLKD);
		return abs(retval) < RTE_BASE ? 0 : EDEADLOCK;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_mutex_trylock, (pthread_mutex_t *mutex))
{
	struct { void *mutex; } arg = { SET_ADR(mutex) };
	if (arg.mutex) {
		if (rtai_lxrt(BIDX, SIZARG, SEM_WAIT_IF, &arg).i[LOW] <= 0) {
			return EBUSY;
		}
		return 0;
	}
	return EINVAL;
}

#ifdef __USE_XOPEN2K
RTAI_PROTO(int, __wrap_pthread_mutex_timedlock, (pthread_mutex_t *mutex, const struct timespec *abstime))
{
	struct { void *mutex; RTIME time; } arg = { SET_ADR(mutex), timespec2count(abstime) };
	if (arg.mutex && abstime->tv_nsec >= 0 && abstime->tv_nsec < 1000000000) {
		int retval;
		while ((retval = rtai_lxrt(BIDX, SIZARG, SEM_WAIT_UNTIL, &arg).i[LOW]) == RTE_UNBLKD);
		if (abs(retval) < RTE_BASE) {
			return 0;
		}
		if (retval == RTE_TIMOUT) {
			return ETIMEDOUT;
		}
	}
	return EINVAL;
}
#endif

RTAI_PROTO(int, __wrap_pthread_mutex_unlock, (pthread_mutex_t *mutex))
{
	struct { void *mutex; } arg = { SET_ADR(mutex) };
	if (arg.mutex) {
		return rtai_lxrt(BIDX, SIZARG, SEM_SIGNAL, &arg).i[LOW] == RTE_PERM ? EPERM : 0;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_mutexattr_init, (pthread_mutexattr_t *attr))
{
	((long *)attr)[0] = RTAI_MUTEX_DEFAULT;
	return 0;
}

RTAI_PROTO(int, __wrap_pthread_mutexattr_destroy, (pthread_mutexattr_t *attr))
{
	return 0;
}

RTAI_PROTO(int, __wrap_pthread_mutexattr_getpshared, (const pthread_mutexattr_t *attr, int *pshared))
{	
	*pshared = (((long *)attr)[0] & RTAI_MUTEX_PSHARED) != 0 ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE;
	return 0;
}

RTAI_PROTO(int, __wrap_pthread_mutexattr_setpshared, (pthread_mutexattr_t *attr, int pshared))
{
	if (pshared == PTHREAD_PROCESS_PRIVATE || pshared == PTHREAD_PROCESS_SHARED) {
		if (pshared == PTHREAD_PROCESS_PRIVATE) {
			((long *)attr)[0] &= ~RTAI_MUTEX_PSHARED;
		} else {
			((long *)attr)[0] |= RTAI_MUTEX_PSHARED;
		}
		return 0;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_mutexattr_settype, (pthread_mutexattr_t *attr, int kind))
{
	switch (kind) {
		case PTHREAD_MUTEX_DEFAULT:
			((long *)attr)[0] = (((long *)attr)[0] & ~(RTAI_MUTEX_RECURSIVE | RTAI_MUTEX_ERRCHECK)) | RTAI_MUTEX_DEFAULT;
			break;
		case PTHREAD_MUTEX_ERRORCHECK:
			((long *)attr)[0] = (((long *)attr)[0] & ~(RTAI_MUTEX_RECURSIVE | RTAI_MUTEX_DEFAULT)) | RTAI_MUTEX_ERRCHECK;
			break;
		case PTHREAD_MUTEX_RECURSIVE:
			((long *)attr)[0] = (((long *)attr)[0] & ~(RTAI_MUTEX_DEFAULT | RTAI_MUTEX_ERRCHECK)) | RTAI_MUTEX_RECURSIVE;
			break;
		default:
			return EINVAL;
	}
	return 0;
}

RTAI_PROTO(int, __wrap_pthread_mutexattr_gettype, (const pthread_mutexattr_t *attr, int *kind))
{
	switch (((long *)attr)[0] & (RTAI_MUTEX_DEFAULT | RTAI_MUTEX_ERRCHECK | RTAI_MUTEX_RECURSIVE)) {
		case RTAI_MUTEX_DEFAULT:
			*kind = PTHREAD_MUTEX_DEFAULT;
			break;
		case RTAI_MUTEX_ERRCHECK:
			*kind = PTHREAD_MUTEX_ERRORCHECK;
			break;
		case RTAI_MUTEX_RECURSIVE:
			*kind = PTHREAD_MUTEX_RECURSIVE;
			break;
	}
	return 0;
}

RTAI_PROTO(int, pthread_make_periodic_np, (pthread_t thread, struct timespec *start_delay, struct timespec *period))
{
        struct { RT_TASK *task; RTIME start_time, period; } arg = { NULL, start_delay->tv_sec*1000000000LL + start_delay->tv_nsec, period->tv_sec*1000000000LL + period->tv_nsec };
	int retval;
        return !(retval = rtai_lxrt(BIDX, SIZARG, MAKE_PERIODIC_NS, &arg).i[LOW]) ? 0 : retval == RTE_UNBLKD ? EINTR : ETIMEDOUT;
}

RTAI_PROTO(int, pthread_wait_period_np, (void))
{
        struct { unsigned long dummy; } arg;
        return rtai_lxrt(BIDX, SIZARG, WAIT_PERIOD, &arg).i[LOW];
}

/*
 * CONDVARS
 */

RTAI_PROTO(int, __wrap_pthread_cond_init, (pthread_cond_t *cond, pthread_condattr_t *cond_attr))
{
	struct { unsigned long name; long value, type; unsigned long *handle; } arg = { rt_get_name(0), 0, BIN_SEM | PRIO_Q, NULL };
	if (!(SET_ADR(cond) = rtai_lxrt(BIDX, SIZARG, NAMED_SEM_INIT, &arg).v[LOW])) {
		return ENOMEM;
	}
	return 0;
}

RTAI_PROTO(int, __wrap_pthread_cond_destroy, (pthread_cond_t *cond))
{
	struct { void *cond; } arg = { SET_ADR(cond) };
	if (arg.cond) {
		if (rtai_lxrt(BIDX, SIZARG, SEM_WAIT_IF, &arg).i[LOW] < 0) {
			return EBUSY;
		}
		SET_ADR(cond) = NULL;
		while (rtai_lxrt(BIDX, SIZARG, NAMED_SEM_DELETE, &arg).i[LOW]);
	}
	return 0;
}

RTAI_PROTO(int, __wrap_pthread_cond_signal, (pthread_cond_t *cond))
{
	struct { void *cond; } arg = { SET_ADR(cond) };
	if (arg.cond) {
		rtai_lxrt(BIDX, SIZARG, COND_SIGNAL, &arg);
		return 0;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_cond_broadcast, (pthread_cond_t *cond))
{
	struct { void *cond; } arg = { SET_ADR(cond) };
	if (arg.cond) {
		rtai_lxrt(BIDX, SIZARG, SEM_BROADCAST, &arg);
		return 0;
	}
	return EINVAL;
}

static void internal_cond_cleanup(void *mutex) { DEC_VAL(mutex); }

RTAI_PROTO(int, __wrap_pthread_cond_wait, (pthread_cond_t *cond, pthread_mutex_t *mutex))
{
	int oldtype, retval;
	struct { void *cond; void *mutex; } arg = { SET_ADR(cond), SET_ADR(mutex) };
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);
	pthread_testcancel();
	if (arg.cond && arg.mutex) {
		pthread_cleanup_push(internal_cond_cleanup, mutex);
		INC_VAL(mutex);
		retval = !rtai_lxrt(BIDX, SIZARG, COND_WAIT, &arg).i[LOW] ? 0 : EPERM;
		DEC_VAL(mutex);
		pthread_cleanup_pop(0);
	} else {
		retval = EINVAL;
	}
	pthread_testcancel();
	pthread_setcanceltype(oldtype, NULL);
	return retval;
}

RTAI_PROTO(int, __wrap_pthread_cond_timedwait, (pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime))
{
	int oldtype, retval;
	struct { void *cond; void *mutex; RTIME time; } arg = { SET_ADR(cond), SET_ADR(mutex), timespec2count(abstime) };
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);
	pthread_testcancel();
	if (arg.cond && arg.mutex && abstime->tv_nsec >= 0 && abstime->tv_nsec < 1000000000) {
		pthread_cleanup_push(internal_cond_cleanup, mutex);
		INC_VAL(mutex);
		if (abs(retval = rtai_lxrt(BIDX, SIZARG, COND_WAIT_UNTIL, &arg).i[LOW]) == RTE_TIMOUT) {
			retval = ETIMEDOUT;
		} else {
			retval = !retval ? 0 : EPERM;
		}
		DEC_VAL(mutex);
		pthread_cleanup_pop(0);
	} else {
		retval = EINVAL;
	}
	pthread_testcancel();
	pthread_setcanceltype(oldtype, NULL);
	return retval;
}

RTAI_PROTO(int, __wrap_pthread_condattr_init, (pthread_condattr_t *attr))
{
	((long *)attr)[0] = 0;
	return 0;
}

RTAI_PROTO(int, __wrap_pthread_condattr_destroy, (pthread_condattr_t *attr))
{
	return 0;
}

RTAI_PROTO(int, __wrap_pthread_condattr_getpshared, (const pthread_condattr_t *attr, int *pshared))
{
	*pshared = (((long *)attr)[0] & RTAI_MUTEX_PSHARED) != 0 ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE;
        return 0;
}

RTAI_PROTO(int, __wrap_pthread_condattr_setpshared, (pthread_condattr_t *attr, int pshared))
{
	if (pshared == PTHREAD_PROCESS_PRIVATE || pshared == PTHREAD_PROCESS_SHARED) {
		if (pshared == PTHREAD_PROCESS_PRIVATE) {
			((long *)attr)[0] &= ~RTAI_MUTEX_PSHARED;
		} else {
			((long *)attr)[0] |= RTAI_MUTEX_PSHARED;
		}
		return 0;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_condattr_setclock, (pthread_condattr_t *condattr, clockid_t clockid))
{
        return clockid == CLOCK_MONOTONIC ? 0 : EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_condattr_getclock, (pthread_condattr_t *condattr, clockid_t *clockid))
{
        if (clockid) {
                *clockid = CLOCK_MONOTONIC;
                return 0;
        }
        return EINVAL;
}

/*
 * RWLOCKS
 */

RTAI_PROTO(int, __wrap_pthread_rwlock_init, (pthread_rwlock_t *rwlock, pthread_rwlockattr_t *attr))
{
	struct { unsigned long name; long type; } arg = { rt_get_name(0), RESEM_CHEKWT };
	((pthread_rwlock_t **)rwlock)[0] = (pthread_rwlock_t *)rtai_lxrt(BIDX, SIZARG, LXRT_RWL_INIT, &arg).v[LOW];
        return 0;
}

RTAI_PROTO(int, __wrap_pthread_rwlock_destroy, (pthread_rwlock_t *rwlock))
{
	struct { void *rwlock; } arg = { SET_ADR(rwlock) };
	if (arg.rwlock) {
		return rtai_lxrt(BIDX, SIZARG, LXRT_RWL_DELETE, &arg).i[LOW] > 0 ? 0 : EINVAL;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_rwlock_rdlock, (pthread_rwlock_t *rwlock))
{
	struct { void *rwlock; } arg = { SET_ADR(rwlock) };
	if (arg.rwlock) {
		return rtai_lxrt(BIDX, SIZARG, RWL_RDLOCK, &arg).i[LOW] ? EDEADLOCK : 0;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_rwlock_tryrdlock, (pthread_rwlock_t *rwlock))
{
	struct { void *rwlock; } arg = { SET_ADR(rwlock) };
	if (arg.rwlock) {
		return rtai_lxrt(BIDX, SIZARG, RWL_RDLOCK_IF, &arg).i[LOW] ? EBUSY : 0;
	}
	return EINVAL;
}

#ifdef __USE_XOPEN2K
RTAI_PROTO(int, __wrap_pthread_rwlock_timedrdlock, (pthread_rwlock_t *rwlock, struct timespec *abstime))
{
	struct { void *rwlock; RTIME time; } arg = { SET_ADR(rwlock), timespec2count(abstime) };
	if (arg.rwlock && abstime->tv_nsec >= 0 && abstime->tv_nsec < 1000000000) {
		return rtai_lxrt(BIDX, SIZARG, RWL_RDLOCK_UNTIL, &arg).i[LOW] ? ETIMEDOUT : 0;
	}
	return EINVAL;
}
#endif

RTAI_PROTO(int, __wrap_pthread_rwlock_wrlock, (pthread_rwlock_t *rwlock))
{
	struct { void *rwlock; } arg = { SET_ADR(rwlock) };
	if (arg.rwlock) {
		return rtai_lxrt(BIDX, SIZARG, RWL_WRLOCK, &arg).i[LOW] ? EDEADLOCK : 0;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_rwlock_trywrlock, (pthread_rwlock_t *rwlock))
{
	struct { void *rwlock; } arg = { SET_ADR(rwlock) };
	if (arg.rwlock) {
		return rtai_lxrt(BIDX, SIZARG, RWL_WRLOCK_IF, &arg).i[LOW] ? EBUSY : 0;
	}
	return EINVAL;
}

#ifdef __USE_XOPEN2K
RTAI_PROTO(int, __wrap_pthread_rwlock_timedwrlock, (pthread_rwlock_t *rwlock, struct timespec *abstime))
{
	struct { void *rwlock; RTIME time; } arg = { SET_ADR(rwlock), timespec2count(abstime) };
	if (arg.rwlock && abstime->tv_nsec >= 0 && abstime->tv_nsec < 1000000000) {
		return rtai_lxrt(BIDX, SIZARG, RWL_WRLOCK_UNTIL, &arg).i[LOW] ? ETIMEDOUT : 0;
	}
	return EINVAL;
}
#endif

RTAI_PROTO(int, __wrap_pthread_rwlock_unlock, (pthread_rwlock_t *rwlock))
{
	struct { void *rwlock; } arg = { SET_ADR(rwlock) };
	if (arg.rwlock) {
		return !rtai_lxrt(BIDX, SIZARG, RWL_UNLOCK, &arg).i[LOW] ? 0 : EPERM;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_rwlockattr_init, (pthread_rwlockattr_t *attr))
{
	((long *)attr)[0] = 0;
	return 0;
}

RTAI_PROTO(int, __wrap_pthread_rwlockattr_destroy, (pthread_rwlockattr_t *attr))
{
	return 0;
}

RTAI_PROTO(int, __wrap_pthread_rwlockattr_getpshared, (const pthread_rwlockattr_t *attr, int *pshared))
{
        *pshared = (((long *)attr)[0] & RTAI_MUTEX_PSHARED) != 0 ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE;
        return 0;

	return 0;
}

RTAI_PROTO(int, __wrap_pthread_rwlockattr_setpshared, (pthread_rwlockattr_t *attr, int pshared))
{
        if (pshared == PTHREAD_PROCESS_PRIVATE || pshared == PTHREAD_PROCESS_SHARED) {
                if (pshared == PTHREAD_PROCESS_PRIVATE) {
                        ((long *)attr)[0] &= ~RTAI_MUTEX_PSHARED;
                } else {
                        ((long *)attr)[0] |= RTAI_MUTEX_PSHARED;
                }
                return 0;
        }
        return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_rwlockattr_getkind_np, (const pthread_rwlockattr_t *attr, int *pref))
{
	return 0;
}

RTAI_PROTO(int, __wrap_pthread_rwlockattr_setkind_np, (pthread_rwlockattr_t *attr, int pref))
{
	return 0;
}

/*
 * BARRIERS
 */

#ifdef __USE_XOPEN2K

RTAI_PROTO(int, __wrap_pthread_barrier_init,(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned int count))
{
	if (count > 0) {
		struct { unsigned long name; long count, type; unsigned long *handle; } arg = { rt_get_name(0), count, CNT_SEM | PRIO_Q, NULL };
		return (((pthread_barrier_t **)barrier)[0] = (pthread_barrier_t *)rtai_lxrt(BIDX, SIZARG, NAMED_SEM_INIT, &arg).v[LOW]) ? 0 : ENOMEM;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_barrier_destroy,(pthread_barrier_t *barrier))
{
	struct { void *sem; } arg = { SET_ADR(barrier) };
	SET_ADR(barrier) = NULL;
	if (rtai_lxrt(BIDX, SIZARG, SEM_WAIT_IF, &arg).i[LOW] < 0) {
		return EBUSY;
	}
	return rtai_lxrt(BIDX, SIZARG, NAMED_SEM_DELETE, &arg).i[LOW] == RT_OBJINV ? EINVAL : 0;
}

RTAI_PROTO(int, __wrap_pthread_barrier_wait,(pthread_barrier_t *barrier))
{
	struct { void *sem; } arg = { SET_ADR(barrier) };
	if (arg.sem) {
		return !rtai_lxrt(BIDX, SIZARG, SEM_WAIT_BARRIER, &arg).i[LOW] ? PTHREAD_BARRIER_SERIAL_THREAD : 0;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_barrierattr_init, (pthread_barrierattr_t *attr))
{
	((long *)attr)[0] = PTHREAD_PROCESS_PRIVATE;
	return 0;
}

RTAI_PROTO(int, __wrap_pthread_barrierattr_destroy, (pthread_barrierattr_t *attr))
{
	return 0;
}

RTAI_PROTO(int, __wrap_pthread_barrierattr_setpshared, (pthread_barrierattr_t *attr, int pshared))
{
	if (pshared == PTHREAD_PROCESS_PRIVATE || pshared == PTHREAD_PROCESS_SHARED) {
		((long *)attr)[0] = pshared;
		return 0;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_barrierattr_getpshared, (const pthread_barrierattr_t *attr, int *pshared))
{
	*pshared = ((long *)attr)[0];
	return 0;
}

#endif

/*
 * SCHEDULING
 */

#define PTHREAD_SOFT_REAL_TIME_NP  1
#define PTHREAD_HARD_REAL_TIME_NP  2

RTAI_PROTO(int, pthread_setschedparam_np, (int priority, int policy, int rr_quantum_ns, unsigned long cpus_allowed, int mode))
{ 
	RT_TASK *task;
	if ((task = rt_buddy())) {
		int hs;
		if (cpus_allowed) {
			hs = MAKE_SOFT();
			rt_task_init_schmod(0, 0, 0, 0, 0, cpus_allowed);
			if (!mode) {
				MAKE_HARD(hs);
			}
		}
		if (priority >= 0) {
			rt_change_prio(task, priority);
		}
	} else if (policy == SCHED_FIFO || policy == SCHED_RR || priority >= 0 || cpus_allowed) {
		rt_task_init_schmod(rt_get_name(NULL), priority, 0, 0, policy, cpus_allowed);
		rt_grow_and_lock_stack(PTHREAD_STACK_MIN);
	} else {
		return EINVAL;
	}
	if (policy == SCHED_FIFO || policy == SCHED_RR) {
		rt_set_sched_policy(task, policy = SCHED_FIFO ? 0 : 1, rr_quantum_ns);
	}
	if (mode) {
		if (mode == PTHREAD_HARD_REAL_TIME_NP) {
			rt_make_hard_real_time();
		} else {
			rt_make_soft_real_time();
		}
	}
	return 0;
}

RTAI_PROTO(void, pthread_hard_real_time_np, (void))
{
	rt_make_hard_real_time();
}

RTAI_PROTO(void, pthread_soft_real_time_np, (void))
{
	rt_make_soft_real_time();
}

RTAI_PROTO(int, pthread_gettid_np, (void))
{
        struct { unsigned long dummy; } arg;
        return rtai_lxrt(BIDX, SIZARG, RT_GETTID, &arg).i[LOW];
}

#define PTHREAD_SOFT_REAL_TIME  PTHREAD_SOFT_REAL_TIME_NP
#define PTHREAD_HARD_REAL_TIME  PTHREAD_HARD_REAL_TIME_NP
#define pthread_init_real_time_np(a, b, c, d, e) \
	pthread_setschedparam_np (b, c, 0, d, e)
#define pthread_make_hard_real_time_np() \
	pthread_hard_real_time_np()
#define pthread_make_soft_real_time_np() \
	pthread_soft_real_time_np()

#if 0
#if 1
int __real_pthread_create(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void *), void *arg);
RTAI_PROTO(int, __wrap_pthread_create,(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void *), void *arg))
{
#include <sys/poll.h>

	int hs, ret;
	hs = MAKE_SOFT();
	ret = __real_pthread_create(thread, attr, start_routine, arg);
	MAKE_HARD(hs);
	return ret;
}
#else
#include <sys/mman.h>

struct local_pthread_args_struct { void *(*start_routine)(void *); void *arg; int pipe[3]; };

#ifndef __SUPPORT_THREAD_FUN_
#define __SUPPORT_THREAD_FUN_

static void *support_thread_fun(struct local_pthread_args_struct *args)
{
        RT_TASK *task;
	void *(*start_routine)(void *) = args->start_routine;
	void *arg = args->arg;
	pthread_t thread;
	int policy;
	struct sched_param param;
	
	pthread_getschedparam(thread = pthread_self(), &policy, &param);
	if (policy == SCHED_OTHER) {
		policy = SCHED_RR;
		param.sched_priority = sched_get_priority_min(SCHED_RR);
	}
	pthread_setschedparam(pthread_self(), policy, &param);
	task = rt_task_init_schmod(rt_get_name(0), sched_get_priority_max(policy) - param.sched_priority, 0, 0, policy, 0xF);
	close(args->pipe[1]);
	mlockall(MCL_CURRENT | MCL_FUTURE);
	rt_make_hard_real_time();
	start_routine(arg);
	rt_make_soft_real_time();
	return NULL;
}

#endif /* __SUPPORT_THREAD_FUN_ */

RTAI_PROTO(int, __wrap_pthread_create,(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void *), void *arg))
{
	int hs, ret;
	struct local_pthread_args_struct args = { start_routine, arg };
	hs = MAKE_SOFT();
	pipe(args.pipe);
	ret = pthread_create(thread, attr, (void *)support_thread_fun, (void *)&args);
	read(args.pipe[0], &args.pipe[2], 1);
	close(args.pipe[0]);
	MAKE_HARD(hs);
	return ret;
}
#endif

int __real_pthread_cancel(pthread_t thread);
RTAI_PROTO(int, __wrap_pthread_cancel,(pthread_t thread))
{
	int hs, ret;
	hs = MAKE_SOFT();
	ret = __real_pthread_cancel(thread);
	MAKE_HARD(hs);
	return ret;
}

int __real_pthread_sigmask(int how, const sigset_t *newmask, sigset_t *oldmask);
RTAI_PROTO(int, __wrap_pthread_sigmask,(int how, const sigset_t *newmask, sigset_t *oldmask))
{
	return __real_pthread_sigmask(how, newmask, oldmask);
	int hs, ret;
	hs = MAKE_SOFT();
	ret = __real_pthread_sigmask(how, newmask, oldmask);
	MAKE_HARD(hs);
	return ret;
}

int __real_pthread_kill(pthread_t thread, int signo);
RTAI_PROTO(int, __wrap_pthread_kill,(pthread_t thread, int signo))
{
	int hs, ret;
	hs = MAKE_SOFT();
	ret = __real_pthread_kill(thread, signo);
	MAKE_HARD(hs);
	return ret;
}


int __real_sigwait(const sigset_t *set, int *sig);
RTAI_PROTO(int, __wrap_sigwait,(const sigset_t *set, int *sig))
{
	int hs, ret;
	hs = MAKE_SOFT();
	ret = __real_sigwait(set, sig);
	MAKE_HARD(hs);
	return ret;
}

void __real_pthread_testcancel(void);
RTAI_PROTO(void, __wrap_pthread_testcancel,(void))
{
	__real_pthread_testcancel();
	return;
	int oldtype, oldstate;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);
	if (oldstate != PTHREAD_CANCEL_DISABLE && oldtype != PTHREAD_CANCEL_DEFERRED) {
		MAKE_SOFT();
		rt_task_delete(rt_buddy());
		pthread_exit(NULL);
	}
	pthread_setcanceltype(oldtype, &oldtype);
	pthread_setcancelstate(oldstate, &oldstate);
}

int __real_pthread_yield(void);
RTAI_PROTO(int, __wrap_pthread_yield,(void))
{
	if (rt_is_hard_real_time(rt_buddy())) {
		struct { unsigned long dummy; } arg;
		rtai_lxrt(BIDX, SIZARG, YIELD, &arg);
		return 0;
	}
	return __real_pthread_yield();
}

void __real_pthread_exit(void *retval);
RTAI_PROTO(void, __wrap_pthread_exit,(void *retval))
{
	MAKE_SOFT();
	rt_task_delete(NULL);
	__real_pthread_exit(retval);
}

int __real_pthread_join(pthread_t thread, void **thread_return);
RTAI_PROTO(int, __wrap_pthread_join,(pthread_t thread, void **thread_return))
{
	int hs, ret;
	hs = MAKE_SOFT();
	ret = __real_pthread_join(thread, thread_return);
	MAKE_HARD(hs);
	return ret;
}
#endif

/*
 * SPINLOCKS
 */

#ifdef __USE_XOPEN2K

#if 0
#define ORIGINAL_TEST
RTAI_PROTO(int, __wrap_pthread_spin_init, (pthread_spinlock_t *lock, int pshared))
{
	return lock ? (((pid_t *)lock)[0] = 0) : EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_spin_destroy, (pthread_spinlock_t *lock))
{
	if (lock) {
		return ((pid_t *)lock)[0] ? EBUSY : (((pid_t *)lock)[0] = 0);
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_spin_lock,(pthread_spinlock_t *lock))
{
	if (lock) {
		while (atomic_cmpxchg(lock, 0, 1));
		return 0;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_spin_trylock,(pthread_spinlock_t *lock))
{
	if (lock) {
		return atomic_cmpxchg(lock, 0, 1) ? EBUSY : 0;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_spin_unlock,(pthread_spinlock_t *lock))
{
	if (lock) {
		return ((pid_t *)lock)[0] = 0;
	}
	return EINVAL;
}
#else
static inline int _pthread_gettid_np(void)
{
        struct { unsigned long dummy; } arg;
        return rtai_lxrt(BIDX, SIZARG, RT_GETTID, &arg).i[LOW];
}

RTAI_PROTO(int, __wrap_pthread_spin_init, (pthread_spinlock_t *lock, int pshared))
{
	return lock ? (((pid_t *)lock)[0] = 0) : EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_spin_destroy, (pthread_spinlock_t *lock))
{
	if (lock) {
		return ((pid_t *)lock)[0] ? EBUSY : (((pid_t *)lock)[0] = 0);
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_spin_lock,(pthread_spinlock_t *lock))
{
	if (lock) {
		pid_t tid;
		if (((pid_t *)lock)[0] == (tid = _pthread_gettid_np())) {
			return EDEADLOCK;
		}
		while (atomic_cmpxchg(lock, 0, tid));
		return 0;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_spin_trylock,(pthread_spinlock_t *lock))
{
	if (lock) {
		return atomic_cmpxchg(lock, 0, _pthread_gettid_np()) ? EBUSY : 0;
	}
	return EINVAL;
}

RTAI_PROTO(int, __wrap_pthread_spin_unlock,(pthread_spinlock_t *lock))
{
	if (lock) {
#if 0
		return ((pid_t *)lock)[0] = 0;
#else
		return ((pid_t *)lock)[0] != _pthread_gettid_np() ? EPERM : (((pid_t *)lock)[0] = 0);
#endif
	}
	return EINVAL;
}
#endif

#endif

/*
 * TIMINGS
 */

RTAI_PROTO(int, __wrap_clock_getres, (clockid_t clockid, struct timespec *res))
{
	if (clockid == CLOCK_MONOTONIC) {
		res->tv_sec = 0;
		if (!(res->tv_nsec = count2nano(1))) {
			res->tv_nsec = 1;
		}
		return 0;
	}
	return ENOTSUP;
}

RTAI_PROTO(int, __wrap_clock_gettime, (clockid_t clockid, struct timespec *tp))
{
	if (clockid == CLOCK_MONOTONIC) {
		count2timespec(rt_get_time(), tp);
		return 0;
	}
	return ENOTSUP;
}

RTAI_PROTO(int, __wrap_clock_settime, (clockid_t clockid, const struct timespec *tp))
{
	return ENOTSUP;
}

RTAI_PROTO(int, __wrap_clock_nanosleep,(clockid_t clockid, int flags, const struct timespec *rqtp, struct timespec *rmtp))
{
	if (clockid == CLOCK_MONOTONIC) {
		RTIME expire;
		if (rqtp->tv_nsec >= 1000000000L || rqtp->tv_nsec < 0 || rqtp->tv_sec < 0) {
			return -EINVAL;
		}
		rt_sleep_until(expire = flags ? timespec2count(rqtp) : rt_get_time() + timespec2count(rqtp));
		if ((expire -= rt_get_time()) > 0) {
			if (rmtp) {
				count2timespec(expire, rmtp);
			}
			return -EINTR;
		}
        	return 0;
	}
	return ENOTSUP;
}

RTAI_PROTO(int, __wrap_nanosleep,(const struct timespec *rqtp, struct timespec *rmtp))
{
	RTIME expire;
	if (rqtp->tv_nsec >= 1000000000L || rqtp->tv_nsec < 0 || rqtp->tv_sec < 0) {
		return -EINVAL;
	}
	rt_sleep_until(expire = rt_get_time() + timespec2count(rqtp));
	if ((expire -= rt_get_time()) > 0) {
		if (rmtp) {
			count2timespec(expire, rmtp);
		}
		return -EINTR;
	}
        return 0;
}

/*
 * FUNCTIONS (LIKELY) SAFELY USABLE IN HARD REAL TIME "AS THEY ARE", 
 * BECAUSE MAKE SENSE IN THE INITIALIZATION PHASE ONLY, I.E. BEFORE 
 * GOING HARD REAL TIME
 */

#define pthread_self_rt                  pthread_self
#define pthread_equal_rt                 pthread_equal
#define pthread_attr_init_rt             pthread_attr_init      
#define pthread_attr_destroy_rt          pthread_attr_destroy
#define pthread_attr_getdetachstate_rt   pthread_attr_getdetachstate
#define pthread_attr_setschedpolicy_rt   pthread_attr_setschedpolicy
#define pthread_attr_getschedpolicy_rt   pthread_attr_getschedpolicy 
#define pthread_attr_setschedparam_rt    pthread_attr_setschedparam
#define pthread_attr_getschedparam_rt    pthread_attr_getschedparam
#define pthread_attr_setinheritsched_rt  pthread_attr_setinheritsched
#define pthread_attr_getinheritsched_rt  pthread_attr_getinheritsched
#define pthread_attr_setscope_rt         pthread_attr_setscope
#define pthread_attr_getscope_rt         pthread_attr_getscope
#ifdef __USE_UNIX98
#define pthread_attr_setguardsize_rt     pthread_attr_setguardsize
#define pthread_attr_getguardsize_rt     pthread_attr_getguardsize
#endif
#define pthread_attr_setstackaddr_rt     pthread_attr_setstackaddr
#define pthread_attr_getstackaddr_rt     pthread_attr_getstackaddr
#ifdef __USE_XOPEN2K
#define pthread_attr_setstack_rt         pthread_attr_setstack
#define pthread_attr_getstack_rt         pthread_attr_getstack
#endif
#define pthread_attr_setstacksize_rt     pthread_attr_setstacksize
#define pthread_attr_getstacksize_rt     pthread_attr_getstacksize

/*
 * WORKING FUNCTIONS USABLE IN HARD REAL TIME, THIS IS THE REAL STUFF
 */

#define pthread_setcancelstate_rt  pthread_setcancelstate
#define pthread_setcanceltype_rt   pthread_setcanceltype

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !__KERNEL__ */

#endif /* !_RTAI_POSIX_H_ */
