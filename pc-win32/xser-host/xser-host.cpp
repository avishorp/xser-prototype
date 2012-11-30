// xser-host.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "xser.h"

#include <Dbt.h>

HANDLE update_event;

// Window message handler
LRESULT WINAPI WindowProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	if ((uiMsg == WM_DEVICECHANGE) && (wParam == DBT_DEVICEARRIVAL)) {
		printf("Device change detected\n");
		SetEvent(update_event);
	}
	return DefWindowProc(hWnd, uiMsg, wParam, lParam);
}

// This function is run in a separate thread, waits for a notification
// and updates the devices whenever one is received
DWORD WINAPI update_thread(LPVOID lpParameter)
{
	while(1) {
		WaitForSingleObject(update_event, INFINITE);
		update_all_adapters();
		ResetEvent(update_event);
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	// Create a hidden window, so we can get Windows messages
	LPTSTR class_name = L"xser_host_class";
    WNDCLASS xh_class;

    xh_class.style = CS_VREDRAW;
    xh_class.lpfnWndProc = &WindowProc;
    xh_class.cbClsExtra = 0;
    xh_class.cbWndExtra = 0;
	xh_class.hInstance = GetModuleHandle(NULL);
    xh_class.hIcon = NULL;
    xh_class.hCursor = NULL;
    xh_class.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    xh_class.lpszMenuName = NULL;
    xh_class.lpszClassName = class_name;

    if (!RegisterClass(&xh_class)) {
		printf("Failed registering window class\n");
		return -1;
	}

    HWND wnd;
    MSG message;
    UINT msgstatus;

    wnd = CreateWindow(class_name, NULL, WS_MINIMIZE, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, NULL, NULL);

	DWORD err = GetLastError();

    if (wnd == NULL) {
		printf("Failed creating main window\n");
		return -1;
	}

	// Create a broadcast message filter
	DEV_BROADCAST_DEVICEINTERFACE filter;
	ZeroMemory(&filter, sizeof(DEV_BROADCAST_DEVICEINTERFACE));
	filter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE); 
	filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE; 
	filter.dbcc_classguid = GUID_DEVINTERFACE_HID; 

	HDEVNOTIFY hdev_notify = RegisterDeviceNotification(wnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
	if (hdev_notify == NULL) {
		printf("Failed installing notification filter\n");
	}

    /*ShowWindow(lolwindow, nCmdShow);
    UpdateWindow(lolwindow);*/

	// Create a notification event
	update_event = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Create a thread for the device updater
	CreateThread(NULL, 0, &update_thread, NULL, 0, NULL);

    do {
        msgstatus = GetMessage(&message, wnd, 0, 0);
        if(!msgstatus) 
			break;

        if (msgstatus == - 1) {
			printf("GetMessage failed\n");
			return -1;
		}

        TranslateMessage(&message);
        DispatchMessage(&message);
    } while(1);



	update_all_adapters();
}

