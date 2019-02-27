/*************************************************************************
      > File Name: http_conn.h
      > Author: ersheng
      > Mail: ershengaaa@163.com 
      > Created Time: Tue 26 Feb 2019 09:30:58 PM CST
 ************************************************************************/

#ifndef _HTTP_CONN_H
#define _HTTP_CONN_H
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
using namespace std;
#define READ_BUF 2000
class http_conn {
public:
	 /*INCOMPLETE_REQUEST�Ǵ���������������Ҫ�ͻ��������룻BAD_REQUEST��HTTP�����﷨����ȷ��FILE_REQUEST�����ò��ҽ�����һ����ȷ��HTTP����FORBIDDEN_REQUEST�Ǵ��������Դ��Ȩ�������⣻
	 GET_REQUEST����GET������Դ����INTERNAL_ERROR����������������⣻NOT_FOUND�����������Դ�ļ������ڣ�DYNAMIC_REQUEST��ʾ��һ����̬����POST_REQUEST��ʾ���һ����POST��ʽ�����HTTP����*/
	enum HTTP_CODE {INCOMPLETE_REQUEST, FILE_REQUEST, BAD_REQUEST, FORBIDDEN_REQUEST, GET_REQUEST,
		INTERNAL_REQUEST, NOT_FOUND, DYNAMIC_REQUEST, POST_REQUEST};
	/*HTTP���������״̬ת�ơ�HEAD��ʾ����ͷ����Ϣ��ROW��ʾ����������*/
	enum CHECK_STATUS {HEAD, ROW};
private:
	char requst_head_buf[1000];//��Ӧͷ�����
    char post_buf[1000];//Post����Ķ�������
    char read_buf[READ_BUF];//�ͻ��˵�http�����ȡ
    char filename[250];//�ļ���Ŀ¼
    int file_size;//�ļ���С
    int check_index;//Ŀǰ��⵽��λ��
    int read_buf_len;//��ȡ�������Ĵ�С
    char *method;//���󷽷�
    char *url;//�ļ�����
    char *version;//Э��汾
    char *argv;//��̬�������
    bool m_conn;//�Ƿ񱣳�����
    int m_http_count;//http����
    char *m_host;//��������¼
    char path_400[17];//������400�򿪵��ļ���������
    char path_403[23];//������403�򿪷��ص��ļ���������
    char path_404[40];//������404��Ӧ�ļ���������
    char message[1000];//��Ӧ��Ϣ�建����
    char body[2000];//post��Ӧ��Ϣ�建����
    CHECK_STATUS status;//״̬ת��
    bool m_flag;//true��ʾ�Ƕ�̬���󣬷�֮�Ǿ�̬����
public:
	int epfd;
	int client_fd;
	int read_count;
	http_conn();
	~http_conn();
	void init(int e_fd, int c_fd);//��ʼ��
    int myread();//��ȡ����
    bool mywrite();//��Ӧ����
    void doit();//�߳̽ӿں���
    void close_coon();//�رտͻ�������
private:
	HTTP_CODE analyse();//����Http����ͷ�ĺ���
    int judge_line(int &check_index, int &read_buf_len);//�������Ƿ�������������\r\n
    HTTP_CODE head_analyse(char *temp);//http����ͷ����
    HTTP_CODE row_analyse(char *temp);//http�����н���
    HTTP_CODE do_post();//��post�����еĲ������н���
    HTTP_CODE do_file();//��GET���󷽷��е�url Э��汾�ķ���
    void modify_fd(int epfd, int sock, int ev);//�ı�socket״̬
    void dynamic(char *filename, char *argv);//ͨ��get��������Ķ�̬������
    void post_respond();//POST������Ӧ���
    bool bad_respond();//�﷨����������Ӧ���
    bool forbiden_respond();//��ԴȨ������������Ӧ�����
    bool succeessful_respond();//�����ɹ�������Ӧ���
    bool not_found_request();//��Դ������������Ӧ���
};

//��ʼ�� 
void http_conn::init(int e_fd, int c_fd) {
	epfd = e_fd;
	client_fd = c_fd;
	read_count = 0;
	m_flag = false;
}

http_conn::http_conn() {

}

http_conn::~http_conn() {

}

/*�رտͻ�������*/
void http_conn::close_conn() {
	epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, 0); //ɾ�������¼� 
	close(client_fd); //�رտͻ�������  
	client_fd = -1;
}

/*�ı��¼����е��¼�����*/
void http_conn::modify_fd(int epfd, int client_fd, int ev) {
	epoll_event event;
	event.data.fd = client_fd;
	event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
	epoll_ctl(epfd, EPOLL_CTL_MOD, client_fd, &event);  //�޸���ע��������� 
}

