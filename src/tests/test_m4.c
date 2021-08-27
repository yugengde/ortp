// 条件变量
// https://cloud.tencent.com/developer/article/1629561
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct msg {
	int num;
	struct msg *next;
};

struct msg *head = NULL;
struct msg *temp = NULL;


// 初始化
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t has_producer = PTHREAD_COND_INITIALIZER;

void *producer(void *arg) {

	while (1) {
	
		pthread_mutex_lock(&mutex);

		temp = malloc(sizeof(struct msg));
		temp->num = rand() % 100 + 1;
		temp->next = head;
		head = temp;
		printf("----producered----%d\n", temp->num);

		pthread_mutex_unlock(&mutex);
		pthread_cond_signal(&has_producer);
		usleep(rand() % 3000);
	}

	return NULL;
}

void *consumer(void *arg) {
	while (1) {
	
		pthread_mutex_lock(&mutex);
		while (head == NULL) {
		
			pthread_cond_wait(&has_producer, &mutex);
		}
		temp = head;
		head = temp->next;
		printf("-----------consumer----%d\n", temp->num);
		free(temp);

		temp = NULL;
		pthread_mutex_unlock(&mutex);
		usleep(rand() % 3000);
	}
	return NULL;
}

int main() {
	pthread_t ptid, ctid;
	srand(time(NULL));

	pthread_create(&ptid, NULL, producer, NULL);
	pthread_create(&ctid, NULL, consumer, NULL);

	pthread_join(ptid, NULL);
	pthread_join(ctid, NULL);

	return 0;
}
