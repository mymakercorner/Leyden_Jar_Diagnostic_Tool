// SPDX-FileCopyrightText: 2024 Eric Becourt <rico@mymakercorner.com>
// SPDX-License-Identifier: MIT

#include <cstdlib>
#include <cstring>
#include <limits>
#include <array>
#include <algorithm>

#include "LeydenJarDiagnosticTool.h"
#include "LeydenJarImGuiHelpers.h"
#include "json/json.h"

// Minlzma library is in pure C99 code.
// We unfortunately have to rely on the extern "C" thing to prevent link errors 
extern "C"
{
#include "minlzma.h"
}

// You can put this variable to true to display ImGui demo window.
// If you plan to tweak the GUI yourself this is a very good place to start learning ImGui API.
bool showDemoWindow = false;

bool LeydenJarDiagnosticTool::Initialize()
{
    m_IsDeviceListParsed = false;
    m_SelectedDeviceIndex = -1;
    m_VialUncompressedKeyboardDefinitionSize = 0;

    m_CurrentLeftPaneLayout = LeftPaneLayoutDeciveDescription;
    m_CurrentRightPaneLayout = RightPaneLayoutDeciveDescription;

    m_RightPaneViewType = RightPaneViewKeyboardLayout;

    return true;
}

bool LeydenJarDiagnosticTool::Finalize()
{
    if (m_Agent.IsDeviceOpened())
    {
        m_Agent.RequestEnable();
        m_Agent.WaitEndRequest();
    }
    return true;
}

void LeydenJarDiagnosticTool::RefreshDeviceList()
{
    m_Agent.RequestDeviceEnumeration();
    m_Agent.WaitEndRequest();
    for (int i = 0; i < m_Agent.GetNbEnumeratedDevices(); i++)
    {
        wcstombs(m_DeviceListNames[i], m_Agent.GetHidDeviceInfo(i)->product_string, 256);
    }

    m_SelectedDeviceIndex = -1;
}

