/**
 *   @ingroup hal
 *   @file
 *
 *   ARTI -- RTAI-compatible Adeos-based Real-Time Interface. Based on
 *   the original RTAI layer for x86.
 *
 *   Original RTAI/x86 layer implementation: \n
 *   Copyright &copy; 2000 Paolo Mantegazza, \n
 *   Copyright &copy; 2000 Steve Papacharalambous, \n
 *   Copyright &copy; 2000 Stuart Hughes, \n
 *   and others.
 *
 *   RTAI/x86 rewrite over Adeos: \n
 *   Copyright &copy 2002 Philippe Gerum.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 675 Mass Ave, Cambridge MA 02139,
 *   USA; either version 2 of the License, or (at your option) any later
 *   version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/**
 * @defgroup hal RTAI services functions.
 *
 * This module defines some functions that can be used by RTAI tasks, for
 * managing interrupts and communication services with Linux processes.
 *
 *@{*/

#include <linux/version.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/console.h>
#include <asm/system.h>
#include <asm/hw_irq.h>
#include <asm/irq.h>
#include <asm/desc.h>
#include <asm/io.h>
#include <asm/mmu_context.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#ifdef CONFIG_X86_LOCAL_APIC
#include <asm/fixmap.h>
#include <asm/bitops.h>
#include <asm/mpspec.h>
#ifdef CONFIG_X86_IO_APIC
#include <asm/io_apic.h>
#endif /* CONFIG_X86_IO_APIC */
#include <asm/apic.h>
#endif /* CONFIG_X86_LOCAL_APIC */
#define __RTAI_HAL__
#include <asm/rtai_hal.h>
#include <asm/rtai_lxrt.h>
#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <rtai_proc_fs.h>
#endif /* CONFIG_PROC_FS */
#include <stdarg.h>

MODULE_LICENSE("GPL");

static unsigned long rtai_cpufreq_arg = RTAI_CALIBRATED_CPU_FREQ;
MODULE_PARM(rtai_cpufreq_arg,"i");

#ifdef CONFIG_X86_LOCAL_APIC
static unsigned long rtai_apicfreq_arg = RTAI_CALIBRATED_APIC_FREQ;

MODULE_PARM(rtai_apicfreq_arg,"i");

static long long rtai_timers_sync_time;

static struct apic_timer_setup_data rtai_timer_mode[RTAI_NR_CPUS];

static inline void rtai_setup_periodic_apic (unsigned count,
					     unsigned vector)
{
    apic_read(APIC_LVTT);
    apic_write(APIC_LVTT,APIC_LVT_TIMER_PERIODIC|vector);
    apic_read(APIC_TMICT);
    apic_write(APIC_TMICT,count);
}

static inline void rtai_setup_oneshot_apic (unsigned count,
					    unsigned vector)
{
    apic_read(APIC_LVTT);
    apic_write(APIC_LVTT,vector);
    apic_read(APIC_TMICT);
    apic_write(APIC_TMICT,count);
}

#else /* !CONFIG_X86_LOCAL_APIC */

#define rtai_setup_periodic_apic(count,vector);

#define rtai_setup_oneshot_apic(count,vector);

#endif /* CONFIG_X86_LOCAL_APIC */

#ifdef CONFIG_SMP
static unsigned long rtai_old_irq_affinity[NR_IRQS],
                     rtai_set_irq_affinity[NR_IRQS];

static spinlock_t rtai_iset_lock = SPIN_LOCK_UNLOCKED;
#endif /* CONFIG_SMP */

#ifdef CONFIG_RTAI_SCHED_ISR_LOCK
static int rtai_isr_nesting[RTAI_NR_CPUS];
static void (*rtai_isr_hook)(int nesting);
#endif /* CONFIG_RTAI_SCHED_ISR_LOCK */

extern struct desc_struct idt_table[];

adomain_t rtai_domain;

static struct {

    void (*handler)(unsigned irq, void *cookie);
    void *cookie;

} rtai_realtime_irq[NR_IRQS];

static struct {

    unsigned long flags;
    int count;

} rtai_linux_irq[NR_IRQS];

static struct {

    void (*k_handler)(void);
    long long (*u_handler)(unsigned);
    unsigned label;

} rtai_sysreq_table[RTAI_NR_SRQS];

static unsigned rtai_sysreq_virq;

static unsigned long rtai_sysreq_map = 3; /* srqs #[0-1] are reserved */

static unsigned long rtai_sysreq_pending;

static unsigned long rtai_sysreq_running;

static spinlock_t rtai_ssrq_lock = SPIN_LOCK_UNLOCKED;

static volatile int rtai_sync_level;

static atomic_t rtai_sync_count = ATOMIC_INIT(1);

static int rtai_last_8254_counter2;

static RTIME rtai_ts_8254;

static struct desc_struct rtai_sysvec;

static RT_TRAP_HANDLER rtai_trap_handler;

static void (*rtai_exit_callback)(void);

struct rt_times rt_times;

struct rt_times rt_smp_times[RTAI_NR_CPUS];

struct rtai_switch_data rtai_linux_context[RTAI_NR_CPUS];

struct calibration_data rtai_tunables;

volatile unsigned long rtai_status ;

volatile unsigned long rtai_cpu_realtime;

volatile unsigned long rtai_cpu_lock;

int rtai_adeos_ptdbase = -1;

unsigned long rtai_critical_enter (void (*synch)(void))

{
    unsigned long flags = adeos_critical_enter(synch);

    if (atomic_dec_and_test(&rtai_sync_count))
	rtai_sync_level = 0;
    else if (synch != NULL)
	printk(KERN_INFO "RTAI[hal]: warning: nested sync will fail.\n");

    return flags;
}

void rtai_critical_exit (unsigned long flags)

{
    atomic_inc(&rtai_sync_count);
    adeos_critical_exit(flags);
}

int rt_request_irq (unsigned irq,
		    void (*handler)(unsigned irq, void *cookie),
		    void *cookie)
{
    unsigned long flags;

    if (handler == NULL || irq >= NR_IRQS)
	return -EINVAL;

    flags = rtai_critical_enter(NULL);

    if (rtai_realtime_irq[irq].handler != NULL)
	{
	rtai_critical_exit(flags);
	return -EBUSY;
	}

    rtai_realtime_irq[irq].handler = handler;
    rtai_realtime_irq[irq].cookie = cookie;

    rtai_critical_exit(flags);

    return 0;
}

int rt_release_irq (unsigned irq)

{
    if (irq >= NR_IRQS)
	return -EINVAL;

    xchg(&rtai_realtime_irq[irq].handler,NULL);

    return 0;
}

void rt_set_irq_cookie (unsigned irq, void *cookie) {

    if (irq < NR_IRQS)
	rtai_realtime_irq[irq].cookie = cookie;
}

/* Note: Adeos already does all the magic that allows the calling the
   interrupt controller routines safely. */

/**
 * start and initialize the PIC to accept interrupt request irq.
 *
 * The above function allow you to manipulate the PIC at hand, but you must
 * know what you are doing. Such a duty does not pertain to this manual and
 * you should refer to your PIC datasheet.
 *
 * Note that Linux has the same functions, but they must be used only for its
 * interrupts. Only the above ones can be safely used in real time handlers.
 *
 * It must also be remarked that when you install a real time interrupt handler,
 * RTAI already calls either rt_mask_and_ack_irq(), for level triggered
 * interrupts, or rt_ack_irq(), for edge triggered interrupts, before passing
 * control to you interrupt handler. hus generally you should just call
 * rt_unmask_irq() at due time, for level triggered interrupts, while nothing
 * should be done for edge triggered ones. Recall that in the latter case you
 * allow also any new interrupts on the same request as soon as you enable
 * interrupts at the CPU level.
 * 
 * Often some of the above functions do equivalent things. Once more there is no
 * way of doing it right except by knowing the hardware you are manipulating.
 * Furthermore you must also remember that when you install a hard real time
 * handler the related interrupt is usually disabled, unless you are overtaking
 * one already owned by Linux which has been enabled by it.   Recall that if
 * have done it right, and interrupts do not show up, it is likely you have just
 * to rt_enable_irq() your irq.
 */
