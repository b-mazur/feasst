#include "utils/test/utils.h"
#include "utils/include/debug.h"
#include "shape/include/cylinder.h"

namespace feasst {

TEST(Shape, Cylinder) {
  Cylinder cylinder({{"radius", "2"},
    {"first_point", "f"}, {"f0", "0"}, {"f1", "0"}, {"f2", "0"},
    {"second_point", "s"}, {"s0", "0"}, {"s1", "0"}, {"s2", "1"}});
  Position point;
  point.set_vector({1.5, 0, 13.535});
  EXPECT_NEAR(-0.5, cylinder.nearest_distance(point), 1e-15);
  EXPECT_TRUE(cylinder.is_inside(point));
  EXPECT_TRUE(cylinder.is_inside(point, 0.9999));
  EXPECT_FALSE(cylinder.is_inside(point, 1.00001));

  auto cylinder2 = test_serialize<Cylinder, Shape>(cylinder);
  EXPECT_TRUE(cylinder2->is_inside(point, 0.9999));
  EXPECT_FALSE(cylinder2->is_inside(point, 1.00001));
}

}  // namespace feasst
