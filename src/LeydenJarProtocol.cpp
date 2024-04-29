#include <iostream>
#include <string>
#include "LeydenJarProtocol.h" 

const uint16_t leyden_jar_protocol_magic = 0x21C0;

const uint8_t id_get_protocol_version = 1;
const uint8_t id_get_keyboard_value = 2;
const uint8_t id_set_keyboard_value = 3;
const uint8_t id_vial_prefix = 0xFE;

enum leyden_jar_keyboard_value_id {
	id_leyden_jar_offset = 0x80, //Sufficiently high value to be safe to use
	id_leyden_jar_protocol_version,
	id_leyden_jar_details,
	id_leyden_jar_enable_keyboard,
	id_leyden_jar_detect_levels,
	id_leyden_jar_dac_threshold,
	id_leyden_jar_col_levels,
	id_leyden_jar_scan_logical_matrix,
	id_leyden_jar_scan_physical_matrix,
	id_leyden_jar_enter_bootloader,
	id_leyden_jar_reboot,
	id_leyden_jar_erase_eeprom,
	id_leyden_jar_logical_matrix_row,
	id_leyden_jar_physical_matrix_vals,
	id_leyden_jar_matrix_mapping,
	id_leyden_jar_dac_ref_level,
	id_leyden_jar_bin_map
};

enum vial_keyboard_value_id {
	vial_get_keyboard_id = 0,
	vial_get_size,
	vial_get_def
};

LeydenJarProtocol::LeydenJarProtocol()
	: m_pEnumeratedDeviceInfo(nullptr)
	, m_pHidDevice(nullptr)
{

}

bool LeydenJarProtocol::Initialize()
{
	if (hid_init())
	{
		printf("ERROR: Cannot initialize HID library.");
		return false;
	}

	return true;
}

bool LeydenJarProtocol::Finalize()
{
	CloseDevice();
	FreeEnumeratedDevices();

	if (hid_exit())
	{
		printf("ERROR: Cannot finalize HID library.");
		return false;
	}

	return true;
}

bool LeydenJarProtocol::EnumerateDevices()
{
	CloseDevice();
	FreeEnumeratedDevices();

	m_pEnumeratedDeviceInfo = hid_enumerate(0x1209, 0x4704);
	if (m_pEnumeratedDeviceInfo == nullptr)
	{
		printf("INFO: Cannot enumerate HID devices or no Leyden Jar devices connected.");
		return false;
	}

	return true;
}

void LeydenJarProtocol::FreeEnumeratedDevices()
{
	if (m_pEnumeratedDeviceInfo != nullptr)
	{
		hid_free_enumeration(m_pEnumeratedDeviceInfo);
		m_pEnumeratedDeviceInfo = nullptr;
	}
}

int LeydenJarProtocol::GetNbEnumeratedDevices()
{
	struct hid_device_info* parseDev = m_pEnumeratedDeviceInfo;
	int nbDevices = 0;
	
	while (parseDev != nullptr)
	{
		if (parseDev->usage == 0x61 && parseDev->usage_page == 0xFF60)
		{
			nbDevices++;
		}
		parseDev = parseDev->next;
	}


	return nbDevices;
}

struct hid_device_info* LeydenJarProtocol::GetDeviceInfo(int deviceIndex)
{
	struct hid_device_info* returnedDev = nullptr;
	struct hid_device_info* parseDev = m_pEnumeratedDeviceInfo;
	int currentDeviceIndex = 0;
	while (parseDev) {
		if (parseDev->usage == 0x61 && parseDev->usage_page == 0xFF60)
		{
			if (currentDeviceIndex == deviceIndex)
			{
				returnedDev = parseDev;
				break;
			}
			currentDeviceIndex++;
		}
		parseDev = parseDev->next;
	}

	return returnedDev;
}

bool LeydenJarProtocol::OpenDevice(int deviceIndex)
{
	CloseDevice();

	struct hid_device_info* selectedHidDeviceInfo = GetDeviceInfo(deviceIndex);
	if (selectedHidDeviceInfo == nullptr)
	{
		printf("ERROR: HID device index not known.");
		return false;
	}

	m_pHidDevice = hid_open_path(selectedHidDeviceInfo->path);
	if (m_pHidDevice == nullptr)
		return false;

	return true;
}

bool LeydenJarProtocol::CloseDevice()
{
	if (m_pHidDevice == nullptr)
		return false;

	hid_close(m_pHidDevice);
	m_pHidDevice = nullptr;

	return true;
}

