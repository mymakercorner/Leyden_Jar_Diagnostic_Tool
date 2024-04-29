#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "LeydenJarProtocol.h"

class LeydenJarAgent
{
public:

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

	enum LeydenJarAck
	{
		LeydenJarAckNone = 0,
		LeydenJarAckPending,
		LeydenJarAckSuccess,
		LeydenJarAckError
	};

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
		uint8_t					via_version_major;
		uint8_t					via_version_minor;
		uint8_t					vial_version_0;
		uint8_t					vial_version_1;
		uint8_t					vial_version_2;
		uint8_t					vial_version_3;
		uint8_t					vial_uid[8];
		uint32_t				vial_keyboard_definition_size;
		uint8_t					vial_keyboard_definition_data[8192];
		uint16_t				dacThreshold[4];
		uint16_t				dacRefLevel[4];
		uint8_t					binningMap[18][8];
	};

public:

	LeydenJarAgent();
	~LeydenJarAgent();

	void SendRequest(LeydenJarReq reqType);
	bool RequestInProgress();
	bool WaitEndRequest();

	void RequestDeviceEnumeration();
	void RequestDeviceConnection(int deviceIndex);
	void RequestEnterBootloader();
	void RequestEraseEeprom();
	void RequestDisable();
	void RequestEnable();
	void RequestLogicalScan();
	void RequestPhysicalScan();
	void RequestDetectLevels();

	int GetNbEnumeratedDevices();
	struct hid_device_info* GetHidDeviceInfo(int deviceIndex);
	bool IsDeviceOpened();
	const LeydenJarDeviceInfo* GetDeviceInfo();
	uint32_t GetLogicalKeyboardState(int row);
	void GetPhysicalKeyboardState(uint8_t* physicalState);
	void GetColLevels(int col, uint16_t* colLevels);

private:

	void ThreadLoop();

private:

	LeydenJarProtocol		m_Protocol;
	int						m_ReqType;
	int						m_ReqDeviceIndex;
	std::atomic<int>		m_AckType;
	std::mutex				m_Mutex;
	std::condition_variable m_CondVar;
	std::thread				m_Thread;

	LeydenJarDeviceInfo		m_DeviceInfo;
	uint32_t				m_LogicKeyboardState[8];
	uint8_t					m_PhysicalKeyboardState[18];
	uint16_t                m_Levels[18][8];
};

