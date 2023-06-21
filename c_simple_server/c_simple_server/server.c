#include "server.h"
#include <arpa/inet.h>  //socket��Ҫ
#include <sys/epoll.h>  //epoll��Ҫ
#include <stdio.h>      //NULLͷ�ļ�
#include <unistd.h>
#include <fcntl.h>  //fcntl
#include <string.h>  //memcpy
#include <errno.h>  //errno
#include <strings.h>  //strcasecmp
#include <sys/stat.h>
#include <assert.h>  //����
#include <unistd.h>  //usleep
#include <sys/sendfile.h>  //sendfile
#include <dirent.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
//���νṹ��
struct FDInfo
{
	int fd;
	int epfd;
	pthread_t tid;
}FDInfo;

//����˿�
int initListenFd(unsigned short port)
{
	//1������������fd
	// AF_INET ����IPV4��0 ��ʽЭ���е�TCPЭ��
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1)
	{
		perror("socket"); //����ʧ��
		return -1;
	}
	//2�����ö˿ڸ���
	int opt = 1; //1�˿ڸ���
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret == -1)
	{
		perror("setsockopt"); //����ʧ��
		return -1;
	}
	//3���󶨶˿�
	struct sockaddr_in addr;
	addr.sin_family = AF_INET; //IPV4Э��
	addr.sin_port = htons(port); //���˿�תΪ�����ֽ���
	addr.sin_addr.s_addr = INADDR_ANY; //��������IP��ַ
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1)
	{
		perror("bind"); //ʧ��
		return -1;
	}
	//4�����ü���  һ���Կ��ԺͶ��ٿͻ�������,�ں����128�������ܴ��ں˻��Ϊ128
	ret = listen(lfd, 128);
	if (ret == -1)
	{
		perror("listen");
		return -1;
	}
	printf("Init successful....\n");
	//����fd
	return lfd;
}

int epollRun(int lfd)
{
	//1������epollʵ��
	int epfd = epoll_create(1); //��������0���ɣ�û��ʵ������
	if (epfd == -1)
	{
		perror("epoll_Create");
		return -1;
	}
	// 2,lfd ����
	struct epoll_event ev;
	ev.data.fd = lfd; //ָ�������ļ�������
	ev.events = EPOLLIN; //����Ƿ���������
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
	if (ret == -1)
	{
		perror("epoll_ctl");
		return -1;
	}
	//printf("epoll_stl up open...!\n");
	// 3,��⣬��һ�������Ĺ���
	struct epoll_event evs[1024];
	int size = sizeof(evs) / sizeof(struct epoll_event); //�ܳ���/���Ե�����С���ó�����
	while (1)
	{

		int num = epoll_wait(epfd, evs, size, -1); //����ʱ��-1û���¼���������һֱ����
		//printf("epoll_wait open...!\n");
		//ȡ��
		for (int i = 0; i < num; ++i)
		{
			int fd = evs[i].data.fd;
			struct FDInfo* info = (struct FDInfo*)malloc(sizeof(struct FDInfo));
			info->fd = fd;
			info->epfd = epfd;
			//�ж��ǲ������ڼ�����
			if (fd == lfd)
			{
				//���������� accept �����ӾͲ����������������ӣ������µ��ļ���������ӵ�epoll����
				//acceptClient(fd, epfd);

				pthread_create(&info->tid, NULL, acceptClient, info);
			}
			else
			{
				//�ж��ǽ��ܽ��ն�����,����Ϊhttp����ĸ�ʽ��
				//recvHttpRequest(fd, epfd);

				pthread_create(&info->tid, NULL, recvHttpRequest, info);
			}

		}

	}
	return 0;
}

