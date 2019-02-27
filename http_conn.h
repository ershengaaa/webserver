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
	 /*INCOMPLETE_REQUEST是代表请求不完整，需要客户继续输入；BAD_REQUEST是HTTP请求语法不正确；FILE_REQUEST代表获得并且解析了一个正确的HTTP请求；FORBIDDEN_REQUEST是代表访问资源的权限有问题；
	 GET_REQUEST代表GET方法资源请求；INTERNAL_ERROR代表服务器自身问题；NOT_FOUND代表请求的资源文件不存在；DYNAMIC_REQUEST表示是一个动态请求；POST_REQUEST表示获得一个以POST方式请求的HTTP请求*/
	enum HTTP_CODE {INCOMPLETE_REQUEST, FILE_REQUEST, BAD_REQUEST, FORBIDDEN_REQUEST, GET_REQUEST,
		INTERNAL_REQUEST, NOT_FOUND, DYNAMIC_REQUEST, POST_REQUEST};
	/*HTTP请求解析的状态转移。HEAD表示解析头部信息，ROW表示解析请求行*/
	enum CHECK_STATUS {HEAD, ROW};
private:
	char requst_head_buf[1000];//响应头的填充
    char post_buf[1000];//Post请求的读缓冲区
    char read_buf[READ_BUF];//客户端的http请求读取
    char filename[250];//文件总目录
    int file_size;//文件大小
    int check_index;//目前检测到的位置
    int read_buf_len;//读取缓冲区的大小
    char *method;//请求方法
    char *url;//文件名称
    char *version;//协议版本
    char *argv;//动态请求参数
    bool m_conn;//是否保持连接
    int m_http_count;//http长度
    char *m_host;//主机名记录
    char path_400[17];//出错码400打开的文件名缓冲区
    char path_403[23];//出错码403打开返回的文件名缓冲区
    char path_404[40];//出错码404对应文件名缓冲区
    char message[1000];//响应消息体缓冲区
    char body[2000];//post响应消息体缓冲区
    CHECK_STATUS status;//状态转移
    bool m_flag;//true表示是动态请求，反之是静态请求
public:
	int epfd;
	int client_fd;
	int read_count;
	http_conn();
	~http_conn();
	void init(int e_fd, int c_fd);//初始化
    int myread();//读取请求
    bool mywrite();//响应发送
    void doit();//线程接口函数
    void close_coon();//关闭客户端连接
private:
	HTTP_CODE analyse();//解析Http请求头的函数
    int judge_line(int &check_index, int &read_buf_len);//该请求是否是完整的以行\r\n
    HTTP_CODE head_analyse(char *temp);//http请求头解析
    HTTP_CODE row_analyse(char *temp);//http请求行解析
    HTTP_CODE do_post();//对post请求中的参数进行解析
    HTTP_CODE do_file();//对GET请求方法中的url 协议版本的分离
    void modify_fd(int epfd, int sock, int ev);//改变socket状态
    void dynamic(char *filename, char *argv);//通过get方法进入的动态请求处理
    void post_respond();//POST请求响应填充
    bool bad_respond();//语法错误请求响应填充
    bool forbiden_respond();//资源权限限制请求响应的填充
    bool succeessful_respond();//解析成功请求响应填充
    bool not_found_request();//资源不存在请求响应填充
};

//初始化 
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

/*关闭客户端连接*/
void http_conn::close_conn() {
	epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, 0); //删除监听事件 
	close(client_fd); //关闭客户描述符  
	client_fd = -1;
}

/*改变事件表中的事件属性*/
void http_conn::modify_fd(int epfd, int client_fd, int ev) {
	epoll_event event;
	event.data.fd = client_fd;
	event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
	epoll_ctl(epfd, EPOLL_CTL_MOD, client_fd, &event);  //修改已注册的描述符 
}

