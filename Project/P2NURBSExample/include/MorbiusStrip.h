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

const static ui32 kMobiusNumPatches  = 4;
const static ui32 kMobiusNumVertices = 64;

// Simple Bezier patch for a Mobius strip
// 4 patches with 16 control points each
const static f32v3 mobiusStripVertices[kMobiusNumVertices] = {
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

const static ui32 mobiusStripPatches[16 * kMobiusNumPatches] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
    22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
    44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};