void* acceptClient(void* arg)
{
	struct FDInfo* info = (struct FDInfo*)arg;
	//printf("start connection ....\n");
	//1����������
	int cfd = accept(info->fd, NULL, NULL);
	if (cfd == -1)
	{
		perror("accept");
		return NULL;
	}
	//ʹ��epoll�ı���ģʽ
	//2,���÷�����
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;  //��λ���������Դ���
	fcntl(cfd, F_SETFL, flag); //��ӵ�cfd���У�F_SETFL�����ļ�״̬
	//3,cfd��ӵ�epoll����
	struct epoll_event ev;
	ev.data.fd = cfd; //ָ�������ļ�������
	ev.events = EPOLLIN | EPOLLET; //����Ƿ��ж��¼�,����ģʽ
	//��ӵ�epoll����
	int ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev);
	if (ret == -1)
	{
		perror("epoll_ctl");
		return NULL;
	}
	printf("recvMsg threadId : %ld\n", info->tid);
	free(info);

	return NULL;
}
void* recvHttpRequest(void* arg)
{
	struct FDInfo* info = (struct FDInfo*)arg;
	printf("recv message....\n");
	// 1�����û����������ȴ浽����
	char buf[4096] = { 0 }; //��
	char tmp[1024] = { 0 }; //��ʱ
	//��Եģʽ����Ҫһ�ΰ���������ȫ������
	int len = 0, total = 0; //lenÿ�ν��ܵ����ݳ��ȣ�totalΪ�ܳ���
	while ((len = (int)recv(info->fd, tmp, sizeof tmp, 0)) > 0)
	{
		//printf("recv while total\n");
		if (total + len < sizeof(buf))
		{
			memcpy(buf + total, tmp, (size_t)len);  //�޸ĸ��Ƶ�ַ
		}
		total += len;

	}
	printf("total = %d\n", total);
	//�ж������Ƿ񱻽������
	if (len == -1 && errno == EAGAIN) //len ����-1 ���Ҵ���ΪEAGAIN
	{
		printf("analyse get request!\n");
		//����������,�ԣ��ȴ���get����
		char* pt = strstr(buf, "\r\n"); // �����������ַ�\r\n ���������ѵ�֮�󣬾ͽ�����
		int reqLen = (int)(pt - buf); //�õ������еĳ��ȣ������ĵ�ַ����ȥ��ʼ�ĵ�ַ
		buf[reqLen] = '\0'; // buf[reqLen-1]����������һ���ַ�
		parseRequestLine(buf, info->fd);
	}
	else if (len == 0)
	{
		printf("client closed!\n");
		//�ͻ��˶Ͽ������ӣ�ɾ����Ӧ���ļ�������
		epoll_ctl(info->epfd, EPOLL_CTL_DEL, info->fd, NULL);
		close(info->fd);
	}
	else
	{
		//printf("recvHttpRequest error len = %d\n", len);
		perror("recv");
	}
	printf("recvMsg threadId : %ld\n", info->tid);
	free(info);
	return NULL;
}

int parseRequestLine(const char* line, int cfd)
{
	//���������У� get / xxx/1.jpg http1.1
	// ������:����ʽ��������Դ������httpЭ��İ汾������֮���пո�
	// sscanf �Ը�ʽ�����ַ������в��
	char method[12];  //get ��post
	char path[1024];  //�洢����ľ�̬��Դ���ļ�·��
	sscanf(line, "%[^ ] %[^ ]", method, path);
	printf("method: %s,path: %s\n", method, path);
	if (strcasecmp(method, "get") != 0)   //strcasecmp�Ƚ�ʱ�����ִ�Сд
	{
		//��get���󲻴���
		return -1;
	}
	decodeMsg(path, path); //�������·��ת�� linux��ת��utf8
	//����ͻ�������ľ�̬��Դ(Ŀ¼���ļ�)
	char* file = NULL;
	if (strcmp(path, "/") == 0) //�ж��ǲ��Ǹ�Ŀ¼
	{
		file = "./";
	}
	else
	{
		file = path + 1;  //�ѿ�ʼ�� /ȥ����

	}
	//�ж�file���ԣ����ļ�����Ŀ¼
	struct stat st;
	int ret = stat(file, &st); //file�ļ����ԣ�ͬʱst�������ļ��Ĵ�С
	if (ret == -1)
	{
		//�ļ�������  -- �ظ�404
		sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
		sendFile("404.html", cfd);
		return 0;

	}
	//�ж��ļ�����
	if (S_ISDIR(st.st_mode)) //���ʱĿ¼����1�����Ƿ���0
	{
		//�����Ŀ¼�е����ݷ��͸��ͻ���
		sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		sendDir(file, cfd);
	}
	else
	{
		//������ļ������ݷ����ͻ���
		sendHeadMsg(cfd, 200, "OK", getFileType(file), (int)st.st_size);
		sendFile(file, cfd);
	}

	return 0;
}

//Ҫ�ظ�http��Ӧ�����ݿ�
int sendFile(const char* filename, int cfd)
{
	//��һ���ַ�һ���֣��������ݵײ�ʹ��TCPЭ��
	//1�����ļ�
	int fd = open(filename, O_RDONLY);
	//�ж��ļ��Ƿ�򿪳ɹ�
	assert(fd > 0); //�������0û�����⣬��������0�������쳣ʧ��

#if 0
	//������
	while (1)
	{
		char buf[1024];
		int len = read(fd, buf, sizeof(buf));
		if (len > 0)
		{
			send(cfd, buf, len, 0); //�������ݸ������
			//�������ͽ���
			usleep(10); // ����������΢��
		}
		else if (len == 0)  //�ļ��Ѿ�������
		{
			break;
		}
		else
		{
			perror("read");
		}
	}
#else
	off_t offset = 0; //sendfile��ƫ�������ж��Ƿ����ݷ������
	int size = (int)lseek(fd, 0, SEEK_END); //��0��ĳλ���ļ�ƫ���������ļ��ж��ٸ��ֽ�
	lseek(fd, 0, SEEK_SET); //�ٰ�ָ���ƶ����ײ�
	//ϵͳ�����������ļ���linux�ں��ṩ��sendfile Ҳ�ܼ��ٿ�������
	//ͨ���ļ������������ļ���������fd��Ӧ���ļ�ƫ����һ��Ϊ�գ�
	while (offset < size)
	{
		int ret = (int)sendfile(cfd, fd, &offset, (size_t)(size - offset));  //�������ļ����ַ��Ͳ�ȫ��offset���Զ��޸ĵ�ǰ��ȡλ��
		printf("ret value: %d\n", ret);
		if (ret == -1 && errno == EAGAIN)
		{
			printf("not data ....");
			perror("sendfile");
		}
	}

#endif
	close(fd); //�رմ򿪵��ļ�
	return 0;
}

