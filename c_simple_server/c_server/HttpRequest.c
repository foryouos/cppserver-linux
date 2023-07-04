#define _GNU_SOURCE 
#include "HttpRequest.h"
#include "TcpConnection.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include "log.h"
#define HeaderSize 12
struct HttpRequest* httpRequestInit()
{
	struct HttpRequest* request = (struct HttpRequest*)malloc(sizeof(struct HttpRequest));
	httpRequetReset(request);
	// �ṹ�����ݣ��������ɸ�Ԫ��
	request->reqheaders = (struct RequestHeader*)malloc(sizeof(struct RequestHeader)* HeaderSize);
	
	return request;
}

void httpRequetReset(struct HttpRequest* req)
{
	req->curState = ParseReqLine;
	req->method = NULL;
	req->url = NULL;
	req->version = NULL;
	// �ṹ�������ݣ��������ɸ�Ԫ��
	req->reqHeadersNum = 0;
}

void httpRequetResetEx(struct HttpRequest* req)
{
	free(req->url);
	free(req->method);
	free(req->version);
	// �ͷŵ�reqHeader����
	if (req->reqheaders != NULL)
	{
		for (int i = 0; i < req->reqHeadersNum; ++i)
		{
			free(req->reqheaders[i].key);
			free(req->reqheaders[i].value);
		}
		free(req->reqheaders);
	}
	httpRequetReset(req);

}

void httpRequetDestory(struct HttpRequest* req)
{
	if (req != NULL)
	{
		httpRequetResetEx(req);
		
		free(req);
	}
	//�����ڿպ�������
}

enum HttpRequestState httpRequestState(struct HttpRequest* request)
{
	return request->curState;
}

void httpRequestAddHeader(struct HttpRequest* request, const char* key, const char* value)
{
	request->reqheaders[request->reqHeadersNum].key = (char*)key;
	request->reqheaders[request->reqHeadersNum].value = (char*)value;
	request->reqHeadersNum++;
}

char* httpRequestGetHeader(struct HttpRequest* request, const char* key)
{

	if (request != NULL)
	{
		for (int i = 0; i < request->reqHeadersNum; ++i)
		{
			//strncasecmp �Ƚ�n���ֽڵ��ַ����Ƿ���Ȳ����ִ�Сд
			if (strncasecmp(request->reqheaders[i].key, key, strlen(key)) == 0)
			{
				return request->reqheaders[i].value; 
			}
		}
	}

	return NULL;
}

//���������
// ����һ��ָ�룬ָ��(4���ֽ�)�ڴ��ݹ����л����һ����������ָ�����һ��������������ָ��ĵ�ַ
// �ں����ڲ����ⲿ��һ��ָ������ڴ棬���ⲿһ��ָ��ĵ�ַ��������������ָ�룬
// ���� ������ָ��
char* splitRequestLine(const char* start,const char* end,const char*sub,char**ptr)
{
	char* space = end; //�󳤶ȣ�βָ�� - ��ʼָ��
	if (sub!=NULL)
	{
		space = memmem(start, end-start, sub, strlen(sub));
		assert(space != NULL); //ȡ�����ڴ�鲻Ϊ��
	}
	// ����ʽ�ĳ���
	int length = space - start;
	// ��ָ�룬��ָ��ĵ�ַ(����ָ��)��������ָ��
	// ��ĳ��ָ�븳ֵ
	char* tmp = (char*)malloc(length + 1); 
	strncpy(tmp,start,length);
	tmp[length] = '\0';
	//*ptr ����ָ������ã���Ϊһ��ָ��
	*ptr = tmp;  //*ptr֮����һ��ָ��
	return space + 1;
}

