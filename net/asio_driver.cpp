#include "driver.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>

#include <cstdio>
#include <memory>

namespace asio = boost::asio;

asio::io_service ioService;

void run()
{
    ioService.run();
}

class AsioSSLTCPConnection : public std::enable_shared_from_this<AsioSSLTCPConnection>
{
public:
    AsioSSLTCPConnection(asio::io_service& ioService, NetConnection* conn) :
    m_resolver(ioService),
    m_context(asio::ssl::context::tlsv12_client),
    m_socket(ioService),
    m_stream(m_socket, m_context),
    m_connection(conn),
    m_secure(false)
    {
        printf("construct tcp\n");
        m_context.set_verify_mode(asio::ssl::verify_none);
    }

    ~AsioSSLTCPConnection()
    {
        printf("destruct tcp\n");
    }

    void disconnect()
    {
        printf("disconnect\n");
        boost::system::error_code temp;
        if(m_secure)
            m_stream.shutdown(temp);
        m_socket.shutdown(asio::socket_base::shutdown_both, temp);
    }

    void asyncConnect(const std::string& hostname)
    {
        printf("connect\n");
        m_secure = false;
        asio::ip::tcp::resolver::query query(hostname, "http");
        m_resolver.async_resolve(query, boost::bind(&AsioSSLTCPConnection::resolveHandler, shared_from_this(),
            asio::placeholders::error, asio::placeholders::iterator));
    }

    void asyncSecureConnect(const std::string& hostname)
    {
        printf("secure connect\n");
        m_secure = true;
        asio::ip::tcp::resolver::query query(hostname, "https");
        m_resolver.async_resolve(query, boost::bind(&AsioSSLTCPConnection::resolveHandler, shared_from_this(),
            asio::placeholders::error, asio::placeholders::iterator));
    }

    void asyncWrite(const void* data, size_t size)
    {
        printf("write %lu bytes\n", size);
        if(m_secure)
        {
            asio::async_write(m_stream, asio::const_buffers_1(data, size),
                boost::bind(&AsioSSLTCPConnection::writeHandler, shared_from_this(), asio::placeholders::error,
                    asio::placeholders::bytes_transferred));
        }
        else
        {
            asio::async_write(m_socket, asio::const_buffers_1(data, size),
                boost::bind(&AsioSSLTCPConnection::writeHandler, shared_from_this(), asio::placeholders::error,
                    asio::placeholders::bytes_transferred));
        }
    }

private:
    typedef asio::ssl::stream<asio::ip::tcp::socket&> Socket;

    asio::ip::tcp::resolver m_resolver;
    asio::ssl::context m_context;
    asio::ip::tcp::socket m_socket;
    Socket m_stream;
    char m_buffer[1024];
    NetConnection* m_connection;
    bool m_secure;

    void resolveHandler(const boost::system::error_code& error, asio::ip::tcp::resolver::iterator iterator)
    {
        if(!error)
        {
            if(iterator != asio::ip::tcp::resolver::iterator())
            {
                printf("resolved %s\n", iterator->endpoint().address().to_string().c_str());
                asio::async_connect(m_socket, iterator,
                    boost::bind(&AsioSSLTCPConnection::connectHandler, shared_from_this(),
                        asio::placeholders::error, asio::placeholders::iterator));
                return;
            }
        }
        throw boost::system::system_error(error);
    }

    void connectHandler(const boost::system::error_code& error, asio::ip::tcp::resolver::iterator)
    {
        if(!error)
        {
            if(m_secure)
            {
                printf("handshake\n");
                m_stream.async_handshake(Socket::client,
                    boost::bind(&AsioSSLTCPConnection::handshakeHandler, shared_from_this(),
                        asio::placeholders::error));
            }
            else
            {
                printf("connected\n");
                m_connection->connectCallback(m_connection->userData, net_ok);
                m_socket.async_read_some(asio::mutable_buffers_1(m_buffer, sizeof(m_buffer)),
                    boost::bind(&AsioSSLTCPConnection::readHandler, shared_from_this(),
                        asio::placeholders::error, asio::placeholders::bytes_transferred));
            }
            return;
        }
        throw boost::system::system_error(error);
    }

