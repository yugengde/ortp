// д���ļ�

#include <stdio.h>



int main() {

	FILE *fp; 
	char str[] = "ttthis is testthis is testthis is testthis is testthis is t";

	fp = fopen("/tmp/test.txt", "w");

	size_t size = fwrite(str, sizeof(str), 1, fp);
	printf("����ʵ��д������ݿ���Ŀ: block_size = %d", size);

	fclose(fp);
	return 0;
}
