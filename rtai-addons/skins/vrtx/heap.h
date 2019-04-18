/*
 * Copyright (C) 2001,2002 IDEALX (http://www.idealx.com/).
 * Written by Julien Pinon <jpinon@idealx.com>.
 * Copyright (C) 2003 Philippe Gerum <rpm@xenomai.org>.
 *
 * Xenomai is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Xenomai is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xenomai; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * As a special exception, the RTAI project gives permission
 * for additional uses of the text contained in its release of
 * Xenomai.
 *
 * The exception is that, if you link the Xenomai libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the Xenomai libraries code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public
 * License.
 *
 * This exception applies only to the code released by the
 * RTAI project under the name Xenomai.  If you copy code from other
 * RTAI project releases into a copy of Xenomai, as the General Public
 * License permits, the exception does not apply to the code that you
 * add in this way.  To avoid misleading anyone as to the status of
 * such modified files, you must delete this exception notice from
 * them.
 *
 * If you write modifications of your own for Xenomai, it is your
 * choice whether to permit this exception to apply to your
 * modifications. If you do not wish that, delete this exception
 * notice.
 */

#ifndef _vrtx_heap_h
#define _vrtx_heap_h

#include "vrtx/defs.h"

#define VRTX_HEAP_MAGIC 0x82820505

typedef struct vrtxuslt { /* Region unit slot */

    unsigned prev : 15;
    unsigned next : 15;
    unsigned busy : 1;
    unsigned heading : 1;
    unsigned units;

} vrtxuslt_t;

#define heap_align_mask   (sizeof(vrtxuslt_t)-1)

typedef struct vrtxheap {

    unsigned magic;   /* Magic code - must be first */

    xnholder_t link;  /* Link in vrtxheapq */

#define link2vrtxheap(laddr) \
((vrtxheap_t *)(((char *)laddr) - (int)(&((vrtxheap_t *)0)->link)))

    xnsynch_t synchbase;

    u_long log2psize;	/* Aligned allocation unit size */

    u_long allocated;	/* count of allocated blocks */

    u_long released;	/* count of allocated then released blocks */

    xnheap_t sysheap;	/* Xenomai heap */

} vrtxheap_t;

#ifdef __cplusplus
extern "C" {
#endif

int vrtxheap_init(u_long heap0size);

void vrtxheap_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* !_vrtx_heap_h */