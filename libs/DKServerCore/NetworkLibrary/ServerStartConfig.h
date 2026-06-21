#pragma once
#include <string>


namespace DKServerCore
{
    struct IocpServerStartConfig
    {
        std::string ip;
        unsigned int port;
        unsigned int workerThreadCount;
        unsigned int concurrentThreadCount;
        unsigned int nagle;
        unsigned int maxSessionCount;
        unsigned int headerSize;
        unsigned char packetCode;
    };

    struct SelectServerStartConfig
    {
        std::string ip;
        unsigned int port;
        unsigned int nagle;
        unsigned int maxSessionCount;
        unsigned int frameMs;
        unsigned char packetCode;
    };



}