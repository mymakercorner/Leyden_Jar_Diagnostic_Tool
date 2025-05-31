// SPDX-FileCopyrightText: 2024 Eric Becourt <rico@mymakercorner.com>
// SPDX-License-Identifier: MIT

#include <cstring>

#include "LeydenJarAgent.h"

bool LeydenJarAgent::LeydenJarDeviceInfo::IsProtocolVersionOlder(uint8_t major, uint8_t mid, uint16_t minor)
{
    if (protocolVerMajor < major)
        return true;
    if (protocolVerMajor > major)
        return false;
    if (protocolVerMid < mid)
        return true;
    if (protocolVerMid > mid)
        return false;
    if (protocolVerMinor < minor)
        return true;
    else
        return false;
}

LeydenJarAgent::LeydenJarAgent()
	: m_ReqType(LeydenJarReqNone)
    , m_ReqDeviceIndex(-1)
	, m_AckType(LeydenJarAckNone)
	, m_Thread(&LeydenJarAgent::ThreadLoop, this)
{
    std::memset(&m_DeviceInfo, 0, sizeof(m_DeviceInfo));
    std::memset(&m_LogicKeyboardState, 0, sizeof(m_LogicKeyboardState));
    std::memset(&m_PhysicalKeyboardState, 0, sizeof(m_PhysicalKeyboardState));
    std::memset(&m_Levels, 0, sizeof(m_Levels));
}

LeydenJarAgent::~LeydenJarAgent()
{
    SendRequest(LeydenJarReqQuit);

    WaitEndRequest();

    m_Thread.join();
}