/*read�����ķ�װ*/
int http_conn::myread() {
	bzero(&read_buf, sizeof(read_buf));
	while (true) {
		//���ܿͻ��˵����󣬶�ȡ���� 
		int ret = recv(client_fd, read_buf + read_count, READ_BUF - read_count, 0);
		if (ret == -1) {
			//�׽����ѱ��Ϊ�������������ղ������������߽��ճ�ʱ ����ʾ��ǰ�������������ݿɶ���������͵����Ǹô��¼��Ѵ���
			if (errno == EAGAIN || errno == EWOULDBLOCK) {  //��ȡ���� 
				break;
			}
			return 0;
		}
		else if (ret == 0) {
			return 0;
		}
		read_count = read_count + ret;  //ѭ����ȡ���������е�����ҲҪ��ȡ����ÿ�ζ�ȡ�����ݵ���
	}
	strcpy(post_buf, read_buf);
	return 1;
}

//sprintf:����ʽ��������д���ַ��� 

/*��Ӧ״̬����䣬���ﷵ�ؿ��Բ�Ϊbool����*/
bool http_conn::successful_respond() {  //200���ͻ�������ɹ�
	m_flag = false;
	bzero(request_head_buf, sizeof(request_head_buf));
	sprintf(request_head_buf, "HTTP/1.1 200 ok\r\nConnection: close\r\ncontent-length:%d\r\n\r\n", file_size);
}

bool http_conn::bad_respond() {  //400���ͻ����������﷨���� 
	bzero(url, strlen(url));
	strcpy(path_400, "bad_respond.html");
	url = path_400;
	bzero(filename, sizeof(filename));
	sprintf(filename, "/home/linux/webserver/%s", url);
	struct stat my_file;
	if (stat(filename, &my_file) < 0) {  //�õ��ļ�����Ϣ 
		cout << "file not exit\n";
	}
	file_size = my_file.st_size;
	bzero(request_head_buf, sizeof(request_head_buf));
	sprintf(request_head_buf, "HTTP/1.1 400 BAD_REQUEST\r\nConnection: close\r\ncontent-length:%d\r\n\r\n",file_size);
}

bool http_conn::forbidden_respond(){  //403���������յ����󣬵��ܾ��ṩ����
	bzero(url, strlen(url));
	strcpy(path_403, "forbidden_request.html");
	url = path_403;
	bzero(filename, sizeof(filename));
	sprintf(filename, "/home/linux/webserver/%s", url);
	struct stat my_file;
	if (stat(filename, &my_file) < 0) {  //ͨ��filename��ȡ�ļ���Ϣ 
		cout << "fail" << endl;
	}
	file_size = my_file.st_size;
	bzero(request_head_buf, sizeof(request_head_buf));
	sprintf(request_head_buf, "HTTP/1.1 403 FORBIDDEN_REQUEST\r\nConnection: close\r\ncontent-length:%d\r\n\r\n", file_size);
}

bool http_conn::not_found_request() {  //404��������Դ������ 
	bzero(url, strlen(url));
	strcpy(path_404, "not_found_request.html");
	url = path_404;
	bzero(filename, sizeof(filename));
	sprintf(filename, "/home/linux/webserver/%s", url);
	struct stat my_file;
	if (stat(filename, &my_file) < 0) {
		cout << "not found" << endl;
	}
	file_size = my_file.st_size;
	bzero(request_head_buf, sizeof(request_head_buf));
	sprintf(request_head_buf, "HTTP/1.1 404 NOT_FOUND\r\nConnection: close\r\ncontent-length:%d\r\n\r\n", file_size);
}

/*��̬������*/
void http_conn::dynamic(char *filename, char *argv) {
	int len = strlen(argv);
	int k = 0;
	int number[2];
	int sum = 0;
	m_flag = true;  //���Ϊ��̬����
	bzero(request_head_buf, sizeof(request_head_buf));
	sscanf(argv, "a=%d&b=%d", &number[0], &number[1]);  //��ȡ��ʽ�����ַ����е����� 
	if (strcmp(filename, "/add") == 0) {
		sum = number[0] + number[1];
		sprintf(body, "<html><body>\r\n<p>%d + %d = %d </p><hr>\r\n</body></html>\r\n", number[0], number[1], sum);
		sprintf(request_head_buf, "HTTP/1.1 200 ok\r\nConnection: close\r\ncontent-length:%d\r\n\r\n", strlen(body));
	}
	else if (strcmp(filename, "/multiplication") == 0) {
		cout << "\t\t\t\tmultiplication\n\n";
		sum = number[0] * number[1];
		sprintf(body, "<html><body>\r\n<p>%d * %d = %d </p><hr>\r\n</body></html>\r\n", number[0], number[1], sum);
		sprintf(request_head_buf, "HTTP/1.1 200 ok\r\nConnection: close\r\ncontent-length:%d\r\n\r\n", strlen(body));
	}
}