unsigned rt_startup_irq (unsigned irq) {

    return irq_desc[irq].handler->startup(irq);
}

/**
 * Shut down an IRQ source.
 *
 * No further interrupt request irq can be accepted.
 *
 * The above function allow you to manipulate the PIC at hand, but you must
 * know what you are doing. Such a duty does not pertain to this manual and
 * you should refer to your PIC datasheet.
 *
 * Note that Linux has the same functions, but they must be used only for its
 * interrupts. Only the above ones can be safely used in real time handlers.
 *
 * It must also be remarked that when you install a real time interrupt handler,
 * RTAI already calls either rt_mask_and_ack_irq(), for level triggered
 * interrupts, or rt_ack_irq(), for edge triggered interrupts, before passing
 * control to you interrupt handler. hus generally you should just call
 * rt_unmask_irq() at due time, for level triggered interrupts, while nothing
 * should be done for edge triggered ones. Recall that in the latter case you
 * allow also any new interrupts on the same request as soon as you enable
 * interrupts at the CPU level.
 * 
 * Often some of the above functions do equivalent things. Once more there is no
 * way of doing it right except by knowing the hardware you are manipulating.
 * Furthermore you must also remember that when you install a hard real time
 * handler the related interrupt is usually disabled, unless you are overtaking
 * one already owned by Linux which has been enabled by it.   Recall that if
 * have done it right, and interrupts do not show up, it is likely you have just
 * to rt_enable_irq() your irq.
 */
void rt_shutdown_irq (unsigned irq) {

    irq_desc[irq].handler->shutdown(irq);
}

/**
 * Enable an IRQ source.
 *
 * The above function allow you to manipulate the PIC at hand, but you must
 * know what you are doing. Such a duty does not pertain to this manual and
 * you should refer to your PIC datasheet.
 *
 * Note that Linux has the same functions, but they must be used only for its
 * interrupts. Only the above ones can be safely used in real time handlers.
 *
 * It must also be remarked that when you install a real time interrupt handler,
 * RTAI already calls either rt_mask_and_ack_irq(), for level triggered
 * interrupts, or rt_ack_irq(), for edge triggered interrupts, before passing
 * control to you interrupt handler. hus generally you should just call
 * rt_unmask_irq() at due time, for level triggered interrupts, while nothing
 * should be done for edge triggered ones. Recall that in the latter case you
 * allow also any new interrupts on the same request as soon as you enable
 * interrupts at the CPU level.
 * 
 * Often some of the above functions do equivalent things. Once more there is no
 * way of doing it right except by knowing the hardware you are manipulating.
 * Furthermore you must also remember that when you install a hard real time
 * handler the related interrupt is usually disabled, unless you are overtaking
 * one already owned by Linux which has been enabled by it.   Recall that if
 * have done it right, and interrupts do not show up, it is likely you have just
 * to rt_enable_irq() your irq.
 */
void rt_enable_irq (unsigned irq) {

    irq_desc[irq].handler->enable(irq);
}

/**
 * Disable an IRQ source.
 *
 * The above function allow you to manipulate the PIC at hand, but you must
 * know what you are doing. Such a duty does not pertain to this manual and
 * you should refer to your PIC datasheet.
 *
 * Note that Linux has the same functions, but they must be used only for its
 * interrupts. Only the above ones can be safely used in real time handlers.
 *
 * It must also be remarked that when you install a real time interrupt handler,
 * RTAI already calls either rt_mask_and_ack_irq(), for level triggered
 * interrupts, or rt_ack_irq(), for edge triggered interrupts, before passing
 * control to you interrupt handler. hus generally you should just call
 * rt_unmask_irq() at due time, for level triggered interrupts, while nothing
 * should be done for edge triggered ones. Recall that in the latter case you
 * allow also any new interrupts on the same request as soon as you enable
 * interrupts at the CPU level.
 * 
 * Often some of the above functions do equivalent things. Once more there is no
 * way of doing it right except by knowing the hardware you are manipulating.
 * Furthermore you must also remember that when you install a hard real time
 * handler the related interrupt is usually disabled, unless you are overtaking
 * one already owned by Linux which has been enabled by it.   Recall that if
 * have done it right, and interrupts do not show up, it is likely you have just
 * to rt_enable_irq() your irq.
 */
void rt_disable_irq (unsigned irq) {

    irq_desc[irq].handler->disable(irq);
}

/**
 * Mask and acknowledge and IRQ source.
 *
 * No  * other interrupts can be accepted, once also the CPU will enable
 * interrupts, which ones depends on the PIC at hand and on how it is
 * programmed.
 *
 * The above function allow you to manipulate the PIC at hand, but you must
 * know what you are doing. Such a duty does not pertain to this manual and
 * you should refer to your PIC datasheet.
 *
 * Note that Linux has the same functions, but they must be used only for its
 * interrupts. Only the above ones can be safely used in real time handlers.
 *
 * It must also be remarked that when you install a real time interrupt handler,
 * RTAI already calls either rt_mask_and_ack_irq(), for level triggered
 * interrupts, or rt_ack_irq(), for edge triggered interrupts, before passing
 * control to you interrupt handler. hus generally you should just call
 * rt_unmask_irq() at due time, for level triggered interrupts, while nothing
 * should be done for edge triggered ones. Recall that in the latter case you
 * allow also any new interrupts on the same request as soon as you enable
 * interrupts at the CPU level.
 * 
 * Often some of the above functions do equivalent things. Once more there is no
 * way of doing it right except by knowing the hardware you are manipulating.
 * Furthermore you must also remember that when you install a hard real time
 * handler the related interrupt is usually disabled, unless you are overtaking
 * one already owned by Linux which has been enabled by it.   Recall that if
 * have done it right, and interrupts do not show up, it is likely you have just
 * to rt_enable_irq() your irq.
 */
void rt_mask_and_ack_irq (unsigned irq) {

    irq_desc[irq].handler->ack(irq);
}

/**
 * Unmask and IRQ source.
 *
 * The related request can then interrupt the CPU again, provided it has also
 * been acknowledged.
 *
 * The above function allow you to manipulate the PIC at hand, but you must
 * know what you are doing. Such a duty does not pertain to this manual and
 * you should refer to your PIC datasheet.
 *
 * Note that Linux has the same functions, but they must be used only for its
 * interrupts. Only the above ones can be safely used in real time handlers.
 *
 * It must also be remarked that when you install a real time interrupt handler,
 * RTAI already calls either rt_mask_and_ack_irq(), for level triggered
 * interrupts, or rt_ack_irq(), for edge triggered interrupts, before passing
 * control to you interrupt handler. hus generally you should just call
 * rt_unmask_irq() at due time, for level triggered interrupts, while nothing
 * should be done for edge triggered ones. Recall that in the latter case you
 * allow also any new interrupts on the same request as soon as you enable
 * interrupts at the CPU level.
 * 
 * Often some of the above functions do equivalent things. Once more there is no
 * way of doing it right except by knowing the hardware you are manipulating.
 * Furthermore you must also remember that when you install a hard real time
 * handler the related interrupt is usually disabled, unless you are overtaking
 * one already owned by Linux which has been enabled by it.   Recall that if
 * have done it right, and interrupts do not show up, it is likely you have just
 * to rt_enable_irq() your irq.
 */
void rt_unmask_irq (unsigned irq) {

    irq_desc[irq].handler->end(irq);
}

/**
 * Acknowledge an IRQ source.
 *
 * The related request can then interrupt the CPU again, provided it has not
 * been masked.
 *
 * The above function allow you to manipulate the PIC at hand, but you must
 * know what you are doing. Such a duty does not pertain to this manual and
 * you should refer to your PIC datasheet.
 *
 * Note that Linux has the same functions, but they must be used only for its
 * interrupts. Only the above ones can be safely used in real time handlers.
 *
 * It must also be remarked that when you install a real time interrupt handler,
 * RTAI already calls either rt_mask_and_ack_irq(), for level triggered
 * interrupts, or rt_ack_irq(), for edge triggered interrupts, before passing
 * control to you interrupt handler. hus generally you should just call
 * rt_unmask_irq() at due time, for level triggered interrupts, while nothing
 * should be done for edge triggered ones. Recall that in the latter case you
 * allow also any new interrupts on the same request as soon as you enable
 * interrupts at the CPU level.
 * 
 * Often some of the above functions do equivalent things. Once more there is no
 * way of doing it right except by knowing the hardware you are manipulating.
 * Furthermore you must also remember that when you install a hard real time
 * handler the related interrupt is usually disabled, unless you are overtaking
 * one already owned by Linux which has been enabled by it.   Recall that if
 * have done it right, and interrupts do not show up, it is likely you have just
 * to rt_enable_irq() your irq.
 */
