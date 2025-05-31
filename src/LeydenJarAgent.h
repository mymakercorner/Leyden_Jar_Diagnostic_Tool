#pragma once

// SPDX-FileCopyrightText: 2024 Eric Becourt <rico@mymakercorner.com>
// SPDX-License-Identifier: MIT

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "LeydenJarProtocol.h"

// This class acts as a daemon, running in a dedicated thread to dot disturb main application.
// It handles:
//   - all back and forth communication with the Leyden Jar controller firmware using the LeydenJarProtocol class.
//   - all back and forth communitation with the main application.
// It uses C++11 language constructs to handle thread management and synchronization, making this technical part totally cross-platform. 

class LeydenJarAgent
{
public:

	// List of all the request types that the main application can send to the deamon.
	enum LeydenJarReq 
	{
		LeydenJarReqNone = 0,
		LeydenJarReqQuit,
		LeydenJarReqEnumerate,
		LeydenJarReqConnect,
		LeydenJarReqEnterBootloader,
		LeydenJarReqEraseEeprom,
		LeydenJarReqDisable,
		LeydenJarReqEnable,
		LeydenJarReqScanLogical,
		LeydenJarReqScanPhysical,
		LeydenJarReqDetectLevels
	};

	// List of all acknowledge types the the deamon can send back to the main application.
	enum LeydenJarAck
	{
		LeydenJarAckNone = 0,
		LeydenJarAckPending,
		LeydenJarAckSuccess,
		LeydenJarAckError
	};

	// Structure containing all Leyden Jar controller firmware information.
	// It can be accessed for reading anytime by the main application.
	struct LeydenJarDeviceInfo
	{
		struct hid_device_info* pHidDeviceInfo;
		uint8_t					protocolVerMajor;
		uint8_t					protocolVerMid;
		uint16_t				protocolVerMinor;
		uint8_t					nbLogicalRows;
		uint8_t					nbLogicalCols;
		uint8_t					nbPhysicalRows;
		uint8_t					nbPhysicalCols;
		uint8_t					nbBins;
		uint8_t					switchTechnology;
		uint8_t					matrixToControllerType;
		uint8_t					matrixToControllerRows[8];
		uint8_t					matrixToControllerCols[18];
		uint8_t					controllerToMatrixRows[8];
		uint8_t					controllerToMatrixCols[18];
		uint8_t					viaVersionMajor;
		uint8_t					viaVersionMinor;
		uint8_t					vialVersion0;
		uint8_t					vialVersion1;
		uint8_t					vialVersion2;
		uint8_t					vialVersion3;
		uint8_t					vialUid[8];
		uint32_t				vialKeyboardDefinitionSize;
		uint8_t					vialKeyboardDefinitionData[8192];
		uint16_t				dacThreshold[16];
		uint16_t				dacRefLevel[16];
		uint8_t					binningMap[18][8];
		bool                    isKeyboardLeft;

		bool IsProtocolVersionOlder(uint8_t major, uint8_t mid, uint16_t minor);
	};

public:

	LeydenJarAgent();
	~LeydenJarAgent();
	
	// To know if a request is in progress, used by the application for asynchonous communication with the daemon
	bool RequestInProgress();
	// Wait the end of a request, used by the application for synchonous communication with the daemon
	bool WaitEndRequest();
	// Ask to enumerate HID devices
	void RequestDeviceEnumeration();
	// Ask to connect to a specific HID device
	void RequestDeviceConnection(int deviceIndex);
	// Ask to enter into the bootloader
	void RequestEnterBootloader();
	// Ask to erase EEPROM
	void RequestEraseEeprom();
	// Ask to disable key outputs of the currently connected HID device
	void RequestDisable();
	// Ask to enable key outputs of the currently connected HID device
	void RequestEnable();
	// Ask to retrieve the logical view of key presses (QMK view)
	void RequestLogicalScan();
	// Ask to retrieve the physical view of key presses (controller view)
	void RequestPhysicalScan();
	// Ask to retrieve currently detected analogic levels 
	void RequestDetectLevels();
	// Returns the number of enumerated HID devices
	int GetNbEnumeratedDevices();
	// Returns information for the selected HID device
	struct hid_device_info* GetHidDeviceInfo(int deviceIndex);
	// To know if we are connected to an HID device
	bool IsDeviceOpened();
	// Gives back to the main application all Leyden Jar controller firmware information
	const LeydenJarDeviceInfo* GetDeviceInfo();
	// Returns last requested logical(QMK view) scan state for a specific controller row
	uint32_t GetLogicalKeyboardState(int row);
	// Returns last requested physical(controller) scan state
	void GetPhysicalKeyboardState(uint8_t* physicalState);
	// Returns last requested analogic levels for a specific controller column
	void GetColLevels(int col, uint16_t* colLevels);

private:

	// Daemon entrypoint
	void ThreadLoop();
	// Low level send request
	void SendRequest(LeydenJarReq reqType);

private:

	LeydenJarProtocol		m_Protocol;
	int						m_ReqType;
	int						m_ReqDeviceIndex;
	std::atomic<int>		m_AckType;
	std::mutex				m_Mutex;
	std::condition_variable m_CondVar;
	std::thread				m_Thread;

	LeydenJarDeviceInfo		m_DeviceInfo;
	uint32_t				m_LogicKeyboardState[16];
	uint8_t					m_PhysicalKeyboardState[18];
	uint16_t                m_Levels[18][8];
};

