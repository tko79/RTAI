/*
 * ARM RTAI over Adeos -- based on ARTI for x86 and RTHAL for ARM.
 *
 * Original RTAI/x86 layer implementation:
 *   Copyright (c) 2000 Paolo Mantegazza (mantegazza@aero.polimi.it)
 *   Copyright (c) 2000 Steve Papacharalambous (stevep@zentropix.com)
 *   Copyright (c) 2000 Stuart Hughes
 *   and others.
 *
 * RTAI/x86 rewrite over Adeos:
 *   Copyright (c) 2002 Philippe Gerum (rpm@xenomai.org)
 *
 * Original RTAI/ARM RTHAL implementation:
 *   Copyright (c) 2000 Pierre Cloutier (pcloutier@poseidoncontrols.com)
 *   Copyright (c) 2001 Alex Z�pke, SYSGO RTS GmbH (azu@sysgo.de)
 *   Copyright (c) 2002 Guennadi Liakhovetski DSA GmbH (gl@dsa-ac.de)
 *   Copyright (c) 2002 Steve Papacharalambous (stevep@zentropix.com)
 *   Copyright (c) 2002 Wolfgang M�ller (wolfgang.mueller@dsa-ac.de)
 *   Copyright (c) 2003 Bernard Haible, Marconi Communications
 *   Copyright (c) 2003 Thomas Gleixner (tglx@linutronix.de)
 *   Copyright (c) 2003 Philippe Gerum (rpm@xenomai.org)
 *
 * RTAI/ARM over Adeos rewrite:
 *   Copyright (c) 2004-2005 Michael Neuhauser, Firmix Software GmbH (mike@firmix.at)
 *
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge MA 02139, USA; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <linux/version.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/stddef.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/mach/irq.h>
#include <asm/proc/ptrace.h>
#define __RTAI_HAL__
#include <asm/rtai_hal.h>
#include <asm/rtai_lxrt.h>
#include <asm/rtai_usi.h>
#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#include <rtai_proc_fs.h>
#endif /* CONFIG_PROC_FS */
#include <rtai_version.h>

MODULE_LICENSE("GPL");

typedef void (*isr_hook_t)(int);

/* global */

struct {
    rt_irq_handler_t handler;
    void *cookie;
    int retmode;
}			rtai_realtime_irq[NR_IRQS]
			__attribute__((__aligned__(L1_CACHE_BYTES)));
adomain_t		rtai_domain;
struct rt_times		rt_times;
struct rt_times		rt_smp_times[RTAI_NR_CPUS] = { { 0 } };
struct rtai_switch_data rtai_linux_context[RTAI_NR_CPUS];
struct calibration_data rtai_tunables;
volatile unsigned long	rtai_cpu_realtime;
volatile unsigned long	rtai_cpu_lock;
int			rtai_adeos_ptdbase = -1;
long long		(*rtai_lxrt_invoke_entry)(unsigned long, void *); /* hook for lxrt calls */
struct { volatile int locked, rqsted; } rt_scheduling[RTAI_NR_CPUS];
#ifdef CONFIG_PROC_FS
struct proc_dir_entry	*rtai_proc_root = NULL;
#endif

/* local */

static struct {
    unsigned long flags;
    int count;
}			rtai_linux_irq[NR_IRQS];
static struct {
    void (*k_handler)(void);
    long long (*u_handler)(unsigned);
    unsigned label;
}			rtai_sysreq_table[RTAI_NR_SRQS];
static unsigned		rtai_sysreq_virq;
static unsigned long	rtai_sysreq_map = 3; /* srqs #[0-1] are reserved */
static unsigned long	rtai_sysreq_pending;
static unsigned long	rtai_sysreq_running;
static spinlock_t	rtai_ssrq_lock = SPIN_LOCK_UNLOCKED;
static volatile int	rtai_sync_level;
static atomic_t		rtai_sync_count = ATOMIC_INIT(1);
static RT_TRAP_HANDLER	rtai_trap_handler;
static int		(*saved_adeos_syscall_handler)(struct pt_regs *regs);
#ifdef CONFIG_RTAI_SCHED_ISR_LOCK
// *TODO* enable in config, do tests
static isr_hook_t	rtai_isr_hook;
#endif /* CONFIG_RTAI_SCHED_ISR_LOCK */

