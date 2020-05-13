#ifndef _NETFILTER_NF_NAT_H
#define _NETFILTER_NF_NAT_H

#include <linux/netfilter.h>
#include <linux/netfilter/nf_conntrack_tuple_common.h>

//��ʾ�Ƿ���Ҫ��IP��ַת��
//������Դ��ַ��Ŀ�ĵ�ַת��
//��������Ҫ��IP��ַӳ�䣬��IP��ַӳ�䵽ָ���ķ�Χ��
//��Ҫ���IP��ַ�Ƿ������õķ�Χ��
//���û�����øñ�־��natģ�齫�����޸����ݰ���IP��ַ
//ͬʱҲ�����޸�conntrack reply tuple �е�IP ��ַ
#define NF_NAT_RANGE_MAP_IPS			(1 << 0)
//�û��Ƿ�ͨ��iptables ����ʱָ���˶˿ڷ�Χ
//����MASQURADE ģ���--to-portsѡ��
//�ں˽��յ�����ʱ����struct nf_nat_range ���������õĶ˿ڷ�Χ
//�ñ�־����˼�û����ù���ʱָ���˶˿�ѡ��ķ�Χ
//��ˣ��ں���Ҫ��������
//1. ��Ҫ������ݰ���ԭʼ�˿��Ƿ������õķ�Χ��
//2. �����û��������ڷ�Χ��ѡ����ʵĶ˿�
#define NF_NAT_RANGE_PROTO_SPECIFIED		(1 << 1)
//������ɶ˿�ƫ��ֵ��Ӱ��˿�ѡ��
#define NF_NAT_RANGE_PROTO_RANDOM		(1 << 2)
//��ҪӰ��find_best_ips_proto IP��ַѡ��ʱ���㷨
//��ͬ��ԴIP��ַ����ӳ�䵽ͬһ��IP��ַ
#define NF_NAT_RANGE_PERSISTENT			(1 << 3)
//����prandom_u32�������һ��offֵ��Ӱ��˿�ѡ��
#define NF_NAT_RANGE_PROTO_RANDOM_FULLY		(1 << 4)
//��Ҫ����l4proto->unique_tuple��������ɶ˿�
#define NF_NAT_RANGE_PROTO_RANDOM_ALL		\
	(NF_NAT_RANGE_PROTO_RANDOM | NF_NAT_RANGE_PROTO_RANDOM_FULLY)

#define NF_NAT_RANGE_MASK					\
	(NF_NAT_RANGE_MAP_IPS | NF_NAT_RANGE_PROTO_SPECIFIED |	\
	 NF_NAT_RANGE_PROTO_RANDOM | NF_NAT_RANGE_PERSISTENT |	\
	 NF_NAT_RANGE_PROTO_RANDOM_FULLY)

struct nf_nat_ipv4_range {
	unsigned int			flags;
	__be32				min_ip;
	__be32				max_ip;
	union nf_conntrack_man_proto	min;
	union nf_conntrack_man_proto	max;
};

struct nf_nat_ipv4_multi_range_compat {
	unsigned int			rangesize;
	struct nf_nat_ipv4_range	range[1];
};

struct nf_nat_range {
	unsigned int			flags;
	//���õ�ipv4/ipv6��ַ��Χ
	union nf_inet_addr		min_addr;
	union nf_inet_addr		max_addr;
	//����udp��tcp��˵ָ�����õĶ˿ں���Сֵ�����ֵ
	union nf_conntrack_man_proto	min_proto;
	union nf_conntrack_man_proto	max_proto;
};

#endif /* _NETFILTER_NF_NAT_H */
