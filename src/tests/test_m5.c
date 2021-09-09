#include<stdarg.h>
#include<stdio.h>

int func1() {

	printf("%d\n", sizeof(unsigned short int));

	printf("%d\n", sizeof(unsigned long int));

	int i = (8 * (int)sizeof(long int)); 
	printf("%d\n", i);
	return 0;
}

int AveInt(int v, ...) {

	int return_value = 0;
	int i = v;

	va_list ap;  // 1. 定义va_list类型的变量
	va_start(ap, v);  // 使用宏初始化变量

	while (i > 0) {
	
		return_value += va_arg(ap, int);
		i--;
	}
	va_end(ap);
	return return_value /= v;
}

int main() {
	printf("%d/t", AveInt(2, 2, 3));
	printf("%d/t", AveInt(2, 2, 4, 6, 8));

}