unsigned long
rtai_critical_enter(void (*synch)(void))
{
    unsigned long flags = adeos_critical_enter(synch);

    if (atomic_dec_and_test(&rtai_sync_count))
	rtai_sync_level = 0;
    else if (synch != NULL)
	printk(KERN_INFO "RTAI[hal]: warning: nested sync will fail.\n");

    return flags;
}

void
rtai_critical_exit(unsigned long flags)
{
    atomic_inc(&rtai_sync_count);
    adeos_critical_exit(flags);
}

int
rt_request_irq(unsigned irq, rt_irq_handler_t handler, void *cookie, int retmode)
{
    unsigned long flags;

    if (handler == NULL || irq >= NR_IRQS)
	return -EINVAL;

    if (rtai_realtime_irq[irq].handler != NULL)
	return -EBUSY;

    flags = rtai_critical_enter(NULL);
    rtai_realtime_irq[irq].handler = handler;
    rtai_realtime_irq[irq].cookie = cookie;
    rtai_critical_exit(flags);

    return 0;
}

int
rt_release_irq(unsigned irq)
{
    unsigned long flags;

    if (irq >= NR_IRQS  || !rtai_realtime_irq[irq].handler)
	return -EINVAL;

    flags = rtai_critical_enter(NULL);
    rtai_realtime_irq[irq].handler = NULL;
    rtai_critical_exit(flags);

    return 0;
}

void
rt_set_irq_cookie(unsigned irq, void *cookie)
{
    if (irq < NR_IRQS)
	rtai_realtime_irq[irq].cookie = cookie;
}


/*
 * The function below allow you to manipulate the PIC at hand, but you must
 * know what you are doing. Such a duty does not pertain to this manual and
 * you should refer to your PIC datasheet.
 *
 * Note that Linux has the same functions, but they must be used only for its
 * interrupts. Only the ones below can be safely used in real time handlers.
 *
 * It must also be remarked that when you install a real time interrupt handler,
 * RTAI already calls either rt_mask_and_ack_irq(), for level triggered
 * interrupts, or rt_ack_irq(), for edge triggered interrupts, before passing
 * control to your interrupt handler. Thus, generally you should just call
 * rt_unmask_irq() at due time, for level triggered interrupts, while nothing
 * should be done for edge triggered ones. Recall that in the latter case you
 * allow also any new interrupts on the same request as soon as you enable
 * interrupts at the CPU level.
 * 
 * Often some of the functions below do equivalent things. Once more there is no
 * way of doing it right except by knowing the hardware you are manipulating.
 * Furthermore you must also remember that when you install a hard real time
 * handler the related interrupt is usually disabled, unless you are overtaking
 * one already owned by Linux which has been enabled by it.   Recall that if
 * have done it right, and interrupts do not show up, it is likely you have just
 * to rt_enable_irq() your irq.
 *
 * Note: Adeos already does all the magic that allows to call the
 * interrupt controller routines safely.
 */

unsigned
rt_startup_irq(unsigned irq)
{
    struct irqdesc *id = &irq_desc[irq];
    id->probing = 0;
    id->triggered = 0;
    id->disable_depth = 0;
    id->unmask(irq);
    return 0;
}

void
rt_shutdown_irq(unsigned irq)
{
    struct irqdesc *id = &irq_desc[irq];
    id->disable_depth = (unsigned int)-1;
    id->mask(irq);
}

void
rt_enable_irq(unsigned irq)
{
    struct irqdesc *id = &irq_desc[irq];
    if (id->disable_depth == 0) {
	printk(KERN_ERR "RTAI[hal]: %s(%u) unbalanced from %p\n",
	       __func__, irq, __builtin_return_address(0));
    } else if (--id->disable_depth == 0) {
	id->probing = 0;
	id->unmask(irq);
    }
}

void
rt_disable_irq(unsigned irq)
{
    struct irqdesc *id = &irq_desc[irq];
    if (id->disable_depth++ == 0)
	id->mask(irq);
}

void
rt_mask_and_ack_irq(unsigned irq)
{
    irq_desc[irq].mask_ack(irq);
}

void
rt_unmask_irq(unsigned irq)
{
    irq_desc[irq].unmask(irq);
}

