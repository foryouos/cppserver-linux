//#define _GNU_SOURCE 
#include "HttpRequest.h"
#include "TcpConnection.h"
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
#include <sys/sendfile.h>
HttpRequest::HttpRequest()
{
	Reset();
}

HttpRequest::~HttpRequest()
{
}

void HttpRequest::Reset()
{
	m_curState = PressState::ParseReqLine;
	m_method = m_url = m_version = string();
	m_reqHeaders.clear();

}


void HttpRequest::addHeader(const string key, const string value)
{
	if (key.empty() || value.empty())
	{
		return;
	}
	m_reqHeaders.insert(make_pair(key, value));
}

string HttpRequest::getHeader(const string key)
{
	auto item = m_reqHeaders.find(key);
	if (item == m_reqHeaders.end())  //�ж��Ƿ���������
	{
		//û��������
		return string();
	}
	// �������ˣ�first��keyֵsecond valueֵ
	return item->second; //ȡvalueֵ
}
bool HttpRequest::parseRequestLine(Buffer* readBuf)
{
	//���������� �����ַ���������ַ
	char* end = readBuf->findCRLF();
	// �����ַ�������ʼ��ַ
	char* start = readBuf->data();
	// �ó������е��ܳ���
	int lineSize = end - start;
	// 
	if (lineSize > 0)
	{
		auto methodFunc = bind(&HttpRequest::setMethod, this, placeholders::_1);
		start = splitRequestLine(start, end, " ", methodFunc);
		auto urlFunc = bind(&HttpRequest::seturl, this, placeholders::_1);
		start = splitRequestLine(start, end, " ", urlFunc);
		auto versionFunc = bind(&HttpRequest::setVersion, this, placeholders::_1);
		splitRequestLine(start, end, NULL, versionFunc);
		// Ϊ��������ͷ��׼�� 
		readBuf->readPosIncrease(lineSize+2); //�ƶ���ָ��
		// �޸�״̬
		setState(PressState::ParseReqHeaders);
		 //����ͷ�����ɹ�
		return true;
	}
	return false;
}

bool HttpRequest::parseRequestHeader(Buffer* readBuf)
{
	char* end = readBuf->findCRLF();
	if (end != nullptr)
	{
		//ͨ�� : ����ȡǰ�������
		// ��ʼλ��
		char* start = readBuf->data();
		int lineSize = end - start;
		//����:�����ַ���
		char* middle = static_cast<char*>(memmem(start, lineSize, ": ", 2)); //��׼����: �ӿո�ģ���������Ǳ�׼��
		if (middle != nullptr)
		{
			int keyLen = middle - start;
			int valueLen = end - middle - 2;
			if (keyLen > 0 && valueLen > 0)
			{
				string key(start,keyLen);
				string value(middle+2,valueLen);
				addHeader(key, value);
			}
			// ָ���ƶ�����һ��
			readBuf->readPosIncrease(lineSize + 2);
		}
		else
		{
			//���middleΪ�գ�����ͷ���������,��������
			readBuf->readPosIncrease(2);
			// �޸Ľ���״̬
			// TODO����post���󣬰���get������
			setState(PressState::ParseReqDone);
		}

		return true;
	}
	return false;
}

bool HttpRequest::parseHttpRequest(Buffer* readBuf, HttpResponse* response, Buffer* sendBuf, int socket)
{
	bool flag = true;
	// �Ƚ���������
	while (m_curState !=PressState::ParseReqDone)
	{
		// ��������ͷĿǰ������״̬����ѡ��
		switch (m_curState)
		{
		case PressState::ParseReqLine:
			flag = parseRequestLine(readBuf);
			break;
		case PressState::ParseReqHeaders:
			flag = parseRequestHeader(readBuf);
			break;
		case PressState::ParseReqBody: //post�������۲�������
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
		if (m_curState == PressState::ParseReqDone)
		{
			// 1�����ݽ�������ԭʼ���ݣ��Կͻ���������������
			processHttpRequest(response);
			// 2,��֯��Ӧ���ݲ�����
			response->prepareMsg(sendBuf, socket);
		}
	}
	// ״̬��ԭ����֤���ܼ�������ڶ������Ժ������
	m_curState = PressState::ParseReqLine;
	//�ٽ�������ͷ
	return flag;
}

bool HttpRequest::processHttpRequest(HttpResponse* response)
{
	if (strcasecmp(m_method.data(), "get") != 0)   //strcasecmp�Ƚ�ʱ�����ִ�Сд
	{
		//��get���󲻴���
		return -1;
	}
	m_url = decodeMsg(m_url); // �������ĵı������� �������·��ת�� linux��ת��utf8
	//����ͻ�������ľ�̬��Դ(Ŀ¼���ļ�)
	const char* file = NULL;
	if (strcmp(m_url.data(), "/") == 0) //�ж��ǲ��Ǹ�Ŀ¼
	{
		file = "./";
	}
	else
	{
		file = m_url.data() + 1;  // ָ��+1 �ѿ�ʼ�� /ȥ����

	}
	//�ж�file���ԣ����ļ�����Ŀ¼
	struct stat st;
	int ret = stat(file, &st); // file�ļ����ԣ�ͬʱ����Ϣ����st�������ļ��Ĵ�С
	if (ret == -1)
	{
		//�ļ�������  -- �ظ�404
		//sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
		//sendFile("404.html", cfd); //����404��Ӧ��html�ļ�
		response->setFileName("404.html");
		response->setStatusCode(StatusCode::NotFound);
		// ��Ӧͷ
		response->addHeader("Content-type", getFileType(".html"));
		response->sendDataFunc = sendFile;
		return 0;

	}
	response->setFileName(file);
	response->setStatusCode(StatusCode::OK);
	//�ж��ļ�����
	if (S_ISDIR(st.st_mode)) //���ʱĿ¼����1�����Ƿ���0
	{
		//�����Ŀ¼�е����ݷ��͸��ͻ���
		//sendHeadMsg(cfd, 200, "OK", getFileType(".html"), (int)st.st_size);
		//sendDir(file, cfd);
		// ��Ӧͷ
		response->addHeader("Content-type", getFileType(".html"));
		response->sendDataFunc = sendDir;
	}
	else
	{
		//������ļ������ݷ����ͻ���
		//sendHeadMsg(cfd, 200, "OK", getFileType(file), (int)st.st_size);
		//sendFile(file, cfd);
		// ��Ӧͷ
		response->addHeader("Content-type", getFileType(file));
		response->addHeader("Content-length", to_string(st.st_size));
		response->sendDataFunc = sendFile;
	}
	return false;
}

string HttpRequest::decodeMsg(string msg)
{
	string str = string();
	const char* from = msg.data();
	for (; *from != '\0';++from) //
	{
		// isxdigit -> �ж��ַ��ǲ���16���Ƹ�ʽ, ȡֵ�� 0-f
		// Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// ��16���Ƶ��� -> ʮ���� �������ֵ��ֵ�����ַ� int -> char
			// B2 == 178
			// ��3���ַ�, �����һ���ַ�, ����ַ�����ԭʼ����
			//��������תΪʮ����
			str.append(1, (char)(hexToDec(from[1]) * 16 + hexToDec(from[2])));

			// ���� from[1] �� from[2] ����ڵ�ǰѭ�����Ѿ��������
			from += 2; //������һ��%
		}
		else
		{
			// �ַ�����, ��ֵ
			str.append(1, *from);
		}

	}
	str.append(1, '\0'); //��Ҫ�����ݴ�ŵ�str����
	return str;
}

