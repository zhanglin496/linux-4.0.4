/*
 * Today's hack: quantum tunneling in structs
 *
 * 'entries' and 'term' are never anywhere referenced by word in code. In fact,
 * they serve as the hanging-off data accessed through repl.data[].
 */

/* tbl has the following structure equivalent, but is C99 compliant:
 * struct {
 *	struct type##_replace repl;
 *	struct type##_standard entries[nhooks];
 *	struct type##_error term;
 * } *tbl;
 */

#if 0
//���� xt_alloc_initial_table(ipt, IPT); չ��������

 unsigned int hook_mask = info->valid_hooks;
//����hook_mask��bitΪ1�ĸ���
 unsigned int nhooks = hweight32(hook_mask);
 unsigned int bytes = 0, hooknum = 0, i = 0;
 struct {
	   struct ipt_replace repl;
	   struct ipt_standard entries[];
 } *tbl;
 struct ipt_error *term;
 //����termƫ�ƣ��ж��ٸ�nhooks���ͷ�����ٸ�ipt_standard
 size_t term_offset = (offsetof(typeof(*tbl), entries[nhooks]) + __alignof__(*term) - 1) & ~(__alignof__(*term) - 1);
 //������ڴ�
 tbl = kzalloc(term_offset + sizeof(*term), GFP_KERNEL);
 if (tbl == NULL)
		 return NULL;
 term = (struct ipt_error *)&(((char *)tbl)[term_offset]);
 //����������ƣ�����filter,nat,raw,mangle,
 strncpy(tbl->repl.name, info->name, sizeof(tbl->repl.name));
 //��ʼ��ipt_error��ipt_errorλ�ڱ��ĩβ
 *term = (struct ipt_error)IPT_ERROR_INIT;
 tbl->repl.valid_hooks = hook_mask;
 tbl->repl.num_entries = nhooks + 1;
 //size ������ipt_replace�Ĵ�С��ֻ��������Ĵ�С
 tbl->repl.size = nhooks * sizeof(struct ipt_standard) + sizeof(struct ipt_error);
 for (; hook_mask != 0; hook_mask >>= 1, ++hooknum) {
		 if (!(hook_mask & 1))
				 continue;
		 //hooknum �� hook_mask�����Ӧ��
		 //hook_mask bit0Ϊ1����hooknumΪ0
		 //hook_mask bit1Ϊ1����hooknumΪ1
		 //��¼ÿ��hook���ipt_standard��ƫ��
		 tbl->repl.hook_entry[hooknum] = bytes;
		 tbl->repl.underflow[hooknum] = bytes;
		 //��ʼ��ipt_standard, Ĭ�϶���NF_ACCEPT
		 tbl->entries[i++] = (struct ipt_standard) IPT_STANDARD_INIT(NF_ACCEPT);
		 bytes += sizeof(struct ipt_standard); 
 }
}
//tbl��ĳ�ʼ�ڴ沼������
----------------------------------------------------------
ipt_replace   | ipt_standard  | ipt_standard | ipt_error |
----------------------------------------------------------
#endif

#define xt_alloc_initial_table(type, typ2) ({ \
	unsigned int hook_mask = info->valid_hooks; \
	unsigned int nhooks = hweight32(hook_mask); \
	unsigned int bytes = 0, hooknum = 0, i = 0; \
	struct { \
		struct type##_replace repl; \
		struct type##_standard entries[]; \
	} *tbl; \
	struct type##_error *term; \
	size_t term_offset = (offsetof(typeof(*tbl), entries[nhooks]) + \
		__alignof__(*term) - 1) & ~(__alignof__(*term) - 1); \
	tbl = kzalloc(term_offset + sizeof(*term), GFP_KERNEL); \
	if (tbl == NULL) \
		return NULL; \
	term = (struct type##_error *)&(((char *)tbl)[term_offset]); \
	strncpy(tbl->repl.name, info->name, sizeof(tbl->repl.name)); \
	*term = (struct type##_error)typ2##_ERROR_INIT;  \
	tbl->repl.valid_hooks = hook_mask; \
	tbl->repl.num_entries = nhooks + 1; \
	tbl->repl.size = nhooks * sizeof(struct type##_standard) + \
			 sizeof(struct type##_error); \
	for (; hook_mask != 0; hook_mask >>= 1, ++hooknum) { \
		if (!(hook_mask & 1)) \
			continue; \
		tbl->repl.hook_entry[hooknum] = bytes; \
		tbl->repl.underflow[hooknum]  = bytes; \
		tbl->entries[i++] = (struct type##_standard) \
			typ2##_STANDARD_INIT(NF_ACCEPT); \
		bytes += sizeof(struct type##_standard); \
	} \
	tbl; \
})
