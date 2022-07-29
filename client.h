/**
 * @brief Класс клиента, который отвечает за обработку входящих запросов от клиентов.
 */

#ifndef CLIENT_H
#define CLIENT_H
#include <atomic>
#include <string>
#include <thread>
#include <vector>
#include <memory>

namespace Client
{
class TcpClient
{
private:
    int subSeqCnt = 3;
    int socketFd_;
    std::string ip_;
    std::atomic< bool > isConnected_;
    std::atomic< bool > subSeq1_;
    std::atomic< bool > subSeq2_;
    std::atomic< bool > subSeq3_;
    std::atomic< bool > isGettingSeq_;
    std::vector< uint64_t > startValues_;
    std::vector< uint64_t > currentValues_;
    std::vector< uint64_t > steps_;
    std::shared_ptr< std::thread > thread_;

    ExitStatus CmdParser( std::string& fullCmd,
                          std::string& cmdName,
                          uint64_t& startValue,
                          uint64_t& step );
    void PreprocessSec();
public:
    TcpClient( const int );
    /**
     * @brief Функция проверки состояния подключения.
     */
    bool IsConnected() const;

    /**
     * @brief Функция установки состояния подключения.
     */
    void SetConnStatus( bool );

    /**
     * @brief Функция получения состояния сокета.
     */
    int GetSocket() const;

    /**
     * @brief Функция для получения и обработки команд клиента.
     */
    void StartListen();

    /**
     * @brief Функция отправки сообщения клиенту.
     */
    void Send( const std::string& msg );

    /**
     * @brief Функция создания и отправки последовательности клиенту.
     */
    ExitStatus SendResSeq();

    /**
     * @brief Функция для получения и обработки команд клиента.
     */
    void RecieveCmd();

    /**
     * @brief Функция обработки команд клиента.
     */
    ExitStatus CmdHadnler( std::string& );

    /**
     * @brief Функция закрытия соединения.
     */
    void CloseConnection();
};
}

#endif