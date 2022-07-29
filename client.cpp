/**
 * @brief Реализация класса клиента.
 */

#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>
#include <mutex>
#include <chrono>
#include "common.h"
#include "client.h"

namespace
{
#define MAX_RECV_SIZE  64

std::mutex clientMtx_;

enum Commands : uint8_t
{
    SEQ1 = 0,
    SEQ2 = 1,
    SEQ3 = 2,
    EXPORT_SEQ = 3,
};

const std::map< std::string, uint8_t > commandsMapper =
{
    { "seq1", SEQ1 },
    { "seq2", SEQ2 },
    { "seq3", SEQ3 },
    { "export seq", EXPORT_SEQ },
};
} // namespace

Client::TcpClient::TcpClient( const int connectedAddr ) :
    socketFd_( connectedAddr ), isConnected_( false )
{
    subSeq1_ = false;
    subSeq2_ = false;
    subSeq3_ = false;
    isGettingSeq_ = false;
    startValues_ = std::vector< uint64_t >( subSeqCnt, 1 );
    currentValues_ = std::vector< uint64_t >( subSeqCnt, 1 );
    steps_ = std::vector< uint64_t >( subSeqCnt, 1 );
}

bool Client::TcpClient::IsConnected() const
{
    return isConnected_;
}

void Client::TcpClient::SetConnStatus( bool status )
{
    isConnected_.store( status ) ;
}

int Client::TcpClient::GetSocket() const
{
    return socketFd_;
}

void Client::TcpClient::Send( const std::string& msg )
{
    if( send( GetSocket(), msg.c_str(), msg.size(), MSG_NOSIGNAL ) <= 0 )
    {
        LogEvent( "Send error: " + std::string( strerror( errno ) ) );
        throw std::runtime_error( strerror( errno ) );
    }
}

ExitStatus Client::TcpClient::SendResSeq()
{
    std::string seqSet;
    clientMtx_.lock();
    for( int i = 0; i < subSeqCnt; ++i )
    {
        seqSet += std::to_string( currentValues_[ i ] ) + " ";
        if( currentValues_[ i ] > 0xFFFFFFFFFFFFFFFF - steps_[ i ] )
        {
            currentValues_[ i ] = startValues_[ i ];
        }
        else
        {
            currentValues_[ i ] += steps_[ i ];
        }
    }

    clientMtx_.unlock();
    clientMtx_.lock();
    Send( seqSet + "\n" );
    clientMtx_.unlock();
    return ExitStatus::Success;
}

ExitStatus Client::TcpClient::CmdParser( std::string& fullCmd,
                                         std::string& cmdName,
                                         uint64_t& startValue,
                                         uint64_t& step )
{
    std::string exportCmd = "export seq";

    if( fullCmd.find(exportCmd) != std::string::npos )
    {
        cmdName = exportCmd;
        startValue = 0;
        step = 0;
        return ExitStatus::Success;
    }

    std::string delimiter = " ";
    size_t pos = 0;
    std::string token;
    pos = fullCmd.find( delimiter );
    if( pos != std::string::npos )
    {
        cmdName = fullCmd.substr( 0, pos );
        fullCmd.erase( 0, pos + delimiter.length() );
    }
    else
    {
        throw std::runtime_error( "Wrong cmd" );
    }
    pos = fullCmd.find( delimiter );
    if( pos != std::string::npos )
    {
        std::string tmp = fullCmd.substr( 0, pos );
        startValue = std::stoull( tmp, &pos );
        fullCmd.erase( 0, pos + delimiter.length() );
    }
    else
    {
        throw std::runtime_error( "Wrong cmd" );
    }
    size_t end = fullCmd.size();
    step = std::stoull( fullCmd, &end );
    return ExitStatus::Success;
}

ExitStatus Client::TcpClient::CmdHadnler( std::string& cmd )
{
    std::string parsedCmd;
    uint64_t startValue;
    uint64_t step;
    clientMtx_.lock();
    try
    {
        CmdParser( cmd, parsedCmd, startValue, step );
    }
    catch(const std::exception& e)
    {
        LogEvent( "Error cmd: " + std::string( e.what() ) );
    }

    clientMtx_.unlock();
    switch( commandsMapper.find(parsedCmd)->second )
    {
    case SEQ1:
        startValues_[ 0 ] = startValue;
        currentValues_[ 0 ] = startValue;
        steps_[ 0 ] = step;
        subSeq1_ = true;
        break;
    case SEQ2:
        startValues_[ 1 ] = startValue;
        currentValues_[ 1 ] = startValue;
        steps_[ 1 ] = step;
        subSeq2_ = true;
        break;
    case SEQ3:
        startValues_[ 2 ] = startValue;
        currentValues_[ 2 ] = startValue;
        steps_[ 2 ] = step;
        subSeq3_ = true;
        break;
    case EXPORT_SEQ:
        if( subSeq1_ && subSeq2_ && subSeq3_ )
        {
            PreprocessSec();
            return ExitStatus::StartPrint;
        }
        else
        {
            LogEvent( "Not all subsequences setuped" );
            return ExitStatus::Failure;
        }
        break;
    default:
        LogEvent( "Unsupported command" );
        return ExitStatus::Failure;
    }
    return ExitStatus::Success;
}

void Client::TcpClient::StartListen()
{
    SetConnStatus( true );
    thread_.reset( new std::thread( &Client::TcpClient::RecieveCmd, this ) );
}

void Client::TcpClient::PreprocessSec()
{
    clientMtx_.lock();
    for( int i = 0; i < subSeqCnt; ++i )
    {
        if( steps_[ i ] == 0 || startValues_[ i ] == 0 )
        {
            steps_.erase( steps_.begin() + i );
            startValues_.erase( startValues_.begin() + i );
            --subSeqCnt;
            --i;
        }
    }
    clientMtx_.unlock();
}

void Client::TcpClient::RecieveCmd()
{
    while( IsConnected() )
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(2ms);
        std::string receivedMessage;
        receivedMessage.resize( MAX_RECV_SIZE );
        const size_t numOfBytesReceived = recv( socketFd_,
                                                const_cast< char * >( receivedMessage.c_str() ),
                                                MAX_RECV_SIZE, 0 );
        if( numOfBytesReceived < 1 )
        {
            std::string disconnectionMessage;
            if( numOfBytesReceived == 0 )
            {
                disconnectionMessage = "Client "
                                        + std::to_string( socketFd_ )
                                        +" closed connection";
            }
            else
            {
                disconnectionMessage = strerror(errno);
            }
            LogEvent( disconnectionMessage );
            CloseConnection();
        }
        else
        {
            LogEvent( receivedMessage );
            if( CmdHadnler( receivedMessage ) == ExitStatus::StartPrint )
            {
                clientMtx_.lock();
                currentValues_ = startValues_;
                clientMtx_.unlock();
                try
                {
                    while( 1 )
                    {
                        using namespace std::chrono_literals;
                        std::this_thread::sleep_for(20ms);
                        if( SendResSeq() == ExitStatus::Failure )
                        {
                            break;
                        }
                    }
                }
                catch( const std::exception& e )
                {
                    SetConnStatus( false );
                }
            }
        }
    }
}

void Client::TcpClient::CloseConnection()
{
    SetConnStatus( false );
}