### ORTP



![image-20210827154006170](C:\Users\admin\AppData\Roaming\Typora\typora-user-images\image-20210827154006170.png)



```c
RtpTimer posix_timer={	0,
						posix_timer_init,
						posix_timer_do,
						posix_timer_uninit,
						{0,POSIXTIMER_INTERVAL}}; 
```





```c
static struct timeval orig,cur;
static uint32_t posix_timer_time=0;		/*in milisecond */

// 初始化函数
void posix_timer_init(void)
{
	posix_timer.state=RTP_TIMER_RUNNING;  // 设置位运行状态
	ortp_gettimeofday(&orig,NULL);        // orig 设置位当前时间
	posix_timer_time=0;                   // 设置定时器时间
}

// 销毁函数
void posix_timer_uninit(void)
{
	posix_timer.state=RTP_TIMER_STOPPED;
}
```

```c
// 定时函数
void posix_timer_do(void)
{
	int diff,time;
	struct timeval tv;
	ortp_gettimeofday(&cur,NULL);
	time=((cur.tv_usec-orig.tv_usec)/1000 ) + ((cur.tv_sec-orig.tv_sec)*1000 );
	if ( (diff=time-posix_timer_time)>50){
		ortp_warning("Must catchup %i miliseconds.",diff);
	}
	while((diff = posix_timer_time-time) > 0)
	{
		tv.tv_sec = diff/1000;
		tv.tv_usec = (diff%1000)*1000;
#if	defined(_WIN32) || defined(_WIN32_WCE)
        /* this kind of select is not supported on windows */
		Sleep(tv.tv_usec/1000 + tv.tv_sec * 1000);
#else
		select(0,NULL,NULL,NULL,&tv);
#endif
		ortp_gettimeofday(&cur,NULL);
		time=((cur.tv_usec-orig.tv_usec)/1000 ) + ((cur.tv_sec-orig.tv_sec)*1000 );
	}
	posix_timer_time+=POSIXTIMER_INTERVAL/1000;
	
}
```

