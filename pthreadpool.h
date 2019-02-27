/*************************************************************************
      > File Name: pthreadpool.h
      > Author: ersheng
      > Mail: ershengaaa@163.com 
      > Created Time: Sat 23 Feb 2019 11:39:39 PM CST
 ************************************************************************/

#ifndef _PTHREAD_POOL_
#define _PTHREAD_POOL_ 

#include "locker.h"
#include <queue>
#include <stdio.h>
#include <exception>
#include <errno.h>
#include <pthread.h>
#include <iostream>

template <class T>
class threadpool {
private:
	int thread_number;  /* 线程池的线程数 */ 
	pthread_t *all_threads;  /* 线程数组 */ 
	std::queue<T *> task_queue;  /* 任务队列 */ 
	mutex_locker queue_mutex_locker;  /* 互斥锁 */ 
	cond_locker queue_cond_locker;  /* 条件变量 */ 
	bool is_stop;  /* 判断是否结束线程 */ 
public:
	threadpool(int thread_number = 20);
	~threadpool();
	bool append_task(T *task);
	void start();
	void stop();
private:
	/* 线程运行的函数 */ 
	static void *worker(void *arg);
	void run();
	T *getTask();
};

/* 初始化线程池 */ 
template <class T>
threadpool<T>::threadpool(int thread_num):
	thread_number(thread_num),
	is_stop(false), all_threads(NULL)
{
	if (thread_number <= 0)
			printf("threadpool can't init because thread_number = 0\n");
	all_threads = new pthread_t[thread_number];
	if (all_threads == 0) {
		printf("can't init threadpool because thread array can't new\n");
	}
}

/* 销毁线程池 */ 
template <class T>
threadpool<T>::~threadpool() {
	delete []all_threads;
	stop();
}

/* 停止线程池 */ 
template <class T>
void threadpool<T>::stop() {
	is_stop = true;
	queue_cond_locker.cond_broadcast();
}

/* 开始线程池 */ 
template <class T>
void threadpool<T>::start() {
	for (int i = 0; i < thread_number; ++i) {
		std::cout << "Create the pthread:" << i << std::endl;
		if (pthread_create(all_threads + i, NULL, worker, this) != 0) {
			/* 线程创建失败，则销毁所有已申请的资源并抛出异常 */ 
			delete []all_threads;
			throw std::exception();
		}
		if (pthread_detach(all_threads[i])) {
			/* 将线程设置为脱离线程，失败则清除成功申请的资源并抛出异常 */ 
			delete []all_threads;
			throw std::exception();
		}
	}
}

/* 添加任务进入任务队列 */ 
template <class T>
bool threadpool<T>::append_task(T *task) {
	/* 获取互斥锁 */ 
	queue_mutex_locker.mutex_lock();
	bool is_signal = task_queue.empty(); /* 判断任务队列是否为空 */ 
	task_queue.push(task);  /* 加入任务队列 */ 
	queue_mutex_locker.mutex_unlock(); /* 释放互斥锁 */
	/* 如果任务队列不为空，则唤醒等待条件变量的线程 */ 
	if (is_signal) { 
		queue_cond_locker.get_signal();
	}
	return true;
}

/* 线程调用函数 */ 
template <class T>
void *threadpool<T>::worker(void *arg) {
	threadpool *pool = (threadpool *)arg;
	pool->run();
	return pool;
}

/* 获取任务 */ 
template <class T>
T* threadpool<T>::getTask() {
	T *task = NULL;
	queue_mutex_locker.mutex_lock();  /* 获取互斥锁 */ 
	/* 判断任务队列是否为空 */ 
	if (!task_queue.empty()) {
		/* 获取队头任务并执行 */ 
		task = task_queue.front();
		task_queue.pop();
	}
	queue_mutex_locker.mutex_unlock();
	return task;
}

template <class T>
void threadpool<T>::run() {
	while (!is_stop) {
		/* 等待任务 */ 
		T *task = getTask();
		if (task == NULL) 
			queue_cond_locker.wait_cond();
		else {
			task->doit(); /* doit是T对象中的方法 */ 
			delete task;
		}
		//for test
		//printf("exit %ld\n", (unsigned long)pthread_self());
	}
}

#endif

