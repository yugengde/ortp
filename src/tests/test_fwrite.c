// 写入文件

#include <stdio.h>



int main() {

	FILE *fp; 
	char str[] = "ttthis is testthis is testthis is testthis is testthis is t";

	fp = fopen("/tmp/test.txt", "w");

	size_t size = fwrite(str, sizeof(str), 1, fp);
	printf("返回实际写入的数据块数目: block_size = %d", size);

	fclose(fp);
	return 0;
}