bool LeydenJarProtocol::IsDeviceOpened()
{
	if (m_pHidDevice == nullptr)
		return false;

	return true;
}

void LeydenJarProtocol::PrintDevice(struct hid_device_info* curDev, int deviceIndex)
{
	printf("Device %d\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls", deviceIndex, curDev->vendor_id, curDev->product_id, curDev->path, curDev->serial_number);
	printf("\n");
	printf("  Manufacturer: %ls\n", curDev->manufacturer_string);
	printf("  Product:      %ls\n", curDev->product_string);
	printf("  Release:      %hx\n", curDev->release_number);
	printf("  Interface:    %d\n", curDev->interface_number);
	printf("  Usage (page): 0x%hx (0x%hx)\n", curDev->usage, curDev->usage_page);
	printf("  Bus type: %d\n", curDev->bus_type);
	printf("\n");
}

void LeydenJarProtocol::PrintDeviceList()
{
	struct hid_device_info* parseDev = m_pEnumeratedDeviceInfo;
	int nbDevices = 0;
	while (parseDev) {
		if (parseDev->usage == 0x61 && parseDev->usage_page == 0xFF60)
			nbDevices++;
		parseDev = parseDev->next;
	}

	printf("%d devices found:\n\n", nbDevices);

	int deviceIndex = 0;
	parseDev = m_pEnumeratedDeviceInfo;
	while (parseDev) {
		if (parseDev->usage == 0x61 && parseDev->usage_page == 0xFF60)
			PrintDevice(parseDev, deviceIndex++);
		parseDev = parseDev->next;
	}
}

void LeydenJarProtocol::FillLeydenJarSendPacketHeader(uint8_t getOrSet, uint8_t command)
{
	memset(m_RawHidSendPacket, 0, sizeof(m_RawHidSendPacket));

	m_RawHidSendPacket[1] = getOrSet;
	m_RawHidSendPacket[2] = command;
	uint16_t* magic_ptr = (uint16_t*)(m_RawHidSendPacket + 3);
	*magic_ptr = leyden_jar_protocol_magic;

	m_pSendPayloadPtr = m_RawHidSendPacket + 5;
	m_pRcvPayloadPtr = m_RawHidRcvPacket + 4;
}

void LeydenJarProtocol::FillViaSendPacketHeader(uint8_t viaCommand, uint8_t command)
{
	memset(m_RawHidSendPacket, 0, sizeof(m_RawHidSendPacket));

	m_RawHidSendPacket[1] = viaCommand;
	m_RawHidSendPacket[2] = command;

	m_pSendPayloadPtr = m_RawHidSendPacket + 3;
	m_pRcvPayloadPtr = m_RawHidRcvPacket;
}

bool LeydenJarProtocol::HidSendCommand(bool hidReceive, bool checkReturn)
{
	int ret = hid_write(m_pHidDevice, m_RawHidSendPacket, sizeof(m_RawHidSendPacket));
	if (ret != sizeof(m_RawHidSendPacket))
	{
		printf("ERROR: hid_write call.");
		return false;
	}
	
	if (hidReceive == true)
	{
		ret = hid_read(m_pHidDevice, m_RawHidRcvPacket, sizeof(m_RawHidRcvPacket));
		if (ret != sizeof(m_RawHidRcvPacket))
		{
			printf("ERROR: hid_read call.");
			return false;
		}
		if (checkReturn == true && *m_RawHidRcvPacket == 0xFF)
		{
			printf("ERROR: Command not recognised by the keyboard.");
			return false;
		}
	}

	return true;
}

bool LeydenJarProtocol::GetDacThreshold(uint16_t& dacThreshold, int binNumber)
{
	FillLeydenJarSendPacketHeader(id_get_keyboard_value, id_leyden_jar_dac_threshold);
	*(uint16_t*)m_pSendPayloadPtr = (uint16_t)binNumber;
	
	if (HidSendCommand() == false)
		return false;

	dacThreshold = *((uint16_t*)(m_pRcvPayloadPtr + 2));

	return true;
}

bool LeydenJarProtocol::GetDacRefLevel(uint16_t& dacRefLevel, int binNumber)
{
	FillLeydenJarSendPacketHeader(id_get_keyboard_value, id_leyden_jar_dac_ref_level);
	*(uint16_t*)m_pSendPayloadPtr = (uint16_t)binNumber;

	if (HidSendCommand() == false)
		return false;

	dacRefLevel = *((uint16_t*)(m_pRcvPayloadPtr + 2));

	return true;
}

