#include "AABB.hpp"
#include <gimslib/contrib/d3dx12/d3dx12.h>
#include <gimslib/d3d/UploadHelper.hpp>
#include <iostream>
#include <vector>

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
  f32v3 diff       = getUpperRightTop() - getLowerLeftBottom();
  f32   diffLength = glm::compMax(diff);

  f32v3 translateVec = (getLowerLeftBottom() + getUpperRightTop()) / 2;
  f32m4 translateMat = glm::translate(-translateVec);

  f32m4 scaleMat = glm::scale(f32v3(1) / f32v3(diffLength));
  f32m4 trafo    = scaleMat * translateMat;

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
  f32v3 lowerLeftTransformed  = transformation * f32v4(getLowerLeftBottom(), 1.0f);
  f32v3 upperRightTransformed = transformation * f32v4(getUpperRightTop(), 1.0f);

  f32v3 posVec[] = {lowerLeftTransformed, upperRightTransformed};

  AABB result(posVec, 2);

  return result;
}

} // namespace gims