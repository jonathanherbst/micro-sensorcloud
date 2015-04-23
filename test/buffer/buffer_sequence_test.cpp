#include <buffer/buffer_sequence.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(BufferSequenceSequenceTest)

BOOST_AUTO_TEST_SUITE(Append)

BOOST_AUTO_TEST_CASE(AppendToSingleBufferSequence)
{
    BufferSequence buf1;
    bufferSequence_init(&buf1, NULL, 0);
    BufferSequence buf2;
    bufferSequence_init(&buf2, NULL, 0);

    bufferSequence_append(&buf1, &buf2);

    BOOST_CHECK_EQUAL(buf1.next, &buf2);
    BOOST_CHECK_EQUAL(buf2.next, static_cast<BufferSequence*>(NULL));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Add)

BOOST_AUTO_TEST_CASE(AdvanceToEnd_EmptyBufferSequence)
{
    char data[10];
    BufferSequence buf;
    bufferSequence_init(&buf, data, 10);

    bufferSequence_advance(&buf, 10);

    BOOST_CHECK_EQUAL(buf.length, 0);
}

BOOST_AUTO_TEST_CASE(AdvancePastEnd_EmptyBufferSequence)
{
    char data[10];
    BufferSequence buf;
    bufferSequence_init(&buf, data, 10);

    bufferSequence_advance(&buf, 11);

    BOOST_CHECK_EQUAL(buf.length, 0);
}

BOOST_AUTO_TEST_CASE(Advance9On10Buf_Size1)
{
    char data[10];
    BufferSequence buf;
    bufferSequence_init(&buf, data, 10);

    bufferSequence_advance(&buf, 9);

    BOOST_CHECK_EQUAL(buf.data, data + 9);
    BOOST_CHECK_EQUAL(buf.length, 1);
}

BOOST_AUTO_TEST_CASE(AdvanceThroughTwoBufferSequences)
{
    char data[10];
    BufferSequence buf1;
    bufferSequence_init(&buf1, data, 5);
    BufferSequence buf2;
    bufferSequence_init(&buf2, data + 5, 5);
    bufferSequence_append(&buf1, &buf2);

    bufferSequence_advance(&buf1, 9);

    BOOST_CHECK_EQUAL(buf1.data, data + 9);
    BOOST_CHECK_EQUAL(buf1.length, 1);
}


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Search)

BOOST_AUTO_TEST_CASE(SearchForSequenceAtEndOfBufferSequence)
{
    char data[] = "200 OK\r\n\r\n";
    BufferSequence input;
    bufferSequence_init(&input, data, 10);
    BufferSequence output;

    bufferSequence_search(&output, &input, "\r\n\r\n", 4);

    BOOST_CHECK_EQUAL(output.data, data + 6);
    BOOST_CHECK_EQUAL(output.length, 4);
}

BOOST_AUTO_TEST_CASE(SearchThroughTwoBufferSequences)
{
    char data[] = "200 OK\r\n\r\n";
    BufferSequence input1;
    bufferSequence_init(&input1, NULL, 0);
    BufferSequence input2;
    bufferSequence_init(&input2, data, 10);
    bufferSequence_append(&input1, &input2);
    BufferSequence output;

    bufferSequence_search(&output, &input1, "\r\n\r\n", 4);

    BOOST_CHECK_EQUAL(output.data, data + 6);
    BOOST_CHECK_EQUAL(output.length, 4);
}

BOOST_AUTO_TEST_CASE(ByteSequenceDoesntExist)
{
    char data[] = "200 OK\r\n\r\r";
    BufferSequence input;
    bufferSequence_init(&input, data, 10);
    BufferSequence output;

    bufferSequence_search(&output, &input, "\r\n\r\n", 4);

    BOOST_CHECK_EQUAL(output.data, static_cast<char*>(NULL));
    BOOST_CHECK_EQUAL(output.length, 0);
}

BOOST_AUTO_TEST_CASE(ByteSequenceTwice_FindFirst)
{
    char data[] = "200 \r\n\r\n\r\n";
    BufferSequence input;
    bufferSequence_init(&input, data, 10);
    BufferSequence output;

    bufferSequence_search(&output, &input, "\r\n\r\n", 4);

    BOOST_CHECK_EQUAL(output.data, data + 4);
    BOOST_CHECK_EQUAL(output.length, 6);
}

BOOST_AUTO_TEST_CASE(EmptyByteSequence_FindBeginning)
{
    char data[] = "200 OK\r\n\r\r";
    BufferSequence input;
    bufferSequence_init(&input, data, 10);
    BufferSequence output;

    bufferSequence_search(&output, &input, NULL, 0);

    BOOST_CHECK_EQUAL(output.data, data);
    BOOST_CHECK_EQUAL(output.length, 10);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
