/**
 * @ingroup shm
 * @file
 *
 * Interface of the @ref shm "RTAI SHM module".
 *
 * @author Paolo Mantegazza
 *
 * @note Copyright &copy; 1999-2003  Paolo Mantegazza <mantegazza@aero.polimi.it>
 * \n Copyright &copy; 2001  Lineo Inc. (Author: <bkuhn@lineo.com>)
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
 */

/*
ACKNOWLEDGMENTS:
- The suggestion and the code for mmapping at a user specified address is due to  Trevor Woolven (trevw@zentropix.com).
*/


#ifndef _RTAI_SHM_H
#define _RTAI_SHM_H

/** @addtogroup shm
 *@{*/

#ifdef CONFIG_RTAI_SHM_BUILTIN
#define SHM_INIT_MODULE     shm_init_module
#define SHM_CLEANUP_MODULE  shm_cleanup_module
#else /* !CONFIG_RTAI_SHM_BUILTIN */
#define SHM_INIT_MODULE     init_module
#define SHM_CLEANUP_MODULE  cleanup_module
#endif /* CONFIG_RTAI_SHM_BUILTIN */

#ifdef CONFIG_RTAI_MALLOC_VMALLOC
#define USE_RT_MALLOC   1
#else
#define USE_RT_MALLOC  -1
#endif
#define USE_NOT_SHARD   0
#define USE_RTAI_SHM   -1

#if defined(__KERNEL__)

#include <linux/module.h>
#include <linux/version.h>
#include <linux/wrapper.h>
#include <linux/vmalloc.h>

#include <asm/rtai_shm.h>

/**
 * Allocate a chunk of memory to be shared inter-intra kernel modules and Linux
 * processes.
 *
 * rtai_kmalloc is a helper macro. rt_named_malloc() does the real job.
 */
#define rtai_kmalloc(name, size) \
	rt_named_malloc(name, size, USE_RTAI_SHM)  // legacy

/**
 * Free a chunk of shared memory being shared inter-intra kernel modules and
 * Linux processes.
 *
 * rtai_kfree is a helper macro. rt_named_free() does the real job.
 */
#define rtai_kfree(name) \
	rt_named_free(name, 0)  // legacy

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int SHM_INIT_MODULE(void);

void SHM_CLEANUP_MODULE(void);

void *rt_named_malloc(unsigned long name,
		      int size,
		      int suprt);

#define rt_named_malloc_adr(adr, name, size, suprt) \
	rt_named_malloc(name, size, suprt)

int rt_named_free(unsigned long name, 
		    void *addr);

void *rvmalloc(unsigned long size);

void rvfree(void *mem,
	    unsigned long size);

int rvmmap(void *mem,
	   unsigned long memsize,
	   struct vm_area_struct *vma);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#else /* !__KERNEL__ */

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include <rtai_lxrt.h>

#define RTAI_SHM_DEV  "/dev/rtai_shm"

//#define SHM_USE_LXRT