void LeydenJarAgent::ThreadLoop()
{
    bool exitThread = false;

    m_Protocol.Initialize();

    do
    {
        std::unique_lock<std::mutex> lk(m_Mutex);
        m_CondVar.wait(lk, [this] { return m_ReqType != LeydenJarReqNone; });

        bool isSuccess = true;
        switch (m_ReqType)
        {
            case LeydenJarReqQuit:
                exitThread = true;
                break;

            case LeydenJarReqEnumerate:
                isSuccess = m_Protocol.EnumerateDevices();
                break;

            case LeydenJarReqConnect:
                isSuccess = m_Protocol.OpenDevice(m_ReqDeviceIndex);
                if (isSuccess == false)
                    break;
                m_DeviceInfo.pHidDeviceInfo = m_Protocol.GetDeviceInfo(m_ReqDeviceIndex);
                isSuccess = m_Protocol.GetProtocolVersion(m_DeviceInfo.protocolVerMajor, m_DeviceInfo.protocolVerMid, m_DeviceInfo.protocolVerMinor);
                if (isSuccess == false)
                    break;
                isSuccess = m_Protocol.GetDetails(m_DeviceInfo.nbLogicalRows, m_DeviceInfo.nbLogicalCols, m_DeviceInfo.nbPhysicalRows, m_DeviceInfo.nbPhysicalCols, m_DeviceInfo.switchTechnology, m_DeviceInfo.nbBins);
                if (isSuccess == false)
                    break;
                isSuccess = m_Protocol.GetMatrixMapping(m_DeviceInfo.matrixToControllerType, m_DeviceInfo.matrixToControllerRows, m_DeviceInfo.matrixToControllerCols, m_DeviceInfo.nbPhysicalRows, m_DeviceInfo.nbPhysicalCols);
                if (isSuccess == false)
                    break;
                
                memset(m_DeviceInfo.controllerToMatrixRows, 255, m_DeviceInfo.nbPhysicalRows);
                for (int row = 0; row < m_DeviceInfo.nbPhysicalRows; row++)
                    for (int i = 0; i < m_DeviceInfo.nbPhysicalRows; i++)
                        if (m_DeviceInfo.matrixToControllerRows[i] == row)
                            m_DeviceInfo.controllerToMatrixRows[row] = i;

                memset(m_DeviceInfo.controllerToMatrixCols, 255, m_DeviceInfo.nbPhysicalCols);
                for (int col = 0; col < m_DeviceInfo.nbPhysicalCols; col++)
                    for (int i = 0; i < m_DeviceInfo.nbPhysicalCols; i++)
                        if (m_DeviceInfo.matrixToControllerCols[i] == col)
                            m_DeviceInfo.controllerToMatrixCols[col] = i;
                
                m_DeviceInfo.vialVersion0 = m_DeviceInfo.vialVersion1 = m_DeviceInfo.vialVersion2 = m_DeviceInfo.vialVersion3 = 0;
                isSuccess = m_Protocol.GetVialInfos(m_DeviceInfo.vialVersion0, m_DeviceInfo.vialVersion1, m_DeviceInfo.vialVersion2, m_DeviceInfo.vialVersion3, m_DeviceInfo.vialUid);
                if (isSuccess == false)
                    break;
                isSuccess = m_Protocol.GetVialKeyboardDefinitionSize(m_DeviceInfo.vialKeyboardDefinitionSize);
                if (isSuccess == false)
                    break;
                isSuccess = m_Protocol.GetVialKeyboardDefinitionData(m_DeviceInfo.vialKeyboardDefinitionSize, m_DeviceInfo.vialKeyboardDefinitionData);
                if (isSuccess == false)
                    break;
                m_DeviceInfo.viaVersionMajor = m_DeviceInfo.viaVersionMinor = 0;
                isSuccess = m_Protocol.GetViaProtocolVersion(m_DeviceInfo.viaVersionMajor, m_DeviceInfo.viaVersionMinor);
                if (isSuccess == false)
                    break;

                memset(m_DeviceInfo.binningMap, 0, sizeof(m_DeviceInfo.binningMap));
                m_DeviceInfo.isKeyboardLeft = true;
                
                if (!m_DeviceInfo.IsProtocolVersionOlder(0,9,1))
                {
                    for (int i = 0; i < m_DeviceInfo.nbBins; i++)
                    {
                        isSuccess = m_Protocol.GetDacThreshold(m_DeviceInfo.dacThreshold[i], i);
                        if (isSuccess == false)
                            break;
                        isSuccess = m_Protocol.GetDacRefLevel(m_DeviceInfo.dacRefLevel[i], i);
                        if (isSuccess == false)
                            break;
                    }

                    for (int i = 0; i < m_DeviceInfo.nbPhysicalCols; i++)
                    {
                        isSuccess = m_Protocol.GetColumnBinMap(i, m_DeviceInfo.binningMap[i]);
                        if (isSuccess == false)
                            break;
                    }

                    if (!m_DeviceInfo.IsProtocolVersionOlder(1, 0, 0))
                    {
                        isSuccess = m_Protocol.GetIsKeyboardLeft(m_DeviceInfo.isKeyboardLeft);
                        if (isSuccess == false)
                            break;
                    }
                }
                else
                {
                    isSuccess = m_Protocol.GetDacThreshold(m_DeviceInfo.dacThreshold[0], 0);
                    if (isSuccess == false)
                        break;
                }
                break;

            case LeydenJarReqEnterBootloader:
                isSuccess = m_Protocol.EnterBootLoader();
                m_Protocol.CloseDevice();
                m_Protocol.FreeEnumeratedDevices();
                break;

            case LeydenJarReqEraseEeprom:
                isSuccess = m_Protocol.EraseEeprom();
                m_Protocol.CloseDevice();
                break;

            case LeydenJarReqDisable:
                isSuccess = m_Protocol.SetKeyboardStatus(false);
                break;

            case LeydenJarReqEnable:
                isSuccess = m_Protocol.SetKeyboardStatus(true);
                break;

            case LeydenJarReqScanLogical:
                isSuccess = m_Protocol.ScanLogicalMatrix();
                if (isSuccess == false)
                    break;
                for (int row = 0; row < m_DeviceInfo.nbLogicalRows; row++)
                {
                    isSuccess = m_Protocol.GetScanLogicalRow(row, m_LogicKeyboardState[row]);
                    if (isSuccess == false)
                        break;
                }
            case LeydenJarReqScanPhysical:
                isSuccess = m_Protocol.ScanPhysicalMatrix();
                if (isSuccess == false)
                    break;
                isSuccess = m_Protocol.GetScanPhysicalVals(m_PhysicalKeyboardState);
                break;

            case LeydenJarReqDetectLevels:
                isSuccess = m_Protocol.DetectLevels();
                if (isSuccess == false)
                    break;
                for (int col = 0; col < m_DeviceInfo.nbPhysicalCols; col++)
                {
                    isSuccess = m_Protocol.GetColumnLevels(col, m_Levels[col]);
                    if (isSuccess == false)
                        break;
                }
        }

        if (isSuccess == true)
            m_AckType.store(LeydenJarAckSuccess, std::memory_order_release);
        else
            m_AckType.store(LeydenJarAckError, std::memory_order_release);
        
        m_ReqType = LeydenJarReqNone;

        lk.unlock();
        m_CondVar.notify_one();
    } 
    while (exitThread == false);

    m_Protocol.Finalize();
}

