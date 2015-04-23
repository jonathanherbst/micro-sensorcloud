#include "sensorcloud.h"

#include <app/esp8266-sensor/uart.h>
#include <xdr/xdr.h>

void ICACHE_FLASH_ATTR sensorCloud_initPointBuffer(SensorCloudPointBuffer* pointBuffer, void* data, size_t dataSize,
    SensorCloudSampleRate sampleRate)
{
    buffer_init(&pointBuffer->data, (char*)data, dataSize);
    xdr_writeInt(&pointBuffer->data, 1);
    xdr_writeInt(&pointBuffer->data, (int32_t)sampleRate.type);
    xdr_writeUInt(&pointBuffer->data, sampleRate.value);
    xdr_writeUInt(&pointBuffer->data, 0);
}

void ICACHE_FLASH_ATTR sensorCloud_addPoint(SensorCloudPointBuffer* pointBuffer, Timestamp time, float value)
{
    xdr_writeUHyper(&pointBuffer->data, time);
    xdr_writeFloat(&pointBuffer->data, value);
}

void ICACHE_FLASH_ATTR sensorCloud_callback(SensorCloud* sensorCloud, SensorCloudError error)
{
    sensorCloud->callback(sensorCloud->userData, error);
}

void ICACHE_FLASH_ATTR sensorCloud_asyncUploadDataCallback(void* userData, const void* data, size_t dataSize, HTTPError error)
{
    SensorCloud* sensorCloud = (SensorCloud*)userData;
    if(error == http_ok)
        return;
    if(error != http_complete)
    {
        sensorCloud_callback(sensorCloud, sensorCloud_netError);
        return;
    }
    
    HTTPResponseCode code;
    const char* reason;
    size_t reasonSize;
    http_getResponseCode(&code, &reason, &reasonSize, &sensorCloud->request);

    if(code == httpResponse_notFound)
    { // sensor doesn't exist
        SensorCloudUploadData* data = &sensorCloud->pendingRequestData.upload;
        sensorCloud_asyncAddSensor(sensorCloud, data->sensor, sensorCloud->callback);
        return;
    }

    sensorCloud->pendingRequest = sensorCloud_noRequest;
    if(code == httpResponse_created)
    { // success
        sensorCloud_callback(sensorCloud, sensorCloud_ok);
    }
    else if(code == httpResponse_unauthorized)
    { // not authorized
        if(reasonSize == 5 && memcmp(reason, "Quota", 5))
            sensorCloud_callback(sensorCloud, sensorCloud_quotaExceeded);
        else
            sensorCloud_callback(sensorCloud, sensorCloud_unauthorized);
    }
    else
    {
        sensorCloud_callback(sensorCloud, sensorCloud_badRequest);
    }
}

void ICACHE_FLASH_ATTR sensorCloud_doUploadData(SensorCloud* sensorCloud)
{
    SensorCloudUploadData* data = &sensorCloud->pendingRequestData.upload;
    char url[256];
    memset(url, '\0', sizeof(url));
    strcat(url, "https://");
    strcat(url, sensorCloud->server);
    strcat(url, "/SensorCloud/devices/");
    strcat(url, sensorCloud->device);
    strcat(url, "/sensors/");
    strcat(url, data->sensor);
    strcat(url, "/channels/");
    strcat(url, data->channel);
    strcat(url, "/streams/timeseries/data/?version=1&auth_token=");
    strcat(url, sensorCloud->token);

    Buffer requestHead;
    buffer_init(&requestHead, sensorCloud->requestBuffer, sizeof(sensorCloud->requestBuffer));
    Buffer responseHead;
    buffer_init(&responseHead, sensorCloud->requestBuffer, sizeof(sensorCloud->requestBuffer));
    //buffer_init(&responseHead, sensorCloud->requestBuffer + 512, 512);
    http_initRequest(&sensorCloud->request, "POST", url, requestHead, responseHead, sensorCloud,
        sensorCloud_asyncUploadDataCallback);
    http_addRequestHeader(&sensorCloud->request, "Content-Type", "application/xdr");

    http_asyncRequest(&sensorCloud->request, data->body);
}