const string HttpRequest::getFileType(const string name)
{
	//string(const char*) //����������������Ĺ��캯��������ʽ����ת��
	//a.jpg,a.mp4,a.html
	//����������� "."�ַ����粻���ڷ���NULL
	const char* dot = strrchr(name.data(), '.'); //strrchr ����������.,��.���ұߵĺ�׺����name
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

int HttpRequest::hexToDec(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;  //+10��λ
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return 0;
}

void HttpRequest::sendFile(string filename, Buffer* sendBuf, int cfd)
{
	//��һ���ַ�һ���֣��������ݵײ�ʹ��TCPЭ��
//1�����ļ�
	int fd = open(filename.data(), O_RDONLY);
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
			sendBuf->appendString(buf);
			// ���漴��
			// #ifndef��"if not defined"
			// #ifndef���û�ж�������꣬���������Ч�ģ���������ˣ�������Ч��
#ifndef MSG_SEND_AUTO
			sendBuf->sendData(cfd);


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
	sendBuf->sendData(cfd, fd, offset, size);
	//while (offset < size)
	//{
	//	//int ret = bufferSendFileData(cfd, fd,offset, size);
	//	int ret = (int)sendfile(cfd, fd, &offset, (size_t)(size - offset));
	//	if (ret == -1 && errno == EAGAIN)
	//	{
	//		printf("not data ....");
	//		perror("sendfile");
	//	}
	//}

#endif
	close(fd); //�رմ򿪵��ļ�
}

void HttpRequest::sendDir(string dirName, Buffer* sendBuf, int cfd)
{
	char buf[4096] = { 0 };
	sprintf(buf, "<html><head><title>%s</title></head><style type=\"text/css\">a{text-decoration:none};</style>\
		<body style=\"color: #4f6b72;background: #E6EAE9;\"><table style=\"margin: 10px;\">", dirName.data());
	struct dirent** namelist;
	int num = scandir(dirName.data(), &namelist, NULL, alphasort); //����Ŀ¼���ж��ٸ��ļ�
	for (int i = 0; i < num; ++i)
	{
		//ȡ���ļ��� namelistָ��һ��ָ������ struct dirent* tmp[]
		char* name = namelist[i]->d_name;
		//�ж��ļ��ǲ���Ŀ¼
		struct stat st;
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName.data(), name);
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
		sendBuf->appendString(buf); //��ƴ�Ӻõ����ݷ��͵���������
#ifndef MSG_SEND_AUTO
		sendBuf->sendData(cfd);
#endif // !MSG_SEND_AUTO
		memset(buf, 0, sizeof(buf));
		free(namelist[i]);
	}
	//��html�����ı�ǩ���͸��ַ���
	sprintf(buf, "</table></body></html>");
	// send(cfd, buf, strlen(buf), 0);
	sendBuf->appendString(buf); //��ƴ�Ӻõ����ݷ��͵���������
#ifndef MSG_SEND_AUTO
	sendBuf->sendData(cfd);
#endif // !MSG_SEND_AUTO
	free(namelist);
}

char* HttpRequest::splitRequestLine(const char* start, const char* end, const char* sub, function<void(string)> callback)
{
	char* space = const_cast<char*>(end); //�󳤶ȣ�βָ�� - ��ʼָ��
	if (sub != nullptr)
	{
		space = static_cast<char*>(memmem(start, end - start, sub, strlen(sub)));
		assert(space != nullptr); //ȡ�����ڴ�鲻Ϊ��
	}
	// ����ʽ�ĳ���
	int length = space - start;
	callback(string(start, length));
	return space + 1;
}
