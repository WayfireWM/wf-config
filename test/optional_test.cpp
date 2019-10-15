#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <config/optional.hpp>

TEST_CASE("wf::optional")
{
    wf::optional<int> empty;
    CHECK(!empty);
    CHECK(empty.get_or(5) == 5);

    wf::optional<double> test{1.234};
    CHECK(test.get_or(2.0) == 1.234);
}

