/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2006 Netfilter Core Team <coreteam@netfilter.org>
 * (C) 2008 Patrick McHardy <kaber@trash.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/random.h>
#include <linux/netfilter.h>
#include <linux/export.h>

#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_nat_core.h>
#include <net/netfilter/nf_nat_l3proto.h>
#include <net/netfilter/nf_nat_l4proto.h>

bool nf_nat_l4proto_in_range(const struct nf_conntrack_tuple *tuple,
			     enum nf_nat_manip_type maniptype,
			     const union nf_conntrack_man_proto *min,
			     const union nf_conntrack_man_proto *max)
{
	__be16 port;

	if (maniptype == NF_NAT_MANIP_SRC)
		port = tuple->src.u.all;
	else
		port = tuple->dst.u.all;

	return ntohs(port) >= ntohs(min->all) &&
	       ntohs(port) <= ntohs(max->all);
}
EXPORT_SYMBOL_GPL(nf_nat_l4proto_in_range);

void nf_nat_l4proto_unique_tuple(const struct nf_nat_l3proto *l3proto,
				 struct nf_conntrack_tuple *tuple,
				 const struct nf_nat_range *range,
				 enum nf_nat_manip_type maniptype,
				 const struct nf_conn *ct,
				 u16 *rover)
{
	unsigned int range_size, min, i;
	__be16 *portptr;
	u_int16_t off;

	if (maniptype == NF_NAT_MANIP_SRC)
		portptr = &tuple->src.u.all;
	else
		portptr = &tuple->dst.u.all;

	/* If no range specified... */
	//����û�û��ָ���˿ڷ�Χ
	if (!(range->flags & NF_NAT_RANGE_PROTO_SPECIFIED)) {
		/* If it's dst rewrite, can't change port */
		//�����Ŀ�Ķ˿�ѡ��
		//�����ǽ�ֹ��
		//��Ϊ���ѡ��Ŀ�Ķ˿ں�
		//�Է����ܸ������ղ������ݰ�
		if (maniptype == NF_NAT_MANIP_DST)
			return;
		//����ΪʲôҪ����512��1024
		//����netfilter�Ĺٷ��ĵ�����Ϊ�Ѷ˿ڷֳ�������
		//
		//When this implicit source mapping occurs, ports are divided into three classes:
		
		//Ports below 512
		//Ports between 512 and 1023
		//Ports 1024 and above.
		//A port will never be implicitly mapped into a different class.
		//��ͬ��Ķ˿ڲ�����ӳ�䵽������
		//����ԭʼ���ݰ��Ķ˿�ֵ���綨ӳ���
		//�˿���Сֵ�����ֵ
		if (ntohs(*portptr) < 1024) {
			/* Loose convention: >> 512 is credential passing */
			if (ntohs(*portptr) < 512) {
				min = 1;
				range_size = 511 - min + 1;				
				//ӳ�䷶Χ[1, 511]
				//����[512, 600)֮��Ķ˿�û��ʹ��
			} else {
				min = 600;
				range_size = 1023 - min + 1; // = 424
				//range_size ��ʾ���ֵ����Сֵ�ľ���				
				//ӳ�䷶Χ[600, 1023]
			}
		} else {
			min = 1024;
			range_size = 65535 - 1024 + 1;
			//ӳ�䷶Χ[1024, 65535]
		}
	} else {
	//����û��Լ������˶˿ڷ�Χ	
	//����������ѡ��Ŀ�Ķ˿�
	//������Ϊ�����û��Լ�ѡ�������
	//�û��Լ�֪���Լ����ʲô
	//���Լ������Ҳ���û��Լ�����
		min = ntohs(range->min_proto.all);
		range_size = ntohs(range->max_proto.all) - min + 1;
	}
//I think use variable 'min' instead of number '1024' is better.

	if (range->flags & NF_NAT_RANGE_PROTO_RANDOM) {
		//����ip��ַ�������
		//����һ����ʱƫ��
		off = l3proto->secure_port(tuple, maniptype == NF_NAT_MANIP_SRC
						  ? tuple->dst.u.all
						  : tuple->src.u.all);
	} else if (range->flags & NF_NAT_RANGE_PROTO_RANDOM_FULLY) {
		//α�������һ��ƫ�ƣ���IP��ַ�޹�
		off = prandom_u32();
	} else {
		//ʹ����һ�ε�offֵ����һ��ѡ��˿�ʱ
		// �������min + off ƫ�ƿ�ʼѡ��
		off = *rover;
	}

	for (i = 0; ; ++off) {
		//��֤portptr��ָ���ķ�Χ��[min��min + range_size - 1]
		*portptr = htons(min + off % range_size);
		//����range_size ָ���Ĵ���������˿ڳ�ͻ��
		//Ҳֻ�ܾ�����Ϊ
		if (++i != range_size && nf_nat_used_tuple(tuple, ct))
			continue;
		//���������������˿�ѡ��
		//����Ҫ����off ƫ��ֵ
		if (!(range->flags & NF_NAT_RANGE_PROTO_RANDOM_ALL))
			*rover = off;
		return;
	}
}
EXPORT_SYMBOL_GPL(nf_nat_l4proto_unique_tuple);

#if IS_ENABLED(CONFIG_NF_CT_NETLINK)
int nf_nat_l4proto_nlattr_to_range(struct nlattr *tb[],
				   struct nf_nat_range *range)
{
	if (tb[CTA_PROTONAT_PORT_MIN]) {
		range->min_proto.all = nla_get_be16(tb[CTA_PROTONAT_PORT_MIN]);
		range->max_proto.all = range->min_proto.all;
		range->flags |= NF_NAT_RANGE_PROTO_SPECIFIED;
	}
	if (tb[CTA_PROTONAT_PORT_MAX]) {
		range->max_proto.all = nla_get_be16(tb[CTA_PROTONAT_PORT_MAX]);
		range->flags |= NF_NAT_RANGE_PROTO_SPECIFIED;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(nf_nat_l4proto_nlattr_to_range);
#endif