    void handshakeHandler(const boost::system::error_code& error)
    {
        if(!error)
        {
            printf("secure connected\n");
            m_connection->connectCallback(m_connection->userData, net_ok);
            m_stream.async_read_some(asio::mutable_buffers_1(m_buffer, sizeof(m_buffer)),
                boost::bind(&AsioSSLTCPConnection::readHandler, shared_from_this(), asio::placeholders::error,
                    asio::placeholders::bytes_transferred));
            return;
        }
        throw boost::system::system_error(error);
    }

    void readHandler(const boost::system::error_code& error, size_t bytesTransferred)
    {
        if(!error)
        {
            printf("read %lu bytes\n", bytesTransferred);
            m_connection->readCallback(m_connection->userData, m_buffer, bytesTransferred, net_ok);
            if(m_secure)
            {
                m_stream.async_read_some(asio::mutable_buffers_1(m_buffer, sizeof(m_buffer)),
                    boost::bind(&AsioSSLTCPConnection::readHandler, shared_from_this(),
                        asio::placeholders::error, asio::placeholders::bytes_transferred));
            }
            else
            {
                m_socket.async_read_some(asio::mutable_buffers_1(m_buffer, sizeof(m_buffer)),
                    boost::bind(&AsioSSLTCPConnection::readHandler, shared_from_this(),
                        asio::placeholders::error, asio::placeholders::bytes_transferred));
            }
            return;
        }
        if(error.value() == asio::error::eof || error.value() == asio::error::shut_down)
        {
            printf("shutdown\n");
            return;
        }
        throw boost::system::system_error(error);
    }

    void writeHandler(const boost::system::error_code& error, size_t)
    {
        if(!error)
        {
            printf("write finished\n");
            m_connection->writeCallback(m_connection->userData, net_ok);
            return;
        }
        throw boost::system::system_error(error);
    }
};

void net_init(NetConnection* conn, void* userData, ConnectCallback connectCallback, ReadCallback readCallback,
    WriteCallback writeCallback, DisconnectCallback disconnectCallback)
{
    conn->userData = userData;
    conn->connectCallback = connectCallback;
    conn->readCallback = readCallback;
    conn->writeCallback = writeCallback;
    conn->disconnectCallback = disconnectCallback;
    conn->driverData = NULL;
}

void destroyConnection(NetConnection* conn)
{
    if(conn->driverData)
    {
        delete static_cast<std::shared_ptr<AsioSSLTCPConnection>*>(conn->driverData);
        conn->driverData = NULL;
    }
}

AsioSSLTCPConnection* createConnection(NetConnection* conn)
{
    destroyConnection(conn);
    auto asioConn = std::make_shared<AsioSSLTCPConnection>(ioService, conn);
    conn->driverData = new std::shared_ptr<AsioSSLTCPConnection>(asioConn);
    return asioConn.get();
}

AsioSSLTCPConnection* getConnection(NetConnection* conn)
{
    if(!conn->driverData)
    {
        return createConnection(conn);
    }
    return static_cast<std::shared_ptr<AsioSSLTCPConnection>*>(conn->driverData)->get();
}

void net_asyncConnect(NetConnection* conn, const char* hostname)
{
    
    auto asioConn = createConnection(conn);
    asioConn->asyncConnect(hostname);
}

void net_asyncSecureConnect(NetConnection* conn, const char* hostname)
{
    auto driver = createConnection(conn);
    driver->asyncSecureConnect(hostname);
}

void net_asyncWrite(NetConnection* conn, const void* data, size_t size)
{
    auto driver = getConnection(conn);
    driver->asyncWrite(data, size);
}

void net_asyncDisconnect(NetConnection* conn)
{
    printf("netAsyncDisconnect\n");
    auto driver = getConnection(conn);
    driver->disconnect();
    destroyConnection(conn);
}
