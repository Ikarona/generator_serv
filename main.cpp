#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <map>

#include "server.h"
#include "client.h"
#include "common.h"

Server::TcpServer server;

int main( int argc, char* argv[] )
{
    if( server.StartServer( 1337, 10 ) != ExitStatus::Success )
    {
        std::cout << "Server setup failed: " << std::endl;
        return EXIT_FAILURE;
    }
    else
    {
        LogEvent( "Server ready" );
    }

    while( 1 )
    {
        try
        {
            LogEvent( "Waiting for incoming client..." );
            if( server.AcceptConnection() == ExitStatus::Failure )
            {
                throw std::runtime_error( "Accept error" );
            }
            LogEvent( "Accepted new client" );
        }
        catch( const std::runtime_error &error )
        {
            LogEvent( "Accepting client failed: " + std::string( error.what() ) );
            return 0;
        }
    }

    return 0;

}