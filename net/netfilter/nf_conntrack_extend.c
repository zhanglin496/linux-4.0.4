/* Structure dynamic extension infrastructure
 * Copyright (C) 2004 Rusty Russell IBM Corporation
 * Copyright (C) 2007 Netfilter Core Team <coreteam@netfilter.org>
 * Copyright (C) 2007 USAGI/WIDE Project <http://www.linux-ipv6.org>
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <net/netfilter/nf_conntrack_extend.h>

static struct nf_ct_ext_type __rcu *nf_ct_ext_types[NF_CT_EXT_NUM];
static DEFINE_MUTEX(nf_ct_ext_type_mutex);

void __nf_ct_ext_destroy(struct nf_conn *ct)
{
	unsigned int i;
	struct nf_ct_ext_type *t;
	struct nf_ct_ext *ext = ct->ext;

	for (i = 0; i < NF_CT_EXT_NUM; i++) {
		if (!__nf_ct_ext_exist(ext, i))
			continue;

		rcu_read_lock();
		t = rcu_dereference(nf_ct_ext_types[i]);

		/* Here the nf_ct_ext_type might have been unregisterd.
		 * I.e., it has responsible to cleanup private
		 * area in all conntracks when it is unregisterd.
		 */
		 
		//���ext�ڵ���__nf_ct_ext_destroy֮ǰ��ע��������ע���������ͷ�����
		//conntrack����չ��ص���Դ���ڴ���conntrack�������
		//ʵ������ò�Ҫ��̬ע����������Ҫ�������е�conntrack
		//�����������չ��destroy������move����Ϊ�յ������
		//��̬ע��û������
		if (t && t->destroy)
			t->destroy(ct);
		rcu_read_unlock();
	}
}
EXPORT_SYMBOL(__nf_ct_ext_destroy);

static void *
nf_ct_ext_create(struct nf_ct_ext **ext, enum nf_ct_ext_id id,
		 size_t var_alloc_len, gfp_t gfp)
{
	unsigned int off, len;
	struct nf_ct_ext_type *t;
	size_t alloc_size;

	rcu_read_lock();
	t = rcu_dereference(nf_ct_ext_types[id]);
	BUG_ON(t == NULL);
	off = ALIGN(sizeof(struct nf_ct_ext), t->align);
	len = off + t->len + var_alloc_len;
	alloc_size = t->alloc_size + var_alloc_len;
	rcu_read_unlock();

	*ext = kzalloc(alloc_size, gfp);
	if (!*ext)
		return NULL;

	(*ext)->offset[id] = off;
	(*ext)->len = len;

	return (void *)(*ext) + off;
}

void *__nf_ct_ext_add_length(struct nf_conn *ct, enum nf_ct_ext_id id,
			     size_t var_alloc_len, gfp_t gfp)
{
	struct nf_ct_ext *old, *new;
	int i, newlen, newoff;
	struct nf_ct_ext_type *t;
	
	//δȷ��״̬ʱ
	//ֻ������һ��skb�����ߣ�������־���
	//���conntrack�Ѿ���ȷ�ϣ�����������µ���չ��
	//��Ϊ��׼�ں��ڷ�����չ��ʱ��û�м���
	/* Conntrack must not be confirmed to avoid races on reallocation. */
	NF_CT_ASSERT(!nf_ct_is_confirmed(ct));
	
	//var_alloc_lenָ���ھ�̬ע��ʱ�Ĺ̶����ȵĻ�����
	//��Ҫ����Ķ��ⳤ�ȣ�����nf_ct_helper_ext_add�õ����������
	old = ct->ext;
	if (!old)
		return nf_ct_ext_create(&ct->ext, id, var_alloc_len, gfp);

	if (__nf_ct_ext_exist(old, id))
		return NULL;

	rcu_read_lock();
	t = rcu_dereference(nf_ct_ext_types[id]);
	BUG_ON(t == NULL);

	newoff = ALIGN(old->len, t->align);
	newlen = newoff + t->len + var_alloc_len;
	rcu_read_unlock();

	new = __krealloc(old, newlen, gfp);
	if (!new)
		return NULL;

	if (new != old) {
		for (i = 0; i < NF_CT_EXT_NUM; i++) {
			if (!__nf_ct_ext_exist(old, i))
				continue;

			rcu_read_lock();
			t = rcu_dereference(nf_ct_ext_types[i]);
			//Ϊʲô��Ҫ�ƶ�������
			//����Ϊ�����е�ʵ�������ָ����չ����ָ��
			//������չ���ĵ�ַ�����˱仯��������Ҫ
			//һ�ַ�����������ָ����չ����ָ�뵽
			//��ȷ�ĵ�ַ
			if (t && t->move)
				t->move((void *)new + new->offset[i],
					(void *)old + old->offset[i]);
			rcu_read_unlock();
		}
		kfree_rcu(old, rcu);
		ct->ext = new;
	}

	new->offset[id] = newoff;
	new->len = newlen;
	//�����·������չ��
	memset((void *)new + newoff, 0, newlen - newoff);
	return (void *)new + newoff;
}
EXPORT_SYMBOL(__nf_ct_ext_add_length);