/*read函数的封装*/
int http_conn::myread() {
	bzero(&read_buf, sizeof(read_buf));
	while (true) {
		//接受客户端的请求，读取数据 
		int ret = recv(client_fd, read_buf + read_count, READ_BUF - read_count, 0);
		if (ret == -1) {
			//套接字已标记为非阻塞，而接收操作被阻塞或者接收超时 ，表示当前缓冲区已无数据可读，在这里就当作是该次事件已处理
			if (errno == EAGAIN || errno == EWOULDBLOCK) {  //读取结束 
				break;
			}
			return 0;
		}
		else if (ret == 0) {
			return 0;
		}
		read_count = read_count + ret;  //循环读取（缓冲区中的数据也要读取），每次读取的数据叠加
	}
	strcpy(post_buf, read_buf);
	return 1;
}

//sprintf:将格式化的数据写入字符串 

/*响应状态的填充，这里返回可以不为bool类型*/
bool http_conn::successful_respond() {  //200：客户端请求成功
	m_flag = false;
	bzero(request_head_buf, sizeof(request_head_buf));
	sprintf(request_head_buf, "HTTP/1.1 200 ok\r\nConnection: close\r\ncontent-length:%d\r\n\r\n", file_size);
}

bool http_conn::bad_respond() {  //400：客户端请求有语法错误 
	bzero(url, strlen(url));
	strcpy(path_400, "bad_respond.html");
	url = path_400;
	bzero(filename, sizeof(filename));
	sprintf(filename, "/home/linux/webserver/%s", url);
	struct stat my_file;
	if (stat(filename, &my_file) < 0) {  //得到文件的信息 
		cout << "file not exit\n";
	}
	file_size = my_file.st_size;
	bzero(request_head_buf, sizeof(request_head_buf));
	sprintf(request_head_buf, "HTTP/1.1 400 BAD_REQUEST\r\nConnection: close\r\ncontent-length:%d\r\n\r\n",file_size);
}

bool http_conn::forbidden_respond(){  //403：服务器收到请求，但拒绝提供服务
	bzero(url, strlen(url));
	strcpy(path_403, "forbidden_request.html");
	url = path_403;
	bzero(filename, sizeof(filename));
	sprintf(filename, "/home/linux/webserver/%s", url);
	struct stat my_file;
	if (stat(filename, &my_file) < 0) {  //通过filename获取文件信息 
		cout << "fail" << endl;
	}
	file_size = my_file.st_size;
	bzero(request_head_buf, sizeof(request_head_buf));
	sprintf(request_head_buf, "HTTP/1.1 403 FORBIDDEN_REQUEST\r\nConnection: close\r\ncontent-length:%d\r\n\r\n", file_size);
}