void LeydenJarDiagnosticTool::DecodeVialKeyboardDefinition(const uint8_t* compressedVialData, uint32_t compressedVialSize)
{
    //Decompress LZMA encoded VIAL keyboard definitions
    bool decodeResult = XzDecode(compressedVialData, compressedVialSize, nullptr, &m_VialUncompressedKeyboardDefinitionSize);
    if (decodeResult == true && m_VialUncompressedKeyboardDefinitionSize < sizeof(m_VialUncompressedKeyboardDefinitionData))
    {
        XzDecode(compressedVialData, compressedVialSize, (uint8_t*)m_VialUncompressedKeyboardDefinitionData, &m_VialUncompressedKeyboardDefinitionSize);
    }

    Json::Reader jsonReader;
    Json::Value root;

    // Decode decompressed json data
    bool ret = jsonReader.parse(m_VialUncompressedKeyboardDefinitionData, m_VialUncompressedKeyboardDefinitionData + m_VialUncompressedKeyboardDefinitionSize, root, false);
    if (ret == true)
    {
        // Read all possible layout options
        Json::Value labels = root["layouts"]["labels"];
        if (labels.isArray())
        {
            Json::ArrayIndex nbLabels = labels.size();
            m_LayoutOptions.resize(nbLabels);

            for (Json::ArrayIndex i = 0; i < nbLabels; i++)
            {
                Json::Value label = root["layouts"]["labels"][i];
                if (label.isArray())
                {
                    Json::ArrayIndex nbLabelItems = label.size();
                    m_LayoutOptions[i].listboxItems.resize(nbLabelItems - 1);
                    m_LayoutOptions[i].isCheckbox = false;
                    m_LayoutOptions[i].isChecked = false;
                    m_LayoutOptions[i].selectionIndex = 0;

                    for (Json::ArrayIndex j = 0; j < nbLabels; j++)
                    {
                        Json::Value labelElem = root["layouts"]["labels"][i][j];
                        if (labelElem.isString())
                        {
                            Json::String str = labelElem.asString();
                            if (j == 0)
                                m_LayoutOptions[i].layoutName = str;
                            else
                                m_LayoutOptions[i].listboxItems[j - 1] = str;
                        }
                    }
                }
                else if (label.isString())
                {
                    Json::String str = label.asString();
                    m_LayoutOptions[i].layoutName = str;
                    m_LayoutOptions[i].isCheckbox = true;
                    m_LayoutOptions[i].isChecked = false;
                    m_LayoutOptions[i].selectionIndex = 0;
                }
            }
        }

        // Read keymap
        Json::Value keymap = root["layouts"]["keymap"];
        if (keymap.isArray())
        {
            Json::ArrayIndex nbRows = keymap.size();
            m_Keys.resize(nbRows);

            for (Json::ArrayIndex row = 0; row < nbRows; row++)
            {
                Json::Value rowArray = root["layouts"]["keymap"][row];
                if (rowArray.isArray())
                {
                    ViaKey key;
                    key.SetDefaultVals();

                    Json::ArrayIndex nbRowElems = rowArray.size();

                    for (Json::ArrayIndex rowElem = 0; rowElem < nbRowElems; rowElem++)
                    {
                        Json::Value elem = root["layouts"]["keymap"][row][rowElem];
                        if (elem.isObject())
                        {
                            Json::Value elem = root["layouts"]["keymap"][row][rowElem]["x"];
                            if (elem.isNumeric())
                                key.x = elem.asFloat();
                            elem = root["layouts"]["keymap"][row][rowElem]["y"];
                            if (elem.isNumeric())
                                key.y = elem.asFloat();
                            elem = root["layouts"]["keymap"][row][rowElem]["w"];
                            if (elem.isNumeric())
                                key.w = key.w2 = elem.asFloat();
                            elem = root["layouts"]["keymap"][row][rowElem]["h"];
                            if (elem.isNumeric())
                                key.h = key.h2 = elem.asFloat();
                            elem = root["layouts"]["keymap"][row][rowElem]["x2"];
                            if (elem.isNumeric())
                                key.x2 = elem.asFloat();
                            elem = root["layouts"]["keymap"][row][rowElem]["y2"];
                            if (elem.isNumeric())
                                key.y2 = elem.asFloat();
                            elem = root["layouts"]["keymap"][row][rowElem]["w2"];
                            if (elem.isNumeric())
                                key.w2 = elem.asFloat();
                            elem = root["layouts"]["keymap"][row][rowElem]["h2"];
                            if (elem.isNumeric())
                                key.h2 = elem.asFloat();
                        }
                        else if (elem.isString())
                        {
                            std::string::size_type rS;
                            key.row = std::stoi(elem.asString(), &rS);
                            std::string::size_type cS;
                            key.col = std::stoi(elem.asString().substr(rS + 1), &cS);

                            if (elem.asString().length() > rS + 1 + cS)
                            {
                                std::string::size_type gnS;
                                key.groupNum = std::stoi(elem.asString().substr(rS + 1 + cS + 3), &gnS);
                                key.groupIdx = std::stoi(elem.asString().substr(rS + 1 + cS + 3 + gnS + 1));
                            }

                            m_Keys[row].push_back(key);
                            key.SetDefaultVals();
                        }
                    }
                }
            }
        }
    }

    //Rework keymap to be usable for rendering keys
    std::vector< std::vector<ImVec2> > layoutPositions;
    layoutPositions.resize(m_LayoutOptions.size());
    for (size_t i = 0; i < m_LayoutOptions.size(); i++)
    {
        size_t nbOptions;
        if (m_LayoutOptions[i].isCheckbox)
            nbOptions = 2;
        else
            nbOptions = m_LayoutOptions[i].listboxItems.size();
        for (size_t j = 0; j < nbOptions; j++)
            layoutPositions[i].push_back(ImVec2(0.f, 0.f));
    }

    //All keys positions are relative, we translate them to absolute positions
    for (size_t row = 0; row < m_Keys.size(); row++)
    {
        for (size_t col = 0; col < m_Keys[row].size(); col++)
        {
            if (row > 0)
            {
                if (col == 0)
                    m_Keys[row][col].y += m_Keys[row - 1][0].y + m_Keys[row - 1][0].h;
                else
                    m_Keys[row][col].y = m_Keys[row][col - 1].y;
            }
            if (col > 0)
            {
                m_Keys[row][col].x += m_Keys[row][col - 1].x + m_Keys[row][col - 1].w;
            }

            if (m_Keys[row][col].groupNum != -1)
            {
                int groupNum = m_Keys[row][col].groupNum;
                int groupIdx = m_Keys[row][col].groupIdx;

                if (layoutPositions[groupNum][groupIdx].x == 0.f && layoutPositions[groupNum][groupIdx].y == 0.f)
                {
                    layoutPositions[groupNum][groupIdx].x = m_Keys[row][col].x + m_Keys[row][col].x2;
                    layoutPositions[groupNum][groupIdx].y = m_Keys[row][col].y;
                }
            }
        }
    }

    //Align all optional layouts to their default layout
    for (size_t row = 0; row < m_Keys.size(); row++)
    {
        for (size_t col = 0; col < m_Keys[row].size(); col++)
        {
            if (m_Keys[row][col].groupNum != -1)
            {
                int groupNum = m_Keys[row][col].groupNum;
                int groupIdx = m_Keys[row][col].groupIdx;

                if (groupIdx > 0)
                {
                    if (layoutPositions[groupNum][groupIdx].x == layoutPositions[groupNum][0].x)
                    {
                        float offsetY = layoutPositions[groupNum][groupIdx].y - layoutPositions[groupNum][0].y;
                        m_Keys[row][col].y -= offsetY;
                    }
                    else if (layoutPositions[groupNum][groupIdx].y == layoutPositions[groupNum][0].y)
                    {
                        float offsetX = layoutPositions[groupNum][groupIdx].x - layoutPositions[groupNum][0].x;
                        m_Keys[row][col].x -= offsetX;
                    }
                }
            }
        }
    }

    //Find minimal X and Y positions for the layouts
    float minX = FLT_MAX;
    float minY = FLT_MAX;

    for (size_t row = 0; row < m_Keys.size(); row++)
    {
        for (size_t col = 0; col < m_Keys[row].size(); col++)
        {
            minX = std::min(minX, m_Keys[row][col].x);
            minY = std::min(minY, m_Keys[row][col].y);
        }
    }

    //Translate layouts to zero
    for (size_t row = 0; row < m_Keys.size(); row++)
    {
        for (size_t col = 0; col < m_Keys[row].size(); col++)
        {
            m_Keys[row][col].x -= minX;
            m_Keys[row][col].y -= minY;
        }
    }
}