void rt_ack_irq (unsigned irq) {

    rt_enable_irq(irq);
}

void rt_do_irq (unsigned irq) {

    adeos_trigger_irq(irq);
}

/**
 * Install shared Linux interrupt handler.
 *
 * rt_request_linux_irq installs function @a handler as a standard Linux
 * interrupt service routine for IRQ level @a irq forcing Linux to share the IRQ
 * with other interrupt handlers, even if it does not want. The handler is
 * appended to any already existing Linux handler for the same irq and is run by
 * Linux irq as any of its handler. In this way a real time application can
 * monitor Linux interrupts handling at its will. The handler appears in
 * /proc/interrupts.
 *
 * @param handler pointer on the interrupt service routine to be installed.
 *
 * @param name is a name for /proc/interrupts.
 *
 * @param dev_id is to pass to the interrupt handler, in the same way as the
 * standard Linux irq request call.
 *
 * The interrupt service routine can be uninstalled with rt_free_linux_irq().
 *
 * @retval 0 on success.
 * @retval EINVAL if @a irq is not a valid IRQ number or handler is @c NULL.
 * @retval EBUSY if there is already a handler of interrupt @a irq.
 */
int rt_request_linux_irq (unsigned irq,
			  irqreturn_t (*handler)(int irq,
						 void *dev_id,
						 struct pt_regs *regs), 
			  char *name,
			  void *dev_id)
{
    unsigned long flags;

    if (irq >= NR_IRQS || !handler)
	return -EINVAL;

    rtai_local_irq_save(flags);

    spin_lock(&irq_desc[irq].lock);

    if (rtai_linux_irq[irq].count++ == 0 && irq_desc[irq].action)
	{
	rtai_linux_irq[irq].flags = irq_desc[irq].action->flags;
	irq_desc[irq].action->flags |= SA_SHIRQ;
	}

    spin_unlock(&irq_desc[irq].lock);

    rtai_local_irq_restore(flags);

    request_irq(irq,handler,SA_SHIRQ,name,dev_id);

    return 0;
}

/**
 * Uninstall shared Linux interrupt handler.
 *
 * @param dev_id is to pass to the interrupt handler, in the same way as the
 * standard Linux irq request call.
 *
 * @param irq is the IRQ level of the interrupt handler to be freed.
 *
 * @retval 0 on success.
 * @retval EINVAL if @a irq is not a valid IRQ number.
 */
int rt_free_linux_irq (unsigned irq, void *dev_id)

{
    unsigned long flags;

    if (irq >= NR_IRQS || rtai_linux_irq[irq].count == 0)
	return -EINVAL;

    rtai_local_irq_save(flags);

    free_irq(irq,dev_id);

    spin_lock(&irq_desc[irq].lock);

    if (--rtai_linux_irq[irq].count == 0 && irq_desc[irq].action)
	irq_desc[irq].action->flags = rtai_linux_irq[irq].flags;

    spin_unlock(&irq_desc[irq].lock);

    rtai_local_irq_restore(flags);

    return 0;
}

/**
 * Pend an IRQ to Linux.
 *
 * rt_pend_linux_irq appends a Linux interrupt irq for processing in Linux IRQ
 * mode, i.e. with hardware interrupts fully enabled.
 *
 * @note rt_pend_linux_irq does not perform any check on @a irq.
 */
void rt_pend_linux_irq (unsigned irq) {

    adeos_propagate_irq(irq);
}

/**
 * Install a system request handler
 *
 * rt_request_srq installs a two way RTAI system request (srq) by assigning
 * @a u_handler, a function to be used when a user calls srq from user space,
 * and @a k_handler, the function to be called in kernel space following its
 * activation by a call to rt_pend_linux_srq(). @a k_handler is in practice
 * used to request a service from the kernel. In fact Linux system requests
 * cannot be used safely from RTAI so you can setup a handler that receives real
 * time requests and safely executes them when Linux is running.
 *
 * @param u_handler can be used to effectively enter kernel space without the
 * overhead and clumsiness of standard Unix/Linux protocols.   This is very
 * flexible service that allows you to personalize your use of  RTAI.
 *
 * @return the number of the assigned system request on success.
 * @retval EINVAL if @a k_handler is @c NULL.
 * @retval EBUSY if no free srq slot is available.
 */
int rt_request_srq (unsigned label,
		    void (*k_handler)(void),
		    long long (*u_handler)(unsigned))
{
    unsigned long flags;
    int srq;

    if (k_handler == NULL)
	return -EINVAL;

    rtai_local_irq_save(flags);

    if (rtai_sysreq_map != ~0)
	{
	srq = ffz(rtai_sysreq_map);
	set_bit(srq,&rtai_sysreq_map);
	rtai_sysreq_table[srq].k_handler = k_handler;
	rtai_sysreq_table[srq].u_handler = u_handler;
	rtai_sysreq_table[srq].label = label;
	}
    else
	srq = -EBUSY;

    rtai_local_irq_restore(flags);

    return srq;
}

/**
 * Uninstall a system request handler
 *
 * rt_free_srq uninstalls the specified system call @a srq, returned by
 * installing the related handler with a previous call to rt_request_srq().
 *
 * @retval EINVAL if @a srq is invalid.
 */
int rt_free_srq (unsigned srq)

{
    if (srq < 2 || srq >= RTAI_NR_SRQS ||
	!test_and_clear_bit(srq,&rtai_sysreq_map))
	return -EINVAL;

    return 0;
}

/**
 * Append a Linux IRQ.
 *
 * rt_pend_linux_srq appends a system call request srq to be used as a service
 * request to the Linux kernel.
 *
 * @param srq is the value returned by rt_request_srq.
 *
 * @note rt_pend_linux_srq does not perform any check on irq.
 */
void rt_pend_linux_srq (unsigned srq)

{
    if (srq > 1 && srq < RTAI_NR_SRQS)
	{
	set_bit(srq,&rtai_sysreq_pending);
	adeos_schedule_irq(rtai_sysreq_virq);
	}
}

#ifdef CONFIG_SMP

static void rtai_critical_sync (void)

{
    struct apic_timer_setup_data *p;

    switch (rtai_sync_level)
	{
	case 1:

	    p = &rtai_timer_mode[adeos_processor_id()];
	    
	    while (rtai_rdtsc() < rtai_timers_sync_time)
		;

	    if (p->mode)
		rtai_setup_periodic_apic(p->count,RTAI_APIC_TIMER_VECTOR);
	    else
		rtai_setup_oneshot_apic(p->count,RTAI_APIC_TIMER_VECTOR);

	    break;

	case 2:

	    rtai_setup_oneshot_apic(0,RTAI_APIC_TIMER_VECTOR);
	    break;

	case 3:

	    rtai_setup_periodic_apic(RTAI_APIC_ICOUNT,LOCAL_TIMER_VECTOR);
	    break;
	}
}

irqreturn_t rtai_broadcast_to_local_timers (int irq,
					    void *dev_id,
					    struct pt_regs *regs)
{
    unsigned long flags;

    rtai_hw_lock(flags);
    apic_wait_icr_idle();
    apic_write_around(APIC_ICR,APIC_DM_FIXED|APIC_DEST_ALLINC|LOCAL_TIMER_VECTOR);
    rtai_hw_unlock(flags);

    return RTAI_LINUX_IRQ_HANDLED;
} 

#else /* !CONFIG_SMP */

#define rtai_critical_sync NULL

irqreturn_t rtai_broadcast_to_local_timers (int irq,
					    void *dev_id,
					    struct pt_regs *regs) {
    return RTAI_LINUX_IRQ_HANDLED;
} 

#endif /* CONFIG_SMP */

#ifdef CONFIG_X86_LOCAL_APIC