void ICACHE_FLASH_ATTR sensorCloud_init(SensorCloud* sensorCloud, const char* device, const char* key, void* userData)
{
    sensorCloud->device = device;
    sensorCloud->key = key;
    sensorCloud->authenticated = 0;
    sensorCloud->userData = userData;
    sensorCloud->pendingRequest = sensorCloud_noRequest;
}

void ICACHE_FLASH_ATTR sensorCloud_executePending(SensorCloud* sensorCloud)
{
    switch(sensorCloud->pendingRequest)
    {
    case sensorCloud_uploadRequest:
        sensorCloud_doUploadData(sensorCloud);
        break;
    default:
        sensorCloud_callback(sensorCloud, sensorCloud_ok);
        break;
    }
}

void ICACHE_FLASH_ATTR sensorCloud_asyncAuthenticateCallback(void* userData, const void* data, size_t dataSize, HTTPError error)
{
    SensorCloud* sensorCloud = (SensorCloud*)userData;
    if(error == http_ok)
    { // got part of the auth response
        buffer_write(&sensorCloud->authDataBuffer, (const char*)data, dataSize);
        return; // wait for the next read
    }
    if(error != http_complete)
    {
        sensorCloud_callback(sensorCloud, sensorCloud_netError);
        return;
    }

    // write the rest of the authentication data
    buffer_write(&sensorCloud->authDataBuffer, (const char*)data, dataSize);

    HTTPResponseCode code;
    const char* reason;
    size_t reasonSize;
    http_getResponseCode(&code, &reason, &reasonSize, &sensorCloud->request);

    if(code == httpResponse_unauthorized)
        sensorCloud_callback(sensorCloud, sensorCloud_unauthorized);
    else if(code != httpResponse_ok)
        sensorCloud_callback(sensorCloud, sensorCloud_netError);
    else
    { // we've authenticated
        sensorCloud->authenticated = 1;
        
        char pageBuffer[20];
        ets_sprintf(pageBuffer, "t: %d\r\n", buffer_size(&sensorCloud->authDataBuffer));
        uart0_tx_buffer(pageBuffer, strlen(pageBuffer));

        // parse out the token and server
        uint32_t tokenSize;
        uint32_t serverSize;
        xdr_readUInt(&tokenSize, &sensorCloud->authDataBuffer);
        sensorCloud->token = sensorCloud->authDataBuffer.getPtr;
        xdr_readString(NULL, tokenSize, &sensorCloud->authDataBuffer);
        xdr_readUInt(&serverSize, &sensorCloud->authDataBuffer);
        sensorCloud->server = sensorCloud->authDataBuffer.getPtr;
        
        char* server = (char*)sensorCloud->authDataBuffer.getPtr;

        memcpy(server, "10.51.50.111\0", 13);
        *((char*)sensorCloud->token + tokenSize) = '\0';
        *((char*)sensorCloud->server + serverSize) = '\0';

        // execute the pending request if there is one
        sensorCloud_executePending(sensorCloud);
    }
}

void ICACHE_FLASH_ATTR sensorCloud_asyncAuthenticate(SensorCloud* sensorCloud, SensorCloudCallback callback)
{
    sensorCloud->callback = callback;

    buffer_init(&sensorCloud->authDataBuffer, sensorCloud->authData, sizeof(sensorCloud->authData));

    // build the url
    char url[256];
    memset(url, '\0', sizeof(url));
    strcat(url, SENSORCLOUD_AUTH_URL);
    strcat(url, "/SensorCloud/devices/");
    strcat(url, sensorCloud->device);
    strcat(url, "/authenticate/?version=1&key=");
    strcat(url, sensorCloud->key);
    
    // build the request
    Buffer requestHead;
    buffer_init(&requestHead, sensorCloud->requestBuffer, 512);
    Buffer responseHead;
    buffer_init(&responseHead, sensorCloud->requestBuffer + 512, 512);
    HTTPError e = http_initRequest(&sensorCloud->request, "GET", url, requestHead, responseHead, sensorCloud,
        sensorCloud_asyncAuthenticateCallback);
    if(e != http_ok)
        return sensorCloud_callback(sensorCloud, sensorCloud_netError);
    e = http_addRequestHeader(&sensorCloud->request, "Accept", "application/xdr");
    if(e != http_ok)
        return sensorCloud_callback(sensorCloud, sensorCloud_netError);

    Buffer body;
    buffer_init(&body, NULL, 0);

    // initiate the request
    e = http_asyncRequest(&sensorCloud->request, body);
    if(e != http_ok)
        return sensorCloud_callback(sensorCloud, sensorCloud_netError);
}

