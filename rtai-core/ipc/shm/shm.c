/**
 * @ingroup shm
 * @file
 *
 * Implementation of the @ref shm "RTAI SHM module".
 *
 * @author Paolo Mantegazza
 *
 * @note Copyright &copy; 1999-2003  Paolo Mantegazza <mantegazza@aero.polimi.it>
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


/**
 * @defgroup shm Shared memory services.
 * 
 * This module contains some functions that allow sharing memory inter-intra
 * real-time tasks and Linux processes. In fact it can be an alternative to
 * SYSTEM V shared memory, the services are symmetrical, i.e. similar calls can
 * be used both in real time tasks, i.e. within the kernel, and Linux processes.
 * The function calls for Linux processes are inlined in this file. This
 * approach has been preferred to a library since: is simpler, more effective,
 * the calls are short, simple and just a few per process.
 *
 *@{*/

#define RTAI_SHM_MISC_MINOR  254 // The same minor used to mknod for major 10.

#include <linux/version.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>

#include <rtai_trace.h>
#include <rtai_schedcore.h>
#include <rtai_registry.h>
#include <rtai_shm.h>

MODULE_LICENSE("GPL");

//#define DEBUG_SHM USE_RT_MALLOC
#ifdef  DEBUG_SHM
#define DEBUG_SUPRT()  do { suprt = DEBUG_SHM; } while (0)
#else
#define DEBUG_SUPRT()
#endif

#define ALIGN2PAGE(adr)        ((void *)PAGE_ALIGN((unsigned long)adr))
#define RT_SHM_OP_PERM(suprt)  (suprt >= 0 ? 1 : (!(rt_whoami()->is_hard)))

static inline void *rt_shm_alloc(unsigned long name, int size, int suprt)
{
	void *adr;

	DEBUG_SUPRT();
#ifndef CONFIG_RTAI_MALLOC_VMALLOC
/*
 * brute force for those that know it too much and do not use the macros
 * defined in rtai_shm.h.
 */
	if (suprt > 0) {
		suprt = -1;
	}
#endif
	if (!(adr = rt_get_adr(name)) && size > 0) {
		if (suprt) {
			size = (((size) - 1) & PAGE_MASK) + (suprt > 0 ? 2*PAGE_SIZE : PAGE_SIZE);
		}
		if ((adr = suprt >= 0 ? rt_malloc(size) : rvmalloc(size))) {
			if (!rt_register(name, adr, suprt >= 0 ? size : -size, 0)) {
				if (suprt >= 0) {
					rt_free(adr);
				} else {
					rvfree(adr, size);
				}
				return 0;
			}
			memset(adr, 0, size);
		}
	}
	return suprt ? ALIGN2PAGE(adr) : adr;
}

static inline int rt_shm_free(unsigned long name, int size)
{
	void *adr;

	if (size && (adr = rt_get_adr(name))) {
		if (!rt_dec_count(name)) {
			rt_drg_on_name(name);
			if (size > 0) {
				rt_free(adr);
			} else {
				rvfree(adr, -size);
			}
		}
		return abs(size);
	}
	return 0;
}

/**
 * Allocate a chunk of memory to be shared inter-intra kernel modules and Linux
 * processes.
 *
 * @internal
 * 
 * rt_named_malloc is used to allocate shared memory.
 * 
 * @param name is an unsigned long identifier;
 * 
 * @param size is the amount of required shared memory;
 * 
 * @param suprt is the alloc/free support to use: USE_RT_MALLOC/USE_RTAI_SHM
 * 
 * Since @a name can be a clumsy identifier, services are provided to
 * convert 6 characters identifiers to unsigned long, and vice versa.
 * 
 * @see nam2num() and num2nam().
 * 
 * It must be remarked that the first allocation does a real allocation, any
 * subsequent call to allocate with the same name from Linux processes just maps
 * the area to the user space, or return the related pointer to the already
 * allocated space in kernel space.  The functions return a pointer to the
 * allocated memory, appropriately mapped to the memory space in use.
 *
 * @returns a valid address on succes, 0 on failure.
 *
 */

void *rt_named_malloc(unsigned long name, int size, int suprt)
{
	void *adr;

	TRACE_RTAI_SHM(TRACE_RTAI_EV_SHM_KMALLOC, name, size, 0);

	if (RT_SHM_OP_PERM(suprt) && (adr = rt_shm_alloc(name, size, suprt))) {
		rt_inc_count(name);
		return adr;
	}
	return 0;
}

#define SMLTMAPS 16
static unsigned long curnam[SMLTMAPS][2];

static int rt_named_usp_malloc_helper(unsigned long name, int size, int suprt)
{
	TRACE_RTAI_SHM(TRACE_RTAI_EV_SHM_MALLOC, name, size, current->pid);

	if (RT_SHM_OP_PERM(suprt) && rt_shm_alloc(name, size, suprt)) {
		int i;
		rt_inc_count(name);
		size = rt_get_type(name);
		for (i = 0; i < SMLTMAPS; i++) {
			if (!cmpxchg(curnam[i], 0, current)) {
				curnam[i][1] = name;
				return abs(size);
			}
		}
		rt_shm_free(name, size);
	}
	printk("***** USP ALLOC OVERFLOW (> %d)*****\n", SMLTMAPS);
	return 0;
}

