/** ****************************************************************************
 * \file    demo_test.cpp
 * \brief   Demonstrates testing with catch
 * \author
 * Year      | Name
 * --------: | :------------
 * 2016      | C.Frescolino
 * \copyright  see LICENSE
 ******************************************************************************/

#include <catch.hpp>

TEST_CASE("Demo Title", "[demo_tag]") { CHECK(1 == 1); }
