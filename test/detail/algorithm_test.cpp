#include <detail/algorithm.h>

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_AUTO_TEST_SUITE(AlgorithmTest)

BOOST_AUTO_TEST_SUITE(memhtoi_test)

BOOST_AUTO_TEST_CASE(a_10)
{
    int data = memhtoi("a", 1);

    BOOST_CHECK_EQUAL(data, 10);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
