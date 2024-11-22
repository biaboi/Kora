设计想法：实现简单，易用的rtos
包含：
1、支持轮询和抢占的内核
2、抢占式调度支持 32 个优先级
3、系统占用小，容易移植到性能差的单片机运行
4、提供mutex、semaphore、message queue 和 event group进行线程间通信
5、支持动态分配和释放内存
6、内核提供debug接口，通过shell进行调试
