#ifndef __LINUX_SPINLOCK_API_SMP_H
#define __LINUX_SPINLOCK_API_SMP_H

#ifndef __LINUX_SPINLOCK_H
# error "please don't include this file directly"
#endif

/*
 * include/linux/spinlock_api_smp.h
 *
 * spinlock API declarations on SMP (and debug)
 * (implemented in kernel/spinlock.c)
 *
 * portions Copyright 2005, Red Hat, Inc., Ingo Molnar
 * Released under the General Public License (GPL).
 */

int in_lock_functions(unsigned long addr);

#define assert_raw_spin_locked(x)	BUG_ON(!raw_spin_is_locked(x))

void __lockfunc _raw_spin_lock(raw_spinlock_t *lock)		__acquires(lock);
void __lockfunc _raw_spin_lock_nested(raw_spinlock_t *lock, int subclass)
								__acquires(lock);
void __lockfunc _raw_spin_lock_bh_nested(raw_spinlock_t *lock, int subclass)
								__acquires(lock);
void __lockfunc
_raw_spin_lock_nest_lock(raw_spinlock_t *lock, struct lockdep_map *map)
								__acquires(lock);
void __lockfunc _raw_spin_lock_bh(raw_spinlock_t *lock)		__acquires(lock);
void __lockfunc _raw_spin_lock_irq(raw_spinlock_t *lock)
								__acquires(lock);

unsigned long __lockfunc _raw_spin_lock_irqsave(raw_spinlock_t *lock)
								__acquires(lock);
unsigned long __lockfunc
_raw_spin_lock_irqsave_nested(raw_spinlock_t *lock, int subclass)
								__acquires(lock);
int __lockfunc _raw_spin_trylock(raw_spinlock_t *lock);
int __lockfunc _raw_spin_trylock_bh(raw_spinlock_t *lock);
void __lockfunc _raw_spin_unlock(raw_spinlock_t *lock)		__releases(lock);
void __lockfunc _raw_spin_unlock_bh(raw_spinlock_t *lock)	__releases(lock);
void __lockfunc _raw_spin_unlock_irq(raw_spinlock_t *lock)	__releases(lock);
void __lockfunc
_raw_spin_unlock_irqrestore(raw_spinlock_t *lock, unsigned long flags)
								__releases(lock);

#ifdef CONFIG_INLINE_SPIN_LOCK
#define _raw_spin_lock(lock) __raw_spin_lock(lock)
#endif

#ifdef CONFIG_INLINE_SPIN_LOCK_BH
#define _raw_spin_lock_bh(lock) __raw_spin_lock_bh(lock)
#endif

#ifdef CONFIG_INLINE_SPIN_LOCK_IRQ
#define _raw_spin_lock_irq(lock) __raw_spin_lock_irq(lock)
#endif

#ifdef CONFIG_INLINE_SPIN_LOCK_IRQSAVE
#define _raw_spin_lock_irqsave(lock) __raw_spin_lock_irqsave(lock)
#endif

#ifdef CONFIG_INLINE_SPIN_TRYLOCK
#define _raw_spin_trylock(lock) __raw_spin_trylock(lock)
#endif

#ifdef CONFIG_INLINE_SPIN_TRYLOCK_BH
#define _raw_spin_trylock_bh(lock) __raw_spin_trylock_bh(lock)
#endif

#ifndef CONFIG_UNINLINE_SPIN_UNLOCK
#define _raw_spin_unlock(lock) __raw_spin_unlock(lock)
#endif

#ifdef CONFIG_INLINE_SPIN_UNLOCK_BH
#define _raw_spin_unlock_bh(lock) __raw_spin_unlock_bh(lock)
#endif

#ifdef CONFIG_INLINE_SPIN_UNLOCK_IRQ
#define _raw_spin_unlock_irq(lock) __raw_spin_unlock_irq(lock)
#endif

#ifdef CONFIG_INLINE_SPIN_UNLOCK_IRQRESTORE
#define _raw_spin_unlock_irqrestore(lock, flags) __raw_spin_unlock_irqrestore(lock, flags)
#endif

static inline int __raw_spin_trylock(raw_spinlock_t *lock)
{
	preempt_disable();
	if (do_raw_spin_trylock(lock)) {
		spin_acquire(&lock->dep_map, 0, 1, _RET_IP_);
		return 1;
	}
	preempt_enable();
	return 0;
}

