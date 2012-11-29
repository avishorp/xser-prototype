#include "stdafx.h"

#include <string>
#include <regex>
#include "xser.h"

#define HWID_STRING L"USB\\VID_" XSER_VID L"&PID_" XSER_PID

typedef struct  {
	int port_number;
	LPCWSTR hid_path;
} comm_assoc_t;

// A regular expression for searching the COMx number
std::tr1::basic_regex<WCHAR> regex_com(L"COM([0-9]+)");

bool process_child(HDEVINFO world_device_info_set, LPCWSTR child_id, comm_assoc_t* comm_assoc)
{
	// Get the device according to its instance ID
	BOOLEAN r;
	SP_DEVINFO_DATA device_info_data;
	memset(&device_info_data, 0x0, sizeof(device_info_data));
	device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);
	r = SetupDiOpenDeviceInfo(world_device_info_set, child_id, NULL, 0, &device_info_data);

	if (!r) {
		printf("    Could not find the child device\n");
		return false;
	}

	// Determine the device class
	WCHAR class_name[300];
	DWORD class_name_type;
	GUID g;
	r = SetupDiGetDeviceProperty(world_device_info_set, &device_info_data, &DEVPKEY_Device_Class, &class_name_type, (PBYTE)class_name, 300, NULL, 0);

	if (!r) {
		printf("    Could not obtain the device class\n");
		return false;
	}

	wprintf(L"    Device class: %s\n", class_name);

	if (wcscmp(class_name, L"HIDClass") == 0) {
		// Process the HID side

		// The HID device is actually the child of this device
		DWORD children_type;
		WCHAR children[1000];
		r = SetupDiGetDeviceProperty(world_device_info_set, &device_info_data, &DEVPKEY_Device_Children, &children_type, (PBYTE)children, 1000, NULL, 0);
		if (!r) {
			printf("    Could not get the child device instance\n");
			return false;
		}

		// Open the child device.
		// I don't know why I have to create a new device information set to enumerate its HID
		// interface, but that's the only way it works.
		HDEVINFO hid_device_info_set = SetupDiGetClassDevs(&GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		r = SetupDiOpenDeviceInfo(hid_device_info_set, children, NULL, 0, &device_info_data);
		if (!r) {
			printf("    Could not open the child HID device\n");
			return false;
		}

		// Enumarate through all device interfaces
		SP_DEVICE_INTERFACE_DATA device_interface_data;
		device_interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

		for(int k = 0;; k++) {
			// Get the interface data
			r = SetupDiEnumDeviceInterfaces(hid_device_info_set, &device_info_data, &GUID_DEVINTERFACE_HID, k, &device_interface_data);
		
			if (!r)
				// No more interfaces
				break;

			// Get the device interface detail
			SP_DEVICE_INTERFACE_DETAIL_DATA_W* detail;
			DWORD detail_size = 0;

			// First call to get the required size
			r = SetupDiGetDeviceInterfaceDetail(hid_device_info_set, &device_interface_data, NULL, 0, &detail_size, &device_info_data);

			if (detail_size > 0) {
				// Allocate data for the detail
				detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_W*)malloc(detail_size);
				detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

				// Call SetupDiGetDeviceInterfaceDetail again, to get the actual data
				r = SetupDiGetDeviceInterfaceDetail(hid_device_info_set, &device_interface_data, detail, detail_size, NULL, &device_info_data);

				if (!r) {
					free(detail);
					continue;
				}

				// Copy the device path into the comm_assoc structure
				// It is the caller's responsibility to delete the allocation
				// when done
				comm_assoc->hid_path = detail->DevicePath;
				wprintf(L"    HID Path is: %s\n", detail->DevicePath);
			}
		}

	}
	else if (wcscmp(class_name, L"Ports") == 0) {
		// Process the COM port side
		WCHAR desc[300];
		DWORD desc_type;
		r = SetupDiGetDeviceProperty(world_device_info_set, &device_info_data, &DEVPKEY_Device_FriendlyName, &desc_type, (PBYTE)desc, 300, NULL, 0);
		if (!r) {
			printf("    Could not obtain port description\n");
			return false;
		}
		wprintf(L"    Port description: %s\n", desc);

		std::tr1::wcmatch mr;
		bool match_result = std::tr1::regex_search<WCHAR>(desc, mr, regex_com);
		if (!match_result)
			return false;

		comm_assoc->port_number = _wtoi(mr[1].str().c_str());
	}
	else {
		printf("    Unknown child type\n");
		return false;
	}

	return true;
}

