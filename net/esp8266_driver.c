#include "driver.h"

#include <mem.h>
#include <ip_addr.h>
#include <espconn.h>

typedef struct
{
    ip_addr_t ip;
    struct espconn connection;
    const void* writeData;
    size_t writeDataSize;
    uint8_t secure;
} HTTPESP8266ConnectionData;

void ICACHE_FLASH_ATTR esp8266_destroyConnection(NetConnection* conn)
{
    if(conn->driverData)
    {
        HTTPESP8266ConnectionData* connData = (HTTPESP8266ConnectionData*)conn->driverData;
        espconn_delete(&connData->connection);
        if(connData->connection.proto.tcp)
            os_free(connData->connection.proto.tcp);
        os_free(connData);
        conn->driverData = NULL;
    }
}

HTTPESP8266ConnectionData* ICACHE_FLASH_ATTR esp8266_createConnection(NetConnection* conn)
{
    if(conn->driverData)
        esp8266_destroyConnection(conn);

    HTTPESP8266ConnectionData* driver = os_zalloc(sizeof(HTTPESP8266ConnectionData));
    ets_memset(driver, 0, sizeof(HTTPESP8266ConnectionData));
    conn->driverData = driver;
    driver->connection.reverse = conn;
    driver->connection.type = ESPCONN_TCP;
    driver->connection.proto.tcp = (esp_tcp*)os_zalloc(sizeof(esp_tcp));
    ets_memset(driver->connection.proto.tcp, 0, sizeof(esp_tcp));
    driver->connection.proto.tcp->local_port = espconn_port();
    return driver;
}

HTTPESP8266ConnectionData* ICACHE_FLASH_ATTR esp8266_getConnection(NetConnection* conn)
{
    if(!conn->driverData)
        return esp8266_createConnection(conn);

    return (HTTPESP8266ConnectionData*)conn->driverData;
}

void ICACHE_FLASH_ATTR esp8266_connectCallback(void* arg)
{
    uart0_tx_buffer("connected\r\n", 11);
    struct espconn* conn = (struct espconn*)arg;
    NetConnection* netConn = (NetConnection*)conn->reverse;

    netConn->connectCallback(netConn->userData, net_ok);
}

void ICACHE_FLASH_ATTR esp8266_disconnectCallback(void* arg)
{
    uart0_tx_buffer("disconnected\r\n", 14);
    struct espconn* conn = (struct espconn*)arg;
    NetConnection* netConn = (NetConnection*)conn->reverse;

    esp8266_destroyConnection(netConn);
    netConn->disconnectCallback(netConn->userData, net_ok);
}

void ICACHE_FLASH_ATTR esp8266_reconnectCallback(void* arg, sint8 err)
{
    char pageBuffer[20];
    ets_sprintf(pageBuffer, "recon: %d\r\n", err);
    uart0_tx_buffer(pageBuffer, strlen(pageBuffer));
    struct espconn* conn = (struct espconn*)arg;
    NetConnection* netConn = (NetConnection*)conn->reverse;

    esp8266_destroyConnection(netConn);
    netConn->readCallback(netConn->userData, NULL, 0, net_error);
}

void ICACHE_FLASH_ATTR esp8266_recvCallback(void* arg, char* buffer, unsigned short size)
{
    char pageBuffer[20];
    ets_sprintf(pageBuffer, "rx: %d\r\n", size);
    uart0_tx_buffer(pageBuffer, strlen(pageBuffer));
    struct espconn* conn = (struct espconn*)arg;
    NetConnection* netConn = (NetConnection*)conn->reverse;

    netConn->readCallback(netConn->userData, buffer, size, net_ok);
}

void ICACHE_FLASH_ATTR esp8266_sendCallback(void* arg)
{
    uart0_tx_buffer("tx\r\n", 4);
    struct espconn* conn = (struct espconn*)arg;
    NetConnection* netConn = (NetConnection*)conn->reverse;
    HTTPESP8266ConnectionData* driver = esp8266_getConnection(netConn);

    if(driver->writeDataSize > 0)
    {
        net_asyncWrite(netConn, driver->writeData, driver->writeDataSize);
    }
    else
    {
        netConn->writeCallback(netConn->userData, net_ok);
    }
}

