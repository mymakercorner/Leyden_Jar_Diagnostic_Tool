#pragma once

// SPDX-FileCopyrightText: 2024 Eric Becourt <rico@mymakercorner.com>
// SPDX-License-Identifier: MIT

// Set of helper functions to deal with low level ImGui rendering.
// For the moment there are only functions to hide complexity of rendering keyboard keys 

#include "imgui.h"

// Draw a generic keyboard key using same key information we can find in KLE editor generated data.
void DrawKey(ImDrawList* pDrawList, const ImVec2& startDrawPos, 
			 float width0, float width1, float height0, float height1, float x0, float x1, float y0, float y1,
			 float unitSize, ImU32 bgCol, ImU32 outlineCol);

// Draw a simple rectangular (or square) keyboard key.
// To draw an ISO key the generic function defined earlier must be used.
void DrawConvexKey(ImDrawList* pDrawList, const ImVec2& startDrawPos, 
				   float width, float height, float x, float y,
				   float unitSize, ImU32 bgCol, ImU32 outlineCol);