bool set_port_display(const comm_assoc_t* comm_assoc)
{

	// Open the device
	HANDLE handle = CreateFile(comm_assoc->hid_path, (GENERIC_WRITE|GENERIC_READ), FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (handle == INVALID_HANDLE_VALUE) {
		printf("  CreateFile failed\n");
		return false;
	}

	// Prepare the message - 65 bytes in which the first
	// is the report number (0) and the second is the value to be displayed.
	// All the other bytes are zero.
	char message[65];
	memset(message, 0, 65);
	message[1] = (char)(comm_assoc->port_number);
	DWORD num_written;
	BOOLEAN r = WriteFile(handle, message, 65, &num_written, NULL);

	if (!r) {
		printf("  Write failed\n");
		return false;
	}

	// Cleanup and wer'e done
	CloseHandle(handle);

	return true;
}

// Scan for all XSer adapters and update their COM port
// display
int update_all_adapters()
{
	// Prepare to scan all USB devices connected
	// to the computer
	HDEVINFO device_info_set = INVALID_HANDLE_VALUE;
	device_info_set = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if (device_info_set == INVALID_HANDLE_VALUE) {
		printf("Could not get device information set for all USB devices\n");
		return -1;
	}

	// Prepare an info set of all the device. It will be used later
	// when device children are processed
	HDEVINFO world_device_info_set = INVALID_HANDLE_VALUE;
	world_device_info_set = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES|DIGCF_PRESENT);
	//world_device_info_set = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if (world_device_info_set == INVALID_HANDLE_VALUE) {
		printf("Could not get all device information set\n");
		return -1;
	}

	// Do the actual scan
	SP_DEVINFO_DATA device_info_data;
	memset(&device_info_data, 0x0, sizeof(device_info_data));
	device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);
	BOOLEAN r;
	int working_device_count = 0;

	int hwid_length = wcslen(HWID_STRING);

	for(int device_index = 0; ; device_index++) {

		// Get the device info data of the next device
		r = SetupDiEnumDeviceInfo(device_info_set, device_index, &device_info_data);
		if (!r)
			// End of lins
			break;


		// Obtain the HardrareIDs of the device
		wchar_t hardware_id[300];
		DWORD hardware_id_type;
		r = SetupDiGetDeviceProperty(device_info_set, &device_info_data, &DEVPKEY_Device_HardwareIds, &hardware_id_type, (PBYTE)hardware_id, 300, NULL, 0);
		if (!r) {
			printf("Device %d - Cannot obtain Hardware ID", device_index);
			continue;
		}

		wprintf(L"Device %d (HwID=%s) ... ", device_index, hardware_id);

		// Compare it against the desired hardware ID
		if (_wcsnicmp(HWID_STRING, hardware_id, hwid_length) == 0) {
			// Hardware ID matche
			wprintf(L"Match\n");

			// Initialize a comm association structure
			comm_assoc_t comm_assoc;
			comm_assoc.port_number = -1;
			comm_assoc.hid_path = NULL;

			// Scan through the children of the device
			DWORD children_type;
			WCHAR children[1000];
			r = SetupDiGetDeviceProperty(device_info_set, &device_info_data, &DEVPKEY_Device_Children, &children_type, (PBYTE)children, 1000, NULL, 0);
			if (!r) {
				printf("  Cannot obtain children\n");
				continue;
			}

			int w;
			LPCWSTR t = (LPCWSTR)children;
			while(1) {
				w = wcslen(t);
				if (w == 0)
					break;

				wprintf(L"  Child: %s\n", t);
				process_child(world_device_info_set, t, &comm_assoc);
				t += w;
				t++;
			}

			// Check if we have all we need
			if ((comm_assoc.port_number == -1) || (comm_assoc.hid_path == NULL)) {
				printf("  Faild to obtain one or more of the required device properties\n");
				continue;
			}

			// Time do the actual work
			set_port_display((const comm_assoc_t*)&comm_assoc);

		}
		else
			// Hardware ID mismatch
			wprintf(L"No match\n");
	}
}
