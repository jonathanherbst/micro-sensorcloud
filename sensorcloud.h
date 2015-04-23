#ifndef SENSORCLOUD
#define SENSORCLOUD

#include <http/request.h>

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SENSORCLOUD_AUTH_URL
#define SENSORCLOUD_AUTH_URL "https://sensorcloud.microstrain.com"
#endif

typedef enum
{
    sensorCloud_ok,
    sensorCloud_unauthorized,
    sensorCloud_tooManyPoints,
    sensorCloud_badRequest,
    sensorCloud_quotaExceeded,
    sensorCloud_netError
} SensorCloudError;

typedef uint64_t Timestamp;

typedef enum
{
    sensorCloud_hertz=1,
    sensorCloud_seconds=0
} SensorCloudSampleRateType;

typedef struct
{
    uint32_t value;
    SensorCloudSampleRateType type;
} SensorCloudSampleRate;

typedef void (*SensorCloudCallback)(void*, SensorCloudError);

typedef enum SensorCloudRequest
{
    sensorCloud_noRequest,
    sensorCloud_uploadRequest
} SensorCloudRequest;

typedef struct
{
    const char* sensor;
    const char* channel;
    SensorCloudSampleRate sampleRate;
    Buffer body;
    void(*callback)(void*, SensorCloudError);
} SensorCloudUploadData;

typedef struct
{
    const char* device;
    const char* key;
    uint8_t authenticated;
    const char* server;
    const char* token;
    char authData[512];
    Buffer authDataBuffer;
    char sensorInfo[16];
    char requestBuffer[1024];
    HTTPRequest request;
    void* userData;
    SensorCloudCallback callback;
    SensorCloudRequest pendingRequest;
    union
    {
        SensorCloudUploadData upload;
    } pendingRequestData;
} SensorCloud;

static const size_t sensorCloud_pointBufferHeaderSize = 16;
static const size_t sensorCloud_pointBufferDataSize = 12;

typedef struct
{
    SensorCloudSampleRate sampleRate;
    Buffer data;
} SensorCloudPointBuffer;

void sensorCloud_initPointBuffer(SensorCloudPointBuffer* pointBuffer, void* data, size_t dataSize,
    SensorCloudSampleRate sampleRate);

void sensorCloud_addPoint(SensorCloudPointBuffer* pointBuffer, Timestamp time, float value);

void sensorCloud_init(SensorCloud* sensorCloud, const char* device, const char* key, void* userData);

void sensorCloud_asyncAuthenticate(SensorCloud* sensorCloud, SensorCloudCallback callback);

void sensorCloud_asyncAddSensor(SensorCloud* sensorCloud, const char* sensor, SensorCloudCallback callback);

void sensorCloud_asyncUploadData(SensorCloud* sensorCloud, const char* sensor, const char* channel,
    SensorCloudPointBuffer* points, SensorCloudCallback callback);

#ifdef __cplusplus
}
#endif

#endif
