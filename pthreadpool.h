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
	int thread_number;  /* �̳߳ص��߳��� */ 
	pthread_t *all_threads;  /* �߳����� */ 
	std::queue<T *> task_queue;  /* ������� */ 
	mutex_locker queue_mutex_locker;  /* ������ */ 
	cond_locker queue_cond_locker;  /* �������� */ 
	bool is_stop;  /* �ж��Ƿ�����߳� */ 
public:
	threadpool(int thread_number = 20);
	~threadpool();
	bool append_task(T *task);
	void start();
	void stop();
private:
	/* �߳����еĺ��� */ 
	static void *worker(void *arg);
	void run();
	T *getTask();
};

/* ��ʼ���̳߳� */ 
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

/* �����̳߳� */ 
template <class T>
threadpool<T>::~threadpool() {
	delete []all_threads;
	stop();
}

/* ֹͣ�̳߳� */ 
template <class T>
void threadpool<T>::stop() {
	is_stop = true;
	queue_cond_locker.cond_broadcast();
}

/* ��ʼ�̳߳� */ 
template <class T>
void threadpool<T>::start() {
	for (int i = 0; i < thread_number; ++i) {
		std::cout << "Create the pthread:" << i << std::endl;
		if (pthread_create(all_threads + i, NULL, worker, this) != 0) {
			/* �̴߳���ʧ�ܣ��������������������Դ���׳��쳣 */ 
			delete []all_threads;
			throw std::exception();
		}
		if (pthread_detach(all_threads[i])) {
			/* ���߳�����Ϊ�����̣߳�ʧ��������ɹ��������Դ���׳��쳣 */ 
			delete []all_threads;
			throw std::exception();
		}
	}
}

/* ����������������� */ 
template <class T>
bool threadpool<T>::append_task(T *task) {
	/* ��ȡ������ */ 
	queue_mutex_locker.mutex_lock();
	bool is_signal = task_queue.empty(); /* �ж���������Ƿ�Ϊ�� */ 
	task_queue.push(task);  /* ����������� */ 
	queue_mutex_locker.mutex_unlock(); /* �ͷŻ����� */
	/* ���������в�Ϊ�գ����ѵȴ������������߳� */ 
	if (is_signal) { 
		queue_cond_locker.get_signal();
	}
	return true;
}

/* �̵߳��ú��� */ 
template <class T>
void *threadpool<T>::worker(void *arg) {
	threadpool *pool = (threadpool *)arg;
	pool->run();
	return pool;
}

/* ��ȡ���� */ 
template <class T>
T* threadpool<T>::getTask() {
	T *task = NULL;
	queue_mutex_locker.mutex_lock();  /* ��ȡ������ */ 
	/* �ж���������Ƿ�Ϊ�� */ 
	if (!task_queue.empty()) {
		/* ��ȡ��ͷ����ִ�� */ 
		task = task_queue.front();
		task_queue.pop();
	}
	queue_mutex_locker.mutex_unlock();
	return task;
}

template <class T>
void threadpool<T>::run() {
	while (!is_stop) {
		/* �ȴ����� */ 
		T *task = getTask();
		if (task == NULL) 
			queue_cond_locker.wait_cond();
		else {
			task->doit(); /* doit��T�����еķ��� */ 
			delete task;
		}
		//for test
		//printf("exit %ld\n", (unsigned long)pthread_self());
	}
}

#endif