static inline void *_rt_named_malloc(void *start, unsigned long name, int size, int suprt)
{
	int hook;
	void *adr;
	if ((hook = open(RTAI_SHM_DEV, O_RDWR)) <= 0) {
		return 0;
	} else {
		struct { unsigned long name, size; int suprt; } arg = { name, size, suprt };
#ifndef _SHM_USE_LXRT
		if ((size = ioctl(hook, NAMED_MALLOC, (unsigned long)(&arg)))) {
#else
		if ((size = rtai_lxrt(BIDX, SIZARG, NAMED_MALLOC, &arg).i[LOW])) {
#endif
			if ((adr = mmap(start, size, PROT_WRITE | PROT_READ, MAP_SHARED | MAP_FILE, hook, 0)) == (void *)-1) {;
#ifndef SHM_USE_LXRT
				ioctl(hook, NAMED_FREE, name);
#else
				rtai_lxrt(BIDX, sizeof(name), NAMED_FREE, &name);
#endif
				adr = 0;
			}
		} else {
			adr = 0;
		}
	}
	close(hook);
	return adr;
}

/**
 * Allocate a chunk of memory to be shared inter-intra kernel modules and Linux
 * processes.
 *
 * rt_named_malloc/rtai_malloc are used to allocate in user space.
 *
 * @param name is an unsigned long identifier;
 *
 * @param size is the amount of required shared memory.
 *
 * @param suprt is the alloc/free support to use: USE_RT_MALLOC/USE_RTAI_SHM
 *
 * Since @c name can be a clumsy identifier, services are provided to
 * convert 6 characters identifiers to unsigned long, and vice versa.
 *
 * @see the functions nam2num() and num2nam().
 *
 * It must be remarked that the first allocation does a real allocation, any
 * subsequent call to allocate with the same name from Linux processes just maps * the area to the user space, or return the related pointer to the already
 * allocated space in kernel space.  The functions return a pointer to the
 * allocated memory, appropriately mapped to the memory space in use.
 *
 * @returns a valid address on succes, 0 on failure.
 *
 * @note If the same process calls rtai_malloc_adr() and rtai_malloc twice in
 * the same process it get a zero return value on the second call.
 *
 */
#define rt_named_malloc(name, size, suprt)  \
	_rt_named_malloc(0, name, size, suprt)
#define rtai_malloc(name, size)  \
	_rt_named_malloc(0, name, size, USE_RTAI_SHM)  // legacy

/**
 * Allocate a chunk of memory to be shared inter-intra kernel modules and Linux
 * processes.
 *
 * rt_named_malloc_adr/rtai_malloc_adr are used to allocate in user space.
 *
 * @param start_address is a user desired address where the allocated memory
 * should be mapped in user space;
 *
 * @param name is an unsigned long identifier;
 *
 * @param size is the amount of required shared memory.
 *
 * @param suprt is the alloc/free support to use: USE_RT_MALLOC/USE_RTAI_SHM
 *
 * Since @c name can be a clumsy identifier, services are provided to
 * convert 6 characters identifiers to unsigned long, and vice versa.
 *
 * @see the functions nam2num() and num2nam().
 *
 * It must be remarked that the first allocation does a real allocation, any
 * subsequent call to allocate with the same name from Linux processes just maps * the area to the user space, or return the related pointer to the already
 * allocated space in kernel space.  The functions return a pointer to the
 * allocated memory, appropriately mapped to the memory space in use.
 *
 * @note If the same process calls rtai_malloc_adr and rtai_malloc() twice in
 * the same process it get a zero return value on the second call.
 *
 * @returns a valid address on succes, 0 on failure.
 *
 */
#define rt_named_malloc_adr(start_address, name, size, suprt)  \
	_rt_named_malloc(start_address, name, size, suprt)
#define rtai_malloc_adr(start_address, name, size)  \
	_rt_named_malloc(start_address, name, size, USE_RTAI_SHM)  // legacy

/**
 * Free a chunk of shared memory being shared inter-intra kernel modules and
 * Linux processes.
 *
 * rt_named_free/rtai_free are used to free from the user space a previously 
 * allocated shared
 * memory.
 *
 * @param name is the unsigned long identifier used when the memory was
 * allocated;
 *
 * @param adr is the related address.
 *
 * Analogously to what done by the allocation functions the freeing calls have
 * just the effect of unmapping any user space shared memory being freed till 
 * the last is done, as that is the one the really frees any allocated memory.
 *
 * @returns the size of the succesfully freed memory, 0 on failure.
 *
 */

static inline int rt_named_free(unsigned long name, void *adr)
{
	int hook, size;
	if (!adr || (hook = open(RTAI_SHM_DEV, O_RDWR)) <= 0) {
		return 0;
	}
// no RT_NAMED_FREE needed, we release it all and munmap will do it through 
// the vma close operation provided by shm.c
#ifndef SHM_USE_LXRT
	if ((size = ioctl(hook, NAMED_SIZE, name))) {
#else
	if ((size = rtai_lxrt(BIDX, sizeof(name), NAMED_SIZE, &name).i[LOW])) {
#endif
		if (munmap(adr, size)) {
			size = 0;
		}	
	}
	close(hook);
	return size;
}
#define rtai_free(name, adr)  \
	rt_named_free(name, adr)  // legacy

#endif /* __KERNEL__ */

/*@}*/

#endif /* !_RTAI_SHM_H */