/*
 * If lockdep is enabled then we use the non-preemption spin-ops
 * even on CONFIG_PREEMPT, because lockdep assumes that interrupts are
 * not re-enabled during lock-acquire (which the preempt-spin-ops do):
 */
#if !defined(CONFIG_GENERIC_LOCKBREAK) || defined(CONFIG_DEBUG_LOCK_ALLOC)

static inline unsigned long __raw_spin_lock_irqsave(raw_spinlock_t *lock)
{
	unsigned long flags;

	local_irq_save(flags);
	preempt_disable();
	spin_acquire(&lock->dep_map, 0, 0, _RET_IP_);
	/*
	 * On lockdep we dont want the hand-coded irq-enable of
	 * do_raw_spin_lock_flags() code, because lockdep assumes
	 * that interrupts are not re-enabled during lock-acquire:
	 */
#ifdef CONFIG_LOCKDEP
	LOCK_CONTENDED(lock, do_raw_spin_trylock, do_raw_spin_lock);
#else
	do_raw_spin_lock_flags(lock, &flags);
#endif
	return flags;
}

//tip2���ڹرձ����жϺ��Ƿ��б�Ҫ�ر���ռ��
//    ǰ���������ѷ�����Ϣ�ʣ���...���о���������ʱ��
//������ spin_lock_irq������Ҳ�������������йر��е�����
//�����У���Ȼ�Ѿ��ر��˱����жϣ��ٽ�ֹ��ռ��û��
//���ࡣҲ����˵����Ȼ�����ж��Ѿ���ֹ�ˣ��ڱ�����
//�������޷�����ϵģ����ص�����Ҳ�޷����У�Ҳ�Ͳ�
//���Ա����ص��ȳ�����ȳ�ȥ..."
//��spinlock���ԭ����ʹ������ʱ�����ٽ����������
//ȷ�����ᷢ�������л������ڵ������ǣ�����Ѿ��ر�
//���жϣ���ͬһ��������������ص��ں���ռ�����ԣ�
//�᲻���н��̵��ȵ�������������û�У����Ҹ��˵�
//����ǣ���local_irq_disable֮����ʹ��peempt_disable�Ͷ��һ���ˡ�
//�����SMPϵͳ���������ˣ�������A��B������������
//ʹ��spin lock�Ľ���(���"�������"����)�����ڴ�����A�ϣ�
//һ�ֺ����Ե����ξ�������и����̣���ơ�˯�߽��̡����ˣ�
//���ڽ������У�������Ϊ�ȴ�������һ�����ݰ�����������sleep״̬��
//Ȼ�󽹵㿪ʼ���������У�������spin lock�����������ٽ�����
//��ʱ�����յ���"˯�߽��̡������ݰ�����Ϊ����ֻ�ǹر���A��
//���жϣ�����B���ǻ���ղ�������жϣ�Ȼ���ѡ�˯�߽��̡���
//���߽������ж��У���ʱ����һ�����ȵ㣬�����˯�ߡ������ȼ����ڡ����㡰��
//��ô���н����л������ˣ��������������ʹ�õ�spin lock�йر�
//���ں���ռ����ô��ʹ����ǰ�Ľ����л���Ϊ�����ܡ�
//     ������ڵ�������ϵͳ�ϣ�local_irq_disableʵ���Ϲر�������
//����ʵ��һ�������������жϣ��������ж�����ĵ��ȵ�
//�������ܴ��ڣ���ʱ�����������ж��޹صĵ��ȵ�����أ�
//��2.4�ϣ���Ϊû����ռ���������ξ��޿��ܣ���ʵ�ϣ�����
//���ں˺ܴ�̶���������local_irq_disable������Դ�����������
//��2.4���ں�Դ��ͺ�����ˣ������д����Ķ�local_irq_disable
//������ֱ�ӵ��á� 
//     2.6������ռ�ĸ��local_irq_save�Ⱥ���ֻ�ǽ�ֹ�˱����жϣ�
//����ǰCPU�ϵ��жϡ��ڵ���CPU�ϣ���Ȼ��ռ�Ͳ����ܷ����ˣ�
//�����ڶ��CPU�������������ϵ��жϲ�û�б���ֹ��
//����Ȼ���ܷ�����ռ�ģ�����CPU�ڲ��ᱻ��ռ��UP�¹ر��жϣ�
//��ǰ������ʵ�����Ѿ��ž����ڲ����ص��µġ�����������
//����һ�����̡�������ȵ�Ŀ���
///���ڲ�����ʵ����ֻʣ����һ�����������쳣��
//���ǹ��жϵ������£��������쳣Ҳ���ᵼ�½��̵��л�����
//��˵��������ǿ�������˵����UP�Ϲر��ж������£�
//preempt_disable��ʵ�Ƕ���ġ���������֪����spin lock��һ���ں�API��
//��ֻ��kernel�Ŀ��������ã�������ں�ģ��
//(.ko��ʵ�ʵ��и���ر�����ʽ���豸��������)������Ҳ��ʹ�á�
//�ں˵������������ͼ���䲻�ܿصĴ��루��ν���ⲿ�����ˣ�
//���ܸ��ں˴�������ʧ��������С�ĳ̶ȣ�����������ں�
//���жϴ����ܵ����ʱ�������ԣ�������UPϵͳ���Ⱥ�
//ʹ��local_disable_irq��preempt_disable��ֻ�Ǿ��������ҿ�����
//spin lock/unlock���ٽ�����ĳЩ����ͷ�Ĵ��벻���ڸ�ϵͳ�������ѣ�
//��Ϊ�ѱ�ĳЩ�˲�����spin lock���ٽ�����,����ȥwake_up_interruptible����
//һ�����̣��������ѵĽ����ڿ���ռ��ϵͳ�����һ����
//���˶������ӡ�
static inline void __raw_spin_lock_irq(raw_spinlock_t *lock)
{
	local_irq_disable();
	preempt_disable();
	spin_acquire(&lock->dep_map, 0, 0, _RET_IP_);
	LOCK_CONTENDED(lock, do_raw_spin_trylock, do_raw_spin_lock);
}

