#pragma once
// #include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/DX12Util.hpp>
// #include <gimslib/d3d/HLSLProgram.hpp>
// #include <gimslib/dbg/HrException.hpp>
// #include <gimslib/types.hpp>
// #include <gimslib/ui/ExaminerController.hpp>
//--------------------------------------------------------------------------------------
//  File: MobiusStrip.h
//
//  This sample shows an simple implementation of the DirectX 11 Hardware Tessellator
//  for rendering a Bezier Patch.
//
//  Copyright (c) Microsoft Corporation.
//  Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------
#pragma once

// Control point for a Bezier patch
struct BEZIER_CONTROL_POINT
{
  f32 m_vPosition[3];
};

// Simple Bezier patch for a Mobius strip
// 4 patches with 16 control points each
const f32v3 g_MobiusStrip[64] = {
    f32v3(1.0f, -0.5f, 0.0f),         f32v3(1.0f, -0.5f, 0.5f),         f32v3(0.5f, -0.3536f, 1.354f),
    f32v3(0.0f, -0.3536f, 1.354f),    f32v3(1.0f, -0.1667f, 0.0f),      f32v3(1.0f, -0.1667f, 0.5f),
    f32v3(0.5f, -0.1179f, 1.118f),    f32v3(0.0f, -0.1179f, 1.118f),    f32v3(1.0f, 0.1667f, 0.0f),
    f32v3(1.0f, 0.1667f, 0.5f),       f32v3(0.5f, 0.1179f, 0.8821f),    f32v3(0.0f, 0.1179f, 0.8821f),
    f32v3(1.0f, 0.5f, 0.0f),          f32v3(1.0f, 0.5f, 0.5f),          f32v3(0.5f, 0.3536f, 0.6464f),
    f32v3(0.0f, 0.3536f, 0.6464f),    f32v3(0.0f, -0.3536f, 1.354f),    f32v3(-0.5f, -0.3536f, 1.354f),
    f32v3(-1.5f, 0.0f, 0.5f),         f32v3(-1.5f, 0.0f, 0.0f),         f32v3(0.0f, -0.1179f, 1.118f),
    f32v3(-0.5f, -0.1179f, 1.118f),   f32v3(-1.167f, 0.0f, 0.5f),       f32v3(-1.167f, 0.0f, 0.0f),
    f32v3(0.0f, 0.1179f, 0.8821f),    f32v3(-0.5f, 0.1179f, 0.8821f),   f32v3(-0.8333f, 0.0f, 0.5f),
    f32v3(-0.8333f, 0.0f, 0.0f),      f32v3(0.0f, 0.3536f, 0.6464f),    f32v3(-0.5f, 0.3536f, 0.6464f),
    f32v3(-0.5f, 0.0f, 0.5f),         f32v3(-0.5f, 0.0f, 0.0f),         f32v3(-1.5f, 0.0f, 0.0f),
    f32v3(-1.5f, 0.0f, -0.5f),        f32v3(-0.5f, 0.3536f, -1.354f),   f32v3(0.0f, 0.3536f, -1.354f),
    f32v3(-1.167f, 0.0f, 0.0f),       f32v3(-1.167f, 0.0f, -0.5f),      f32v3(-0.5f, 0.1179f, -1.118f),
    f32v3(0.0f, 0.1179f, -1.118f),    f32v3(-0.8333f, 0.0f, 0.0f),      f32v3(-0.8333f, 0.0f, -0.5f),
    f32v3(-0.5f, -0.1179f, -0.8821f), f32v3(0.0f, -0.1179f, -0.8821f),  f32v3(-0.5f, 0.0f, 0.0f),
    f32v3(-0.5f, 0.0f, -0.5f),        f32v3(-0.5f, -0.3536f, -0.6464f), f32v3(0.0f, -0.3536f, -0.6464f),
    f32v3(0.0f, 0.3536f, -1.354f),    f32v3(0.5f, 0.3536f, -1.354f),    f32v3(1.0f, 0.5f, -0.5f),
    f32v3(1.0f, 0.5f, 0.0f),          f32v3(0.0f, 0.1179f, -1.118f),    f32v3(0.5f, 0.1179f, -1.118f),
    f32v3(1.0f, 0.1667f, -0.5f),      f32v3(1.0f, 0.1667f, 0.0f),       f32v3(0.0f, -0.1179f, -0.8821f),
    f32v3(0.5f, -0.1179f, -0.8821f),  f32v3(1.0f, -0.1667f, -0.5f),     f32v3(1.0f, -0.1667f, 0.0f),
    f32v3(0.0f, -0.3536f, -0.6464f),  f32v3(0.5f, -0.3536f, -0.6464f),  f32v3(1.0f, -0.5f, -0.5f),
    f32v3(1.0f, -0.5f, 0.0f),
};