static void update_alloc_size(struct nf_ct_ext_type *type)
{
	int i, j;
	struct nf_ct_ext_type *t1, *t2;
	enum nf_ct_ext_id min = 0, max = NF_CT_EXT_NUM - 1;

	/* unnecessary to update all types */
	//���ûָ��NF_CT_EXT_F_PREALLOC
	//����Ҫ���������Ѿ�ע��type��alloc_size��С�����ǿ��ܻ���������alloc_size��
	//��Ϊ֮ǰע���type���������˱�־NF_CT_EXT_F_PREALLOC��������Ҫ���¼����С
	//���ָ���˱�־NF_CT_EXT_F_PREALLOC��������������ע��type��alloc_size���������չ��ʱ��
	//�ͻ�һ���Է������NF_CT_EXT_F_PREALLOC��־type�����ܵ���չ�ռ䣬
	//���������չ��ʱ��Ͳ���Ҫ�����·���ռ䣬��Ϊ�ռ��Ѿ���ǰ������ˣ�
	//�ô��ǿ��Ա���realloc�������ǿ��ܻ��˷ѿռ�
	if ((type->flags & NF_CT_EXT_F_PREALLOC) == 0) {
		min = type->id;
		max = type->id;
	}

	/* This assumes that extended areas in conntrack for the types
	   whose NF_CT_EXT_F_PREALLOC bit set are allocated in order */
	for (i = min; i <= max; i++) {
		t1 = rcu_dereference_protected(nf_ct_ext_types[i],
				lockdep_is_held(&nf_ct_ext_type_mutex));
		//������չ���ͻ�û��ע��
		if (!t1)
			continue;

		t1->alloc_size = ALIGN(sizeof(struct nf_ct_ext), t1->align) +
				 t1->len;
				 
		//����������ע�����չ
		for (j = 0; j < NF_CT_EXT_NUM; j++) {
			t2 = rcu_dereference_protected(nf_ct_ext_types[j],
				lockdep_is_held(&nf_ct_ext_type_mutex));
			//t2û����NF_CT_EXT_F_PREALLOC��־���Ͳ������alloc_size
			//Ŀǰֻ��nat_extend�����˸ñ�־
			//ע��t2==t1��������ǲ��ܸ���alloc_size
			//NF_CT_EXT_F_PREALLOC��Ŀ�����ڷ����������͵���չʱ���ѵ�ǰע��Ŀռ������ȥ
			//������t1��t2��t3�������ͣ�ֻ��t3������NF_CT_EXT_F_PREALLOC��
			//��ô����t1��t2ʱ���t3�Ŀռ������ȥ�����Ƿ���t3ʱֻ����t3����Ĵ�С
			//�������t1��t2Ҳ������NF_CT_EXT_F_PREALLOC��
			//���������͵�alloc_size��С��Ϊ��t1+t2+t3��
			//���t1��t2���ö�t3û����NF_CT_EXT_F_PREALLOC
			//��ôt1=(t1+t2),t2=(t2+t1),t3=(t3+t1+t2)
			//Ҳ����˵��չ��alloc_sizeΪ�����������������NF_CT_EXT_F_PREALLOC����չ��С
			if (t2 == NULL || t2 == t1 ||
			    (t2->flags & NF_CT_EXT_F_PREALLOC) == 0)
				continue;
			//���t2������NF_CT_EXT_F_PREALLOC��־������Ҫ����t1��alloc_size
			//�ۼ���Ҫ������ܴ�С��t1��
			t1->alloc_size = ALIGN(t1->alloc_size, t2->align)
					 + t2->len;
		}
	}
}

/* This MUST be called in process context. */
int nf_ct_extend_register(struct nf_ct_ext_type *type)
{
	int ret = 0;

	mutex_lock(&nf_ct_ext_type_mutex);
	//δ��Խ���⣬����ں�δ����ģ��crcУ�� 
	//��Ǳ�ڵ�Խ������
	if (nf_ct_ext_types[type->id]) {
		ret = -EBUSY;
		goto out;
	}

	/* This ensures that nf_ct_ext_create() can allocate enough area
	   before updating alloc_size */
	type->alloc_size = ALIGN(sizeof(struct nf_ct_ext), type->align)
			   + type->len;
	rcu_assign_pointer(nf_ct_ext_types[type->id], type);
	update_alloc_size(type);
out:
	mutex_unlock(&nf_ct_ext_type_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(nf_ct_extend_register);

/* This MUST be called in process context. */
void nf_ct_extend_unregister(struct nf_ct_ext_type *type)
{
	mutex_lock(&nf_ct_ext_type_mutex);
	RCU_INIT_POINTER(nf_ct_ext_types[type->id], NULL);
	update_alloc_size(type);
	mutex_unlock(&nf_ct_ext_type_mutex);
	rcu_barrier(); /* Wait for completion of call_rcu()'s */
}
EXPORT_SYMBOL_GPL(nf_ct_extend_unregister);
