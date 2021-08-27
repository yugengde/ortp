// 信号量
#include <semaphore.h>
#include <stdio.h>
#include <pthread.h>

#define MAXNUM 2

sem_t semDownload;

pthread_t a_thread, b_thread, c_thread;

int g_phreadNum = 1;

void InputInfo(void) {

	printf("*****************************************\n");
	printf("**** which task you want to download? ***\n");
	printf("***  you can enter [1-3], [0] is done ***\n");
	printf("*****************************************\n");

}

void *func1(void *arg) {

	sem_wait(&semDownload);
	printf("======================== download task 1 ================\n");
	sleep(5);
	g_phreadNum--;

	pthread_join(a_thread, NULL);
}

void *func2(void *arg) {

	sem_wait(&semDownload);
	printf("====================== downloading task 2 ===================\n");
	sleep(3);
	printf("====================== finished task 2 ======================\n");
	g_phreadNum--;
	pthread_join(b_thread, NULL);
}

void *func3(void *arg) {

	sem_wait(&semDownload);
	printf("==================== Download Task 3 =======================\n");
	sleep(5);
	printf("==================== Finished Task 3 =======================\n");
	g_phreadNum--;
	pthread_join(c_thread, NULL);
}

int main() {

	int taskNum;
	InputInfo();

	while (scanf("%d", &taskNum) != EOF) {
	
		if (taskNum == 0 && g_phreadNum <= 1) {
			break;
		}
		if (taskNum == 0) {
		
			printf("can not quit, casue count of threads is [%d]\n", g_phreadNum - 1);
		}
		// 初始化信号量
		sem_init(&semDownload, 0, 0);
		printf("your choose downloading task [%d]\n", taskNum);

		if (g_phreadNum > MAXNUM) {
		
			printf("!!! you've reached a limit on the number of threads !!!\n");
			continue;
		}

		switch (taskNum) {
		case 1: 
			pthread_create(&a_thread, NULL, func1, NULL);
			sem_post(&semDownload);
			g_phreadNum++;
			break;
		case 2:
			pthread_create(&b_thread, NULL, func2, NULL);
			sem_post(&semDownload);
			g_phreadNum++;
			break;
		case 3:
			pthread_create(&c_thread, NULL, func3, NULL);
			sem_post(&semDownload);
			g_phreadNum++;
			break;
		default: 
			printf("!!!! error task [%d] !!!\n", taskNum);
		}
	}
	// 销毁信号量
	sem_destroy(&semDownload);
	return 0;
}