/**
 * @brief Класс сервера, который отвечает за открытие сокета, привязку, приём соединений.
 */

#ifndef CONNECT_H
#define CONNECT_H

#include <cstdint>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#include "common.h"
#include "client.h"

#define SEQ_SIZE 3

namespace Server
{

enum class ConnectStatus : uint8_t
{
    Connected = 0,
    ErrSocketInit,
    ErrSocketBind,
    ErrSocketConnect,
    ErrSockerKeepAlive,
    ErrSocketListen,
    Disconnected,
};

class TcpServer
{
private:
    int socketFd_;
    struct sockaddr_in serverAddress_;
    std::vector< Client::TcpClient* > clients_;
    std::mutex clientsMtx_;
    void ListenToClients( int maxConnections );
    ExitStatus Send( const Client::TcpClient& client, const std::string& msg );
    ExitStatus HandleClientCmd( const Client::TcpClient& clien, std::string& cmd );
public:
    TcpServer();
    ~TcpServer();

    /**
     * @brief Функция для старта сервера.
     */
    ExitStatus StartServer( const int port, int maxConnections );

    /**
     * @brief Функция установки опций сокета.
     */
    ExitStatus InitSocket();

    /**
     * @brief Функция привязки к сокету.
     */
    ConnectStatus BindAddr( int port );

    /**
     * @brief Функция обработки соединения.
     */
    ExitStatus AcceptConnection();

    /**
     * @brief Функция закрытия сервера.
     */
    ExitStatus Close();
};

} // namespace Server
#endif // CONNECT_H