static inline void __raw_spin_lock_bh(raw_spinlock_t *lock)
{
	__local_bh_disable_ip(_RET_IP_, SOFTIRQ_LOCK_OFFSET);
	spin_acquire(&lock->dep_map, 0, 0, _RET_IP_);
	LOCK_CONTENDED(lock, do_raw_spin_trylock, do_raw_spin_lock);
}

static inline void __raw_spin_lock(raw_spinlock_t *lock)
{
	preempt_disable();
	spin_acquire(&lock->dep_map, 0, 0, _RET_IP_);
	LOCK_CONTENDED(lock, do_raw_spin_trylock, do_raw_spin_lock);
}

#endif /* !CONFIG_GENERIC_LOCKBREAK || CONFIG_DEBUG_LOCK_ALLOC */

static inline void __raw_spin_unlock(raw_spinlock_t *lock)
{
	spin_release(&lock->dep_map, 1, _RET_IP_);
	do_raw_spin_unlock(lock);
	preempt_enable();
}

static inline void __raw_spin_unlock_irqrestore(raw_spinlock_t *lock,
					    unsigned long flags)
{
	spin_release(&lock->dep_map, 1, _RET_IP_);
	do_raw_spin_unlock(lock);
	local_irq_restore(flags);
	preempt_enable();
}

static inline void __raw_spin_unlock_irq(raw_spinlock_t *lock)
{
	spin_release(&lock->dep_map, 1, _RET_IP_);
	do_raw_spin_unlock(lock);
	local_irq_enable();
	preempt_enable();
}

static inline void __raw_spin_unlock_bh(raw_spinlock_t *lock)
{
	spin_release(&lock->dep_map, 1, _RET_IP_);
	do_raw_spin_unlock(lock);
	__local_bh_enable_ip(_RET_IP_, SOFTIRQ_LOCK_OFFSET);
}

static inline int __raw_spin_trylock_bh(raw_spinlock_t *lock)
{
	__local_bh_disable_ip(_RET_IP_, SOFTIRQ_LOCK_OFFSET);
	if (do_raw_spin_trylock(lock)) {
		spin_acquire(&lock->dep_map, 0, 1, _RET_IP_);
		return 1;
	}
	__local_bh_enable_ip(_RET_IP_, SOFTIRQ_LOCK_OFFSET);
	return 0;
}

#include <linux/rwlock_api_smp.h>

#endif /* __LINUX_SPINLOCK_API_SMP_H */
