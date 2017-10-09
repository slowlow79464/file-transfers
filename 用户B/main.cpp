#include "stdafx.h"
#include "InitSocket.h"
#define BUFFER_SIZE 1024

bool SndTxt(const char* text);
bool SndFil(const char* fileName);
std::string RevTxt(SOCKET sock);
bool RevFil(SOCKET sock);

void SndTxt();
void SndFil();

struct Head {
#define TYPE_SEND_TEXT 1
#define TYPE_SEND_FILE 2
	int type;//传输类型
	unsigned int len;//传输数据的大小
};

struct FileInfo {
	char name[100];
	unsigned int fileLen;//文件大小
};

void main() {
	InitSocket initSocket;//初始化socket
	char c;
	char ip[32];

	while (1) {

		//std::cout << "please input ip to send" << std::endl;
		//std::cin >> ip;

		std::cout << "please choose what to send (1 text,2 file,q quit)" << std::endl;
		std::cin >> c;
		switch (c)
		{
		case '1':
			SndTxt();
			break;
		case '2':
			SndFil();
			break;
		case 'q':
			return;
		default:
			break;
		}
	}
}

bool SndTxt(const char * text)
{
	Head head;
	SOCKET server;
	u_long lService;
	sockaddr_in addrServer;
	char buffer[BUFFER_SIZE];
	ZeroMemory(buffer, BUFFER_SIZE);
	head.type = 1;
	head.len = strlen(text) + 1;
	server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	inet_pton(AF_INET, "127.0.0.1", &lService);
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.s_addr = lService;
	addrServer.sin_port = htons(4999);
	/*((CIPAddressCtrl*)GetDlgItem(IDC_IPADDRESS_IP))->GetAddress(dwIP);*/
	//addrServer.sin_addr.s_addr = htonl(dwIP);
	int retVal = connect(server, (sockaddr*)&addrServer, sizeof(addrServer));
	if (SOCKET_ERROR == retVal) {
		std::cout << "connect failed!" << std::endl;
		closesocket(server); //关闭套接字  
		return false;
	}
	std::cout << "connect success...\n" << std::endl;
	
	retVal = send(server, (char*)&head, sizeof(head), 0);//发送文本数据头
	if (retVal <= 0) {
		std::cout << "send text failed!" << std::endl;
		closesocket(server);
		return false;
	}
	retVal = send(server, text, strlen(text) + 1, 0);//发送文本数据
	if (retVal <= 0) {
		std::cout << "send text failed!" << std::endl;
		closesocket(server);
		return false;
	}
	retVal = recv(server, buffer, BUFFER_SIZE, 0);
	if (retVal <= 0) {
		std::cout << "receive failed!" << std::endl;
		closesocket(server);
		return false;
	}
	closesocket(server);
	printf(buffer);
	return true;
}


bool SndFil(const char * fileName)
{
	Head head;
	FileInfo fileInfo;
	FILE* fileSend = NULL;
	SOCKET server;
	u_long lService;
	sockaddr_in addrServer;
	char buffer[BUFFER_SIZE];
	ZeroMemory(buffer, BUFFER_SIZE);
	head.type = 2;
	strcpy_s(fileInfo.name, strrchr(fileName,'\\')+1);
	if (fopen_s(&fileSend, fileName, "rb")) {
		std::cout << "open file failed,fileName is "<< fileName << std::endl;
		return false;
	}
	//获取文件大小以及文件头的发送数据区的大小
	fseek(fileSend, 0L, SEEK_END); 
	long size = ftell(fileSend);
	if (size <= 0) {
		std::cout << "get file length failed" << std::endl;
		fclose(fileSend);
		return false;
	}
	fileInfo.fileLen = static_cast<unsigned int>(size);
	head.len = sizeof(fileInfo) + fileInfo.fileLen;
	//将光标移动到文件头
	fseek(fileSend, 0L, SEEK_SET);

	server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	inet_pton(AF_INET, "127.0.0.1", &lService);
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.s_addr = lService;
	addrServer.sin_port = htons(4999);
	int retVal = connect(server, (sockaddr*)&addrServer, sizeof(addrServer));
	if (SOCKET_ERROR == retVal) {
		std::cout << "connect failed!" << std::endl;
		closesocket(server); //关闭套接字  
		return false;
	}
	std::cout << "connect success...\n" << std::endl;
	retVal = send(server, (char*)&head, sizeof(head), 0);//发送文件数据头
	if (retVal <= 0) {
		std::cout << "send text failed!" << std::endl;
		closesocket(server);
		return false;
	}
	retVal = send(server, (char*)&fileInfo, sizeof(fileInfo), 0);//发送文件信息
	if (retVal <= 0) {
		std::cout << "send text failed!" << std::endl;
		closesocket(server);
		return false;
	}
	//发送文件
	int nCount;
	while ((nCount = fread(buffer, 1, BUFFER_SIZE, fileSend)) > 0) {
		send(server, buffer, nCount, 0);
		ZeroMemory(buffer, BUFFER_SIZE);
	}
	fclose(fileSend);

	retVal = recv(server, buffer, BUFFER_SIZE, 0);
	if (retVal <= 0) {
		std::cout << "receive failed!" << std::endl;
		closesocket(server);
		return false;
	}
	printf(buffer);
	closesocket(server);
	return true;
}

std::string RevTxt(SOCKET sock)
{
	return std::string();
}

bool RevFil(SOCKET sock)
{
	return false;
}

void SndTxt()
{
	char buffer[BUFFER_SIZE];
	std::cout << "please input text to send\n" << std::endl;
	std::cin >> buffer;
	SndTxt(buffer);
}

void SndFil()
{
	char szBuffer[MAX_PATH] = { 0 };
	OPENFILENAMEA file = { 0 };
	file.hwndOwner = NULL; 
	file.lStructSize = sizeof(file);
	file.lpstrFilter = "所有文件(*.*)\0*.*\0";//要选择的文件后缀 
	file.lpstrInitialDir = "C:\\";//默认的文件路径 
	file.lpstrFile = szBuffer;//存放文件的缓冲区 
	file.nMaxFile = sizeof(szBuffer) / sizeof(*szBuffer);
	file.nFilterIndex = 0;
	file.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;//标志如果是多选要加上OFN_ALLOWMULTISELECT
	BOOL bSel = GetOpenFileNameA(&file);
	std::cout << file.lpstrFile << std::endl;
	SndFil(file.lpstrFile);
}