/**
 * Install a local APICs timer interrupt handler
 *
 * rt_request_apic_timers requests local APICs timers and defines the mode and
 * count to be used for each local APIC timer. Modes and counts can be chosen
 * arbitrarily for each local APIC timer.
 *
 * @param apic_timer_data is a pointer to a vector of structures
 * @code struct apic_timer_setup_data { int mode, count; }
 * @endcode sized with the number of CPUs available.
 *
 * Such a structure defines:
 * - mode: 0 for a oneshot timing, 1 for a periodic timing.
 * - count: is the period in nanoseconds you want to use on the corresponding
 * timer, not used for oneshot timers.  It is in nanoseconds to ease its
 * programming when different values are used by each timer, so that you do not
 * have to care converting it from the CPU on which you are calling this
 * function.
 *
 * The start of the timing should be reasonably synchronized.   You should call
 * this function with due care and only when you want to manage the related
 * interrupts in your own handler.   For using local APIC timers in pacing real
 * time tasks use the usual rt_start_timer(), which under the MUP scheduler sets
 * the same timer policy on all the local APIC timers, or start_rt_apic_timers()
 * that allows you to use @c struct @c apic_timer_setup_data directly.
 */
void rt_request_apic_timers (void (*handler)(void),
			     struct apic_timer_setup_data *tmdata)
{
    volatile struct rt_times *rtimes;
    struct apic_timer_setup_data *p;
    unsigned long flags;
    int cpuid;

    TRACE_RTAI_TIMER(TRACE_RTAI_EV_TIMER_REQUEST_APIC,handler,0);

    flags = rtai_critical_enter(rtai_critical_sync);

    rtai_sync_level = 1;

    rtai_timers_sync_time = rtai_rdtsc() + rtai_imuldiv(LATCH,
							rtai_tunables.cpu_freq,
							RTAI_FREQ_8254);
    for (cpuid = 0; cpuid < RTAI_NR_CPUS; cpuid++)
	{
	p = &rtai_timer_mode[cpuid];
	*p = tmdata[cpuid];
	rtimes = &rt_smp_times[cpuid];

	if (p->mode)
	    {
	    rtimes->linux_tick = RTAI_APIC_ICOUNT;
	    rtimes->tick_time = rtai_llimd(rtai_timers_sync_time,
					   RTAI_FREQ_APIC,
					   rtai_tunables.cpu_freq);
	    rtimes->periodic_tick = rtai_imuldiv(p->count,
						 RTAI_FREQ_APIC,
						 1000000000);
	    p->count = rtimes->periodic_tick;
	    }
	else
	    {
	    rtimes->linux_tick = rtai_imuldiv(LATCH,
					      rtai_tunables.cpu_freq,
					      RTAI_FREQ_8254);
	    rtimes->tick_time = rtai_timers_sync_time;
	    rtimes->periodic_tick = rtimes->linux_tick;
	    p->count = RTAI_APIC_ICOUNT;
	    }

	rtimes->intr_time = rtimes->tick_time + rtimes->periodic_tick;
	rtimes->linux_time = rtimes->tick_time + rtimes->linux_tick;
	}

    p = &rtai_timer_mode[adeos_processor_id()];

    while (rtai_rdtsc() < rtai_timers_sync_time)
	;

    if (p->mode)
	rtai_setup_periodic_apic(p->count,RTAI_APIC_TIMER_VECTOR);
    else
	rtai_setup_oneshot_apic(p->count,RTAI_APIC_TIMER_VECTOR);

    rt_release_irq(RTAI_APIC_TIMER_IPI);

    rt_request_irq(RTAI_APIC_TIMER_IPI,(rt_irq_handler_t)handler,NULL);

    rt_request_linux_irq(RTAI_TIMER_8254_IRQ,
			 &rtai_broadcast_to_local_timers,
			 "broadcast",
			 &rtai_broadcast_to_local_timers);

    for (cpuid = 0; cpuid < RTAI_NR_CPUS; cpuid++)
	{
	p = &tmdata[cpuid];

	if (p->mode)
	    p->count = rtai_imuldiv(p->count,RTAI_FREQ_APIC,1000000000);
	else
	    p->count = rtai_imuldiv(p->count,rtai_tunables.cpu_freq,1000000000);
	}

    rtai_critical_exit(flags);
}

/**
 * Uninstall a local APICs timer interrupt handler
 */
void rt_free_apic_timers(void)

{
    unsigned long flags;

    TRACE_RTAI_TIMER(TRACE_RTAI_EV_TIMER_APIC_FREE,0,0);

    rt_free_linux_irq(RTAI_TIMER_8254_IRQ,&rtai_broadcast_to_local_timers);

    flags = rtai_critical_enter(rtai_critical_sync);

    rtai_sync_level = 3;
    rtai_setup_periodic_apic(RTAI_APIC_ICOUNT,LOCAL_TIMER_VECTOR);
    rt_release_irq(RTAI_APIC_TIMER_IPI);

    rtai_critical_exit(flags);
}

#else /* !CONFIG_X86_LOCAL_APIC */

void rt_request_apic_timers (void (*handler)(void),
			     struct apic_timer_setup_data *tmdata) {
}

void rt_free_apic_timers(void) {
    rt_free_timer();
}

#endif /* CONFIG_X86_LOCAL_APIC */

#ifdef CONFIG_SMP

/**
 * Set IRQ->CPU assignment
 *
 * rt_assign_irq_to_cpu forces the assignment of the external interrupt @a irq
 * to the CPU @a cpu.
 *
 * @retval 1 if there is one CPU in the system.
 * @retval 0 on success if there are at least 2 CPUs.
 * @return the number of CPUs if @a cpu refers to a non-existent CPU.
 * @retval EINVAL if @a irq is not a valid IRQ number or some internal data
 * inconsistency is found.
 *
 * @note This functions has effect only on multiprocessors systems.
 * @note With Linux 2.4.xx such a service has finally been made available
 * natively within the raw kernel. With such Linux releases
 * rt_reset_irq_to_sym_mode() resets the original Linux delivery mode, or
 * deliver affinity as they call it. So be warned that such a name is kept
 * mainly for compatibility reasons, as for such a kernel the reset operation
 * does not necessarily implies a symmetric external interrupt delivery.
 */
int rt_assign_irq_to_cpu (int irq, unsigned long cpumask)

{
    unsigned long oldmask, flags;

    rtai_local_irq_save(flags);

    spin_lock(&rtai_iset_lock);

    oldmask = CPUMASK(adeos_set_irq_affinity(irq, CPUMASK_T(cpumask)));

    if (oldmask == 0)
	{
	/* Oops... Something went wrong. */
	spin_unlock(&rtai_iset_lock);
	rtai_local_irq_restore(flags);
	return -EINVAL;
	}

    rtai_old_irq_affinity[irq] = oldmask;
    rtai_set_irq_affinity[irq] = cpumask;

    spin_unlock(&rtai_iset_lock);

    rtai_local_irq_restore(flags);

    return 0;
}

/**
 * reset IRQ->CPU assignment
 *
 * rt_reset_irq_to_sym_mode resets the interrupt irq to the symmetric interrupts
 * management. The symmetric mode distributes the IRQs over all the CPUs.
 *
 * @retval 1 if there is one CPU in the system.
 * @retval 0 on success if there are at least 2 CPUs.
 * @return the number of CPUs if @a cpu refers to a non-existent CPU.
 * @retval EINVAL if @a irq is not a valid IRQ number or some internal data
 * inconsistency is found.
 *
 * @note This function has effect only on multiprocessors systems.
 * @note With Linux 2.4.xx such a service has finally been made available
 * natively within the raw kernel. With such Linux releases
 * rt_reset_irq_to_sym_mode() resets the original Linux delivery mode, or
 * deliver affinity as they call it. So be warned that such a name is kept
 * mainly for compatibility reasons, as for such a kernel the reset operation
 * does not necessarily implies a symmetric external interrupt delivery.
 */
int rt_reset_irq_to_sym_mode (int irq)