bool parseHttpRequestLine(struct HttpRequest* request,struct Buffer* readBuf)
{
	//���������� �����ַ���������ַ
	char* end = bufferFindCRLF(readBuf);
	// �����ַ�������ʼ��ַ
	char* start = readBuf->data + readBuf->readPos;
	// �ó������е��ܳ���
	int lineSize = end - start;
	// 
	if (lineSize)
	{
		start = splitRequestLine(start, end, " ", &request->method);
		start = splitRequestLine(start, end, " ", &request->url);
		splitRequestLine(start, end, NULL, &request->version);
#if 0
		// get / xxx/xx.txt http/1.1
		// ����ʽ
		char* space = memmem(start, lineSize, " ", 1);
		assert(space != NULL); //ȡ�����ڴ�鲻Ϊ��
		// ����ʽ�ĳ���
		int methodSize = space - start;
		request->method = (char*)malloc(methodSize + 1);
		strncpy(request->method, start, methodSize);
		request->method[methodSize] = '\0';   

		// ����ľ�̬��Դ
		start = space + 1;
		char* space = memmem(start, end-start, " ", 1);
		assert(space != NULL); //ȡ�����ڴ�鲻Ϊ��
		// ����ʽ�ĳ���
		int urlSize = space - start;
		request->url = (char*)malloc(urlSize + 1);
		strncpy(request->url, start, urlSize);
		request->method[urlSize] = '\0';     

		// ������������Э��汾
		start = space + 1;
		// char* space = memmem(start, end - start, " ", 1);
		// assert(space != NULL); //ȡ�����ڴ�鲻Ϊ��
		// ����ʽ�ĳ���
		// int urlSize = space - start;
		request->version = (char*)malloc(end-start + 1);
		strncpy(request->version, start, end - start);
		request->method[end - start] = '\0'; 
#endif

		// Ϊ��������ͷ��׼�� 
		readBuf->readPos += lineSize; //�ƶ���ָ��
		readBuf->readPos += 2;        // ��ָ��
		// �޸�״̬
		request->curState = ParseReqHeaders; //����ͷ�����ɹ�
		return true;
	}
	return false;
}
// readBuf�������Ѿ����µ���ʼλ��
bool parseHttpRequestHeader(struct HttpRequest* request,struct Buffer* readBuf)
{
	char* end = bufferFindCRLF(readBuf);
	if (end != NULL)
	{
		//ͨ�� : ����ȡǰ�������
		// ��ʼλ��
		char* start = readBuf->data + readBuf->readPos;
		int lineSize = end - start;
		//����:�����ַ���
		char* middle = memmem(start, lineSize, ": ",2); //��׼����: �ӿո�ģ���������Ǳ�׼��
		if (middle != NULL)
		{
			char* key = malloc(middle - start + 1);  // 1Ϊ\0�������ַ���
			strncpy(key, start, middle - start);
			key[middle - start] = '\0';

			char* value = malloc(end-middle-2 + 1);  // 2Ϊ��
			strncpy(value, middle+2, end - middle - 2);
			value[end - middle - 2] = '\0';

			httpRequestAddHeader(request, key, value);
			// ָ���ƶ�����һ��
			readBuf->readPos += lineSize;
			readBuf->readPos += 2;
		}
		else
		{
			//���middleΪ�գ�����ͷ���������,��������
			readBuf->readPos += 2;
			// �޸Ľ���״̬
			// TODO����post���󣬰���get������
			request->curState = ParseReqDone;

		}

		return true;
	}
	return false;
}

bool parseHttpRequest(struct HttpRequest* request,struct Buffer* readBuf,
	struct HttpResponse* response, struct Buffer* sendBuf, int socket)
{
	bool flag = true;
	// �Ƚ���������
	while (request->curState != ParseReqDone)
	{
		// ��������ͷĿǰ������״̬����ѡ��
		switch (request->curState)
		{
		case ParseReqLine:
			flag = parseHttpRequestLine(request, readBuf);
			break;
		case ParseReqHeaders:
			flag = parseHttpRequestHeader(request, readBuf);
			break;
		case ParseReqBody: //post�������۲�������
			// ��ȡpost����
			break;
		default:
			break;
		}
		if (!flag)
		{
			return flag;
		}
		//�ж��Ƿ������ϣ������ϣ���Ҫ׼���ظ�������
		if (request->curState == ParseReqDone)
		{
			// 1�����ݽ�������ԭʼ���ݣ��Կͻ���������������
			processHttpRequest(request,response);
			// 2,��֯��Ӧ���ݲ�����
			httpResponsePrepareMsg(response, sendBuf, socket);
		}
	}
	// ״̬��ԭ����֤���ܼ�������ڶ������Ժ������
	request->curState = ParseReqLine;
	//�ٽ�������ͷ
	return flag;
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
// �������get��http����
bool processHttpRequest(struct HttpRequest* request,struct HttpResponse* response)
{
	
	if (strcasecmp(request->method, "get") != 0)   //strcasecmp�Ƚ�ʱ�����ִ�Сд
	{
		//��get���󲻴���
		return -1;
	}
	decodeMsg(request->url, request->url); // �������ĵı������� �������·��ת�� linux��ת��utf8
	//����ͻ�������ľ�̬��Դ(Ŀ¼���ļ�)
	char* file = NULL;
	if (strcmp(request->url, "/") == 0) //�ж��ǲ��Ǹ�Ŀ¼
	{
		file = "./";
	}
	else
	{
		file = request->url + 1;  // ָ��+1 �ѿ�ʼ�� /ȥ����

	}
	//�ж�file���ԣ����ļ�����Ŀ¼
	struct stat st;
	int ret = stat(file, &st); // file�ļ����ԣ�ͬʱ����Ϣ����st�������ļ��Ĵ�С
	if (ret == -1)
	{
		//�ļ�������  -- �ظ�404
		//sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
		//sendFile("404.html", cfd); //����404��Ӧ��html�ļ�
		strcpy(response->fineName, "404.html");
		response->statusCode = NotFound;
		strcpy(response->statusMsg, "Not Found");
		// ��Ӧͷ
		httpResponseAddHeader(response, "content-type", getFileType(".html"));
		response->sendDataFunc = sendFile;
		return 0;

	}
	strcpy(response->fineName, file);
	response->statusCode = OK;
	strcpy(response->statusMsg, "OK");

	//�ж��ļ�����
	if (S_ISDIR(st.st_mode)) //���ʱĿ¼����1�����Ƿ���0
	{
		//�����Ŀ¼�е����ݷ��͸��ͻ���
		//sendHeadMsg(cfd, 200, "OK", getFileType(".html"), (int)st.st_size);
		//sendDir(file, cfd);
		// ��Ӧͷ
		httpResponseAddHeader(response, "content-type", getFileType(".html"));
		response->sendDataFunc = sendDir;
	}
	else
	{
		//������ļ������ݷ����ͻ���
		//sendHeadMsg(cfd, 200, "OK", getFileType(file), (int)st.st_size);
		//sendFile(file, cfd);
		// ��Ӧͷ
		char tmp[12] = { 0 };
		// TODo�޸��ļ��ڴ� ��ʽ
		sprintf(tmp,"%ld", st.st_size);
		httpResponseAddHeader(response, "Content-type", getFileType(file));
		httpResponseAddHeader(response, "Content-length", tmp);
		response->sendDataFunc = sendFile;
	}
	return false;
}


//Ҫ�ظ�http��Ӧ�����ݿ�
void sendFile(const char* filename, struct Buffer* sendBuf, int cfd)
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
			//send(cfd, buf, len, 0); //�������ݸ������
			// ��������ӵ����ͻ�������
			bufferAppendData(sendBuf, buf, len);
			// ���漴��
			// #ifndef��"if not defined"
			// #ifndef���û�ж�������꣬���������Ч�ģ���������ˣ�������Ч��
#ifndef MSG_SEND_AUTO
			bufferSendData(sendBuf, cfd);

			
#endif // !MSG_SEND_AUTO
			//�������ͽ���
			//usleep(10); // ����������΢��
		}
		else if (len == 0)  //�ļ��Ѿ�������
		{
			break;
		}
		else
		{
			close(fd);
			perror("read");
		}
	}
