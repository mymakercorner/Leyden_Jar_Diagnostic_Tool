#pragma once

// SPDX-FileCopyrightText: 2024 Eric Becourt <rico@mymakercorner.com>
// SPDX-License-Identifier: MIT

#include "imgui.h"

void DrawKey(ImDrawList* pDrawList, const ImVec2& startDrawPos, 
			 float width0, float width1, float height0, float height1, float x0, float x1, float y0, float y1,
			 float unitSize, ImU32 bgCol, ImU32 outlineCol);

void DrawConvexKey(ImDrawList* pDrawList, const ImVec2& startDrawPos, 
				   float width, float height, float x, float y,
				   float unitSize, ImU32 bgCol, ImU32 outlineCol);