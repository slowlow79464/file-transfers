#include "stdafx.h"
#include "InitSocket.h"

/*
2017-10-09
软件存在问题：
1.发送放没有选择IP
2.发送一次即断开连接
3.没有实现多线程文件传输
.....
*/

DWORD WINAPI AcpThdProc(LPVOID lpparam);
DWORD WINAPI RevThdProc(LPVOID lpparam);
bool InitSevSock();
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
	InitSevSock();

	system("pause");
}

bool InitSevSock()
{
	SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//服务器套接字  
	if (INVALID_SOCKET == server)
	{
		printf("socket failed!");
		return  false;
	}
	SOCKADDR_IN     addrServ;;      //服务器地址  
									//服务器套接字地址   
	addrServ.sin_family = AF_INET;
	addrServ.sin_port = htons(4999);
	addrServ.sin_addr.s_addr = INADDR_ANY;
	//绑定套接字  
	int retVal = bind(server, (LPSOCKADDR)&addrServ, sizeof(SOCKADDR_IN));
	if (SOCKET_ERROR == retVal)
	{
		printf("bind failed!");
		closesocket(server);   //关闭套接字  
		return false;
	}
	//开始监听   
	retVal = listen(server, 100);
	if (SOCKET_ERROR == retVal)
	{
		printf("listen failed!");
		closesocket(server);   //关闭套接字  
		return false;
	}
	//接受客户端连接
	CreateThread(NULL, 0, AcpThdProc, (LPVOID)server, 0, NULL);
	printf("初始化服务端socket成功!\n");
	return true;
}

DWORD WINAPI AcpThdProc(LPVOID lpparam)
{
	//传来的服务端套接字
	SOCKET  server = (SOCKET)lpparam;
	SOCKADDR_IN addrClient;
	int len = sizeof(addrClient);
	while (true) {
		printf("Waiting for a connection...");
		SOCKET handler = accept(server, (LPSOCKADDR)&addrClient, &len);
		if (INVALID_SOCKET == handler)
		{
			printf("accept failed!");
			closesocket(server);   //关闭套接字  
			return -1;
		}
		char addr[20];
		inet_ntop(AF_INET, &addrClient.sin_addr, addr, 20);
		printf("%s connected .", addr);
		//接收客户端发来的数据
		CreateThread(NULL, 0, RevThdProc, (LPVOID)handler, 0, NULL);
	}
	//closesocket(server);
	return 0;
}

DWORD WINAPI RevThdProc(LPVOID lpparam)
{
#define BUFFER_SIZE 1024
	//传来的连接套接字
	SOCKET  handler = (SOCKET)lpparam;
	Head head;
	int byteRecv;
	byteRecv = recv(handler, (char*)&head, sizeof(head), 0);
	if (byteRecv == SOCKET_ERROR) {
		printf("received head failed, error code is %d", WSAGetLastError());
		closesocket(handler);
		return -1;
	}
	char buffer[BUFFER_SIZE];
	ZeroMemory(buffer, BUFFER_SIZE);//相当于	memset
	int byteRead = 0;

	switch (head.type)
	{
	case TYPE_SEND_TEXT:
	{
		printf("传输的是文本数据\n");
		std::string strRecv;
		//接收到数据
		while (head.len - byteRead>0 && (byteRecv = recv(handler, buffer, head.len - byteRead < BUFFER_SIZE ? head.len - byteRead : BUFFER_SIZE, 0)) > 0) {
			byteRead += byteRecv;
			strRecv.append(buffer);
			ZeroMemory(buffer, BUFFER_SIZE);
		}
		if (byteRecv == SOCKET_ERROR) {
			printf("received data failed, error code is %d", WSAGetLastError());
			closesocket(handler);
			return -1;
		}
		printf("收到的数据为：\n%s\n", strRecv.c_str());
	}
	break;
	case TYPE_SEND_FILE:
	{
		printf("传输的是文件数据\n");
		//接收文件信息
		FileInfo fileInfo;
		while (byteRead != sizeof(fileInfo)) {
			byteRecv = recv(handler, (char*)&fileInfo, sizeof(fileInfo) - byteRead, 0);
			if (byteRecv == SOCKET_ERROR) {
				printf("received fileInfo failed, error code is %d", WSAGetLastError());
				closesocket(handler);
				return -1;
			}
			byteRead += byteRecv;
		}
		printf("收到的文件名为：%s,文件大小为：%d Byte\n", fileInfo.name, fileInfo.fileLen);
		//如果目标文件已存在，则在其文件名后+ （i）
		FILE * pRecvFile = NULL;
		int i = 0;
		std::string fileName = fileInfo.name;
		char* p = strrchr(fileInfo.name, '.');
		while (!fopen_s(&pRecvFile, fileName.c_str(), "r")) {
			fclose(pRecvFile);
			i++; fileName = "";
			fileName.append(fileInfo.name, p)
			.append("(")
			.append(std::to_string(i))
			.append(")").append(p);
		}
		//以二进制写的方式打开二进制文件
		int retVal = fopen_s(&pRecvFile, fileName.c_str(), "wb");
		if (retVal != 0) {
			printf("写文件时打开文件失败，文件名为：%s\n", fileName.c_str());
			fclose(pRecvFile);
			closesocket(handler);
			return -1;
		}
		while (head.len - byteRead > 0 && (byteRecv = recv(handler, buffer, head.len - byteRead < BUFFER_SIZE ? head.len - byteRead : BUFFER_SIZE, 0)) > 0) {
			byteRead += byteRecv;
			//将接收到的数据写入文件
			if (fwrite(buffer, sizeof(char), byteRecv, pRecvFile) != byteRecv) {
				printf("写入文件错误，文件名为：%s\n", fileName.c_str());
				fclose(pRecvFile);
				closesocket(handler);
				return -1;
			}
			printf("传输进度：%lf\n", (double)byteRead / head.len);
			ZeroMemory(buffer, BUFFER_SIZE);
		}
		fclose(pRecvFile);//关闭写入的文件  切记不要忘了这步
		if (byteRecv == SOCKET_ERROR) {
			printf("received data failed, error code is %d", WSAGetLastError());
			closesocket(handler);
			return -1;
		}
		printf("文件写入成功！文件名为%s\n", fileName.c_str());
	}
	break;
	default:
		break;
	}

	char sendBuffer[] = "接收成功\n";
	send(handler, sendBuffer, strlen(sendBuffer) + 1, 0);//给客户端返回消息
	closesocket(handler);
	return 0;
}