#include <http_buffer.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(HTTPBufferTest)

BOOST_AUTO_TEST_CASE(EmptyBuffer_Empty)
{
    HTTPBuffer buffer;
    http_initBuffer(&buffer, NULL, 0);

    BOOST_CHECK_EQUAL(http_bufferSize(&buffer), 0);
    BOOST_CHECK_EQUAL(http_bufferLeft(&buffer), 0);
}

BOOST_AUTO_TEST_CASE(WriteRead_BufferSize)
{
    char bufferData[1024];
    HTTPBuffer buffer;
    http_initBuffer(&buffer, bufferData, sizeof(bufferData));

    char testData[] = "abcdefg";
    HTTPError ew = http_bufferWrite(&buffer, testData, sizeof(testData));
    HTTPError er = http_bufferRead(&buffer, testData, 3);

    BOOST_CHECK_EQUAL(ew, http_ok);
    BOOST_CHECK_EQUAL(er, http_ok);
    BOOST_CHECK_EQUAL(http_bufferSize(&buffer), sizeof(testData) - 3);
    BOOST_CHECK_EQUAL(http_bufferLeft(&buffer), sizeof(bufferData) - sizeof(testData));
}

BOOST_AUTO_TEST_CASE(WriteTwice_SizeCorrect)
{
    char bufferData[1024];
    HTTPBuffer buffer;
    http_initBuffer(&buffer, bufferData, sizeof(bufferData));

    char testData[] = "abcdefg";
    HTTPError e1 = http_bufferWrite(&buffer, testData, sizeof(testData));
    HTTPError e2 = http_bufferWrite(&buffer, testData, sizeof(testData));

    BOOST_CHECK_EQUAL(e1, http_ok);
    BOOST_CHECK_EQUAL(e2, http_ok);
    BOOST_CHECK_EQUAL(http_bufferSize(&buffer), 2 * sizeof(testData));
    BOOST_CHECK_EQUAL(http_bufferLeft(&buffer), sizeof(bufferData) - (2 * sizeof(testData)));
}

BOOST_AUTO_TEST_SUITE_END()
