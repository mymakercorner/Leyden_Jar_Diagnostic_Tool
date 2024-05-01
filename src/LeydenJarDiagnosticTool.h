#pragma once

// SPDX-FileCopyrightText: 2024 Eric Becourt <rico@mymakercorner.com>
// SPDX-License-Identifier: MIT

#include <vector>
#include "LeydenJarAgent.h"

class LeydenJarDiagnosticTool
{
private:

	struct ViaKey
	{
		float x;
		float y;
		float w;
		float h;
		float x2;
		float y2;
		float w2;
		float h2;
		int row;
		int col;
		int groupNum;
		int groupIdx;

		void SetDefaultVals() 
		{
			x = 0.f;
			y = 0.f;
			w = 1.f;
			h = 1.f;
			x2 = 0.f;
			y2 = 0.f;
			w2 = 1.f;
			h2 = 1.f;
			row = -1;
			col = -1;
			groupNum = -1;
			groupIdx = -1;
		}
	};

	struct ViaLayoutOption
	{
		bool isCheckbox;
		bool isChecked;
		std::string layoutName;
		std::vector<std::string> listboxItems;
		int selectionIndex;
	};

	enum LeftPaneLayout
	{
		LeftPaneLayoutDeciveDescription,
		LeftPaneLayoutKeyPressMonitor,
		LeftPaneLayoutSignalMonitor
	};

	enum RightPaneLayout
	{
		RightPaneLayoutDeciveDescription,
		RightaneLayoutKeyPressMonitor,
		RightPaneLayoutSignalMonitor
	};

	enum SwitchTechnology
	{
		SwitchTechnologyModelF = 0,
		SwitchTechnologyBeamSpring
	};

	enum RightPaneView
	{
		RightPaneViewKeyboardLayout = 0,
		RightPaneViewQmkMatrix,
		RightPaneViewPhysicalMatrix
	};

public:

	bool Initialize();
	bool Finalize();

	void GuiLoop();

private:

	void LeftPaneRendering();
	void LeftPaneRenderingDeviceDescription();
	void LeftPaneRenderingKeyPresses();
	void LeftPaneRenderingSignalLevels();

	void LeftPaneDrawViaLayoutOptions();
	void LeftPaneDrawLeydenJarInfos();

	void RightPaneRendering();
	void RightPaneRenderingDeviceDescription();
	void RightPaneRenderingSignalLevels();
	void RightPaneRenderingKeyPresses();

	void RightPaneDrawKeyboardLayout(bool drawLevels);
	void RightPaneDrawPhysicalLayout(bool drawLevels);

	void RefreshDeviceList();
	void DecodeVialKeyboardDefinition(const uint8_t* compressedVialData, uint32_t compressedVialSize);

private: 

	LeydenJarAgent m_Agent;
	bool m_IsDeviceListParsed;
	int m_SelectedDeviceIndex;
	char m_DeviceListNames[16][256];
	
	uint32_t m_VialUncompressedKeyboardDefinitionSize;
	char m_VialUncompressedKeyboardDefinitionData[8192];

	std::vector< ViaLayoutOption> m_LayoutOptions;
	std::vector< std::vector<ViaKey> > m_Keys;

	LeftPaneLayout  m_CurrentLeftPaneLayout;
	RightPaneLayout m_CurrentRightPaneLayout;
	int             m_RightPaneViewType;
	bool			m_LogicalKeyboardStateRequestSent;
	bool			m_PhysicalKeyboardStateRequestSent;
	bool			m_KeyboardLevelsRequestSent;
	bool			m_KeyboardLevelsAcquired;
	uint32_t		m_LogicKeyboardState[8];
	uint8_t         m_PhysicalKeyboardState[18];
	int             m_CurLevelIdx;
	uint16_t		m_CurLevels[3][18][8];
	uint16_t		m_MinLevels[18][8];
	uint16_t		m_MaxLevels[18][8];
};