{
    unsigned long oldmask, flags;

    rtai_local_irq_save(flags);

    spin_lock(&rtai_iset_lock);

    if (rtai_old_irq_affinity[irq] == 0)
	{
	spin_unlock(&rtai_iset_lock);
	rtai_local_irq_restore(flags);
	return -EINVAL;
	}

    oldmask = CPUMASK(adeos_set_irq_affinity(irq, CPUMASK_T(0))); /* Query -- no change. */

    if (oldmask == rtai_set_irq_affinity[irq])
	{
	/* Ok, proceed since nobody changed it in the meantime. */
	adeos_set_irq_affinity(irq, CPUMASK_T(rtai_old_irq_affinity[irq]));
	rtai_old_irq_affinity[irq] = 0;
	}

    spin_unlock(&rtai_iset_lock);

    rtai_local_irq_restore(flags);

    return 0;
}

void rt_request_timer_cpuid (void (*handler)(void),
			     unsigned tick,
			     int cpuid)
{
    unsigned long flags;
    int count;

    set_bit(RTAI_USE_APIC,&rtai_status);
    rtai_timers_sync_time = 0;

    for (count = 0; count < RTAI_NR_CPUS; count++)
	rtai_timer_mode[count].mode = rtai_timer_mode[count].count = 0;

    flags = rtai_critical_enter(rtai_critical_sync);

    rtai_sync_level = 1;

    rt_times.tick_time = rtai_rdtsc();

    if (tick > 0)
	{
	rt_times.linux_tick = RTAI_APIC_ICOUNT;
	rt_times.tick_time = ((RTIME)rt_times.linux_tick)*(jiffies + 1);
	rt_times.intr_time = rt_times.tick_time + tick;
	rt_times.linux_time = rt_times.tick_time + rt_times.linux_tick;
	rt_times.periodic_tick = tick;

	if (cpuid == adeos_processor_id())
	    rtai_setup_periodic_apic(tick,RTAI_APIC_TIMER_VECTOR);
	else
	    {
	    rtai_timer_mode[cpuid].mode = 1;
	    rtai_timer_mode[cpuid].count = tick;
	    rtai_setup_oneshot_apic(0,RTAI_APIC_TIMER_VECTOR);
	    }
	}
    else
	{
	rt_times.linux_tick = rtai_imuldiv(LATCH, rtai_tunables.cpu_freq,RTAI_FREQ_8254);
	rt_times.intr_time = rt_times.tick_time + rt_times.linux_tick;
	rt_times.linux_time = rt_times.tick_time + rt_times.linux_tick;
	rt_times.periodic_tick = rt_times.linux_tick;

	if (cpuid == adeos_processor_id())
	    rtai_setup_oneshot_apic(RTAI_APIC_ICOUNT,RTAI_APIC_TIMER_VECTOR);
	else
	    {
	    rtai_timer_mode[cpuid].mode = 0;
	    rtai_timer_mode[cpuid].count = RTAI_APIC_ICOUNT;
	    rtai_setup_oneshot_apic(0,RTAI_APIC_TIMER_VECTOR);
	    }
	}

    rt_release_irq(RTAI_APIC_TIMER_IPI);

    rt_request_irq(RTAI_APIC_TIMER_IPI,(rt_irq_handler_t)handler,NULL);

    rt_request_linux_irq(RTAI_TIMER_8254_IRQ,
			 &rtai_broadcast_to_local_timers,
			 "broadcast",
			 &rtai_broadcast_to_local_timers);

    rtai_critical_exit(flags);
}

#else  /* !CONFIG_SMP */

int rt_assign_irq_to_cpu (int irq, unsigned long cpus_mask) {

    return 0;
}

int rt_reset_irq_to_sym_mode (int irq) {

    return 0;
}

void rt_request_timer_cpuid (void (*handler)(void),
			     unsigned tick,
			     int cpuid) {
}

#endif /* CONFIG_SMP */

/**
 * Install a timer interrupt handler.
 *
 * rt_request_timer requests a timer of period tick ticks, and installs the
 * routine @a handler as a real time interrupt service routine for the timer.
 *
 * Set @a tick to 0 for oneshot mode (in oneshot mode it is not used).
 * If @a apic has a nonzero value the local APIC timer is used.   Otherwise
 * timing is based on the 8254.
 *
 */
int rt_request_timer (void (*handler)(void),
		      unsigned tick,
		      int use_apic)
{
    unsigned long flags;

    TRACE_RTAI_TIMER(TRACE_RTAI_EV_TIMER_REQUEST,handler,tick);

    if (use_apic)
	set_bit(RTAI_USE_APIC,&rtai_status);
    else
	clear_bit(RTAI_USE_APIC,&rtai_status);

    flags = rtai_critical_enter(rtai_critical_sync);

    rt_times.tick_time = rtai_rdtsc();

    if (tick > 0)
	{
	rt_times.linux_tick = use_apic ? RTAI_APIC_ICOUNT : LATCH;
	rt_times.tick_time = ((RTIME)rt_times.linux_tick)*(jiffies + 1);
	rt_times.intr_time = rt_times.tick_time + tick;
	rt_times.linux_time = rt_times.tick_time + rt_times.linux_tick;
	rt_times.periodic_tick = tick;

	if (use_apic)
	    {
	    rtai_sync_level = 2;
	    rt_release_irq(RTAI_APIC_TIMER_IPI);
	    rt_request_irq(RTAI_APIC_TIMER_IPI,(rt_irq_handler_t)handler,NULL);
	    rtai_setup_periodic_apic(tick,RTAI_APIC_TIMER_VECTOR);
	    }
	else
	    {
	    outb(0x34,0x43);
	    outb(tick & 0xff,0x40);
	    outb(tick >> 8,0x40);

	    rt_release_irq(RTAI_TIMER_8254_IRQ);

	    if (rt_request_irq(RTAI_TIMER_8254_IRQ,(rt_irq_handler_t)handler,NULL) < 0)
		{
		rtai_critical_exit(flags);
		return -EINVAL;
		}
	    }
	}
    else
	{
	rt_times.linux_tick = rtai_imuldiv(LATCH,rtai_tunables.cpu_freq,RTAI_FREQ_8254);
	rt_times.intr_time = rt_times.tick_time + rt_times.linux_tick;
	rt_times.linux_time = rt_times.tick_time + rt_times.linux_tick;
	rt_times.periodic_tick = rt_times.linux_tick;

	if (use_apic)
	    {
	    rtai_sync_level = 2;
	    rt_release_irq(RTAI_APIC_TIMER_IPI);
	    rt_request_irq(RTAI_APIC_TIMER_IPI,(rt_irq_handler_t)handler,NULL);
	    rtai_setup_oneshot_apic(RTAI_APIC_ICOUNT,RTAI_APIC_TIMER_VECTOR);
	    }
	else
	    {
	    outb(0x30,0x43);
	    outb(LATCH & 0xff,0x40);
	    outb(LATCH >> 8,0x40);

	    rt_release_irq(RTAI_TIMER_8254_IRQ);

	    if (rt_request_irq(RTAI_TIMER_8254_IRQ,(rt_irq_handler_t)handler,NULL) < 0)
		{
		rtai_critical_exit(flags);
		return -EINVAL;
		}
	    }
	}

    rtai_critical_exit(flags);

    return use_apic ? rt_request_linux_irq(RTAI_TIMER_8254_IRQ,
					   &rtai_broadcast_to_local_timers,
					   "rtai_broadcast",
					   &rtai_broadcast_to_local_timers) : 0;
}

/**
 * Uninstall a timer interrupt handler.
 *
 * rt_free_timer uninstalls a timer previously set by rt_request_timer().
 */
void rt_free_timer (void)

{
    unsigned long flags;

    TRACE_RTAI_TIMER(TRACE_RTAI_EV_TIMER_FREE,0,0);

    if (test_bit(RTAI_USE_APIC,&rtai_status))
	rt_free_linux_irq(RTAI_TIMER_8254_IRQ,
			  &rtai_broadcast_to_local_timers);

    flags = rtai_critical_enter(rtai_critical_sync);

    if (test_bit(RTAI_USE_APIC,&rtai_status))
	{
	rtai_sync_level = 3;
	rtai_setup_periodic_apic(RTAI_APIC_ICOUNT,LOCAL_TIMER_VECTOR);
	clear_bit(RTAI_USE_APIC,&rtai_status);
	}
    else
	{
	outb(0x34,0x43);
	outb(LATCH & 0xff,0x40);
	outb(LATCH >> 8,0x40);
	rt_release_irq(RTAI_TIMER_8254_IRQ);
	}

    rtai_critical_exit(flags);
}

