
//�ж����ɶ���������ascii����� 0~9 A~F a~f
//��Ӧ��ֵ�����������ģ�
#define isxdigit(x) (isupper(x) || islower(x))
#define isdigit(x)	((x) >= '0' && (x) <= '9')
#define islower(x) ((x) >= 'a') && (x) <= 'f')
#define isupper(x) ((x) >= 'A') && (x) <= 'F')
//A~F ��a~f ֮��Ĳ�ֵ�ǹ̶�
#define toupper(x) (islower(x) ? (x) - ('a' - 'A') : (x))
#define tolower(x) (isupper(x) ? (x) + ('a' - 'A') : (x))



