#include <gimslib/d3d/DX12Util.hpp>
#pragma once

const static ui32 knotVector[] = {0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4};

const static f32v4 controlPoints[] = {
    f32v4(1, 0, 0, 1),  f32v4(1, 1, 0, sqrt(2) / 2),   f32v4(0, 1, 0, 1),  f32v4(-1, 1, 0, sqrt(2) / 2),
    f32v4(-1, 0, 0, 1), f32v4(-1, -1, 0, sqrt(2) / 2), f32v4(0, -1, 0, 1), f32v4(1, -1, 0, sqrt(2) / 2),
    f32v4(1, 0, 0, 1)};

const static ui16 degree            = 2;
const static ui16 order             = 3;
const static ui16 kNumControlPoints = 9;
const static ui16 kKnotVectorLength = kNumControlPoints + degree + 1;

const static f32 N[kNumControlPoints];

static const void computeBasisFunctions(int k, int i, float t)
{
  if (k == 1)
  {
  }

  for (k = 0; k <= order; k++)
  {
    for (i = 0; i <= kNumControlPoints; i++)
    {
    }
  }
}
// degree = 2
// order = 3
// number Control Points = 9
// Size Knot Vector = 12
// 0 to 4 --> four segments