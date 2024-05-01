// SPDX-FileCopyrightText: 2024 Eric Becourt <rico@mymakercorner.com>
// SPDX-License-Identifier: MIT

#include "LeydenJarImGuiHelpers.h"

void DrawKey(ImDrawList* pDrawList, const ImVec2& startDrawPos, 
             float width0, float width1, float height0, float height1, float x0, float x1, float y0, float y1,
             float unitSize, ImU32 bgCol, ImU32 outlineCol)
{
    const float spacing = 4.f;
    const float radius = 2.f;
    const float fillOffset = 1.f;

    if (height0 != height1 && x1 != 0.f) //ISO case
    {
        pDrawList->PathLineTo(ImVec2(startDrawPos.x + (x0 + x1) * unitSize + fillOffset + spacing, startDrawPos.y + y0 * unitSize + fillOffset + spacing));
        pDrawList->PathLineTo(ImVec2(startDrawPos.x + (x0 + width0) * unitSize - fillOffset - spacing, startDrawPos.y + y0 * unitSize + fillOffset + spacing));
        pDrawList->PathLineTo(ImVec2(startDrawPos.x + (x0 + width0) * unitSize - fillOffset - spacing, startDrawPos.y + (y0 + height1) * unitSize - fillOffset - spacing));
        pDrawList->PathLineTo(ImVec2(startDrawPos.x + (x0 + x1) * unitSize + fillOffset + spacing, startDrawPos.y + (y0 + height1) * unitSize - fillOffset - spacing));

        pDrawList->PathFillConvex(bgCol);

        pDrawList->PathLineTo(ImVec2(startDrawPos.x + x0 * unitSize + fillOffset + spacing, startDrawPos.y + (y0 + height1) * unitSize - fillOffset - spacing));
        pDrawList->PathLineTo(ImVec2(startDrawPos.x + (x0 + width0) * unitSize - fillOffset - spacing, startDrawPos.y + (y0 + height1) * unitSize - fillOffset - spacing));
        pDrawList->PathLineTo(ImVec2(startDrawPos.x + (x0 + width0) * unitSize - fillOffset - spacing, startDrawPos.y + (y0 + height0) * unitSize - fillOffset - spacing));
        pDrawList->PathLineTo(ImVec2(startDrawPos.x + x0 * unitSize + fillOffset + spacing, startDrawPos.y + (y0 + height0) * unitSize - fillOffset - spacing));

        pDrawList->PathFillConvex(bgCol);

        pDrawList->PathArcToFast(ImVec2(startDrawPos.x + (x0 + x1) * unitSize + radius + spacing, startDrawPos.y + y0 * unitSize + radius + spacing), radius, 6, 9);
        pDrawList->PathArcToFast(ImVec2(startDrawPos.x + (x0 + width0) * unitSize - radius - spacing, startDrawPos.y + y0 * unitSize + radius + spacing), radius, 9, 12);
        pDrawList->PathArcToFast(ImVec2(startDrawPos.x + (x0 + width0) * unitSize - radius - spacing, startDrawPos.y + (y0 + height0) * unitSize - radius - spacing), radius, 0, 3);
        pDrawList->PathArcToFast(ImVec2(startDrawPos.x + x0 * unitSize + radius + spacing, startDrawPos.y + (y0 + height0) * unitSize - radius - spacing), radius, 3, 6);

        pDrawList->PathLineTo(ImVec2(startDrawPos.x + x0 * unitSize + spacing, startDrawPos.y + (y0 + height1) * unitSize - spacing));
        pDrawList->PathArcToFast(ImVec2(startDrawPos.x + (x0 + x1) * unitSize + radius + spacing, startDrawPos.y + (y0 + height1) * unitSize - radius - spacing), radius, 3, 6);

        pDrawList->PathStroke(outlineCol, ImDrawFlags_Closed, 2.f);
    }
    else
    {
        pDrawList->PathLineTo(ImVec2(startDrawPos.x + x0 * unitSize + fillOffset + spacing, startDrawPos.y + y0 * unitSize + fillOffset + spacing));
        pDrawList->PathLineTo(ImVec2(startDrawPos.x + (x0 + width0) * unitSize - fillOffset - spacing, startDrawPos.y + y0 * unitSize + fillOffset + spacing));
        pDrawList->PathLineTo(ImVec2(startDrawPos.x + (x0 + width0) * unitSize - fillOffset - spacing, startDrawPos.y + (y0 + height0) * unitSize - fillOffset - spacing));
        pDrawList->PathLineTo(ImVec2(startDrawPos.x + x0 * unitSize + fillOffset + spacing, startDrawPos.y + (y0 + height0) * unitSize - fillOffset - spacing));
        
        pDrawList->PathFillConvex(bgCol);

        pDrawList->PathArcToFast(ImVec2(startDrawPos.x + x0 * unitSize + radius + spacing, startDrawPos.y + y0 * unitSize + radius + spacing), radius, 6, 9);
        pDrawList->PathArcToFast(ImVec2(startDrawPos.x + (x0 + width0) * unitSize - radius - spacing, startDrawPos.y + y0 * unitSize + radius + spacing), radius, 9, 12);
        pDrawList->PathArcToFast(ImVec2(startDrawPos.x + (x0 + width0) * unitSize - radius - spacing, startDrawPos.y + (y0 + height0) * unitSize - radius - spacing), radius, 0, 3);
        pDrawList->PathArcToFast(ImVec2(startDrawPos.x + x0 * unitSize + radius + spacing, startDrawPos.y + (y0 + height0) * unitSize - radius - spacing), radius, 3, 6);

        pDrawList->PathStroke(outlineCol, ImDrawFlags_Closed, 2.f);
    }
}

void DrawConvexKey(ImDrawList* pDrawList, const ImVec2& startDrawPos, 
                   float width, float height, float x, float y, 
                   float unitSize, ImU32 bgCol, ImU32 outlineCol)
{
    DrawKey(pDrawList, startDrawPos, width, width, height, height, x, x, y, y, unitSize, bgCol, outlineCol);
}
