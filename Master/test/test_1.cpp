#define BOOST_TEST_MODULE MyTest
#include <boost/test/included/unit_test.hpp>
#include "../lib/Itho/DemandItho.h"
 
//BOOST_AUTO_TEST_SUITE (enum-test) 
 
BOOST_AUTO_TEST_CASE( my_test )
{
  BOOST_CHECK(1);
  BOOST_CHECK(1 == 1);

}


BOOST_AUTO_TEST_CASE( decode1 )
{
  BOOST_CHECK(1);
  CC1101Packet p = {10, {0,0,0,0,0,0,0,0, 0b11100001, 0b10000000}};
  CC1101Packet decoded;
  DemandItho::decode(p, decoded);

  printf("l = %d\n", decoded.length);
  BOOST_CHECK(decoded.length == 1);
}

//BOOST_AUTO_TEST_SUITE_END( )