void LeydenJarDiagnosticTool::RunStep()
{
    ImGuiIO& io = ImGui::GetIO();
    
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0,0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(displaySize, ImGuiCond_Always);

    ImGui::Begin("Hello, world!", NULL,
        ImGuiWindowFlags_MenuBar
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::BeginChild("left pane", ImVec2(320, 0), true);
    LeftPaneRendering();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("Right pane", ImVec2(0, 0), true);
    RightPaneRendering();
    ImGui::EndChild();

    ImGui::End();

    if (showDemoWindow)
        ImGui::ShowDemoWindow(&showDemoWindow);
}

void LeydenJarDiagnosticTool::LeftPaneRendering()
{
    switch (m_CurrentLeftPaneLayout)
    {
    case LeftPaneLayoutDeciveDescription:
        LeftPaneRenderingDeviceDescription();
        break;
    case LeftPaneLayoutKeyPressMonitor:
        LeftPaneRenderingKeyPresses();
        break;
    case LeftPaneLayoutSignalMonitor:
        LeftPaneRenderingSignalLevels();
        break;
    }
}

void LeydenJarDiagnosticTool::RightPaneRendering()
{
    if (m_Agent.IsDeviceOpened())
    {
        switch (m_CurrentLeftPaneLayout)
        {
        case LeftPaneLayoutDeciveDescription:
            RightPaneRenderingDeviceDescription();
            break;
        case LeftPaneLayoutKeyPressMonitor:
            RightPaneRenderingKeyPresses();
            break;
        case LeftPaneLayoutSignalMonitor:
            RightPaneRenderingSignalLevels();
            break;
        }
    }
}

void LeydenJarDiagnosticTool::LeftPaneDrawViaLayoutOptions()
{
    ImGui::SeparatorText("VIA Layouts");
    for (size_t layoutOption = 0; layoutOption < m_LayoutOptions.size(); layoutOption++)
    {
        if (m_LayoutOptions[layoutOption].isCheckbox)
        {
            ImGui::Checkbox(m_LayoutOptions[layoutOption].layoutName.c_str(), &m_LayoutOptions[layoutOption].isChecked);
            if (m_LayoutOptions[layoutOption].isChecked)
                m_LayoutOptions[layoutOption].selectionIndex = 1;
            else
                m_LayoutOptions[layoutOption].selectionIndex = 0;
        }
        else
        {
            const char* comboItems[8];
            for (size_t i = 0; i < m_LayoutOptions[layoutOption].listboxItems.size(); i++)
                comboItems[i] = m_LayoutOptions[layoutOption].listboxItems[i].c_str();
            ImGui::Combo(m_LayoutOptions[layoutOption].layoutName.c_str(), &m_LayoutOptions[layoutOption].selectionIndex, comboItems, int(m_LayoutOptions[layoutOption].listboxItems.size()));
        }
    }
}

void LeydenJarDiagnosticTool::LeftPaneDrawLeydenJarInfos()
{
    const LeydenJarAgent::LeydenJarDeviceInfo* pDeviceInfo = m_Agent.GetDeviceInfo();

    ImGui::SeparatorText("Leyden Jar Infos");

    ImGui::Text("Protocol Version: %d.%d.%d", pDeviceInfo->protocolVerMajor, pDeviceInfo->protocolVerMid, pDeviceInfo->protocolVerMinor);

    const char* matrixLayoutName[3] = { "Native Leyden Jar", "XWhatsit", "Wcass" };
    const char* switchTechnologyName[3] = { "Model F", "BeamSpring" };
    ImGui::Text("Matrix Layout: %s", matrixLayoutName[pDeviceInfo->matrixToControllerType]);
    ImGui::Text("Number of QMK Cols: %d", pDeviceInfo->nbLogicalCols);
    ImGui::Text("Number of QMK Rows: %d", pDeviceInfo->nbLogicalRows);
    ImGui::Text("Number of Controller Cols: %d", pDeviceInfo->nbPhysicalCols);
    ImGui::Text("Number of Controller Rows: %d", pDeviceInfo->nbPhysicalRows);
    ImGui::Text("Switch Technology: %s", switchTechnologyName[pDeviceInfo->switchTechnology]);
    ImGui::Text("We have %d DAC bins:", pDeviceInfo->nbBins);
    for (int i = 0; i < pDeviceInfo->nbBins; i++)
    {
        ImGui::Text("  - DAC Threshold %d: %d", i, pDeviceInfo->dacThreshold[i]);
        ImGui::Text("  - DAC Ref Level %d: %d", i, pDeviceInfo->dacRefLevel[i]);
    }
}

void LeydenJarDiagnosticTool::LeftPaneRenderingDeviceDescription()
{
    ImGui::SeparatorText("Device List");

    if (ImGui::BeginListBox("Device List", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
    {
        for (int n = 0; n < m_Agent.GetNbEnumeratedDevices(); n++)
        {
            const bool is_selected = (m_SelectedDeviceIndex == n) || (m_SelectedDeviceIndex == -1);
            bool selectionChanged = ImGui::Selectable(m_DeviceListNames[n], is_selected) || (m_SelectedDeviceIndex == -1);

            ImGui::SetItemTooltip(m_DeviceListNames[n]);

            if (selectionChanged)
            {
                m_SelectedDeviceIndex = n;
                
                if (m_Agent.IsDeviceOpened())
                {
                    m_Agent.RequestEnable();
                    m_Agent.WaitEndRequest();
                }

                m_Agent.RequestDeviceConnection(m_SelectedDeviceIndex);
                m_Agent.WaitEndRequest();

                m_Agent.RequestDisable();
                m_Agent.WaitEndRequest();

                ImGui::SetItemDefaultFocus();
                
                m_VialUncompressedKeyboardDefinitionSize = 0;
                m_LayoutOptions.clear();
                m_Keys.clear();
                std::memset(m_LogicKeyboardState, 0, sizeof(m_LogicKeyboardState));
                std::memset(m_PhysicalKeyboardState, 0, sizeof(m_PhysicalKeyboardState));
                m_LogicalKeyboardStateRequestSent = false;
                m_PhysicalKeyboardStateRequestSent = false;
                m_KeyboardLevelsRequestSent = false;
                m_KeyboardLevelsAcquired = false;
                m_CurLevelIdx = -1;
                std::memset(m_CurLevels, 0, sizeof(m_CurLevels));
                std::memset(m_MinLevels, 0xFF, sizeof(m_MinLevels));
                std::memset(m_MaxLevels, 0, sizeof(m_MaxLevels));
            }
        }
        ImGui::EndListBox();
    }

    if (ImGui::Button("Refresh Device List", ImVec2(-FLT_MIN, 0)))
    {
        RefreshDeviceList();
    }

    ImGui::SeparatorText("Device actions");

    if (ImGui::Button("Enter Bootloader", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0)))
    {
        if (m_Agent.IsDeviceOpened())
        {
            m_Agent.RequestEnterBootloader();
            m_Agent.WaitEndRequest();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Erase EEPROM", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        if (m_Agent.IsDeviceOpened())
        {
            m_Agent.RequestEraseEeprom();
            m_Agent.WaitEndRequest();
        }
    }
    if (ImGui::Button("Keypress Monitor", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0)))
    {
        if (m_Agent.IsDeviceOpened())
        {
            m_CurrentLeftPaneLayout = LeftPaneLayoutKeyPressMonitor;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Level Monitor", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        if (m_Agent.IsDeviceOpened())
        {
            m_CurrentLeftPaneLayout = LeftPaneLayoutSignalMonitor;
        }
    }

    if (m_Agent.IsDeviceOpened())
    {
        const LeydenJarAgent::LeydenJarDeviceInfo* pDeviceInfo = m_Agent.GetDeviceInfo();

        LeftPaneDrawLeydenJarInfos();

        if (pDeviceInfo->viaVersionMajor != 0 || pDeviceInfo->viaVersionMinor != 0)
        {
            ImGui::SeparatorText("VIA Infos");
            ImGui::Text("Protocol Version: %d.%d", pDeviceInfo->viaVersionMajor, pDeviceInfo->viaVersionMinor);
        }

        if (pDeviceInfo->vialVersion0 != 0 || pDeviceInfo->vialVersion1 != 0 || pDeviceInfo->vialVersion2 != 0 || pDeviceInfo->vialVersion3 != 0)
        {
            ImGui::SeparatorText("VIAL Infos");
            ImGui::Text("Protocol Version: %d.%d.%d.%d", pDeviceInfo->vialVersion0, pDeviceInfo->vialVersion1, pDeviceInfo->vialVersion2, pDeviceInfo->vialVersion3);
            ImGui::Text("Keyboard UID: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
                pDeviceInfo->vialUid[0], pDeviceInfo->vialUid[1], pDeviceInfo->vialUid[2], pDeviceInfo->vialUid[3],
                pDeviceInfo->vialUid[4], pDeviceInfo->vialUid[5], pDeviceInfo->vialUid[6], pDeviceInfo->vialUid[7]);
            ImGui::SetItemTooltip("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", 
                                  pDeviceInfo->vialUid[0], pDeviceInfo->vialUid[1], pDeviceInfo->vialUid[2], pDeviceInfo->vialUid[3],
                                  pDeviceInfo->vialUid[4], pDeviceInfo->vialUid[5], pDeviceInfo->vialUid[6], pDeviceInfo->vialUid[7]);
            ImGui::Text("Keyboard definition size: %d", pDeviceInfo->vialKeyboardDefinitionSize);

            if (m_VialUncompressedKeyboardDefinitionSize == 0 && pDeviceInfo->vialKeyboardDefinitionSize > 0)
                DecodeVialKeyboardDefinition(pDeviceInfo->vialKeyboardDefinitionData, pDeviceInfo->vialKeyboardDefinitionSize);
        }

        ImGui::SeparatorText("HID Infos");   

        ImGui::Text("Path: %s", pDeviceInfo->pHidDeviceInfo->path);
        ImGui::SetItemTooltip("%s", pDeviceInfo->pHidDeviceInfo->path);
        ImGui::Text("Vendor: 0x%04x", pDeviceInfo->pHidDeviceInfo->vendor_id);
        ImGui::Text("Product: 0x%04x", pDeviceInfo->pHidDeviceInfo->product_id);
        ImGui::Text("Serial Number: %S", pDeviceInfo->pHidDeviceInfo->serial_number);
        ImGui::Text("Release Number: 0x%04x", pDeviceInfo->pHidDeviceInfo->release_number);
        ImGui::Text("Usage Page: 0x%04x", pDeviceInfo->pHidDeviceInfo->usage_page);
        ImGui::Text("Usage: 0x%04x", pDeviceInfo->pHidDeviceInfo->usage);
        ImGui::Text("Manufacturer String: %S", pDeviceInfo->pHidDeviceInfo->manufacturer_string);

        LeftPaneDrawViaLayoutOptions();
    }
}

void LeydenJarDiagnosticTool::LeftPaneRenderingKeyPresses()
{
    if (ImGui::Button("Device List", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        m_CurrentLeftPaneLayout = LeftPaneLayoutDeciveDescription;
    }
    if (ImGui::Button("Level Monitor", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        m_CurrentLeftPaneLayout = LeftPaneLayoutSignalMonitor;
    }
    
    ImGui::SeparatorText("");

    const char* comboItems[3] = { "Keyboard Layout", "QMK Matrix", "Physical Matrix" };
    ImGui::Combo("View Type", &m_RightPaneViewType, comboItems, 3);

    LeftPaneDrawLeydenJarInfos();

    if (m_RightPaneViewType == RightPaneViewKeyboardLayout)
        LeftPaneDrawViaLayoutOptions();
}

void LeydenJarDiagnosticTool::LeftPaneRenderingSignalLevels()
{
    if (ImGui::Button("Device List", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        m_CurrentLeftPaneLayout = LeftPaneLayoutDeciveDescription;
    }
    if (ImGui::Button("Keypress Monitor", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        m_CurrentLeftPaneLayout = LeftPaneLayoutKeyPressMonitor;
    }

    ImGui::SeparatorText("");

    const char* comboItems[3] = { "Keyboard Layout", "QMK Matrix", "Physical Matrix" };
    ImGui::Combo("View Type", &m_RightPaneViewType, comboItems, 3);

    if (m_RightPaneViewType == RightPaneViewKeyboardLayout)
        LeftPaneDrawViaLayoutOptions();

    LeftPaneDrawLeydenJarInfos();
}

void LeydenJarDiagnosticTool::RightPaneRenderingDeviceDescription()
{
    RightPaneDrawKeyboardLayout(false);
}

void LeydenJarDiagnosticTool::RightPaneRenderingKeyPresses()
{
    if (m_RightPaneViewType == RightPaneViewKeyboardLayout)
    {
        if (!m_Agent.RequestInProgress())
        {
            if (m_LogicalKeyboardStateRequestSent)
            {
                const LeydenJarAgent::LeydenJarDeviceInfo* pDeviceInfo = m_Agent.GetDeviceInfo();
                for (int row = 0; row < pDeviceInfo->nbLogicalRows; row++)
                    m_LogicKeyboardState[row] = m_Agent.GetLogicalKeyboardState(row);
            }
            m_Agent.RequestLogicalScan();
            m_LogicalKeyboardStateRequestSent = true;
        }

        RightPaneDrawKeyboardLayout(false);
    }
    else
    {
        if (!m_Agent.RequestInProgress())
        {
            if (m_PhysicalKeyboardStateRequestSent)
            {
                const LeydenJarAgent::LeydenJarDeviceInfo* pDeviceInfo = m_Agent.GetDeviceInfo();
            
                m_Agent.GetPhysicalKeyboardState(m_PhysicalKeyboardState);
            }
            
            m_Agent.RequestPhysicalScan();
            m_PhysicalKeyboardStateRequestSent = true;
        }

        RightPaneDrawPhysicalLayout(false);
    }
}

static inline uint16_t medianLevelVal(uint16_t val0, uint16_t val1, uint16_t val2)
{
    std::array<uint16_t, 3> levels = { val0, val1, val2 };

    std::sort(levels.begin(), levels.end());
    return levels[1];
}

void LeydenJarDiagnosticTool::RightPaneRenderingSignalLevels()
{
    if (!m_Agent.RequestInProgress())
    {
        if (m_KeyboardLevelsRequestSent)
        {
            const LeydenJarAgent::LeydenJarDeviceInfo* pDeviceInfo = m_Agent.GetDeviceInfo();
            
            m_CurLevelIdx++;

            int idx = m_CurLevelIdx % 3;

            for (int col = 0; col < pDeviceInfo->nbPhysicalCols; col++)
            {
                m_Agent.GetColLevels(col, m_CurLevels[idx][col]);
            }

            for (int col = 0; col < pDeviceInfo->nbPhysicalCols; col++)
            {
                if (m_CurLevelIdx >= 2)
                {
                    int idxPrev1 = (m_CurLevelIdx - 1) % 3;
                    int idxPrev2 = (m_CurLevelIdx - 2) % 3;

                    for (int row = 0; row < pDeviceInfo->nbPhysicalRows; row++)
                    {
                        int binIdx = pDeviceInfo->binningMap[col][row];

                        if (m_CurLevels[idx][col][row] < pDeviceInfo->dacThreshold[binIdx] &&
                            m_CurLevels[idxPrev1][col][row] < pDeviceInfo->dacThreshold[binIdx] &&
                            m_CurLevels[idxPrev2][col][row] < pDeviceInfo->dacThreshold[binIdx])
                        {
                            m_MinLevels[col][row] = std::min(m_MinLevels[col][row], medianLevelVal(m_CurLevels[idx][col][row], m_CurLevels[idxPrev1][col][row], m_CurLevels[idxPrev2][col][row]));
                        }

                        if (m_CurLevels[idx][col][row] >= pDeviceInfo->dacThreshold[binIdx] &&
                            m_CurLevels[idxPrev1][col][row] >= pDeviceInfo->dacThreshold[binIdx] &&
                            m_CurLevels[idxPrev2][col][row] >= pDeviceInfo->dacThreshold[binIdx])
                        {
                            m_MaxLevels[col][row] = std::max(m_MaxLevels[col][row], medianLevelVal(m_CurLevels[idx][col][row], m_CurLevels[idxPrev1][col][row], m_CurLevels[idxPrev2][col][row]));
                        }
                    }
                }
            }

            m_KeyboardLevelsAcquired = true;
        }
    
        m_Agent.RequestDetectLevels();
        m_KeyboardLevelsRequestSent = true;
    }

    if (m_RightPaneViewType == RightPaneViewKeyboardLayout)
        RightPaneDrawKeyboardLayout(true);
    else
        RightPaneDrawPhysicalLayout(true);
}

ImU32 LeydenJarDiagnosticTool::GetKeyColorFromLevel(const LeydenJarAgent::LeydenJarDeviceInfo* pDeviceInfo, int idx, int matrixCol, int matrixRow)
{
    ImU32 gbColUnpressed = IM_COL32(45, 45, 45, 255);
    ImU32 gbColPressed = IM_COL32(45, 45, 255, 255);
    ImU32 gbColPressedLight = IM_COL32(128, 128, 255, 255);

    ImU32 colKey = gbColUnpressed;

    int binIdx = pDeviceInfo->binningMap[matrixCol][matrixRow];

    if (m_KeyboardLevelsAcquired)
    {
        if (pDeviceInfo->switchTechnology == SwitchTechnologyModelF)
        {
            if (m_CurLevels[idx][matrixCol][matrixRow] >= pDeviceInfo->dacThreshold[binIdx])
            {
                if ((m_CurLevels[idx][matrixCol][matrixRow] - pDeviceInfo->dacThreshold[binIdx]) <= 3)
                    colKey = gbColPressedLight;
                else
                    colKey = gbColPressed;
            }
        }
        else if (pDeviceInfo->switchTechnology == SwitchTechnologyBeamSpring)
        {
            if (m_CurLevels[idx][matrixCol][matrixRow] <= pDeviceInfo->dacThreshold[binIdx])
            {
                if ((pDeviceInfo->dacThreshold[binIdx] - m_CurLevels[idx][matrixCol][matrixRow]) <= 3)
                    colKey = gbColPressedLight;
                else
                    colKey = gbColPressed;
            }
        }
    }

    return colKey;
}

void LeydenJarDiagnosticTool::RightPaneDrawPhysicalLayout(bool drawLevels)
{
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    ImVec2 rowConnectorDrawPos = ImVec2(pos.x + 1.5f * 40.f, pos.y + 2.f * 40.f + (1.5f * 40.f) / 2.f);

    const LeydenJarAgent::LeydenJarDeviceInfo* pDeviceInfo = m_Agent.GetDeviceInfo();

    int maxCol;
    int maxRow;
    int startRow = 0;

    if (m_RightPaneViewType == RightPaneViewPhysicalMatrix)
    {
        maxCol = pDeviceInfo->nbPhysicalCols;
        maxRow = pDeviceInfo->nbPhysicalRows;
    }
    else
    {
        maxCol = pDeviceInfo->nbLogicalCols;
        maxRow = pDeviceInfo->nbLogicalRows;
        if (!pDeviceInfo->isKeyboardLeft)
            startRow = 8;
        else
        {
            if (maxRow > 8)
                maxRow /= 2;
        }
    }

    for (int row = startRow; row < maxRow; row++)
    {
        char rowNumberString[4];
        sprintf(rowNumberString, "R%d", row);
        pDrawList->AddText(NULL, 0.0f, ImVec2(rowConnectorDrawPos.x - 35.f, rowConnectorDrawPos.y - 6.f), ImGui::GetColorU32(ImGuiCol_Text), rowNumberString, rowNumberString + strlen(rowNumberString), 0.0f, NULL);

        pDrawList->AddCircleFilled(rowConnectorDrawPos, 10.f, IM_COL32(45, 45, 45, 255));
        pDrawList->AddCircle(rowConnectorDrawPos, 10.f, IM_COL32(255, 255, 0, 255), 0, 8.f);
        rowConnectorDrawPos.y += 1.55f * 40.f;
    }

    ImVec2 colConnectorDrawPos = ImVec2(pos.x + 2.f * 40.f + (1.35f * 40.f) / 2.f, pos.y + 60.f);

    float keyDrawPosX = 2.f * 40.f;
    float keyDrawPosY = 2.f * 40.f;
    float keyDrawIncX = 1.35f * 40.f;
    float keyDrawIncY = 1.55f * 40.f;
    
    ImU32 gbColUnpressed    = IM_COL32(45, 45, 45, 255);
    ImU32 gbColPressed      = IM_COL32(45, 45, 255, 255);
    ImU32 outlineCol        = IM_COL32(200, 200, 200, 255);
    ImU32 binCol            = IM_COL32(128, 255, 128, 255);

    int idx = m_CurLevelIdx % 3;
    
    for (int col = 0; col < maxCol; col++)
    {
        char colNumberString[4];
        sprintf(colNumberString, "C%d", col);
        size_t lenString = strlen(colNumberString);
        pDrawList->AddText(NULL, 0.0f, ImVec2(colConnectorDrawPos.x - float(6 * (lenString - 1)), colConnectorDrawPos.y - 35.f), ImGui::GetColorU32(ImGuiCol_Text), colNumberString, colNumberString + strlen(colNumberString), 0.0f, NULL);

        pDrawList->AddCircleFilled(colConnectorDrawPos, 10.f, IM_COL32(45, 45, 45, 255));
        pDrawList->AddCircle(colConnectorDrawPos, 10.f, IM_COL32(255, 255, 0, 255), 0, 8.f);
        colConnectorDrawPos.x += 1.35f * 40.f;

        ImVec2 keyDrawPos = ImVec2(pos.x + keyDrawPosX, pos.y + keyDrawPosY);

        for (int row = startRow; row < maxRow; row++)
        {
            int matrixCol;
            int matrixRow;

            if (m_RightPaneViewType == RightPaneViewPhysicalMatrix)
            {
                matrixCol = col;
                matrixRow = row;
            }
            else
            {
                matrixCol = pDeviceInfo->matrixToControllerCols[col];
                if (row < 8)
                    matrixRow = pDeviceInfo->matrixToControllerRows[row];
                else
                    matrixRow = pDeviceInfo->matrixToControllerRows[row - 8];
            }

            if (!drawLevels)
            {
                ImU32 colKey;

                colKey = gbColUnpressed;
                if ((pDeviceInfo->isKeyboardLeft && row < 8) ||
                    (!pDeviceInfo->isKeyboardLeft && row >= 8) ||
                    (m_RightPaneViewType == RightPaneViewPhysicalMatrix))
                {
                    if (m_PhysicalKeyboardState[matrixCol] & (1 << matrixRow))
                        colKey = gbColPressed;
                }

                DrawConvexKey(pDrawList, keyDrawPos, 1.30f, 1.5f, 0.f, 0.f, 40.f, colKey, outlineCol);
            }
            else
            {
                ImU32 colKey = GetKeyColorFromLevel(pDeviceInfo, idx, matrixCol, matrixRow);

                DrawConvexKey(pDrawList, keyDrawPos, 1.30f, 1.5f, 0.f, 0.f, 40.f, colKey, outlineCol);

                if (m_KeyboardLevelsAcquired == true)
                {
                    char levelString[6];
                    if (m_MaxLevels[matrixCol][matrixRow] != 0)
                    {
                        sprintf(levelString, "%d", m_MaxLevels[matrixCol][matrixRow]);
                        pDrawList->AddText(NULL, 0.0f, ImVec2(keyDrawPos.x + 8.f, keyDrawPos.y + 5.f), ImGui::GetColorU32(ImGuiCol_Text), levelString, levelString + strlen(levelString), 0.0f, NULL);
                    }
                    sprintf(levelString, "%d", m_CurLevels[idx][matrixCol][matrixRow]);
                    pDrawList->AddText(NULL, 0.0f, ImVec2(keyDrawPos.x + 8.f, keyDrawPos.y + 22.f), ImGui::GetColorU32(ImGuiCol_Text), levelString, levelString + strlen(levelString), 0.0f, NULL);
                    if (m_MinLevels[matrixCol][matrixRow] != 0xFFFF)
                    {
                        sprintf(levelString, "%d", m_MinLevels[matrixCol][matrixRow]);
                        pDrawList->AddText(NULL, 0.0f, ImVec2(keyDrawPos.x + 8.f, keyDrawPos.y + 40.f), ImGui::GetColorU32(ImGuiCol_Text), levelString, levelString + strlen(levelString), 0.0f, NULL);
                    }

                    if (pDeviceInfo->binningMap[matrixCol][matrixRow] != 255)
                    {
                        sprintf(levelString, "%d", pDeviceInfo->binningMap[matrixCol][matrixRow]);
                        pDrawList->AddText(NULL, 0.0f, ImVec2(keyDrawPos.x + 36.f, keyDrawPos.y + 40.f), binCol, levelString, levelString + strlen(levelString), 0.0f, NULL);
                    }
                }
            }

            keyDrawPos.y += keyDrawIncY;
        }

        keyDrawPosX += keyDrawIncX;
    }
}

void LeydenJarDiagnosticTool::RightPaneDrawKeyboardLayout(bool drawLevels)
{
    const LeydenJarAgent::LeydenJarDeviceInfo* pDeviceInfo = m_Agent.GetDeviceInfo();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    ImU32 gbColUnpressed = IM_COL32(45, 45, 45, 255);
    ImU32 gbColPressed = IM_COL32(45, 45, 255, 255);
    ImU32 outlineCol = IM_COL32(200, 200, 200, 255);
    ImU32 binCol = IM_COL32(128, 255, 128, 255);

    int idx = m_CurLevelIdx % 3;

    for (size_t row = 0; row < m_Keys.size(); row++)
    {
        for (size_t col = 0; col < m_Keys[row].size(); col++)
        {
            if (!drawLevels)
            {
                ImU32 colKey;

                if (m_LogicKeyboardState[m_Keys[row][col].row] & (1 << m_Keys[row][col].col))
                    colKey = gbColPressed;
                else
                    colKey = gbColUnpressed;

                if (m_Keys[row][col].groupNum == -1 || m_Keys[row][col].groupIdx == m_LayoutOptions[m_Keys[row][col].groupNum].selectionIndex)
                    DrawKey(drawList, pos,
                        m_Keys[row][col].w, m_Keys[row][col].w2, m_Keys[row][col].h, m_Keys[row][col].h2,
                        m_Keys[row][col].x, m_Keys[row][col].x2, m_Keys[row][col].y, m_Keys[row][col].y2,
                        50.f, colKey, outlineCol);
            }
            else
            {
                if (m_Keys[row][col].groupNum == -1 || m_Keys[row][col].groupIdx == m_LayoutOptions[m_Keys[row][col].groupNum].selectionIndex)
                {
                    int matrixCol = pDeviceInfo->matrixToControllerCols[m_Keys[row][col].col];
                    int keyRow = m_Keys[row][col].row;
                    if (keyRow >= 8)
                        keyRow -= 8;
                    int matrixRow = pDeviceInfo->matrixToControllerRows[keyRow];

                    ImU32 colKey;
                    bool deadKey = false;

                    if ((pDeviceInfo->isKeyboardLeft && m_Keys[row][col].row >= 8) ||
                        (!pDeviceInfo->isKeyboardLeft && m_Keys[row][col].row < 8))
                    {
                        deadKey = true;
                        colKey = gbColUnpressed;
                    }
                    else
                        colKey = GetKeyColorFromLevel(pDeviceInfo, idx, matrixCol, matrixRow);

                    DrawKey(drawList, pos,
                        m_Keys[row][col].w, m_Keys[row][col].w2, m_Keys[row][col].h, m_Keys[row][col].h2,
                        m_Keys[row][col].x, m_Keys[row][col].x2, m_Keys[row][col].y, m_Keys[row][col].y2,
                        50.f, colKey, outlineCol);

                    if (m_KeyboardLevelsAcquired == true && !deadKey)
                    {
                        ImVec2 keyDrawPos;
                        keyDrawPos.x = pos.x + (m_Keys[row][col].x + m_Keys[row][col].w / 2 - 0.5f) * 50.f;
                        keyDrawPos.y = pos.y + (m_Keys[row][col].y + m_Keys[row][col].h / 2 - 0.5f) * 50.f;

                        char levelString[6];
                        if (m_MaxLevels[matrixCol][matrixRow] != 0)
                        {
                            sprintf(levelString, "%d", m_MaxLevels[matrixCol][matrixRow]);
                            drawList->AddText(NULL, 0.0f, ImVec2(keyDrawPos.x + 8.f, keyDrawPos.y + 5.f), ImGui::GetColorU32(ImGuiCol_Text), levelString, levelString + strlen(levelString), 0.0f, NULL);
                        }
                        sprintf(levelString, "%d", m_CurLevels[idx][matrixCol][matrixRow]);
                        drawList->AddText(NULL, 0.0f, ImVec2(keyDrawPos.x + 8.f, keyDrawPos.y + 18.f), ImGui::GetColorU32(ImGuiCol_Text), levelString, levelString + strlen(levelString), 0.0f, NULL);
                        if (m_MinLevels[matrixCol][matrixRow] != 0xFFFF)
                        {
                            sprintf(levelString, "%d", m_MinLevels[matrixCol][matrixRow]);
                            drawList->AddText(NULL, 0.0f, ImVec2(keyDrawPos.x + 8.f, keyDrawPos.y + 31.f), ImGui::GetColorU32(ImGuiCol_Text), levelString, levelString + strlen(levelString), 0.0f, NULL);
                        }

                        if (pDeviceInfo->binningMap[matrixCol][matrixRow] != 255)
                        {
                            sprintf(levelString, "%d", pDeviceInfo->binningMap[matrixCol][matrixRow]);
                            drawList->AddText(NULL, 0.0f, ImVec2(keyDrawPos.x + 36.f, keyDrawPos.y + 31.f), binCol, levelString, levelString + strlen(levelString), 0.0f, NULL);
                        }
                    }
                }
            }
        }
    }
}

