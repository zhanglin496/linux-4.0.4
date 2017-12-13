#ifndef LINUX_PREEMPT_MASK_H
#define LINUX_PREEMPT_MASK_H

#include <linux/preempt.h>

/*
 * We put the hardirq and softirq counter into the preemption
 * counter. The bitmask has the following meaning:
 *
 * - bits 0-7 are the preemption count (max preemption depth: 256)
 * - bits 8-15 are the softirq count (max # of softirqs: 256)
 *
 * The hardirq count could in theory be the same as the number of
 * interrupts in the system, but we run all interrupt handlers with
 * interrupts disabled, so we cannot have nesting interrupts. Though
 * there are a few palaeontologic drivers which reenable interrupts in
 * the handler, so we need more than one bit here.
 *
 * PREEMPT_MASK:	0x000000ff
 * SOFTIRQ_MASK:	0x0000ff00
 * HARDIRQ_MASK:	0x000f0000
 *     NMI_MASK:	0x00100000
 * PREEMPT_ACTIVE:	0x00200000
 */
#define PREEMPT_BITS	8
#define SOFTIRQ_BITS	8
#define HARDIRQ_BITS	4
#define NMI_BITS	1

#define PREEMPT_SHIFT	0
//8
#define SOFTIRQ_SHIFT	(PREEMPT_SHIFT + PREEMPT_BITS)
//16
#define HARDIRQ_SHIFT	(SOFTIRQ_SHIFT + SOFTIRQ_BITS)
//20
#define NMI_SHIFT	(HARDIRQ_SHIFT + HARDIRQ_BITS)

// (1 << 8 ) -1  ���ɵ�λȫ1 ������
#define __IRQ_MASK(x)	((1UL << (x))-1)

//��ռ����ֵ����			       	    11111111
#define PREEMPT_MASK	(__IRQ_MASK(PREEMPT_BITS) << PREEMPT_SHIFT)
//���жϼ���ֵ����		    1111111100000000
#define SOFTIRQ_MASK	(__IRQ_MASK(SOFTIRQ_BITS) << SOFTIRQ_SHIFT)
//Ӳ�жϼ���ֵ����	    11110000000000000000
#define HARDIRQ_MASK	(__IRQ_MASK(HARDIRQ_BITS) << HARDIRQ_SHIFT)
//NMI �жϼ���ֵ����  100000000000000000000
#define NMI_MASK	(__IRQ_MASK(NMI_BITS)     << NMI_SHIFT)
// 2^0
#define PREEMPT_OFFSET	(1UL << PREEMPT_SHIFT)
// 2^8
#define SOFTIRQ_OFFSET	(1UL << SOFTIRQ_SHIFT)
// 2^16
#define HARDIRQ_OFFSET	(1UL << HARDIRQ_SHIFT)
// 2^20
#define NMI_OFFSET	(1UL << NMI_SHIFT)


/*
 * preempt_count and SOFTIRQ_OFFSET usage:
 * - preempt_count is changed by SOFTIRQ_OFFSET on entering or leaving
 *   softirq processing.
 * - preempt_count is changed by SOFTIRQ_DISABLE_OFFSET (= 2 * SOFTIRQ_OFFSET)
 *   on local_bh_disable or local_bh_enable.
 * This lets us distinguish between whether we are currently processing
 * softirq and whether we just have bh disabled.
 */
#define SOFTIRQ_DISABLE_OFFSET	(2 * SOFTIRQ_OFFSET)

#define PREEMPT_ACTIVE_BITS	1
// 21
#define PREEMPT_ACTIVE_SHIFT	(NMI_SHIFT + NMI_BITS)
// 1000000000000000000000
#define PREEMPT_ACTIVE	(__IRQ_MASK(PREEMPT_ACTIVE_BITS) << PREEMPT_ACTIVE_SHIFT)
//Ӳ�жϼ���ֵ
#define hardirq_count()	(preempt_count() & HARDIRQ_MASK)
//���жϼ���ֵ
#define softirq_count()	(preempt_count() & SOFTIRQ_MASK)
#define irq_count()	(preempt_count() & (HARDIRQ_MASK | SOFTIRQ_MASK \
				 | NMI_MASK))

/*
 * Are we doing bottom half or hardware interrupt processing?
 * Are we in a softirq context? Interrupt context?
 * in_softirq - Are we currently processing softirq or have bh disabled?
 * in_serving_softirq - Are we currently processing softirq?
 */
#define in_irq()		(hardirq_count())
//���жϼ���
#define in_softirq()		(softirq_count())
//in_interrupt ������Ӳ�жϣ����жϣ�NMI�ж�
#define in_interrupt()		(irq_count())
//softirq context��û����ô��ֱ�ӣ�һ���˻���Ϊ��sofirq handler
//����ִ�е�ʱ�����softirq context������˵��Ȼû�д�
//sofirq handler����ִ�е�ʱ�򣬻�����softirq count��
//��Ȼ��softirq context��������������context������£�
//��������������У����п�����Ϊͬ����Ҫ�������
//local_bh_disable����ʱ��ͨ��local_bh_disable/enable��������
//�Ĵ���Ҳ��ִ����softirq context�С���Ȼ����ʱ����ʵ
//��û������ִ��softirq handler�������ȷʵ��֪����ǰ
//�Ƿ�����ִ��softirq handler��in_serving_softirq����������ʹ����
//����ͨ������preempt_count��bit 8����ɵ�
#define in_serving_softirq()	(softirq_count() & SOFTIRQ_OFFSET)

/*
 * Are we in NMI context?
 */
#define in_nmi()	(preempt_count() & NMI_MASK)

#if defined(CONFIG_PREEMPT_COUNT)
# define PREEMPT_CHECK_OFFSET 1
#else
# define PREEMPT_CHECK_OFFSET 0
#endif

/*
 * The preempt_count offset needed for things like:
 *
 *  spin_lock_bh()
 *
 * Which need to disable both preemption (CONFIG_PREEMPT_COUNT) and
 * softirqs, such that unlock sequences of:
 *
 *  spin_unlock();
 *  local_bh_enable();
 *
 * Work as expected.
 */
#define SOFTIRQ_LOCK_OFFSET (SOFTIRQ_DISABLE_OFFSET + PREEMPT_CHECK_OFFSET)

/*
 * Are we running in atomic context?  WARNING: this macro cannot
 * always detect atomic context; in particular, it cannot know about
 * held spinlocks in non-preemptible kernels.  Thus it should not be
 * used in the general case to determine whether sleeping is possible.
 * Do not use in_atomic() in driver code.
 */
#define in_atomic()	((preempt_count() & ~PREEMPT_ACTIVE) != 0)

/*
 * Check whether we were atomic before we did preempt_disable():
 * (used by the scheduler, *after* releasing the kernel lock)
 */
#define in_atomic_preempt_off() \
		((preempt_count() & ~PREEMPT_ACTIVE) != PREEMPT_CHECK_OFFSET)

#ifdef CONFIG_PREEMPT_COUNT
# define preemptible()	(preempt_count() == 0 && !irqs_disabled())
#else
# define preemptible()	0
#endif

#endif /* LINUX_PREEMPT_MASK_H */
