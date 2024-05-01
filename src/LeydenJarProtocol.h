#pragma once

// SPDX-FileCopyrightText: 2024 Eric Becourt <rico@mymakercorner.com>
// SPDX-License-Identifier: MIT

#include <stdint.h> 

#include "hidapi.h" 

// This class handles all raw HID communications with the Leyden Jar controller firmware

class LeydenJarProtocol
{
public:
	LeydenJarProtocol();
	~LeydenJarProtocol() {}

	bool Initialize();
	bool Finalize();

	bool EnumerateDevices();
	void FreeEnumeratedDevices();
	int  GetNbEnumeratedDevices();
	bool IsDeviceOpened();
	struct hid_device_info* GetDeviceInfo(int deviceIndex);
	bool OpenDevice(int deviceIndex);
	bool CloseDevice();

	bool GetDacRefLevel(uint16_t& dacRefLevel, int binNumber);
	bool GetDacThreshold(uint16_t& dacThreshold, int binNumber);
	bool GetColumnBinMap(int columnIndex, uint8_t* binMap);
	bool GetColumnLevels(int columnIndex, uint16_t* column_levels);
	bool SetDac(int dacThreshold);
	bool SetKeyboardStatus(bool enable);
	bool GetKeyboardStatus(bool& enable);
	bool Reboot();
	bool EnterBootLoader();
	bool EraseEeprom();
	bool DetectLevels();
	bool ScanLogicalMatrix();
	bool ScanPhysicalMatrix();
	bool GetScanLogicalRow(int rowIndex, uint32_t& rowVal);
	bool GetScanPhysicalVals(uint8_t* rawVals);
	bool GetProtocolVersion(uint8_t& major, uint8_t& mid, uint16_t& minor);
	bool GetDetails(uint8_t& nbLogicalRows, uint8_t& nbLogicalCols, uint8_t& nbPhysicalRows, uint8_t& nbPhysicalCols, uint8_t& switchTechnology, uint8_t& nbBins);
	bool GetMatrixMapping(uint8_t& matrixToControllerType, uint8_t* matrixToControllerRows, uint8_t* matrixToControllerCols, uint8_t nbPhysicalRows, uint8_t nbPhysicalCols);
	
	bool GetVialInfos(uint8_t& version0, uint8_t& version1, uint8_t& version2, uint8_t& version3, uint8_t* pUid);
	bool GetVialKeyboardDefinitionSize(uint32_t& definitionSize);
	bool GetVialKeyboardDefinitionData(uint32_t definitionSize, uint8_t* pKeyboardDefinitionData);
	

	bool GetViaProtocolVersion(uint8_t& major, uint8_t& minor);

	void PrintDeviceList();

private:
	void PrintDevice(struct hid_device_info* curDev, int deviceIndex);
	void FillLeydenJarSendPacketHeader(uint8_t getOrSet, uint8_t command);
	void FillViaSendPacketHeader(uint8_t viaCommand, uint8_t command);
	bool HidSendCommand(bool hidReceive = true, bool checkReturn = true);
	bool GenericCommandNoPayload(uint8_t getOrSet, uint8_t command, bool hidReceive = true);

	bool GetVialKeyboardDefinitionDataBlock(uint16_t blockNumber, uint8_t* pBlockData);

private:
	struct hid_device_info* m_pEnumeratedDeviceInfo;
	hid_device* m_pHidDevice;
	uint8_t m_RawHidSendPacket[33];
	uint8_t m_RawHidRcvPacket[32];
	uint8_t* m_pSendPayloadPtr;
	uint8_t* m_pRcvPayloadPtr;
};