#ifndef NET_DRIVER
#define NET_DRIVER

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    net_ok,
    net_error
} NetError;

typedef void(*ConnectCallback)(void*, NetError);
typedef void(*ReadCallback)(void*, const void*, size_t, NetError);
typedef void(*WriteCallback)(void*, NetError);
typedef void(*DisconnectCallback)(void*, NetError);

typedef struct
{
    void* driverData;
    void* userData;
    ConnectCallback connectCallback;
    ReadCallback readCallback;
    WriteCallback writeCallback;
    DisconnectCallback disconnectCallback;
} NetConnection;

extern void net_init(NetConnection* conn, void* userData, ConnectCallback connectCallback, ReadCallback readCallback,
    WriteCallback writeCallback, DisconnectCallback disconnectCallback);

extern void net_asyncConnect(NetConnection* conn, const char* hostname);

extern void net_asyncSecureConnect(NetConnection* conn, const char* hostname);

extern void net_asyncWrite(NetConnection* conn, const void* data, size_t size);

extern void net_asyncDisconnect(NetConnection* conn);

#ifdef __cplusplus
}
#endif

#endif