RT_TRAP_HANDLER rt_set_trap_handler (RT_TRAP_HANDLER handler) {

    return (RT_TRAP_HANDLER)xchg(&rtai_trap_handler,handler);
}

RTIME rd_8254_ts (void)

{
    unsigned long flags;
    int inc, c2;
    RTIME t;

    adeos_hw_local_irq_save(flags);	/* local hw masking is
					   required here. */
    outb(0xD8,0x43);
    c2 = inb(0x42);
    inc = rtai_last_8254_counter2 - (c2 |= (inb(0x42) << 8));
    rtai_last_8254_counter2 = c2;
    t = (rtai_ts_8254 += (inc > 0 ? inc : inc + RTAI_COUNTER_2_LATCH));

    adeos_hw_local_irq_restore(flags);

    return t;
}

void rt_setup_8254_tsc (void)

{
    unsigned long flags;
    int c;

    flags = rtai_critical_enter(NULL);

    outb_p(0x00,0x43);
    c = inb_p(0x40);
    c |= inb_p(0x40) << 8;
    outb_p(0xB4, 0x43);
    outb_p(RTAI_COUNTER_2_LATCH & 0xff, 0x42);
    outb_p(RTAI_COUNTER_2_LATCH >> 8, 0x42);
    rtai_ts_8254 = c + ((RTIME)LATCH)*jiffies;
    rtai_last_8254_counter2 = 0; 
    outb_p((inb_p(0x61) & 0xFD) | 1, 0x61);

    rtai_critical_exit(flags);
}

void rt_mount (void) {

    TRACE_RTAI_MOUNT();
}

void rt_umount (void) {

    TRACE_RTAI_UMOUNT();
}

static void rtai_irq_trampoline (unsigned irq)

{
    TRACE_RTAI_GLOBAL_IRQ_ENTRY(irq,0);

    if (rtai_realtime_irq[irq].handler)
	{
#ifdef CONFIG_RTAI_SCHED_ISR_LOCK
	adeos_declare_cpuid;

#ifdef adeos_load_cpuid
	adeos_load_cpuid();
#endif /* adeos_load_cpuid */

	if (rtai_isr_nesting[cpuid]++ == 0 && rtai_isr_hook)
	    rtai_isr_hook(rtai_isr_nesting[cpuid]);
#endif /* CONFIG_RTAI_SCHED_ISR_LOCK */
	rtai_realtime_irq[irq].handler(irq,rtai_realtime_irq[irq].cookie);
#ifdef CONFIG_RTAI_SCHED_ISR_LOCK
	if (--rtai_isr_nesting[cpuid] == 0 && rtai_isr_hook)
	    rtai_isr_hook(rtai_isr_nesting[cpuid]);
#endif /* CONFIG_RTAI_SCHED_ISR_LOCK */
	}
    else
	adeos_propagate_irq(irq);

    TRACE_RTAI_GLOBAL_IRQ_EXIT();
}

static void rtai_trap_fault (adevinfo_t *evinfo)

{
    adeos_declare_cpuid;

    static const int trap2sig[] = {
    	SIGFPE,         //  0 - Divide error
	SIGTRAP,        //  1 - Debug
	SIGSEGV,        //  2 - NMI (but we ignore these)
	SIGTRAP,        //  3 - Software breakpoint
	SIGSEGV,        //  4 - Overflow
	SIGSEGV,        //  5 - Bounds
	SIGILL,         //  6 - Invalid opcode
	SIGSEGV,        //  7 - Device not available
	SIGSEGV,        //  8 - Double fault
	SIGFPE,         //  9 - Coprocessor segment overrun
	SIGSEGV,        // 10 - Invalid TSS
	SIGBUS,         // 11 - Segment not present
	SIGBUS,         // 12 - Stack segment
	SIGSEGV,        // 13 - General protection fault
	SIGSEGV,        // 14 - Page fault
	0,              // 15 - Spurious interrupt
	SIGFPE,         // 16 - Coprocessor error
	SIGBUS,         // 17 - Alignment check
	SIGSEGV,        // 18 - Reserved
	SIGFPE,         // 19 - XMM fault
	0,0,0,0,0,0,0,0,0,0,0,0
    };

    TRACE_RTAI_TRAP_ENTRY(evinfo->event,0);

    /* Notes:

    1) GPF needs to be propagated downstream whichever domain caused
    it. This is required so that we don't spuriously raise a fatal
    error when some fixup code is available to solve the error
    condition. For instance, Linux always attempts to reload the %gs
    segment register when switching a process in (__switch_to()),
    regardless of its value. It is then up to Linux's GPF handling
    code to search for a possible fixup whenever some exception
    occurs. In the particular case of the %gs register, such an
    exception could be raised for an exiting process if a preemption
    occurs inside a short time window, after the process's LDT has
    been dropped, but before the kernel lock is taken.  The same goes
    for LXRT switching back a Linux thread in non-RT mode which
    happens to have been preempted inside do_exit() after the MM
    context has been dropped (thus the LDT too). In such a case, %gs
    could be reloaded with what used to be the TLS descriptor of the
    exiting thread, but unfortunately after the LDT itself has been
    dropped. Since the default LDT is only 5 entries long, any attempt
    to refer to an LDT-indexed descriptor above this value would cause
    a GPF.
    2) NMI is not pipelined by Adeos. */

    if (evinfo->domid != RTAI_DOMAIN_ID)
	goto propagate;

#ifdef adeos_load_cpuid
    adeos_load_cpuid();
#endif /* adeos_load_cpuid */

	if (!test_bit(cpuid, &rtai_cpu_realtime)) {
		goto propagate;
	}
	if (evinfo->event == 7)	{ /* (FPU) Device not available. */
	/* A trap must come from a well estabilished Linux task context; from
	   anywhere else it is a bug to fix and not a hal.c problem */
		struct task_struct *linux_task = current;

	/* We need to keep this to avoid going through Linux in case users
	   do not set the FPU, for hard real time operations, either by 
	   calling the appropriate LXRT function or by doing any FP operation
	   before going to hard mode. Notice that after proper initialization
	   LXRT anticipate restoring the hard FP context at any task switch.
	   So just the initialisation should be needed, but we do what Linux
	   does in math_state_restore anyhow, to stay on the safe side. 
	   In any case we inform the user. */
		adeos_hw_cli();  /* in task context, so we can be preempted */
		if (!linux_task->used_math) {
			init_xfpu();	/* Does clts(). */
			linux_task->used_math = 1;
			rt_printk("\nUNEXPECTED FPU INITIALIZATION FROM PID = %d\n", linux_task->pid);
		} else {	
			rt_printk("\nUNEXPECTED FPU TRAP FROM HARD PID = %d\n", linux_task->pid);
		}
		restore_task_fpenv(linux_task);	/* Does clts(). */
		set_tsk_used_fpu(linux_task);
		adeos_hw_sti();
		goto endtrap;
	}

#if ADEOS_RELEASE_NUMBER >= 0x02060601

    if (evinfo->event == 14)	/* Page fault. */
	{
	struct pt_regs *regs = (struct pt_regs *)evinfo->evdata;
	unsigned long address;

	/* Handle RTAI-originated faults in kernel space caused by
	   on-demand virtual memory mappings. We can specifically
	   process this case through the Linux fault handler since we
	   know that it is context-agnostic and does not wreck the
	   determinism. Any other case would lead us to panicking. */

	adeos_hw_cli();
	__asm__("movl %%cr2,%0":"=r" (address));

	if (address >= TASK_SIZE && !(regs->orig_eax & 5)) /* i.e. trap error code. */
	    {
	    asmlinkage void do_page_fault(struct pt_regs *regs, unsigned long error_code);
	    do_page_fault(regs,regs->orig_eax);
	    goto endtrap;
	    }
	 adeos_hw_sti();
    }

#endif /* ADEOS_RELEASE_NUMBER >= 0x02060601 */

    if (rtai_trap_handler != NULL &&
	rtai_trap_handler(evinfo->event,
			  trap2sig[evinfo->event],
			  (struct pt_regs *)evinfo->evdata,
			  (void *)cpuid) != 0)
	goto endtrap;

propagate:

    adeos_propagate_event(evinfo);

endtrap:

    TRACE_RTAI_TRAP_EXIT();
}

