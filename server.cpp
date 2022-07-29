/**
 * @brief Реализация класса сервера.
 */

#include <iostream>

#include <thread>
#include <cstring>
#include <algorithm>
#include <unistd.h>
#include <arpa/inet.h>

#include "server.h"

namespace Server
{
TcpServer::TcpServer()
{
    clients_.reserve(10);
}
TcpServer::~TcpServer()
{
    Close();
}

ExitStatus TcpServer::StartServer( int port, int maxConnections )
{
    try
    {
        InitSocket();
        BindAddr( port );
        ListenToClients( maxConnections );
    } catch( const std::runtime_error &error )
    {
        LogEvent( "Error: " + std::string( error.what() ) );
        return ExitStatus::Failure;
    }
    return ExitStatus::Success;
}

ExitStatus TcpServer::InitSocket()
{
    socketFd_ = socket( AF_INET, SOCK_STREAM, 0 );
    if( socketFd_ < 0 )
    {
        std::string errorMsg;
        const int size = 256;
        errorMsg.resize( size );
        strerror_r( errno, const_cast< char* >( errorMsg.data() ), size );
        LogEvent( errorMsg );
        throw std::runtime_error( errorMsg.data() );
    }

    const int option = 1;
    if( setsockopt( socketFd_, SOL_SOCKET, SO_REUSEADDR, &option, sizeof( option ) ) == 0 )
    {
        return ExitStatus::Success;
    }
    else
    {
        return ExitStatus::Failure;
    }
}

ConnectStatus TcpServer::BindAddr( int port )
{
    memset( &serverAddress_, 0, sizeof( serverAddress_ ) );
    serverAddress_.sin_family = AF_INET;
    serverAddress_.sin_addr.s_addr = htonl( INADDR_ANY );
    serverAddress_.sin_port = htons( port );

    const int bindResult = bind( socketFd_,
                                 reinterpret_cast< struct sockaddr * >( &serverAddress_ ),
                                 sizeof( serverAddress_ ) );
    if( bindResult == -1 )
    {
        throw std::runtime_error( strerror( errno ) );
    }
    return ConnectStatus::Connected;
}

void TcpServer::ListenToClients( int maxNumOfClients )
{
    const int clientsQueueSize = maxNumOfClients;
    if( listen( socketFd_, clientsQueueSize ) == -1 )
    {
        throw std::runtime_error( strerror( errno ) );
    }
}

ExitStatus TcpServer::Send( const Client::TcpClient& client, const std::string& msg )
{
    try
    {
        clientsMtx_.lock();
        send( client.GetSocket(), msg.c_str(), msg.size(), 0 );
        clientsMtx_.unlock();
    } catch( const std::runtime_error &error )
    {
        LogEvent( error.what() );
        return ExitStatus::Failure;
    }

    return ExitStatus::Success;
}

ExitStatus TcpServer::AcceptConnection()
{
    sockaddr_in newClientAddr;
    socklen_t socketSize  = sizeof( newClientAddr );
    const int fileDescriptor = accept( socketFd_,
                                       reinterpret_cast< struct sockaddr* >( &newClientAddr ),
                                       &socketSize );

    if (fileDescriptor == -1)
    {
        throw std::runtime_error( strerror( errno ) );
    }
    clientsMtx_.lock();
    auto newClient = new Client::TcpClient( fileDescriptor );
    clientsMtx_.unlock();
    newClient->StartListen();

    clientsMtx_.lock();
    clients_.push_back( newClient );
    clientsMtx_.unlock();

    return ExitStatus::Success;
}

ExitStatus TcpServer::Close()
{
    std::lock_guard< std::mutex > lock( clientsMtx_ );
    LogEvent( "Closing socket" );
    for( auto client : clients_ )
    {
        try
        {
            if( ::close( client->GetSocket() ) == -1 )
            {
                LogEvent( "Failed to close connection with: " +
                          std::to_string( client->GetSocket() ) );
                throw std::runtime_error( strerror( errno ) );
            }
        } catch( const std::runtime_error &error )
        {
            LogEvent( error.what() );
            return ExitStatus::Failure;
        }
    }

    if ( ::close( socketFd_ ) == -1 )
    {
        LogEvent( "error.what()" );
        return ExitStatus::Failure;
    }
    return ExitStatus::Success;
}

} // namespace