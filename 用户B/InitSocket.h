#pragma once
#include <mutex>
class InitSocket {
//private:
//	InitSocket();
	//InitSocket(const InitSocket&) = delete;
	//InitSocket& InitSocket::operator=(const InitSocket&) = delete;
	////µ¥Àý //ÀÁººÊ½
	//static InitSocket* pInstance;
	//static std::mutex mt;
public:
	//static InitSocket* instance();
	//static void destroyInstance();
	InitSocket();
	~InitSocket();
};