/**
 * Free a chunk of shared memory being shared inter-intra kernel modules and
 * Linux processes.
 *
 * @internal
 * 
 * rt_named_free is used to free a previously allocated shared memory.
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

int rt_named_free(unsigned long name, void *adr)
{
	int size;
	TRACE_RTAI_SHM(TRACE_RTAI_EV_SHM_KFREE, name, 0, 0);
	if ((size = rt_get_type(name))) {
		return RT_SHM_OP_PERM(size) ? rt_shm_free(name, size) : 0;
	}
	return 0;
}

int rt_named_size(unsigned long name, void *adr)
{
	return abs(rt_get_type(name));
}

static void rtai_shm_vm_open(struct vm_area_struct *vma)
{
	rt_inc_count((unsigned long)vma->vm_private_data);
}

static void rtai_shm_vm_close(struct vm_area_struct *vma)
{
	rt_shm_free((unsigned long)vma->vm_private_data, rt_get_type((unsigned long)vma->vm_private_data));
}

struct vm_operations_struct rtai_shm_vm_ops = {
	open:  	rtai_shm_vm_open,
	close: 	rtai_shm_vm_close
};

static int rtai_shm_f_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int retval;
	switch (cmd) {
		case NAMED_MALLOC: {
			TRACE_RTAI_SHM(TRACE_RTAI_EV_SHM_MALLOC, ((unsigned long *)arg)[0], cmd, current->pid);
			retval = ((int *)arg)[2] ? rt_named_usp_malloc_helper(((unsigned long *)arg)[0], ((int *)arg)[1], ((int *)arg)[2]) : 0;
			break;
		}
		case NAMED_FREE: {
			int size;
			TRACE_RTAI_SHM(TRACE_RTAI_EV_SHM_FREE, arg, cmd, current->pid);
			retval = (size = rt_get_type(arg)) ? rt_shm_free(arg, size) : 0;
			break;
		}
		case NAMED_SIZE: {
			TRACE_RTAI_SHM(TRACE_RTAI_EV_SHM_GET_SIZE, arg, cmd, current->pid);

			retval = abs(rt_get_type(arg));
			break;
		}
		default: {
			retval = 0;
			break;
		}
	}
	return retval;
}

static int rtai_shm_f_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long i, nam;
	for (i = 0; i < SMLTMAPS; i++) {
		nam = curnam[i][1];
		if (cmpxchg(curnam[i], current, 0) == (int)current) {
			if(!vma->vm_ops) {
				vma->vm_ops = &rtai_shm_vm_ops;
				vma->vm_private_data = (void *)nam;
			}		
			return rvmmap(ALIGN2PAGE(rt_get_adr(nam)), abs(rt_get_type(nam)), vma); 
		}
	}
	printk("***** USP MMAP OVERFLOW (> %d)*****\n", SMLTMAPS);
	return -EFAULT;
}

static struct file_operations rtai_shm_fops = {
	ioctl:	rtai_shm_f_ioctl,
	mmap:	rtai_shm_f_mmap
};

static struct miscdevice rtai_shm_dev = 
	{ RTAI_SHM_MISC_MINOR, "RTAI_SHM", &rtai_shm_fops, NULL, NULL };

struct rt_native_fun_entry rt_shm_entries[] = {
        { { 0, rt_named_usp_malloc_helper },	NAMED_MALLOC },
        { { 0, rt_named_free },			NAMED_FREE },
        { { 0, rt_named_size },			NAMED_SIZE },
        { { 0, 0 },				000 }
};

extern int set_rt_fun_entries(struct rt_native_fun_entry *entry);
extern void reset_rt_fun_entries(struct rt_native_fun_entry *entry);

int SHM_INIT_MODULE (void)
{
	if (misc_register(&rtai_shm_dev) < 0) {
		printk("***** COULD NOT REGISTER SHARED MEMORY DEVICE *****\n");
		return -EBUSY;
	}
	return set_rt_fun_entries(rt_shm_entries);
}

void SHM_CLEANUP_MODULE (void)
{
        int slot;
        struct rt_registry_entry_struct entry;
	for (slot = 1; slot <= MAX_SLOTS; slot++) {
		if (rt_get_registry_slot(slot, &entry) && entry.adr) {
			if (abs(entry.type) >= PAGE_SIZE) {
        			char name[8];
				while (rt_shm_free(entry.name, entry.type));
                        	num2nam(entry.name, name);
	                        rt_printk("\nSHM_CLEANUP_MODULE releases: '%s':%lx:%lu (%d).\n", name, entry.name, entry.name, entry.type);
                        }
		}
	}
	reset_rt_fun_entries(rt_shm_entries);
	misc_deregister(&rtai_shm_dev);
	return;
}

/*@}*/