bool http_conn::not_found_request() {  //404：请求资源不存在 
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

/*动态请求处理*/
void http_conn::dynamic(char *filename, char *argv) {
	int len = strlen(argv);
	int k = 0;
	int number[2];
	int sum = 0;
	m_flag = true;  //标记为动态请求
	bzero(request_head_buf, sizeof(request_head_buf));
	sscanf(argv, "a=%d&b=%d", &number[0], &number[1]);  //读取格式化的字符串中的数据 
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

/*POST请求处理*/
void http_conn::post_respond() {
	if (fork() == 0) {
		dup2(client_fd,STDOUT_FILENO); //将标准输出重定向到浏览器的sockfd上 
        execl(filename,argv,NULL); //列出文件，相当于list 
	}
	wait(NULL);
}

/*判断一行是否读取完整*/
int http_conn::judge_line(int &check_index, int read_buf_len) {
	cout << read_buf << endl;
	char ch;
	for (; check_index < read_buf_len; check_index++) {
		ch = read_buf[check_index];
		if (ch == '\r' && check_index + 1 < read_buf_len && ch == '\n') {
			read_buf[check_index++] = '\0';
			read_buf[check_index++] = '\0';
			return 1;  //完整读入一行
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

/*解析请求行*/
http_conn::HTTP_CODE http_conn::row_analyse(char *temp) {
	char *p = temp;
	cout << "p=" << p << endl;
	for (int i = 0; i < 2; ++i) {
		if (i == 0) {
			method = p;  //请求方法保存
			int j = 0;
			while ((*p != ' ') && (*p != '\r')) {
				p++;
			}
			p[0] = '\0';
			p++;
			cout << "method:" << method << endl;
		}
		if (i == 1) {
			url = p;  //文件路径保存
			while ((*p != ' ') && (*p != '\r')) {
				p++;
			}
			p[0] = '\0';
			p++;
			cout << "url:" << endl;
		}
	}
	version = p;  //请求协议保存
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
	status = HEAD;  //状态转移到解析头部
	return INCOMPLETE_REQUEST;  //继续解析
}

/*解析头部信息*/
http_conn::HTTP_CODE http_conn::head_analyse(char *temp) {
	if (temp[0] = '\0') {
		//获得一个完整http请求
		return FILE_REQUEST;
	}
	//处理其他头部
	else if (strncasecmp(temp, "Connnection:", 11) == 0) {  //比较前n个字符 
		temp = temp + 11;
		while (*temp == ' ') {
			temp++;
		}
		if (strcasecmp(temp, "keep-alive") == 0) {
			m_conn = true;  //保持连接 
		}
	}
	else if (strncasecmp(temp, "Content-Length:", 15) == 0) {
		temp = temp + 15;
		while (*temp == ' ') {
			cout << *temp << endl;
			temp++;
		}
		m_http_count = atol(temp);  //content-length需要填充，http长度 
	}
	else if(strncasecmp(temp, "Host:", 5) == 0) {
		temp = temp + 5;
		while (*temp == ' ') {
			temp++;
		}
		m_host = temp;  //主机名 
	}
	else {
		cout << "can't handle it's hand\n" << endl;
	}
	return INTERNAL_REQUEST;
}

http_conn::HTTP_CODE http_conn::do_file() {  //GET方法请求，对其请求行进行解析，存写资源路径
	char path[40] = "/home/linux/webserver";
	char *ch;
	if (ch = strchr(url, '?')) {  //查找url中首次出现？的位置 
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
		//S_IROTH：其他组读权限    st_mode：文件的类型和读取的权限
		if (!(m_file_stat.st_mode & S_IROTH)) {  //FORBIDDEN_REQUESTION 403
			return FORBIDDEN_REQUEST;
		}
		if (S_ISDIR(m_file_stat.st_mode)) {  //判断路径是否为目录 
			return BAD_REQUEST;  //BAD_REQUESTION 400
		}
		file_size = m_file_stat.st_size;  //st_size：文件字节数 
		return GET_REQUEST;
	}
}

http_conn::HTTP_CODE http_conn::do_post() {  //POST方法请求，分解并且存入参数
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

/*http请求解析*/
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
			case ROW:  //请求行分析，包括文件名称和请求方法
			{
				cout << "row request" << endl;
				int ret;
				ret = row_analyse(temp);
				if (ret == BAD_REQUEST) {
					cout << "ret == BAD_REQUEST " << endl;
					//请求格式不正确
					return BAD_REQUEST;
				}
				break;
			}
			case HEAD: //请求头的分析
			{
				int ret;
				ret =head_analyse(temp);
				if (ret == FILE_REQUEST) {  //获取完整的HTTP请求
					if (strcmp(method, "GET") == 0) {
						return do_file();  //GET请求文件名分离函数     
					}
					else if (strcmp(method, "POST") == 0) {
						return do_post();  //POST请求参数分离函数
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
	return INCOMPLETE_REQUEST;  //请求不完整，需要继续读入
}

/*线程取出工作任务的接口函数*/
void http_conn::doit() {
	int choice = analyse();  //根据解析请求头的结果做选择
	switch(choice) {
		case INCOMPLETE_REQUEST:  //请求不完整
		{
			cout << "INCOMPLETE_REQUEST" << endl;
			/*改变epoll的属性*/
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
		case GET_REQUEST:  //GET文件资源无问题
		{
			cout << "file request" << endl;
			successful_respond();
			modify_fd(epfd, client_fd, EPOLLOUT);
			break;
		}
		case DYNAMIC_REQUEST:  //动态请求处理
		{
			cout << "dynamic request" << endl;
			cout << filename << " " << argv << endl;
			dynamic(filename, argv);
			modify_fd(epfd, client_fd, EPOLLOUT);
			break;
		}
		case POST_REQUEST: //POST 方法处理
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
	if (m_flag) {  //如果是动态请求，返回填充体
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
		ret = sendfile(client_fd, fd, NULL, file_size); //发送回客户端
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