bool LeydenJarProtocol::SetDac(int dacThreshold)
{
	FillLeydenJarSendPacketHeader(id_set_keyboard_value, id_leyden_jar_dac_threshold);

	*(uint16_t*)m_pSendPayloadPtr = (uint16_t)dacThreshold;

	if (HidSendCommand() == false)
		return false;

	return true;
}

bool LeydenJarProtocol::GetColumnBinMap(int columnIndex, uint8_t* binMap)
{
	FillLeydenJarSendPacketHeader(id_get_keyboard_value, id_leyden_jar_bin_map);

	*(uint16_t*)m_pSendPayloadPtr = (uint16_t)columnIndex;

	if (HidSendCommand() == false)
		return false;

	uint8_t* columnBinMap = m_pRcvPayloadPtr + 2;
	for (int i = 0; i < 8; i++)
		binMap[i] = columnBinMap[i];

	return true;
}

bool LeydenJarProtocol::GetColumnLevels(int columnIndex, uint16_t* columnLevels)
{
	FillLeydenJarSendPacketHeader(id_get_keyboard_value, id_leyden_jar_col_levels);

	*(uint16_t*)m_pSendPayloadPtr = (uint16_t)columnIndex;

	if (HidSendCommand() == false)
		return false;

	uint16_t* levels = (uint16_t*)(m_pRcvPayloadPtr + 2);
	for (int i = 0; i < 8; i++)
		columnLevels[i] = levels[i];

	return true;
}

bool LeydenJarProtocol::SetKeyboardStatus(bool enable)
{
	FillLeydenJarSendPacketHeader(id_set_keyboard_value, id_leyden_jar_enable_keyboard);

	*m_pSendPayloadPtr = (enable == true) ? 1 : 0;

	if (HidSendCommand() == false)
		return false;

	return true;
}

bool LeydenJarProtocol::GetKeyboardStatus(bool& enable)
{
	FillLeydenJarSendPacketHeader(id_get_keyboard_value, id_leyden_jar_enable_keyboard);

	if (HidSendCommand() == false)
		return false;

	enable = (*m_pRcvPayloadPtr == 0) ? false : true;

	return true;
}

bool LeydenJarProtocol::GenericCommandNoPayload(uint8_t getOrSet, uint8_t command, bool hidReceive)
{
	FillLeydenJarSendPacketHeader(getOrSet, command);

	if (HidSendCommand(hidReceive) == false)
		return false;

	return true;
}

bool LeydenJarProtocol::Reboot()
{
	return GenericCommandNoPayload(id_set_keyboard_value, id_leyden_jar_reboot, false);
}

bool LeydenJarProtocol::EnterBootLoader()
{
	return GenericCommandNoPayload(id_set_keyboard_value, id_leyden_jar_enter_bootloader, false);
}

bool LeydenJarProtocol::EraseEeprom()
{
	return GenericCommandNoPayload(id_set_keyboard_value, id_leyden_jar_erase_eeprom, false);
}

bool LeydenJarProtocol::DetectLevels()
{
	return GenericCommandNoPayload(id_set_keyboard_value, id_leyden_jar_detect_levels);
}

bool LeydenJarProtocol::ScanLogicalMatrix()
{
	return GenericCommandNoPayload(id_set_keyboard_value, id_leyden_jar_scan_logical_matrix);
}

bool LeydenJarProtocol::ScanPhysicalMatrix()
{
	return GenericCommandNoPayload(id_set_keyboard_value, id_leyden_jar_scan_physical_matrix);
}

bool LeydenJarProtocol::GetScanLogicalRow(int rowIndex, uint32_t& rowVal)
{
	FillLeydenJarSendPacketHeader(id_get_keyboard_value, id_leyden_jar_logical_matrix_row);

	*(uint16_t*)m_pSendPayloadPtr = (uint16_t)rowIndex;

	if (HidSendCommand() == false)
		return false;

	rowVal = *(uint32_t*)(m_pRcvPayloadPtr + 4);

	return true;
}

bool LeydenJarProtocol::GetScanPhysicalVals(uint8_t* rawVals)
{
	FillLeydenJarSendPacketHeader(id_get_keyboard_value, id_leyden_jar_physical_matrix_vals);

	if (HidSendCommand() == false)
		return false;

	memcpy(rawVals, m_pRcvPayloadPtr, 18);

	return true;
}