/*POST������*/
void http_conn::post_respond() {
	if (fork() == 0) {
		dup2(client_fd,STDOUT_FILENO); //����׼����ض����������sockfd�� 
        execl(filename,argv,NULL); //�г��ļ����൱��list 
	}
	wait(NULL);
}

/*�ж�һ���Ƿ��ȡ����*/
int http_conn::judge_line(int &check_index, int read_buf_len) {
	cout << read_buf << endl;
	char ch;
	for (; check_index < read_buf_len; check_index++) {
		ch = read_buf[check_index];
		if (ch == '\r' && check_index + 1 < read_buf_len && ch == '\n') {
			read_buf[check_index++] = '\0';
			read_buf[check_index++] = '\0';
			return 1;  //��������һ��
		} 
		if (ch == '\r' && check_index + 1 == read_buf_len) {
			return 0;
		}
		if (ch == '\n') {
			if (check_index > 1 && read_buf[check_index + 1] == '\r') {
				read_buf[check_index-1] = '\0';
				read_buf[check_index++] = '\0';
				return 0;
			}
			else {
				return 0;
			}
		}
	}
	return 0;
}

/*����������*/
http_conn::HTTP_CODE http_conn::row_analyse(char *temp) {
	char *p = temp;
	cout << "p=" << p << endl;
	for (int i = 0; i < 2; ++i) {
		if (i == 0) {
			method = p;  //���󷽷�����
			int j = 0;
			while ((*p != ' ') && (*p != '\r')) {
				p++;
			}
			p[0] = '\0';
			p++;
			cout << "method:" << method << endl;
		}
		if (i == 1) {
			url = p;  //�ļ�·������
			while ((*p != ' ') && (*p != '\r')) {
				p++;
			}
			p[0] = '\0';
			p++;
			cout << "url:" << endl;
		}
	}
	version = p;  //����Э�鱣��
	while (*p != '\r') {
		p++;
	}
	p[0] = '\0';
	p++;
	p[0] = '\0';
	p++;
	cout << version << endl;
	if (strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0) {
		return BAD_REQUEST;
	}
	if (!url || url[0] != '/') {
		return BAD_REQUEST;
	}
	if (strcmp(version, "HTTP/1.1") != 0) {
		return BAD_REQUEST;
	}
	status = HEAD;  //״̬ת�Ƶ�����ͷ��
	return INCOMPLETE_REQUEST;  //��������
}

/*����ͷ����Ϣ*/
http_conn::HTTP_CODE http_conn::head_analyse(char *temp) {
	if (temp[0] = '\0') {
		//���һ������http����
		return FILE_REQUEST;
	}
	//��������ͷ��
	else if (strncasecmp(temp, "Connnection:", 11) == 0) {  //�Ƚ�ǰn���ַ� 
		temp = temp + 11;
		while (*temp == ' ') {
			temp++;
		}
		if (strcasecmp(temp, "keep-alive") == 0) {
			m_conn = true;  //�������� 
		}
	}
	else if (strncasecmp(temp, "Content-Length:", 15) == 0) {
		temp = temp + 15;
		while (*temp == ' ') {
			cout << *temp << endl;
			temp++;
		}
		m_http_count = atol(temp);  //content-length��Ҫ��䣬http���� 
	}
	else if(strncasecmp(temp, "Host:", 5) == 0) {
		temp = temp + 5;
		while (*temp == ' ') {
			temp++;
		}
		m_host = temp;  //������ 
	}
	else {
		cout << "can't handle it's hand\n" << endl;
	}
	return INTERNAL_REQUEST;
}

http_conn::HTTP_CODE http_conn::do_file() {  //GET�������󣬶��������н��н�������д��Դ·��
	char path[40] = "/home/linux/webserver";
	char *ch;
	if (ch = strchr(url, '?')) {  //����url���״γ��֣���λ�� 
		argv = ch + 1;
		*ch = '\0';
		strcpy(filename, url);
		return DYNAMIC_REQUEST;
	}
	else {
		strcpy(filename, path);
		strcat(filename, url);
		struct stat m_file_stat;
		if (stat(filename, &m_file_stat) < 0) {
			return NOT_FOUND;  //NOT_FOUND 404
		}
		//S_IROTH���������Ȩ��    st_mode���ļ������ͺͶ�ȡ��Ȩ��
		if (!(m_file_stat.st_mode & S_IROTH)) {  //FORBIDDEN_REQUESTION 403
			return FORBIDDEN_REQUEST;
		}
		if (S_ISDIR(m_file_stat.st_mode)) {  //�ж�·���Ƿ�ΪĿ¼ 
			return BAD_REQUEST;  //BAD_REQUESTION 400
		}
		file_size = m_file_stat.st_size;  //st_size���ļ��ֽ��� 
		return GET_REQUEST;
	}
}