void
rt_ack_irq(unsigned irq)
{
    /* ARM has no "ack" slot in irqdesc, do mask_ack and then unmask */
    struct irqdesc *id = &irq_desc[irq];
    id->mask_ack(irq);
    id->unmask(irq);
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
int
rt_request_linux_irq(unsigned irq,
		     irqreturn_t (*handler)(int irq, void *dev_id, struct pt_regs *regs),
		     char *name,
		     void *dev_id)
{
    unsigned long flags;

    if (irq >= NR_IRQS || !handler)
	return -EINVAL;

    rtai_save_flags_and_cli(flags);

    if (rtai_linux_irq[irq].count++ == 0 && irq_desc[irq].action) {
	rtai_linux_irq[irq].flags = irq_desc[irq].action->flags;
	irq_desc[irq].action->flags |= SA_SHIRQ;
    }

    rtai_restore_flags(flags);

    request_irq(irq, handler, SA_SHIRQ, name, dev_id);

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
int
rt_free_linux_irq(unsigned irq, void *dev_id)
{
    unsigned long flags;

    if (irq >= NR_IRQS || rtai_linux_irq[irq].count == 0)
	return -EINVAL;

    rtai_save_flags_and_cli(flags);

    free_irq(irq, dev_id);

    if (--rtai_linux_irq[irq].count == 0 && irq_desc[irq].action)
	irq_desc[irq].action->flags = rtai_linux_irq[irq].flags;

    rtai_restore_flags(flags);

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
void
rt_pend_linux_irq(unsigned irq)
{
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
int
rt_request_srq(unsigned label, void (*k_handler)(void), long long (*u_handler)(unsigned))
{
    unsigned long flags;
    int srq;

    if (k_handler == NULL)
	return -EINVAL;

    rtai_save_flags_and_cli(flags);

    if (rtai_sysreq_map != ~0) {
	srq = ffz(rtai_sysreq_map);
	set_bit(srq, &rtai_sysreq_map);
	rtai_sysreq_table[srq].k_handler = k_handler;
	rtai_sysreq_table[srq].u_handler = u_handler;
	rtai_sysreq_table[srq].label = label;
    } else
	srq = -EBUSY;

    rtai_restore_flags(flags);

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
int
rt_free_srq(unsigned srq)
{
    return (srq < 2 || srq >= RTAI_NR_SRQS || !test_and_clear_bit(srq, &rtai_sysreq_map))
	? -EINVAL
	: 0;
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
void
rt_pend_linux_srq(unsigned srq)
{
    int cpuid;
    if (srq > 0 && srq < RTAI_NR_SRQS) {
	set_bit(srq, &rtai_sysreq_pending);
	cpuid = rtai_cpuid();
	if (adp_cpu_current[cpuid] == &rtai_domain)
	    adeos_propagate_irq(rtai_sysreq_virq);
	else
	    adeos_schedule_irq(rtai_sysreq_virq);
    }
}

/* rt_request_timer(): install a timer interrupt handler and set hardware-timer
 * to requested period. This is arch-specific (stopping/reprogramming/...
 * timer). Hence, the function is contained in mach-ARCH/ARCH-timer.c
 */

/* rt_free_timer(): uninstall a timer handler previously set by
 * rt_request_timer() and reset hardware-timer to Linux HZ-tick.
 * This is arch-specific (stopping/reprogramming/... timer).
 * Hence, the function is contained in mach-ARCH/ARCH-timer.c
 */

RT_TRAP_HANDLER
rt_set_trap_handler(RT_TRAP_HANDLER handler)
{
    return (RT_TRAP_HANDLER)xchg(&rtai_trap_handler, handler);
}

static void
rtai_irq_trampoline(unsigned irq)
{
    TRACE_RTAI_GLOBAL_IRQ_ENTRY(irq, 0);

    if (rtai_realtime_irq[irq].handler)
	{
#ifdef CONFIG_RTAI_SCHED_ISR_LOCK
	adeos_declare_cpuid;
	adeos_load_cpuid();
	if (!rt_scheduling[cpuid].locked++)
	    rt_scheduling[cpuid].rqsted = 0;
#endif /* CONFIG_RTAI_SCHED_ISR_LOCK */
	rtai_realtime_irq[irq].handler(irq, rtai_realtime_irq[irq].cookie);
#ifdef CONFIG_RTAI_SCHED_ISR_LOCK
	if (rt_scheduling[cpuid].locked && !(--rt_scheduling[cpuid].locked))
	    if (rt_scheduling[cpuid].rqsted > 0 && rtai_isr_hook)
		rtai_isr_hook(cpuid);
#endif /* CONFIG_RTAI_SCHED_ISR_LOCK */
	}
    else
	adeos_propagate_irq(irq);

    TRACE_RTAI_GLOBAL_IRQ_EXIT();
}

static void
rtai_trap_fault(adevinfo_t *evinfo)
{
    adeos_declare_cpuid;

    TRACE_RTAI_TRAP_ENTRY(evinfo->event, 0);

    if (evinfo->domid != RTAI_DOMAIN_ID)
	goto propagate;

    adeos_load_cpuid();

    /* We don't treat SIGILL as "FPU usage" as there is no FPU support in RTAI for ARM.
     * *FIXME* The whole FPU kernel emulation issue has to be sorted out (is it
     * reentrentant, do we need to save the emulated registers, can it be used
     * in kernel space, etc.). */

    if (rtai_trap_handler != NULL
	&& rtai_trap_handler(evinfo->event, evinfo->event, (struct pt_regs *)evinfo->evdata,
			     (void *)cpuid) != 0)
	goto endtrap;

propagate:

    adeos_propagate_event(evinfo);

endtrap:

    TRACE_RTAI_TRAP_EXIT();
}

static void
rtai_ssrq_trampoline(unsigned virq)
{
    unsigned long pending;

    spin_lock(&rtai_ssrq_lock);

    while ((pending = rtai_sysreq_pending & ~rtai_sysreq_running) != 0) {
	unsigned srq = ffnz(pending);
	set_bit(srq, &rtai_sysreq_running);
	clear_bit(srq, &rtai_sysreq_pending);
	spin_unlock(&rtai_ssrq_lock);

	if (test_bit(srq, &rtai_sysreq_map))
	    rtai_sysreq_table[srq].k_handler();

	clear_bit(srq, &rtai_sysreq_running);
	spin_lock(&rtai_ssrq_lock);
	}

    spin_unlock(&rtai_ssrq_lock);
}

extern inline long long
rtai_usrq_trampoline(unsigned long srq, unsigned long label)
{
    long long r = 0;

    TRACE_RTAI_SRQ_ENTRY(srq, label);

    if (srq > 1 && srq < RTAI_NR_SRQS
	&& test_bit(srq, &rtai_sysreq_map)
	&& rtai_sysreq_table[srq].u_handler != NULL)
	r = rtai_sysreq_table[srq].u_handler(label);
    else
	for (srq = 2; srq < RTAI_NR_SRQS; srq++)
	    if (test_bit(srq, &rtai_sysreq_map)
		&& rtai_sysreq_table[srq].label == label)
		r = (long long)srq;

    TRACE_RTAI_SRQ_EXIT();

    return r;
}

/* this handles the special RTAI syscall (see linux/arch/arm/kernel/entry-common.S)
 * that is used to implement LXRT calls and user requests */
static int
rtai_syscall_trampoline(struct pt_regs *regs)
{
    unsigned long srq = regs->ARM_r0;
    unsigned long arg = regs->ARM_r1;

#ifdef USI_SRQ_MASK
    IF_IS_A_USI_SRQ_CALL_IT();
#endif

    {
	long long r = srq > RTAI_NR_SRQS
	    ? rtai_lxrt_invoke_entry != NULL
		? rtai_lxrt_invoke_entry(srq, (void *)arg)
		: -ENODEV
	    : rtai_usrq_trampoline(srq, arg);
	*(long long*)&regs->ARM_r0 = r;	/* return value of call is expected in saved r0/r1 */
    }
    if (in_hrt_mode(rtai_cpuid()))
	    return 1;			/* hard real-time => fast return to user-space */
    local_irq_enable();
    return 0;				/* !hard real-time => slow return to user-space */
}

isr_hook_t
rt_set_ihook(isr_hook_t hookfn)
{
#ifdef CONFIG_RTAI_SCHED_ISR_LOCK
    return (isr_hook_t)xchg(&rtai_isr_hook, hookfn); /* This is atomic */
#else  /* !CONFIG_RTAI_SCHED_ISR_LOCK */
    return NULL;
#endif /* CONFIG_RTAI_SCHED_ISR_LOCK */
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
void
rtai_set_linux_task_priority(struct task_struct *task, int policy, int prio)
{
    task->policy = policy;
    task->rt_priority = prio;
    set_tsk_need_resched(current);
}
#else
#error "Sorry, Kernels >= 2.6.0 not supported (yet)"
#endif  /* KERNEL_VERSION < 2.6.0 */

#ifdef CONFIG_PROC_FS

static int
rtai_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    PROC_PRINT_VARS;
    int i, none;

    PROC_PRINT("\n** RTAI/ARM %s over Adeos %s\n\n", RTAI_RELEASE, ADEOS_VERSION_STRING);
    PROC_PRINT("    TSC frequency: %d Hz\n", RTAI_TSC_FREQ);
    PROC_PRINT("    Timer frequency: %d Hz\n", RTAI_TIMER_FREQ);
    PROC_PRINT("    Timer latency: %d ns, %d TSC ticks\n", RTAI_TIMER_LATENCY,
	rtai_imuldiv(RTAI_TIMER_LATENCY, RTAI_TSC_FREQ, 1000000000));
    PROC_PRINT("    Timer setup: %d ns\n", RTAI_TIMER_SETUP_TIME);
    PROC_PRINT("    Timer setup: %d TSC ticks, %d IRQ-timer ticks\n",
	rtai_imuldiv(RTAI_TIMER_SETUP_TIME, RTAI_TSC_FREQ, 1000000000),
	rtai_imuldiv(RTAI_TIMER_SETUP_TIME, RTAI_TIMER_FREQ, 1000000000));

    none = 1;

    PROC_PRINT("\n** Real-time IRQs used by RTAI: ");

    for (i = 0; i < NR_IRQS; i++) {
	if (rtai_realtime_irq[i].handler) {
	    if (none) {
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
    PROC_PRINT("    SYSREQ=0x%x\n", RTAI_SYS_VECTOR);
#if 0
    PROC_PRINT("       SHM=0x%x\n", RTAI_SHM_VECTOR);
#endif
    PROC_PRINT("\n");

    none = 1;
    PROC_PRINT("** RTAI SYSREQs in use: ");

    for (i = 0; i < RTAI_NR_SRQS; i++) {
	if (rtai_sysreq_table[i].k_handler || rtai_sysreq_table[i].u_handler) {
	    PROC_PRINT("#%d ", i);
	    none = 0;
	}
    }

    if (none)
	PROC_PRINT("none");

    PROC_PRINT("\n\n");

    PROC_PRINT_DONE;
}

static int
rtai_proc_register(void)
{
    struct proc_dir_entry *ent;

    rtai_proc_root = create_proc_entry("rtai", S_IFDIR, 0);

    if (!rtai_proc_root) {
	printk(KERN_ERR "RTAI[hal]: Unable to initialize /proc/rtai.\n");
	return -1;
    }

    rtai_proc_root->owner = THIS_MODULE;

    ent = create_proc_entry("rtai", S_IFREG|S_IRUGO|S_IWUSR, rtai_proc_root);

    if (!ent) {
	printk(KERN_ERR "RTAI[hal]: Unable to initialize /proc/rtai/rtai.\n");
	return -1;
    }

    ent->read_proc = rtai_read_proc;

    return 0;
}

static void
rtai_proc_unregister(void)
{
    remove_proc_entry("rtai", rtai_proc_root);
    remove_proc_entry("rtai", 0);
}

#endif /* CONFIG_PROC_FS */

static void
rtai_domain_entry(int iflag)
{
    unsigned irq, trapnr;

    if (iflag) {
	for (irq = 0; irq < NR_IRQS; irq++)
	    adeos_virtualize_irq(irq,
				 &rtai_irq_trampoline,
				 NULL,
				 IPIPE_DYNAMIC_MASK);
	/* Trap all faults. (Adeos for ARM doesn't generate any traps at all!) */
	for (trapnr = 0; trapnr < ADEOS_NR_FAULTS; trapnr++)
	    adeos_catch_event(trapnr, &rtai_trap_fault);

	printk(KERN_INFO "RTAI[hal]: %s mounted over Adeos %s.\n", PACKAGE_VERSION, ADEOS_VERSION_STRING);
	printk(KERN_INFO "RTAI[hal]: compiled with %s.\n", CONFIG_RTAI_COMPILER);
    }

#ifdef CONFIG_ADEOS_THREADS
    for (;;)
	adeos_suspend_domain();
#endif /* CONFIG_ADEOS_THREADS */
}

int
__rtai_hal_init(void)
{
    unsigned long flags;
    adattr_t attr;

    /* Allocate a virtual interrupt to handle sysreqs within the Linux
       domain. */
    rtai_sysreq_virq = adeos_alloc_irq();

    if (!rtai_sysreq_virq) {
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
    rtai_adeos_ptdbase = adeos_alloc_ptdkey();
    if (adeos_alloc_ptdkey() != rtai_adeos_ptdbase + 1) {
	rtai_critical_exit(flags);
	printk(KERN_ERR "RTAI[hal]: per-thread keys not available.\n");
	return 1;
    }
    /* install RTAI syscall handler */
    rtai_lxrt_invoke_entry = NULL;
    saved_adeos_syscall_handler = xchg(&adeos_syscall_entry, rtai_syscall_trampoline);
    rtai_critical_exit(flags);

    adeos_virtualize_irq(rtai_sysreq_virq,
			 &rtai_ssrq_trampoline,
			 NULL,
			 IPIPE_HANDLE_MASK);

    /* set TSC frequency, other values of rtai_tunables are set by the scheduler */
    rtai_tunables.cpu_freq = RTAI_TSC_FREQ;

#ifdef CONFIG_PROC_FS
    rtai_proc_register();
#endif

    /* do (sub-)architecture specific initializations */
    rtai_archdep_init();

    /* Let Adeos do its magic for our piped irq dispatching real-time domain. */
    adeos_init_attr(&attr);
    attr.name = "RTAI";
    attr.domid = RTAI_DOMAIN_ID;
    attr.entry = &rtai_domain_entry;
    attr.priority = ADEOS_ROOT_PRI + 100; /* Precede Linux in the pipeline */

    printk(KERN_INFO "RTAI[hal]: mounted (PIPED).\n");

    return adeos_register_domain(&rtai_domain, &attr);
}

void
__rtai_hal_exit(void)
{
    unsigned long flags;

    /* do (sub-)architecture specific cleanup */
    rtai_archdep_exit();

#ifdef CONFIG_PROC_FS
    rtai_proc_unregister();
#endif

    adeos_virtualize_irq(rtai_sysreq_virq, NULL, NULL, 0);
    adeos_free_irq(rtai_sysreq_virq);

    /* deinstall RTAI syscall handler */
    flags = rtai_critical_enter(NULL);
    adeos_syscall_entry = saved_adeos_syscall_handler;
    rtai_lxrt_invoke_entry = NULL;
    rtai_critical_exit(flags);

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
EXPORT_SYMBOL(rt_request_linux_irq);
EXPORT_SYMBOL(rt_free_linux_irq);
EXPORT_SYMBOL(rt_pend_linux_irq);
EXPORT_SYMBOL(rt_request_srq);
EXPORT_SYMBOL(rt_free_srq);
EXPORT_SYMBOL(rt_pend_linux_srq);
EXPORT_SYMBOL(rt_request_timer);
EXPORT_SYMBOL(rt_free_timer);
EXPORT_SYMBOL(rt_set_trap_handler);
EXPORT_SYMBOL(rt_set_ihook);
EXPORT_SYMBOL(rtai_critical_enter);
EXPORT_SYMBOL(rtai_critical_exit);
EXPORT_SYMBOL(rtai_set_linux_task_priority);
EXPORT_SYMBOL(rtai_linux_context);
EXPORT_SYMBOL(rtai_domain);
EXPORT_SYMBOL(rtai_proc_root);
EXPORT_SYMBOL(rtai_tunables);
EXPORT_SYMBOL(rtai_cpu_lock);
EXPORT_SYMBOL(rtai_cpu_realtime);
EXPORT_SYMBOL(rt_times);
EXPORT_SYMBOL(rt_smp_times);
EXPORT_SYMBOL(rtai_lxrt_invoke_entry);
EXPORT_SYMBOL(rtai_realtime_irq);
EXPORT_SYMBOL(rt_scheduling);