void LeydenJarAgent::SendRequest(LeydenJarReq reqType)
{
    {
        std::lock_guard<std::mutex> lk(m_Mutex);
        m_AckType.store(LeydenJarAckPending, std::memory_order_release);
        m_ReqType = reqType;
    }
    m_CondVar.notify_one();
}

bool LeydenJarAgent::RequestInProgress()
{
    return (m_AckType.load(std::memory_order_relaxed) == LeydenJarAckPending);
}

bool LeydenJarAgent::WaitEndRequest()
{
    if (m_AckType == LeydenJarAckNone || m_AckType == LeydenJarAckSuccess)
        return true;

    {
        std::unique_lock<std::mutex> lk(m_Mutex);
        m_CondVar.wait(lk, [this] { return m_AckType != LeydenJarAckPending; });
        if (m_AckType == LeydenJarAckSuccess)
            return true;
        else
            return false;
    }
}

void LeydenJarAgent::RequestDeviceEnumeration()
{
    SendRequest(LeydenJarReqEnumerate);
}

void LeydenJarAgent::RequestDeviceConnection(int deviceIndex)
{
    m_ReqDeviceIndex = deviceIndex;
    SendRequest(LeydenJarReqConnect);
}

void LeydenJarAgent::RequestEnterBootloader()
{
    SendRequest(LeydenJarReqEnterBootloader);
}

void LeydenJarAgent::RequestEraseEeprom()
{
    SendRequest(LeydenJarReqEraseEeprom);
}

void LeydenJarAgent::RequestLogicalScan()
{
    SendRequest(LeydenJarReqScanLogical);
}

void LeydenJarAgent::RequestDisable()
{
    SendRequest(LeydenJarReqDisable);
}

void LeydenJarAgent::RequestEnable()
{
    SendRequest(LeydenJarReqEnable);
}

void LeydenJarAgent::RequestPhysicalScan()
{
    SendRequest(LeydenJarReqScanPhysical);
}

void LeydenJarAgent::RequestDetectLevels()
{
    SendRequest(LeydenJarReqDetectLevels);
}

int LeydenJarAgent::GetNbEnumeratedDevices()
{
    return m_Protocol.GetNbEnumeratedDevices();
}

const LeydenJarAgent::LeydenJarDeviceInfo* LeydenJarAgent::GetDeviceInfo()
{
    return &m_DeviceInfo;
}

struct hid_device_info* LeydenJarAgent::GetHidDeviceInfo(int deviceIndex)
{
    return m_Protocol.GetDeviceInfo(deviceIndex);
}

uint32_t LeydenJarAgent::GetLogicalKeyboardState(int row)
{
    return m_LogicKeyboardState[row];
}

void LeydenJarAgent::GetPhysicalKeyboardState(uint8_t* physicalState)
{
    std::memcpy(physicalState, m_PhysicalKeyboardState, m_DeviceInfo.nbPhysicalCols);
}

void LeydenJarAgent::GetColLevels(int col, uint16_t* colLevels)
{
    std::memcpy(colLevels, m_Levels[col], 8 * sizeof(uint16_t));
}

bool LeydenJarAgent::IsDeviceOpened()
{
    return m_Protocol.IsDeviceOpened();
}