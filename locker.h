/*************************************************************************
      > File Name: locker.h
      > Author: ersheng
      > Mail: ershengaaa@163.com 
      > Created Time: Sat 23 Feb 2019 10:47:14 PM CST
 ************************************************************************/

#ifndef _LOCKER_H
#define _LOCKER_H 
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>

/* 信号量类 */ 
class sem_locker {
private:
	sem_t _sem;
public:
	/* 初始化信号量 */ 
	sem_locker() {
		if (sem_init(&_sem, 0, 0)) {
			printf("sem init error\n");
		}
	}
	/* 销毁信号量 */ 
	~sem_locker() {
		sem_destroy(&_sem);
	}
	/* 等待信号量 */ 
	bool wait_sem() {
		return sem_wait(&_sem) == 0;
	}
	/* 添加信号量 */ 
	bool add_sem() {
		return sem_post(&_sem) == 0;
	}
};

/* 互斥量类 */ 
class mutex_locker {
private:
	pthread_mutex_t _mutex;
public:
	/* 初始化互斥量 */ 
	mutex_locker() {
		if (pthread_mutex_init(&_mutex, NULL)) {
			printf("mutex init error\n");
		}
	}
	/* 销毁互斥量 */ 
	~mutex_locker() {
		pthread_mutex_destroy(&_mutex);
	}
	/* 上锁 */ 
	bool mutex_lock() {
		return pthread_mutex_lock(&_mutex) == 0;
	}
	/* 解锁 */ 
	bool mutex_unlock() {
		return pthread_mutex_unlock(&_mutex) == 0;
	}
};

/* 条件变量类 */ 
class cond_locker {
private:
	pthread_mutex_t _mutex;
	pthread_cond_t _cond;
public:
	/* 初始化条件变量与互斥量 */ 
	cond_locker() {
		if (pthread_mutex_init(&_mutex, NULL)) 
			printf("mutex init error\n");
		if (pthread_cond_init(&_cond, NULL)) {
			pthread_mutex_destroy(&_mutex);
			printf("cond init error\n");
		}
	}
	/* 销毁条件变量与互斥量 */ 
	~cond_locker() {
		pthread_mutex_destroy(&_mutex);
		pthread_cond_destroy(&_cond);
	}
	/* 等待条件变量 */ 
	bool wait_cond() {
		int ans = 0;
		pthread_mutex_lock(&_mutex);
		ans = pthread_cond_wait(&_cond, &_mutex);
		pthread_mutex_unlock(&_mutex);
		return ans == 0;
	}
	/* 唤醒等待条件变量的线程 */ 
	bool get_signal() {
		return pthread_cond_signal(&_cond) == 0;
	}
	/* 激活所有线程 */ 
	bool cond_broadcast() {
		return pthread_cond_broadcast(&_cond) == 0;
	}
};

#endif
