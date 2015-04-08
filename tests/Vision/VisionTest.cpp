#include "VisionTest.h"

// declare external tests here, to save us some trivial header files
void TestPointUndistorion();


VisionTest::VisionTest()
	: boost::unit_test::test_suite( "VisionTests" )
{
	add( BOOST_TEST_CASE( &TestPointUndistorion ) );	
}

