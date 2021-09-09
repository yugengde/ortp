// 说明
/*
	设置一个缓冲队列。
	1. 将抓包的数据存在缓冲队列中
	2. 每次取出最小的数据存储到磁盘中
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
