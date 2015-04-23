#include "http.h"
#include "http_parsing.h"

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_AUTO_TEST_SUITE(HTTPParsingTest)

BOOST_AUTO_TEST_CASE(EOL)
{
    char eolData[2];
    memset(eolData, 0, sizeof(eolData));
    char* ret = eol(eolData);

    BOOST_CHECK_EQUAL(memcmp(eolData, "\r\n", 2), 0);
    BOOST_CHECK_EQUAL(eolData + 2, ret);
}

struct Fixture
{
    HTTPRequest request;
    char requestBuffer[4096];
    char responseBuffer[4096];

    Fixture()
    {
        http_defaultInitRequest(&request, requestBuffer, sizeof(requestBuffer), responseBuffer,
            sizeof(responseBuffer));
    }
};

BOOST_FIXTURE_TEST_SUITE(RequestParsing, Fixture)

BOOST_AUTO_TEST_CASE(HeaderWithNoHeaders)
{
    std::string header("200 OK\r\n\r\n");
    const void* unconsumed;
    HTTPError error = writeHeader(&unconsumed, &request, header.data(), header.size());

    BOOST_CHECK_EQUAL(unconsumed, header.data() + header.size());
    BOOST_CHECK_EQUAL(error, http_ok);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(WriteChunk, Fixture)

BOOST_AUTO_TEST_CASE(SingleChunk)
{
    std::string body("a\r\nabcdefghij\r\n0\r\n\r\n");
    HTTPError error = writeChunked(&request, body.data(), body.size());

    BOOST_CHECK_EQUAL(memcmp(request.responseBuffer.getPtr, body.data() + 3, 10), 0);
    BOOST_CHECK_EQUAL(error, http_requestComplete);
}

BOOST_AUTO_TEST_CASE(PartialChunk)
{
    std::string body("a\r\nabcdefghij\r\n0\r\n\r\n");
    HTTPError error = writeChunked(&request, body.data(), 2);

    BOOST_CHECK_EQUAL(error, http_ok);

    error = writeChunked(&request, body.data() + 2, body.size() - 2);

    BOOST_CHECK_EQUAL(memcmp(request.responseBuffer.getPtr, body.data() + 3, 10), 0);
    BOOST_CHECK_EQUAL(error, http_requestComplete);
}

BOOST_AUTO_TEST_CASE(FullChunkWithoutEndOfTransfer)
{
    std::string body("a\r\nabcdefghij\r\n0\r\n\r\n");
    HTTPError error = writeChunked(&request, body.data(), 16);

    BOOST_CHECK_EQUAL(error, http_ok);

    error = writeChunked(&request, body.data() + 16, body.size() - 16);

    BOOST_CHECK_EQUAL(memcmp(request.responseBuffer.getPtr, body.data() + 3, 10), 0);
    BOOST_CHECK_EQUAL(error, http_requestComplete);
}

BOOST_AUTO_TEST_CASE(FullChunkWithStartOfNext)
{
    std::string body("a\r\nabcdefghij\r\n10");
    HTTPError error = writeChunked(&request, body.data(), body.size());

    BOOST_CHECK_EQUAL(memcmp(request.responseBuffer.getPtr, body.data() + 3, 10), 0);
    BOOST_CHECK_EQUAL(error, http_ok);
}

BOOST_AUTO_TEST_CASE(MultipleChunks)
{
    std::string data("5\r\nabcde\r\n5\r\nfghij\r\n0\r\n\r\n");
    std::string actualData("abcdefghij");
    HTTPError e1 = writeChunked(&request, data.data(), 6);
    HTTPError e2 = writeChunked(&request, data.data() + 6, data.size() - 6);

    BOOST_CHECK_EQUAL(e1, http_ok);
    BOOST_CHECK_EQUAL(e2, http_requestComplete);
    BOOST_CHECK_EQUAL(memcmp(request.responseBuffer.getPtr, actualData.data(), actualData.size()), 0);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
