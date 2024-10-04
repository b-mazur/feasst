#include <memory>
#include "utils/include/serialize.h"
#include "shape/include/shape_union.h"

namespace feasst {

FEASST_MAPPER(ShapeUnion,);

ShapeUnion::ShapeUnion(
    std::shared_ptr<Shape> shape1,
    std::shared_ptr<Shape> shape2) : ShapeUnion() {
  shape1_ = shape1;
  shape2_ = shape2;
}

bool ShapeUnion::is_inside(const Position& point) const {
  if (shape1_->is_inside(point)) {
    return true;
  }
  if (shape2_->is_inside(point)) {
    return true;
  }
  return false;
}

bool ShapeUnion::is_inside(const Position& point, const double diameter) const {
  if (shape1_->is_inside(point, diameter)) {
    return true;
  }
  if (shape2_->is_inside(point, diameter)) {
    return true;
  }
  return false;
}

double ShapeUnion::nearest_distance(const Position& point) const {
  const double dist1 = shape1_->nearest_distance(point),
               dist2 = shape2_->nearest_distance(point);
  if (dist1 < dist2) {
    return dist1;
  } else {
    return dist2;
  }
}

void ShapeUnion::serialize(std::ostream& ostr) const {
  ostr << class_name_ << " ";
  serialize_shape_(ostr);
  feasst_serialize_version(172, ostr);
  feasst_serialize_fstdr(shape1_, ostr);
  feasst_serialize_fstdr(shape2_, ostr);
}

ShapeUnion::ShapeUnion(std::istream& istr) : Shape(istr) {
  const int version = feasst_deserialize_version(istr);
  ASSERT(172 == version, version);

  // HWH for unknown reasons, below template isn't working in this case.
  // feasst_deserialize_fstdr(shape1, istr);
  // feasst_deserialize_fstdr(shape2, istr);
  int existing;
  istr >> existing;
  if (existing != 0) {
    shape1_ = shape1_->deserialize(istr);
  }
  istr >> existing;
  if (existing != 0) {
    shape2_ = shape2_->deserialize(istr);
  }
}

}  // namespace feasst
