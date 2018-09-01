#include <QCoreApplication>
#include <iostream>
#include "TcpPackClient.h"


class TcpClientListenerImpl: public CTcpClientListener{
public:

    virtual EnHandleResult OnPrepareConnect(ITcpClient* pSender, CONNID dwConnID, SOCKET socket)			override {
        return HR_IGNORE;
    }

    virtual EnHandleResult OnConnect(ITcpClient* pSender, CONNID dwConnID)									override {
        return HR_IGNORE;
    }

    virtual EnHandleResult OnHandShake(ITcpClient* pSender, CONNID dwConnID)								override {
        return HR_IGNORE;
    }

    virtual EnHandleResult OnReceive(ITcpClient* pSender, CONNID dwConnID, int iLength)						override {
        return HR_IGNORE;
    }

    virtual EnHandleResult OnReceive(ITcpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength)	override{
        std::string strTmp((const char*)pData, iLength);
        std::cout << strTmp << std::endl;
        return HR_OK;
    }

    virtual EnHandleResult OnSend(ITcpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength)		override {
        return HR_IGNORE;
    }

    virtual EnHandleResult OnClose(ITcpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)	override{

        return HR_IGNORE;
    }

public:
    static TcpClientListenerImpl instance;
};

TcpClientListenerImpl TcpClientListenerImpl::instance;

CTcpPackClient g_tcpPackClient(&TcpClientListenerImpl::instance);
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if(!g_tcpPackClient.Start(nullptr, 9999, false)){
        printf("Start server error!");
        std::exit(0);
    }

    return a.exec();
}
