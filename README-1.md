


启动进程
---

```sh
# ./rtprecv /tmp/recvfile 7788

rm /tmp/recvfile && /root/.vs/ortp/b93efca9-1cc6-46d6-b1d6-68e41fd4fb5c/out/build/Linux-GCC-Debug/src/tests/rtprecv /tmp/recvfile 7788
```

```sh
# ./rtpsend /tmp/sendfile 192.168.10.100 7788
/root/.vs/ortp/b93efca9-1cc6-46d6-b1d6-68e41fd4fb5c/out/build/Linux-GCC-Debug/src/tests/rtpsend /tmp/sendfile 127.0.0.1 7788
/root/.vs/ortp/b93efca9-1cc6-46d6-b1d6-68e41fd4fb5c/out/build/Linux-GCC-Debug/src/tests/rtpsend /tmp/sendfile_t2 127.0.0.1 7788
/root/.vs/ortp/b93efca9-1cc6-46d6-b1d6-68e41fd4fb5c/out/build/Linux-GCC-Debug/src/tests/rtpsend /tmp/sendfile_t2 192.168.10.100 7788

```





[ortp数据收发调度器源码分析_bjrxyz的专栏-CSDN博客](https://blog.csdn.net/bjrxyz/article/details/56494641)

[2 c++sockaddr，sockaddr_in_宣的专栏-CSDN博客_c++ sockaddr](https://blog.csdn.net/kturing/article/details/78240718)

[3oRTP在ubuntu下的编译安装_wang3141128的博客-CSDN博客](https://blog.csdn.net/wang3141128/article/details/80481111)



[ortp使用详解1](https://www.cnblogs.com/elisha-blogs/p/4029412.html)

[ortp使用详解2](https://www.cnblogs.com/elisha-blogs/p/4029413.html)

[Jitter buffer](https://www.pianshen.com/article/2992516241/)

