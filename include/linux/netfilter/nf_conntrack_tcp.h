#ifndef _NF_CONNTRACK_TCP_H
#define _NF_CONNTRACK_TCP_H

#include <uapi/linux/netfilter/nf_conntrack_tcp.h>


struct ip_ct_tcp_state {
//����ֵ���Ǹ�����Ӧ������̬�仯��
//֮���Զ�Ҫ��¼���ֵ������Ϊ���ݰ���
//�����������Ϊ�˷�ֹ����Ķ���
//�Ϸ������ݰ���ʹ�����ֵ������
//ϵͳ���ݴ�����


//�������е�ǰ��ЧACK�߽��ȷ����
//��ΪA������Ϊ��δ�յ������ݽ���ȷ�ϣ�
//���Ա����е�ACK�����ܴ��������յ����ĵ����SEQ��
//������ЧACK������Ϊ��
//A ��ack <= B ��max{ seq + len}  
//��¼�������������͵�������ݰ��������к�ֵ
	u_int32_t	td_end;		/* max of seq + len */
//��¼�����������յ��������Խ��յ�ack �ֽ����к�ֵ
	u_int32_t	td_maxend;	/* max of ack + max(win, 1) */
//��¼�����������յ����ͨ�洰��ֵ
	u_int32_t	td_maxwin;	/* max(win) */
//��¼�����������յ��������Чackֵ
	u_int32_t	td_maxack;	/* max of ack */
//ͨ�洰����չ����
	u_int8_t	td_scale;	/* window scale factor */
	u_int8_t	flags;		/* per direction options */
};

struct ip_ct_tcp {
	struct ip_ct_tcp_state seen[2];	/* connection parameters per direction */
	u_int8_t	state;		/* state of the connection (enum tcp_conntrack) */
	/* For detecting stale connections */
	u_int8_t	last_dir;	/* Direction of the last packet (enum ip_conntrack_dir) */
	u_int8_t	retrans;	/* Number of retransmitted packets */
	u_int8_t	last_index;	/* Index of the last packet */
	u_int32_t	last_seq;	/* Last sequence number seen in dir */
	u_int32_t	last_ack;	/* Last sequence number seen in opposite dir */
	u_int32_t	last_end;	/* Last seq + len */
	u_int16_t	last_win;	/* Last window advertisement seen in dir */
	/* For SYN packets while we may be out-of-sync */
	u_int8_t	last_wscale;	/* Last window scaling factor seen */
	u_int8_t	last_flags;	/* Last flags set */
};

#endif /* _NF_CONNTRACK_TCP_H */