http_conn::HTTP_CODE http_conn::do_post() {  //POST�������󣬷ֽⲢ�Ҵ������
	int k = 0;
	int star;
	char path[34] = "/home/linux/webserver";
	strcpy(filename, path);
	strcat(filename, url);
	star = read_buf_len - m_http_count;
	argv = post_buf + star;
	argv[strlen(argv) + 1] = '\0';
	if (filename != NULL && argv != NULL) {
		return POST_REQUEST;
	}
	return BAD_REQUEST;
}

/*http�������*/
http_conn::HTTP_CODE http_conn::analyse() {
	status = ROW;
	int flag;
	char *temp = read_buf;
	int star_line = 0;
	check_index = 0;
	int star =  0;
	read_buf_len = strlen(read_buf);
	int len = read_buf_len;
	while ((flag = judge_line(check_index, len)) == 1) {
		temp = read_buf + star_line;
		star_line = check_index;
		switch(status) {
			case ROW:  //�����з����������ļ����ƺ����󷽷�
			{
				cout << "row request" << endl;
				int ret;
				ret = row_analyse(temp);
				if (ret == BAD_REQUEST) {
					cout << "ret == BAD_REQUEST " << endl;
					//�����ʽ����ȷ
					return BAD_REQUEST;
				}
				break;
			}
			case HEAD: //����ͷ�ķ���
			{
				int ret;
				ret =head_analyse(temp);
				if (ret == FILE_REQUEST) {  //��ȡ������HTTP����
					if (strcmp(method, "GET") == 0) {
						return do_file();  //GET�����ļ������뺯��     
					}
					else if (strcmp(method, "POST") == 0) {
						return do_post();  //POST����������뺯��
					}
					else {
						return BAD_REQUEST;
					}
				}
				break;
			}
			default:
			{
				return INTERNAL_REQUEST;
			}
		}
	}
	return INCOMPLETE_REQUEST;  //������������Ҫ��������
}

/*�߳�ȡ����������Ľӿں���*/
void http_conn::doit() {
	int choice = analyse();  //���ݽ�������ͷ�Ľ����ѡ��
	switch(choice) {
		case INCOMPLETE_REQUEST:  //��������
		{
			cout << "INCOMPLETE_REQUEST" << endl;
			/*�ı�epoll������*/
			modify_fd(epfd, client_fd, EPOLLIN);
			return;
		}
		case BAD_REQUEST: //400 
		{
			cout << "BAD_REQUEST" << endl;
			bad_respond();
			modify_fd(epfd, client_fd, EPOLLOUT);
			break;
		}
		case FORBIDDEN_REQUEST: //403 
		{
			cout << "forbidden_respond" << endl;
			forbidden_respond();
			modify_fd(epfd, client_fd, EPOLLOUT);
			break;
		}
		case NOT_FOUND: //404 
		{
			cout << "not_found_request" << endl;
			not_found_request();
			modify_fd(epfd, client_fd, EPOLLOUT);
			break;
		} 
		case GET_REQUEST:  //GET�ļ���Դ������
		{
			cout << "file request" << endl;
			successful_respond();
			modify_fd(epfd, client_fd, EPOLLOUT);
			break;
		}
		case DYNAMIC_REQUEST:  //��̬������
		{
			cout << "dynamic request" << endl;
			cout << filename << " " << argv << endl;
			dynamic(filename, argv);
			modify_fd(epfd, client_fd, EPOLLOUT);
			break;
		}
		case POST_REQUEST: //POST ��������
		{
			cout << "post_respond" << endl;
			post_respond();
			break;
		}
		default:
		{
			close_conn();
		}
	}
}

bool http_conn::mywrite() {
	if (m_flag) {  //����Ƕ�̬���󣬷��������
		int ret = send(client_fd, request_head_buf, strlen(request_head_buf), 0);
		int r = send(client_fd, body, strlen(body), 0);
		if (ret > 0 && r > 0) {
			return true;
		}
	}
	else {
		int fd = open(filename, O_RDONLY);
		assert(fd != -1);
		int ret;
		ret = write(client_fd, request_head_buf, strlen(request_head_buf));
		if (ret < 0) {
			close(fd);
			return false;
		}
		ret = sendfile(client_fd, fd, NULL, file_size); //���ͻؿͻ���
		if (ret < 0) {
			close(fd);
			return false;
		}
		close(fd);
		return true;
	}
	return false;
}

#endif