void ICACHE_FLASH_ATTR sensorCloud_asyncAddSensorCallback(void* userData, const void* data, size_t dataSize, HTTPError error)
{
    SensorCloud* sensorCloud = (SensorCloud*)userData;
    if(error == http_ok) // response not complete
        return;
    if(error != http_complete)
    {
        sensorCloud_callback(sensorCloud, sensorCloud_netError);
        return;
    }
    
    HTTPResponseCode code;
    const char* reason;
    size_t reasonSize;
    http_getResponseCode(&code, &reason, &reasonSize, &sensorCloud->request);
    
    if(code == httpResponse_created)
        sensorCloud_executePending(sensorCloud);
    else
    { // we've authenticated
        sensorCloud_callback(sensorCloud, sensorCloud_badRequest);
    }
}

void ICACHE_FLASH_ATTR sensorCloud_asyncAddSensor(SensorCloud* sensorCloud, const char* sensor, SensorCloudCallback callback)
{
    sensorCloud->callback = callback;

    // build the url
    char url[256];
    memset(url, '\0', sizeof(url));
    strcat(url, "https://");
    strcat(url, sensorCloud->server);
    strcat(url, "/SensorCloud/devices/");
    strcat(url, sensorCloud->device);
    strcat(url, "/sensors/");
    strcat(url, sensor);
    strcat(url, "/?version=1&auth_token=");
    strcat(url, sensorCloud->token);

    // build the header
    Buffer requestHead;
    buffer_init(&requestHead, sensorCloud->requestBuffer, 512);
    Buffer responseHead;
    buffer_init(&responseHead, sensorCloud->requestBuffer + 512, 512);
    http_initRequest(&sensorCloud->request, "PUT", url, requestHead, responseHead, sensorCloud, 
        sensorCloud_asyncAddSensorCallback);
    http_addRequestHeader(&sensorCloud->request, "Content-Type", "application/xdr");

    // build the body
    Buffer body;
    buffer_init(&body, sensorCloud->sensorInfo, sizeof(sensorCloud->sensorInfo));
    xdr_writeInt(&body, 1);
    xdr_writeUInt(&body, 0);
    xdr_writeUInt(&body, 0);
    xdr_writeUInt(&body, 0);

    // initiate the request
    http_asyncRequest(&sensorCloud->request, body);
}

void ICACHE_FLASH_ATTR sensorCloud_asyncUploadData(SensorCloud* sensorCloud, const char* sensor, const char* channel,
    SensorCloudPointBuffer* points, SensorCloudCallback callback)
{
    // put upload into pending until we complete it
    sensorCloud->pendingRequest = sensorCloud_uploadRequest;
    SensorCloudUploadData* data = &sensorCloud->pendingRequestData.upload;
    data->sensor = sensor;
    data->channel = channel;
    data->body = points->data;
    sensorCloud->callback = callback;

    // write the point count to the body data
    size_t pointCount = (buffer_size(&data->body) - sensorCloud_pointBufferHeaderSize) / 
        sensorCloud_pointBufferDataSize;
    Buffer pointCountWriter;
    buffer_init(&pointCountWriter, data->body.data + sensorCloud_pointBufferHeaderSize - 4, 4);
    xdr_writeUInt(&pointCountWriter, pointCount);

    if(!sensorCloud->authenticated)
        sensorCloud_asyncAuthenticate(sensorCloud, callback);
    else
        sensorCloud_doUploadData(sensorCloud);
}