#else
	// TODO
	off_t offset = 0; //sendfile��ƫ�������ж��Ƿ����ݷ������
	int size = (int)lseek(fd, 0, SEEK_END); //��0��ĳλ���ļ�ƫ���������ļ��ж��ٸ��ֽ�
	lseek(fd, 0, SEEK_SET); //�ٰ�ָ���ƶ����ײ�
	//ϵͳ�����������ļ���linux�ں��ṩ��sendfile Ҳ�ܼ��ٿ�������
	// sendfile�����ļ�Ч�ʸߣ����ļ�Ŀ¼ʹ��send
	//ͨ���ļ������������ļ���������fd��Ӧ���ļ�ƫ����һ��Ϊ�գ�
	//ѭ������
	int count = 0;
	while (offset < size)
	{
		//int ret = bufferSendFileData(cfd, fd,offset, size);
		int ret = (int)sendfile(cfd, fd, &offset, (size_t)(size - offset));
		if (ret == -1 && errno == EAGAIN)
		{
			printf("not data ....");
			perror("sendfile");
		}
		count += ret;
		//Debug("cout: %d,offset:%d",count, offset);
		
	}
	
		
	//while (offset < size)
	//{
	//	// int ret = (int)sendfile(cfd, fd, &offset, (size_t)(size - offset));  //�������ļ����ַ��Ͳ�ȫ��offset���Զ��޸ĵ�ǰ��ȡλ��
	//	bufferSendFileData(sendBuf, cfd, fd);
	//	printf("ret value: %d\n", ret);
	//	if (ret == -1 && errno == EAGAIN)
	//	{
	//		printf("not data ....");
	//		perror("sendfile");
	//	}
	//}

#endif
	close(fd); //�رմ򿪵��ļ�
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
// ����Ӻõ����ݿ�ŵ����ͻ�������
void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd)
{
	char buf[4096] = { 0 };
	sprintf(buf, "<html><head><title>%s</title></head><style type=\"text/css\">a{text-decoration:none};</style>\
		<body style=\"color: #4f6b72;background: #E6EAE9;\"><table style=\"margin: 10px;\">", dirName);
	struct dirent** namelist;
	int num = scandir(dirName, &namelist, NULL, alphasort); //����Ŀ¼���ж��ٸ��ļ�
	for (int i = 0; i < num; ++i)
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

		//send(cfd, buf, strlen(buf), 0);
		bufferAppendString(sendBuf, buf); //��ƴ�Ӻõ����ݷ��͵���������
#ifndef MSG_SEND_AUTO
		bufferSendData(sendBuf, cfd);
#endif // !MSG_SEND_AUTO
		memset(buf, 0, sizeof(buf));
		free(namelist[i]);
	}
	//��html�����ı�ǩ���͸��ַ���
	sprintf(buf, "</table></body></html>");
	// send(cfd, buf, strlen(buf), 0);
	bufferAppendString(sendBuf, buf); //��ƴ�Ӻõ����ݷ��͵���������
#ifndef MSG_SEND_AUTO
	bufferSendData(sendBuf, cfd);
#endif // !MSG_SEND_AUTO
	free(namelist);
}

