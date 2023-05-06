#include "AABB.hpp"
#include <iostream>

namespace gims
{
AABB::AABB()
    : m_lowerLeftBottom(std::numeric_limits<f32>::max())
    , m_upperRightTop(-std::numeric_limits<f32>::max())
{
}
AABB::AABB(f32v3 const* const positions, ui32 nPositions)
    : m_lowerLeftBottom(std::numeric_limits<f32>::max())
    , m_upperRightTop(-std::numeric_limits<f32>::max())

{
  for (ui32 i = 0; i < nPositions; i++)
  {
    const auto& p     = positions[i];
    m_lowerLeftBottom = glm::min(m_lowerLeftBottom, p);
    m_upperRightTop   = glm::max(m_upperRightTop, p);
  }
}
f32m4 AABB::getNormalizationTransformation() const
{
  f32 diffX = getUpperRightTop().x - getLowerLeftBottom().x;
  f32 diffY = getUpperRightTop().y - getLowerLeftBottom().y;
  f32 diffZ = getUpperRightTop().z - getLowerLeftBottom().z;

  f32 diffLength = std::max(std::max(diffX, diffY), diffZ);

  f32v3 translateVec = (getLowerLeftBottom() + getUpperRightTop()) / 2;
  f32m4 translateMat = glm::translate(-translateVec);

  f32m4 scaleMat = glm::scale(f32v3(1) / f32v3(/*5000*/ diffLength));
  /*std::cout << glm::to_string(scaleMat) << "\n";
  std::cout << glm::to_string(translateMat) << "\n";*/
  f32m4 trafo = scaleMat * translateMat;

  return trafo;
}
AABB AABB::getUnion(const AABB& other) const
{
  f32v3 posVec[] = {other.getLowerLeftBottom(), other.getUpperRightTop(), getLowerLeftBottom(), getUpperRightTop()};

  AABB result(posVec, 4);

  return result;
}
const f32v3& AABB::getLowerLeftBottom() const
{
  return m_lowerLeftBottom;
}
const f32v3& AABB::getUpperRightTop() const
{
  return m_upperRightTop;
}
AABB AABB::getTransformed(f32m4& transformation) const
{
  f32v3 p1 = transformation * f32v4(getUpperRightTop().x, getUpperRightTop().y, getUpperRightTop().z, 1.0f);
  f32v3 p2 = transformation * f32v4(getUpperRightTop().x, getUpperRightTop().y, getLowerLeftBottom().z, 1.0f);

  f32v3 p3 = transformation * f32v4(getUpperRightTop().x, getLowerLeftBottom().y, getUpperRightTop().z, 1.0f);
  f32v3 p4 = transformation * f32v4(getUpperRightTop().x, getLowerLeftBottom().y, getLowerLeftBottom().z, 1.0f);

  f32v3 p5 = transformation * f32v4(getLowerLeftBottom().x, getLowerLeftBottom().y, getUpperRightTop().z, 1.0f);
  f32v3 p6 = transformation * f32v4(getLowerLeftBottom().x, getLowerLeftBottom().y, getLowerLeftBottom().z, 1.0f);

  f32v3 p7 = transformation * f32v4(getLowerLeftBottom().x, getUpperRightTop().y, getUpperRightTop().z, 1.0f);
  f32v3 p8 = transformation * f32v4(getLowerLeftBottom().x, getUpperRightTop().y, getLowerLeftBottom().z, 1.0f);

  f32v3 posVec[] = {p1, p2, p3, p4, p5, p6, p7, p8};

  AABB result(posVec, 8);
  // f32v3 p7 = transformation * f32v4(getLowerLeftBottom(), 1.0f);
  // f32v3 p8 = transformation * f32v4(getUpperRightTop(), 1.0f);

  // f32v3 posVec[] = {p7, p8};

  // AABB result(posVec, 2);

  return result;
}
} // namespace gims