void ICACHE_FLASH_ATTR esp8266_resolveCallback(const char* name, ip_addr_t* ip, void* arg)
{
    
    struct espconn* conn = (struct espconn*)arg;
    NetConnection* netConn = (NetConnection*)conn->reverse;
    uart0_tx_buffer("get conn\r\n", 10);
    HTTPESP8266ConnectionData* driver = esp8266_getConnection(netConn);

    uart0_tx_buffer("check ip\r\n", 10);
    if(!ip)
    { // failed to lookup the hostname
        uart0_tx_buffer("bad ip\r\n", 8);
        esp8266_destroyConnection(netConn);
        netConn->readCallback(netConn->userData, NULL, 0, net_error);
        return;
    }

    char pageBuffer[20];
    ets_sprintf(pageBuffer, "r: %d.%d.%d.%d\r\n", IP2STR(ip));
    uart0_tx_buffer(pageBuffer, strlen(pageBuffer));

    uart0_tx_buffer("set tcp callbacks\r\n", 19);
    espconn_regist_connectcb(conn, esp8266_connectCallback);
    espconn_regist_disconcb(conn, esp8266_disconnectCallback);
    espconn_regist_reconcb(conn, esp8266_reconnectCallback);
    uart0_tx_buffer("set callbacks\r\n", 15);
    espconn_regist_recvcb(conn, esp8266_recvCallback);
    espconn_regist_sentcb(conn, esp8266_sendCallback);

    uart0_tx_buffer("set ip\r\n", 8);
    ets_memcpy(&conn->proto.tcp->remote_ip, ip, 4);
    if(driver->secure)
    {
        uart0_tx_buffer("async sconnect\r\n", 16);
        conn->proto.tcp->remote_port = 443;
        
        ets_sprintf(pageBuffer, "port: %d\r\n", conn->proto.tcp->remote_port);
        uart0_tx_buffer(pageBuffer, strlen(pageBuffer));

        sint8 r = espconn_secure_connect(conn);
        ets_sprintf(pageBuffer, "c_conn: %d\r\n", r);
        uart0_tx_buffer(pageBuffer, strlen(pageBuffer));
    }
    else
    {
        uart0_tx_buffer("async connect\r\n", 15);
        conn->proto.tcp->remote_port = 80;
        
        ets_sprintf(pageBuffer, "port: %d\r\n", conn->proto.tcp->remote_port);
        uart0_tx_buffer(pageBuffer, strlen(pageBuffer));

        sint8 r = espconn_connect(conn);
        ets_sprintf(pageBuffer, "c_conn: %d\r\n", r);
        uart0_tx_buffer(pageBuffer, strlen(pageBuffer));
    }
}

void ICACHE_FLASH_ATTR net_init(NetConnection* conn, void* userData, ConnectCallback connectCallback, ReadCallback readCallback,
    WriteCallback writeCallback, DisconnectCallback disconnectCallback)
{
    conn->userData = userData;
    conn->connectCallback = connectCallback;
    conn->readCallback = readCallback;
    conn->writeCallback = writeCallback;
    conn->disconnectCallback = disconnectCallback;
    conn->driverData = NULL;
}

void ICACHE_FLASH_ATTR net_asyncConnect(NetConnection* conn, const char* hostname)
{
    uart0_tx_buffer("connect\r\n", 9);
    HTTPESP8266ConnectionData* driver = esp8266_createConnection(conn);
    driver->secure = 1;
    char pageBuffer[20];
    ets_sprintf(pageBuffer, "r: %s\r\n", hostname);
    uart0_tx_buffer(pageBuffer, strlen(pageBuffer));
    err_t e = espconn_gethostbyname(&driver->connection, hostname, &driver->ip, esp8266_resolveCallback);
    switch(e)
    {
    case ESPCONN_OK: // dns cached, no async call
        esp8266_resolveCallback(hostname, &driver->ip, &driver->connection);
    case ESPCONN_INPROGRESS: // dns request queued, let callback happen
        return;
    default: // error fetching hostname
        esp8266_resolveCallback(hostname, NULL, &driver->connection);
    };
}

void ICACHE_FLASH_ATTR net_asyncSecureConnect(NetConnection* conn, const char* hostname)
{
    net_asyncConnect(conn, hostname);
    HTTPESP8266ConnectionData* driver = esp8266_getConnection(conn);
    driver->secure = 1;
}

void ICACHE_FLASH_ATTR net_asyncWrite(NetConnection* conn, const void* data, size_t size)
{   
    char pageBuffer[20];
    ets_sprintf(pageBuffer, "tx: %d\r\n", size);
    uart0_tx_buffer(pageBuffer, strlen(pageBuffer));
    HTTPESP8266ConnectionData* driver = esp8266_getConnection(conn);
    uint16 writeSize = size <= 65535 ? size : 65535;
    driver->writeData = data + writeSize;
    driver->writeDataSize = size - writeSize;
    if(driver->secure)
        espconn_secure_sent(&driver->connection, (uint8*)data, writeSize);
    else
        espconn_sent(&driver->connection, (uint8*)data, writeSize);
}

void ICACHE_FLASH_ATTR net_asyncDisconnect(NetConnection* conn)
{
    uart0_tx_buffer("disconnect\r\n", 12);
    HTTPESP8266ConnectionData* driver = esp8266_getConnection(conn);
    //esp8266_destroyConnection(conn);
    //conn->disconnectCallback(conn->userData, net_ok);
    if(driver->secure)
        espconn_secure_disconnect(&driver->connection);
    else
        espconn_disconnect(&driver->connection);
}