bool LeydenJarProtocol::GetProtocolVersion(uint8_t& major, uint8_t& mid, uint16_t& minor)
{
	FillLeydenJarSendPacketHeader(id_get_keyboard_value, id_leyden_jar_protocol_version);

	if (HidSendCommand() == false)
		return false;

	major = *m_pRcvPayloadPtr;
	mid = *(m_pRcvPayloadPtr + 1);
	minor = *(uint16_t*)(m_pRcvPayloadPtr + 2);

	return true;
}

bool LeydenJarProtocol::GetDetails(uint8_t& nbLogicalRows, uint8_t& nbLogicalCols, uint8_t& nbPhysicalRows, uint8_t& nbPhysicalCols, uint8_t& switchTechnology, uint8_t& nbBins)
{
	FillLeydenJarSendPacketHeader(id_get_keyboard_value, id_leyden_jar_details);

	if (HidSendCommand() == false)
		return false;

	nbLogicalRows = *m_pRcvPayloadPtr;
	nbLogicalCols = *(m_pRcvPayloadPtr + 1);
	nbPhysicalRows = *(m_pRcvPayloadPtr + 2);
	nbPhysicalCols = *(m_pRcvPayloadPtr + 3);
	switchTechnology = *(m_pRcvPayloadPtr + 4);
	nbBins = *(m_pRcvPayloadPtr + 5);

	return true;
}

bool LeydenJarProtocol::GetMatrixMapping(uint8_t& matrixToControllerType, uint8_t* matrixToControllerRows, uint8_t* matrixToControllerCols, uint8_t nbPhysicalRows, uint8_t nbPhysicalCols)
{
	FillLeydenJarSendPacketHeader(id_get_keyboard_value, id_leyden_jar_matrix_mapping);

	if (HidSendCommand() == false)
		return false;

	matrixToControllerType = *(m_pRcvPayloadPtr + 0);
	memcpy(matrixToControllerCols, m_pRcvPayloadPtr + 1, nbPhysicalCols);
	memcpy(matrixToControllerRows, m_pRcvPayloadPtr + 1 + nbPhysicalCols, nbPhysicalRows);

	return true;
}

bool LeydenJarProtocol::GetViaProtocolVersion(uint8_t& major, uint8_t& minor)
{
	FillViaSendPacketHeader(id_get_protocol_version, 0);

	if (HidSendCommand() == false)
		return false;

	major = *m_pRcvPayloadPtr;
	minor = *(m_pRcvPayloadPtr + 1);

	return true;
}

bool LeydenJarProtocol::GetVialInfos(uint8_t& version0, uint8_t& version1, uint8_t& version2, uint8_t& version3, uint8_t* pUid)
{
	FillViaSendPacketHeader(id_vial_prefix, vial_get_keyboard_id);

	if (HidSendCommand() == false)
		return false;

	version0 = *m_pRcvPayloadPtr;
	version1 = *(m_pRcvPayloadPtr + 1);
	version2 = *(m_pRcvPayloadPtr + 2);
	version3 = *(m_pRcvPayloadPtr + 3);
	memcpy(pUid, m_pRcvPayloadPtr + 4, 8);

	return true;
}

bool LeydenJarProtocol::GetVialKeyboardDefinitionSize(uint32_t& definitionSize)
{
	FillViaSendPacketHeader(id_vial_prefix, vial_get_size);

	if (HidSendCommand() == false)
		return false;

	definitionSize = *((uint32_t*)m_pRcvPayloadPtr);

	return true;
}

bool LeydenJarProtocol::GetVialKeyboardDefinitionDataBlock(uint16_t blockNumber, uint8_t* pBlockData)
{
	FillViaSendPacketHeader(id_vial_prefix, vial_get_def);
	*(uint16_t*)m_pSendPayloadPtr = blockNumber;

	if (HidSendCommand(true, false) == false)
		return false;

	memcpy(pBlockData + blockNumber * 32, m_pRcvPayloadPtr, 32);

	return true;
}

bool LeydenJarProtocol::GetVialKeyboardDefinitionData(uint32_t definitionSize, uint8_t* pKeyboardDefinitionData)
{
	uint32_t nbBlocks = definitionSize / 32;
	if (definitionSize % 32)
		nbBlocks++;

	for (uint16_t blockNumber = 0; blockNumber < nbBlocks; blockNumber++)
	{
		if (GetVialKeyboardDefinitionDataBlock(blockNumber, pKeyboardDefinitionData) == false)
			return false;
	}

	return true;
}