//�����ļ�֮ǰ�ȵ���
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length)
{
	// ״̬��
	char buf[4096] = { 0 };
	sprintf(buf, "http/1.1 %d %s\r\n", status, descr); //���ж�Ӧ��\r\n
	//��Ӧͷ
	sprintf(buf + strlen(buf), "content-type: %s\r\n", type); //buf + strlen(buf) ָ�����
	sprintf(buf + strlen(buf), "content-length: %d\r\n\r\n", length); //���һ���ǿ���

	send(cfd, buf, strlen(buf), 0);
	return 0;
}

const char* getFileType(const char* name)
{
	//a.jpg,a.mp4,a.html
	//����������� "."�ַ����粻���ڷ���NULL
	const char* dot = strrchr(name, '.'); //strrchr ����������.,��.���ұߵĺ�׺����name
	if (dot == NULL)
		return "text/plain; charset=utf-8";	// ���ı�
	if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
		return "text/html; charset=utf-8";
	if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
		return "image/jpeg";
	if (strcmp(dot, ".gif") == 0)
		return "image/gif";
	if (strcmp(dot, ".png") == 0)
		return "image/png";
	if (strcmp(dot, ".css") == 0)
		return "text/css";
	if (strcmp(dot, ".au") == 0)
		return "audio/basic";
	if (strcmp(dot, ".wav") == 0)
		return "audio/wav";
	if (strcmp(dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
		return "video/quicktime";
	if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
		return "model/vrml";
	if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
		return "audio/midi";
	if (strcmp(dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcmp(dot, ".ogg") == 0)
		return "application/ogg";
	if (strcmp(dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";

	return "text/plain; charset=utf-8";
}
/*
<html>
	<head>
		<title>test</title>
	</head>
<body>
	<table>
		<tr>
			<td></td>
			<td></td>
		</tr>
		<tr>
			<td></td>
			<td></td>
		</tr>
	</table>
</body>
</html>
*/
int sendDir(const char* dirName, int cfd)
{
	char buf[4096] = { 0 };
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
	struct dirent** namelist;
	int name = scandir(dirName, &namelist, NULL, alphasort); //����Ŀ¼���ж��ٸ��ļ�
	for (int i = 0; i < name; ++i)
	{
		//ȡ���ļ��� namelistָ��һ��ָ������ struct dirent* tmp[]
		char* name = namelist[i]->d_name;
		//�ж��ļ��ǲ���Ŀ¼
		struct stat st;
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName, name);
		//�õ��ļ�����
		stat(subPath, &st); //nameֻ���ļ����ƣ���Ҫƴ�����·�� 
		if (S_ISDIR(st.st_mode))  //�ǲ���Ŀ¼
		{

			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		else
		{
			//�ļ�a��ǩ����Ҫ��б��
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}

		send(cfd, buf, strlen(buf), 0);
		memset(buf, 0, sizeof(buf));
		free(namelist[i]);
	}
	//��html�����ı�ǩ���͸��ַ���
	sprintf(buf, "</table></body></html>");
	send(cfd, buf, strlen(buf), 0);
	free(namelist);
	return 0;
}

// ���ַ�ת��Ϊ������
int hexToDec(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;  //+10��λ
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return 0;
}

// ����
// to �洢����֮�������, ��������, from�����������, �������
void decodeMsg(char* to, char* from)
{
	for (; *from != '\0'; ++to, ++from) //
	{
		// isxdigit -> �ж��ַ��ǲ���16���Ƹ�ʽ, ȡֵ�� 0-f
		// Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// ��16���Ƶ��� -> ʮ���� �������ֵ��ֵ�����ַ� int -> char
			// B2 == 178
			// ��3���ַ�, �����һ���ַ�, ����ַ�����ԭʼ����
			*to = (char)(hexToDec(from[1]) * 16 + hexToDec(from[2]));  //��������תΪʮ����

			// ���� from[1] �� from[2] ����ڵ�ǰѭ�����Ѿ��������
			from += 2; //������һ��%
		}
		else
		{
			// �ַ�����, ��ֵ
			*to = *from;
		}

	}
	*to = '\0';
}
