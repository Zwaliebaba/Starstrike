#include "pch.h"

#include "NetClient.h"
#include "NetHost.h"
#include "NetLayer.h"
#include <mmsystem.h>



NetClient::NetClient(const NetAddr& server_addr)
    : addr(server_addr), sock(0), delta(0), time(0), err(0)
{
}

NetClient::~NetClient()
{
    delete sock;
}



bool
NetClient::Send(std::string msg)
{
    if (msg.length() > 0) {
        if (sock)
            delete sock;

        sock  = NEW NetSock(addr, true);
        delta = 0;
        time  = timeGetTime();

        if (!sock) {
            err = ERR_NOBUFS;
            return false;
        }

        err = sock->send(msg);
        if (err < 0) {
            err = NetLayer::GetLastError();
            return false;
        }

        err = sock->shutdown_output();
        if (err < 0) {
            err = NetLayer::GetLastError();
            return false;
        }

        return true;
    }

    else {
        delete sock;
        sock = 0;
    }

    return false;
}

std::string
NetClient::Recv()
{
    std::string response;

    if (sock) {
        int ready = sock->select();

        while (!ready && timeGetTime() - time < 2000) {
            Sleep(5);
            ready = sock->select();
        }

        if (ready) {
            std::string msg = sock->recv();

            while (msg.length() > 0) {
                response += msg;
                msg = sock->recv();
            }

            delta = timeGetTime() - time;
        }

        delete sock;
        sock = 0;
    }

    return response;
}

std::string
NetClient::SendRecv(std::string msg)
{
    std::string response;

    if (msg.length() > 0 && Send(msg)) {
        response = Recv();
    }

    return response;
}
