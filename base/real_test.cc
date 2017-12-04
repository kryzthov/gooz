#include "base/real.h"

#include <gtest/gtest.h>

namespace base { namespace real {

TEST(BaseReal, Basic) {
  Real r("3.14159");
  EXPECT_EQ("0.31415", r.String().substr(0, 7));
}

}}  // namespace base::real
