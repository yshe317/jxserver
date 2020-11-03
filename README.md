# jxserver
## JX Server -- 一个基于TCP Socket的高并发文件服务器
* 概述: 用C语言从零实现了一个多线程的TCP Socket文件服务器, 它支持文件的多线程下载和压缩
* 线程池优化: 为了多线程冷启动性能损耗的问题, 基于Linux的condition-variable同步原语和queue实现了一个线程池
* 传输速度优化: 基于哈夫曼压缩算法的思想, 建立哈夫曼树, 生成了一个压缩字典, 从而实现了bit级别的无损压缩和解压缩.
* bit数组抽象: 通过对位操作的的封装, 实现了bit array的get和set方法, 从而方便了压缩字典的生成, 以及提高了解压与压缩代码的可读性
* 用到的技术: C, TCP Socket, Linux, 高并发, 线程池思想
