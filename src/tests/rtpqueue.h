// ˵��
/*
	����һ��������С�
	1. ��ץ�������ݴ��ڻ��������
	2. ÿ��ȡ����С�����ݴ洢��������
*/
struct __queue {

	__queue *next;
	__queue *prev;

	int size;
};

typedef __queue rtp_queue;


struct __voice_queue{
	char *ssrc;
	int payload;
	queue;
};

typedef __voice_queue voice_queue;