static void rtai_ssrq_trampoline (unsigned virq)

{
    unsigned long pending;

    spin_lock(&rtai_ssrq_lock);

    while ((pending = rtai_sysreq_pending & ~rtai_sysreq_running) != 0)
	{
	unsigned srq = ffnz(pending);
	set_bit(srq,&rtai_sysreq_running);
	clear_bit(srq,&rtai_sysreq_pending);
	spin_unlock(&rtai_ssrq_lock);

	if (test_bit(srq,&rtai_sysreq_map))
	    rtai_sysreq_table[srq].k_handler();

	clear_bit(srq,&rtai_sysreq_running);
	spin_lock(&rtai_ssrq_lock);
	}

    spin_unlock(&rtai_ssrq_lock);
}

long long rtai_usrq_trampoline (unsigned srq, unsigned label)

{
    long long r = 0;

    TRACE_RTAI_SRQ_ENTRY(srq);

    if (srq > 1 && srq < RTAI_NR_SRQS &&
	test_bit(srq,&rtai_sysreq_map) &&
	rtai_sysreq_table[srq].u_handler != NULL)
	r = rtai_sysreq_table[srq].u_handler(label);
    else
	for (srq = 2; srq < RTAI_NR_SRQS; srq++)
	    if (test_bit(srq,&rtai_sysreq_map) &&
		rtai_sysreq_table[srq].label == label)
		r = (long long)srq;

    TRACE_RTAI_SRQ_EXIT();

    return r;
}

static void rtai_uvec_handler (void)

{
    __asm__ __volatile__ ( \
	"cld\n\t" \
        "pushl %es\n\t" \
        "pushl %ds\n\t" \
        "pushl %ebp\n\t" \
	"pushl %edi\n\t" \
        "pushl %esi\n\t" \
        "pushl %ecx\n\t" \
	"pushl %ebx\n\t" \
        "pushl %edx\n\t" \
        "pushl %eax\n\t" \
	__LXRT_GET_DATASEG(ebx) \
        "movl %ebx,%ds\n\t" \
        "movl %ebx,%es\n\t" \
        "call "SYMBOL_NAME_STR(rtai_usrq_trampoline)"\n\t" \
	"addl $8,%esp\n\t" \
        "popl %ebx\n\t" \
        "popl %ecx\n\t" \
        "popl %esi\n\t" \
	"popl %edi\n\t" \
        "popl %ebp\n\t" \
        "popl %ds\n\t" \
        "popl %es\n\t" \
        "iret");
}

#define rtai_set_gate(gate_addr,type,dpl,addr) \
do { \
  int __d0, __d1; \
  __asm__ __volatile__ ("movw %%dx,%%ax\n\t" \
	"movw %4,%%dx\n\t" \
	"movl %%eax,%0\n\t" \
	"movl %%edx,%1" \
	:"=m" (*((long *) (gate_addr))), \
	 "=m" (*(1+(long *) (gate_addr))), "=&a" (__d0), "=&d" (__d1) \
	:"i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
	 "3" ((char *) (addr)),"2" (__KERNEL_CS << 16)); \
} while (0)

static void rtai_install_archdep (void)

{
    unsigned long flags;

    flags = rtai_critical_enter(NULL);

    /* Backup and replace the sysreq vector. */
    rtai_sysvec = idt_table[RTAI_SYS_VECTOR];
    rtai_set_gate(idt_table+RTAI_SYS_VECTOR,15,3,&rtai_uvec_handler);

    rtai_critical_exit(flags);

    if (rtai_cpufreq_arg == 0)
	{
	adsysinfo_t sysinfo;
	adeos_get_sysinfo(&sysinfo);
	rtai_cpufreq_arg = (unsigned long)sysinfo.cpufreq; /* FIXME: 4Ghz barrier is close... */
	}

    rtai_tunables.cpu_freq = rtai_cpufreq_arg;

#ifdef CONFIG_X86_LOCAL_APIC
    if (rtai_apicfreq_arg == 0)
	rtai_apicfreq_arg = apic_read(APIC_TMICT) * HZ;

    rtai_tunables.apic_freq = rtai_apicfreq_arg;
#endif /* CONFIG_X86_LOCAL_APIC */
}

static void rtai_uninstall_archdep (void) {

    unsigned long flags;

    flags = rtai_critical_enter(NULL);
    idt_table[RTAI_SYS_VECTOR] = rtai_sysvec;
    rtai_critical_exit(flags);
}

int rtai_calibrate_8254 (void)

{
    unsigned long flags;
    RTIME t, dt;
    int i;

    flags = rtai_critical_enter(NULL);

    outb(0x34,0x43);

    t = rtai_rdtsc();

    for (i = 0; i < 10000; i++)
	{ 
	outb(LATCH & 0xff,0x40);
	outb(LATCH >> 8,0x40);
	}

    dt = rtai_rdtsc() - t;

    rtai_critical_exit(flags);

    return rtai_imuldiv(dt,100000,RTAI_CPU_FREQ);
}

void (*rt_set_ihook (void (*hookfn)(int)))(int) {

#ifdef CONFIG_RTAI_SCHED_ISR_LOCK
    return (void (*)(int))xchg(&rtai_isr_hook,hookfn); /* This is atomic */
#else  /* !CONFIG_RTAI_SCHED_ISR_LOCK */
    return NULL;
#endif /* CONFIG_RTAI_SCHED_ISR_LOCK */
}

void rtai_exit_trampoline (adevinfo_t *evinfo)

{
    struct task_struct *task = (struct task_struct *)evinfo->evdata;

    if (task->ptd[0])
	rtai_exit_callback();

    adeos_propagate_event(evinfo);
}

int set_rtai_callback (void (*fun)(void))

{
    rtai_exit_callback = fun;
    return adeos_catch_event(ADEOS_EXIT_PROCESS,&rtai_exit_trampoline);
}

void remove_rtai_callback (void (*fun)(void))
{
    adeos_catch_event(ADEOS_EXIT_PROCESS,0);
}

static int errno;

static inline _syscall3(int,
			sched_setscheduler,
			pid_t,pid,
			int,policy,
			struct sched_param *,param)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
void rtai_set_linux_task_priority (struct task_struct *task, int policy, int prio)

{
    task->policy = policy;
    task->rt_priority = prio;
    set_tsk_need_resched(current);
}
#else /* KERNEL_VERSION >= 2.6.0 */
void rtai_set_linux_task_priority (struct task_struct *task, int policy, int prio)

{
    struct sched_param __user param;
    mm_segment_t old_fs;
    int rc;

    param.sched_priority = prio;
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    rc = sched_setscheduler(task->pid,policy,&param);
    set_fs(old_fs);

    if (rc)
	printk("RTAI[hal]: sched_setscheduler(policy=%d,prio=%d) failed, code %d (%s -- pid=%d)\n",
	       policy,
	       prio,
	       rc,
	       task->comm,
	       task->pid);
}
#endif  /* KERNEL_VERSION < 2.6.0 */

#ifdef CONFIG_PROC_FS

struct proc_dir_entry *rtai_proc_root = NULL;

