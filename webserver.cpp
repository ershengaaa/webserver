/*************************************************************************
      > File Name: webserver.cpp
      > Author: ersheng
      > Mail: ershengaaa@163.com 
      > Created Time: Tue 26 Feb 2019 08:30:32 PM CST
 ************************************************************************/

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/epoll.h>
#include "pthreadpool.h"
#include "http_conn.h"
using namespace std;
const int port = 8888;

//����Ϊ������ 
int set_nonblocking(int fd) {
	int old_option = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, old_option |O_NONBLOCK);
	return old_option;
}

//����¼� 
void add_fd(int epfd, int fd, bool flag) {
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET | EPOLLRDHUP; //�����¼����ļ���������д����Ե����(ET)���ļ����������Ҷ� 
	if(flag)
    {
        ev.events = ev.events | EPOLLONESHOT; //�����Ƿ�ֻ����һ���¼� 
    }
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);  //ע���¼�������µ�fd��epfd 
    setnonblocking(fd); //�����¼�Ϊ������ 
}

int main(int argc, char *argv[]) {
	cout << "fhh\n";
	threadpool<http_conn> pool(10); //�����̳߳� 
	cout << "66\n";
	http_conn* users = new http_conn[100];
	assert(users);
	pool.start(); //�����̳߳� 
	
	//�����׽��֣��󶨼����� 
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htons(INADDR_ANY);

	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(listen >= 0);

	int ret;
	ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
	assert(ret != -1);

	ret = listen(listenfd, 5);
	assert(ret >= 0);

	int epfd;
	//���������� 
    epoll_event events[1000];
    epfd = epoll_create(5);
    assert(epfd != -1);
    //��Ӽ����������¼� 
    addfd(epfd, listenfd, false);

	while (true) {
		//�ȴ��¼����� 
		int number = epoll_wait(epfd, events, 1000, -1);
		if ( (number < 0) && (errno != EINTR)) {
			printf("my epoll is failure\n");
			break;
		}
		for (int i = 0; i < number; ++i) {
			int sockfd = events[i].data.fd;
			if (sockfd == listenfd) { //�����û�����
				struct sockaddr_in client_address;
				socklen_t client_addresslength = sizeof(client_address);
				//�������� 
				int client_fd = accept(listenfd, (struct sockaddr*)&client_address, &client_addresslength);
				if (client_fd < 0) {
					printf("errno is %d\n", errno);
					continue;
				}
				cout << epfd << " " << client_fd << endl;
				add_fd(epfd, client_fd, true);   //��ӿͻ����������¼� 
				users[client_fd].init(epfd, client_fd);
			}
			else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
				/*�����쳣��رտͻ�������*/
				users[sockfd].close_conn();
			}
			else if (events[i].events & EPOLLIN) { //���Զ�ȡ
				cout << "fhh\n"; 				
				if (users[sockfd].myread()) {
					cout << "kedu\n";
					/*��ȡ�ɹ�������������*/
					pool.append_task(users + sockfd);
				}
				else {
					users[sockfd].close_conn(); //��ȡ���ɹ���رտͻ������� 
				}
			}
			else if (events[i].events & EPOLLOUT) { //��д��
				if (!users[sockfd].mywrite()) {
					users[sockfd].close_conn();
				}
			}
		}
	}
	//�ر��¼���socket 
	close(epfd);
	close(listenfd);
	delete[] users;
	return 0;
} 