static int rtai_read_proc (char *page,
			   char **start,
			   off_t off,
			   int count,
			   int *eof,
			   void *data)
{
    PROC_PRINT_VARS;
    int i, none;

    PROC_PRINT("\n** RTAI/x86:\n\n");
#ifdef CONFIG_X86_LOCAL_APIC
    PROC_PRINT("    APIC Frequency: %lu\n",rtai_tunables.apic_freq);
    PROC_PRINT("    APIC Latency: %d ns\n",RTAI_LATENCY_APIC);
    PROC_PRINT("    APIC Setup: %d ns\n",RTAI_SETUP_TIME_APIC);
#endif /* CONFIG_X86_LOCAL_APIC */
    
    none = 1;

    PROC_PRINT("\n** Real-time IRQs used by RTAI: ");

    for (i = 0; i < NR_IRQS; i++)
	{
	if (rtai_realtime_irq[i].handler)
	    {
	    if (none)
		{
		PROC_PRINT("\n");
		none = 0;
		}

	    PROC_PRINT("\n    #%d at %p", i, rtai_realtime_irq[i].handler);
	    }
        }

    if (none)
	PROC_PRINT("none");

    PROC_PRINT("\n\n");

    PROC_PRINT("** RTAI extension traps: \n\n");
    PROC_PRINT("    SYSREQ=0x%x\n",RTAI_SYS_VECTOR);
    PROC_PRINT("      LXRT=0x%x\n",RTAI_LXRT_VECTOR);
    PROC_PRINT("       SHM=0x%x\n\n",RTAI_SHM_VECTOR);

    none = 1;
    PROC_PRINT("** RTAI SYSREQs in use: ");

    for (i = 0; i < RTAI_NR_SRQS; i++)
	{
	if (rtai_sysreq_table[i].k_handler ||
	    rtai_sysreq_table[i].u_handler)
	    {
	    PROC_PRINT("#%d ", i);
	    none = 0;
	    }
        }

    if (none)
	PROC_PRINT("none");

    PROC_PRINT("\n\n");

    PROC_PRINT_DONE;
}

static int rtai_proc_register (void)

{
    struct proc_dir_entry *ent;

    rtai_proc_root = create_proc_entry("rtai",S_IFDIR, 0);

    if (!rtai_proc_root)
	{
	printk(KERN_ERR "Unable to initialize /proc/rtai.\n");
	return -1;
        }

    rtai_proc_root->owner = THIS_MODULE;

    ent = create_proc_entry("rtai",S_IFREG|S_IRUGO|S_IWUSR,rtai_proc_root);

    if (!ent)
	{
	printk(KERN_ERR "Unable to initialize /proc/rtai/rtai.\n");
	return -1;
        }

    ent->read_proc = rtai_read_proc;

    return 0;
}

static void rtai_proc_unregister (void)

{
    remove_proc_entry("rtai",rtai_proc_root);
    remove_proc_entry("rtai",0);
}

#endif /* CONFIG_PROC_FS */

static void rtai_domain_entry (int iflag)

{
    unsigned irq, trapnr;

    if (iflag)
	{
	for (irq = 0; irq < NR_IRQS; irq++)
	    adeos_virtualize_irq(irq,
				 &rtai_irq_trampoline,
				 NULL,
				 IPIPE_DYNAMIC_MASK);
	/* Trap all faults. */
	for (trapnr = 0; trapnr < ADEOS_NR_FAULTS; trapnr++)
	    adeos_catch_event(trapnr,&rtai_trap_fault);

	printk(KERN_INFO "RTAI[hal]: %s mounted over Adeos %s.\n",PACKAGE_VERSION,ADEOS_VERSION_STRING);
	printk(KERN_INFO "RTAI[hal]: compiled with %s.\n",CONFIG_RTAI_COMPILER);
	}

    for (;;)
	adeos_suspend_domain();
}

int __rtai_hal_init (void)

{
    unsigned long flags;
    int key0, key1;
    adattr_t attr;

    /* Allocate a virtual interrupt to handle sysreqs within the Linux
       domain. */
    rtai_sysreq_virq = adeos_alloc_irq();

    if (!rtai_sysreq_virq)
	{
	printk(KERN_ERR "RTAI[hal]: no virtual interrupt available.\n");
	return 1;
	}

    /* Reserve the first two _consecutive_ per-thread data key in the
       Linux domain. This is rather crappy, since we depend on
       statically defined PTD key values, which is exactly what the
       PTD scheme is here to prevent. Unfortunately, reserving these
       specific keys is the only way to remain source compatible with
       the current LXRT implementation. */
    flags = rtai_critical_enter(NULL);
    rtai_adeos_ptdbase = key0 = adeos_alloc_ptdkey();
    key1 = adeos_alloc_ptdkey();
    rtai_critical_exit(flags);

    if (key0 != 0 && key1 != 1)
	{
	printk(KERN_ERR "RTAI[hal]: per-thread keys #0 and/or #1 are busy.\n");
	return 1;
	}

    adeos_virtualize_irq(rtai_sysreq_virq,
			 &rtai_ssrq_trampoline,
			 NULL,
			 IPIPE_HANDLE_MASK);

    rtai_install_archdep();

#ifdef CONFIG_PROC_FS
    rtai_proc_register();
#endif

    /* Let Adeos do its magic for our real-time domain. */
    adeos_init_attr(&attr);
    attr.name = "RTAI";
    attr.domid = RTAI_DOMAIN_ID;
    attr.entry = &rtai_domain_entry;
    attr.priority = ADEOS_ROOT_PRI + 100; /* Precede Linux in the pipeline */

    return adeos_register_domain(&rtai_domain,&attr);
}

void __rtai_hal_exit (void)

{
#ifdef CONFIG_PROC_FS
    rtai_proc_unregister();
#endif

    adeos_virtualize_irq(rtai_sysreq_virq,NULL,NULL,0);
    adeos_free_irq(rtai_sysreq_virq);
    rtai_uninstall_archdep();
    adeos_free_ptdkey(rtai_adeos_ptdbase); /* #0 and #1 actually */
    adeos_free_ptdkey(rtai_adeos_ptdbase + 1);
    adeos_unregister_domain(&rtai_domain);
    printk(KERN_INFO "RTAI[hal]: unmounted.\n");
}

module_init(__rtai_hal_init);
module_exit(__rtai_hal_exit);

EXPORT_SYMBOL(rt_request_irq);
EXPORT_SYMBOL(rt_release_irq);
EXPORT_SYMBOL(rt_set_irq_cookie);
EXPORT_SYMBOL(rt_startup_irq);
EXPORT_SYMBOL(rt_shutdown_irq);
EXPORT_SYMBOL(rt_enable_irq);
EXPORT_SYMBOL(rt_disable_irq);
EXPORT_SYMBOL(rt_mask_and_ack_irq);
EXPORT_SYMBOL(rt_unmask_irq);
EXPORT_SYMBOL(rt_ack_irq);
EXPORT_SYMBOL(rt_do_irq);
EXPORT_SYMBOL(rt_request_linux_irq);
EXPORT_SYMBOL(rt_free_linux_irq);
EXPORT_SYMBOL(rt_pend_linux_irq);
EXPORT_SYMBOL(rt_request_srq);
EXPORT_SYMBOL(rt_free_srq);
EXPORT_SYMBOL(rt_pend_linux_srq);
EXPORT_SYMBOL(rt_assign_irq_to_cpu);
EXPORT_SYMBOL(rt_reset_irq_to_sym_mode);
EXPORT_SYMBOL(rt_request_timer_cpuid);
EXPORT_SYMBOL(rt_request_apic_timers);
EXPORT_SYMBOL(rt_free_apic_timers);
EXPORT_SYMBOL(rt_request_timer);
EXPORT_SYMBOL(rt_free_timer);
EXPORT_SYMBOL(rt_set_trap_handler);
EXPORT_SYMBOL(rd_8254_ts);
EXPORT_SYMBOL(rt_setup_8254_tsc);
EXPORT_SYMBOL(rt_set_ihook);
EXPORT_SYMBOL(rt_mount);
EXPORT_SYMBOL(rt_umount);

EXPORT_SYMBOL(rtai_calibrate_8254);
EXPORT_SYMBOL(rtai_broadcast_to_local_timers);
EXPORT_SYMBOL(rtai_critical_enter);
EXPORT_SYMBOL(rtai_critical_exit);
EXPORT_SYMBOL(rtai_set_linux_task_priority);

EXPORT_SYMBOL(rtai_linux_context);
EXPORT_SYMBOL(rtai_domain);
EXPORT_SYMBOL(rtai_proc_root);
EXPORT_SYMBOL(rtai_tunables);
EXPORT_SYMBOL(rtai_cpu_lock);
EXPORT_SYMBOL(rtai_cpu_realtime);
EXPORT_SYMBOL(set_rtai_callback);
EXPORT_SYMBOL(remove_rtai_callback);
EXPORT_SYMBOL(rt_times);
EXPORT_SYMBOL(rt_smp_times);

/*@}*/