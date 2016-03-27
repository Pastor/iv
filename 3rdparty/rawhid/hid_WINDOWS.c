/* Simple Raw HID functions for Windows - for use with Teensy RawHID example
 * http://www.pjrc.com/teensy/rawhid.html
 * Copyright (c) 2009 PJRC.COM, LLC
 *
 *  rawhid_open - open 1 or more devices
 *  rawhid_recv - receive a packet
 *  rawhid_send - send a packet
 *  rawhid_close - close a device
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above description, website URL and copyright notice and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Version 1.0: Initial Release
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>
#include <setupapi.h>

#include "hid.h"

	typedef struct _HIDD_ATTRIBUTES{
		ULONG Size;
		USHORT VendorID;
		USHORT ProductID;
		USHORT VersionNumber;
	} HIDD_ATTRIBUTES, *PHIDD_ATTRIBUTES;

	typedef USHORT USAGE;
	typedef struct _HIDP_CAPS {
		USAGE Usage;
		USAGE UsagePage;
		USHORT InputReportByteLength;
		USHORT OutputReportByteLength;
		USHORT FeatureReportByteLength;
		USHORT Reserved[17];
		USHORT fields_not_used_by_hidapi[10];
	} HIDP_CAPS, *PHIDP_CAPS;
	typedef char* HIDP_PREPARSED_DATA;
	typedef HIDP_PREPARSED_DATA * PHIDP_PREPARSED_DATA;
        #define HIDP_STATUS_SUCCESS 0x0

	typedef BOOLEAN (__stdcall *HidD_GetAttributes_)(HANDLE device, PHIDD_ATTRIBUTES attrib);
	typedef BOOLEAN (__stdcall *HidD_GetSerialNumberString_)(HANDLE device, PVOID buffer, ULONG buffer_len);
	typedef BOOLEAN (__stdcall *HidD_GetManufacturerString_)(HANDLE handle, PVOID buffer, ULONG buffer_len);
	typedef BOOLEAN (__stdcall *HidD_GetProductString_)(HANDLE handle, PVOID buffer, ULONG buffer_len);
	typedef BOOLEAN (__stdcall *HidD_SetFeature_)(HANDLE handle, PVOID data, ULONG length);
	typedef BOOLEAN (__stdcall *HidD_GetFeature_)(HANDLE handle, PVOID data, ULONG length);
	typedef BOOLEAN (__stdcall *HidD_GetIndexedString_)(HANDLE handle, ULONG string_index, PVOID buffer, ULONG buffer_len);
	typedef BOOLEAN (__stdcall *HidD_GetPreparsedData_)(HANDLE handle, HIDP_PREPARSED_DATA **preparsed_data);
	typedef BOOLEAN (__stdcall *HidD_FreePreparsedData_)(HIDP_PREPARSED_DATA *preparsed_data);
	typedef BOOLEAN (__stdcall *HidP_GetCaps_)(HIDP_PREPARSED_DATA *preparsed_data, HIDP_CAPS *caps);
        typedef VOID    (__stdcall *HidD_GetHidGuid_)(LPGUID HidGuid);

	static HidD_GetAttributes_ HidD_GetAttributes;
	static HidD_GetSerialNumberString_ HidD_GetSerialNumberString;
	static HidD_GetManufacturerString_ HidD_GetManufacturerString;
	static HidD_GetProductString_ HidD_GetProductString;
	static HidD_SetFeature_ HidD_SetFeature;
	static HidD_GetFeature_ HidD_GetFeature;
	static HidD_GetIndexedString_ HidD_GetIndexedString;
	static HidD_GetPreparsedData_ HidD_GetPreparsedData;
	static HidD_FreePreparsedData_ HidD_FreePreparsedData;
	static HidP_GetCaps_ HidP_GetCaps;
        static HidD_GetHidGuid_ HidD_GetHidGuid;

// a list of all opened HID devices, so the caller can
// simply refer to them by number
typedef struct hid_struct hid_t;
static hid_t *first_hid = NULL;
static hid_t *last_hid = NULL;
struct hid_struct {
    WCHAR  product[512];
	HANDLE handle;
	int open;
	struct hid_struct *prev;
	struct hid_struct *next;
};
static HANDLE rx_event=NULL;
static HANDLE tx_event=NULL;
static CRITICAL_SECTION rx_mutex;
static CRITICAL_SECTION tx_mutex;


// private functions, not intended to be used from outside this file
static void add_hid(hid_t *h);
static hid_t * get_hid(int num);
static void free_all_hid(void);
static void hid_close(hid_t *hid);
void print_win32_err(void);


static int
hid_init()
{
	HANDLE lib_handle = LoadLibraryA("hid.dll");
	if (lib_handle) {
#define RESOLVE(x) x = (x##_)GetProcAddress(lib_handle, #x); if (!x) return -1;
		RESOLVE(HidD_GetAttributes);
		RESOLVE(HidD_GetSerialNumberString);
		RESOLVE(HidD_GetManufacturerString);
		RESOLVE(HidD_GetProductString);
		RESOLVE(HidD_SetFeature);
		RESOLVE(HidD_GetFeature);
		RESOLVE(HidD_GetHidGuid);
                RESOLVE(HidD_GetIndexedString);
		RESOLVE(HidD_GetPreparsedData);
		RESOLVE(HidD_FreePreparsedData);
		RESOLVE(HidP_GetCaps);
	}
    return 0;
}

//  rawhid_recv - receive a packet
//    Inputs:
//	num = device to receive from (zero based)
//	buf = buffer to receive packet
//	len = buffer's size
//	timeout = time to wait, in milliseconds
//    Output:
//	number of bytes received, or -1 on error
//
int rawhid_recv(int num, void *buf, int len, int timeout)
{
	hid_t *hid;
	unsigned char tmpbuf[516];
	OVERLAPPED ov;
	DWORD n, r;

	if (sizeof(tmpbuf) < len + 1) return -1;
	hid = get_hid(num);
	if (!hid || !hid->open) return -1;
	EnterCriticalSection(&rx_mutex);
	ResetEvent(&rx_event);
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = rx_event;
	if (!ReadFile(hid->handle, tmpbuf, len + 1, NULL, &ov)) {
		if (GetLastError() != ERROR_IO_PENDING) goto return_error;
		r = WaitForSingleObject(rx_event, timeout);
		if (r == WAIT_TIMEOUT) goto return_timeout;
		if (r != WAIT_OBJECT_0) goto return_error;
	}
	if (!GetOverlappedResult(hid->handle, &ov, &n, FALSE)) goto return_error;
	LeaveCriticalSection(&rx_mutex);
	if (n <= 0) return -1;
	n--;
	if ((int)n > len) n = len;
	memcpy(buf, tmpbuf + 1, n);
	return n;
return_timeout:
	CancelIo(hid->handle);
	LeaveCriticalSection(&rx_mutex);
	return 0;
return_error:
	print_win32_err();
	LeaveCriticalSection(&rx_mutex);
	return -1;
}

//  rawhid_send - send a packet
//    Inputs:
//	num = device to transmit to (zero based)
//	buf = buffer containing packet to send
//	len = number of bytes to transmit
//	timeout = time to wait, in milliseconds
//    Output:
//	number of bytes sent, or -1 on error
//
int rawhid_send(int num, void *buf, int len, int timeout)
{
	hid_t *hid;
	unsigned char tmpbuf[516];
	OVERLAPPED ov;
	DWORD n, r;
    unsigned char *p;
    int            plen;

	if (sizeof(tmpbuf) < len + 1) return -1;
	hid = get_hid(num);
	if (!hid || !hid->open) return -1;
	EnterCriticalSection(&tx_mutex);
	ResetEvent(&tx_event);
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = tx_event;
    p = buf;
    plen = len;
next_b:
	if (!WriteFile(hid->handle, p, plen, NULL, &ov)) {
        if (GetLastError() != ERROR_IO_PENDING) {
            if (p != tmpbuf) {
                tmpbuf[0] = 0;
                memcpy(tmpbuf + 1, buf, len);
                p = tmpbuf;
                plen = len + 1;
                goto next_b;
            }
            goto return_error;
        }
		r = WaitForSingleObject(tx_event, timeout);
		if (r == WAIT_TIMEOUT) goto return_timeout;
		if (r != WAIT_OBJECT_0) goto return_error;
	}
	if (!GetOverlappedResult(hid->handle, &ov, &n, FALSE)) goto return_error;
	LeaveCriticalSection(&tx_mutex);
	if (n <= 0) return -1;
	return n - 1;
return_timeout:
	CancelIo(hid->handle);
	LeaveCriticalSection(&tx_mutex);
	return 0;
return_error:
	print_win32_err();
	LeaveCriticalSection(&tx_mutex);
	return -1;
}

wchar_t *
rawhid_product(int num)
{
    hid_t *hid;


    hid = get_hid(num);
    if (!hid || !hid->open) 
        return 0;
    return hid->product;
}

//  rawhid_open - open 1 or more devices
//
//    Inputs:
//	max = maximum number of devices to open
//	vid = Vendor ID, or -1 if any
//	pid = Product ID, or -1 if any
//	usage_page = top level usage page, or -1 if any
//	usage = top level usage number, or -1 if any
//    Output:
//	actual number of devices opened
//
int rawhid_open(int max, int vid, int pid, int usage_page, int usage)
{
        GUID guid;
        HDEVINFO info;
        DWORD index=0, reqd_size;
        SP_DEVICE_INTERFACE_DATA iface;
        SP_DEVICE_INTERFACE_DETAIL_DATA *details;
        HIDD_ATTRIBUTES attrib;
        PHIDP_PREPARSED_DATA hid_data;
        HIDP_CAPS capabilities;
        HANDLE h;
        BOOL ret;
	hid_t *hid;
	int count=0;

	hid_init();
	if (first_hid) free_all_hid();
	if (max < 1) return 0;
	if (!rx_event) {
		rx_event = CreateEvent(NULL, TRUE, TRUE, NULL);
		tx_event = CreateEvent(NULL, TRUE, TRUE, NULL);
		InitializeCriticalSection(&rx_mutex);
		InitializeCriticalSection(&tx_mutex);
	}
	HidD_GetHidGuid(&guid);
	info = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (info == INVALID_HANDLE_VALUE) return 0;
	for (index=0; 1 ;index++) {
		iface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		ret = SetupDiEnumDeviceInterfaces(info, NULL, &guid, index, &iface);
		if (!ret) return count;
		SetupDiGetInterfaceDeviceDetail(info, &iface, NULL, 0, &reqd_size, NULL);
		details = (SP_DEVICE_INTERFACE_DETAIL_DATA *)malloc(reqd_size);
		if (details == NULL) continue;

		memset(details, 0, reqd_size);
		details->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		ret = SetupDiGetDeviceInterfaceDetail(info, &iface, details,
			reqd_size, NULL, NULL);
		if (!ret) {
			free(details);
			continue;
		}
		h = CreateFile(details->DevicePath, GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		free(details);
		if (h == INVALID_HANDLE_VALUE) continue;
		attrib.Size = sizeof(HIDD_ATTRIBUTES);
		ret = HidD_GetAttributes(h, &attrib);
		//printf("vid: %4x\n", attrib.VendorID);
		if (!ret || (vid > 0 && attrib.VendorID != vid) ||
		  (pid > 0 && attrib.ProductID != pid) ||
		  !HidD_GetPreparsedData(h, &hid_data)) {
			CloseHandle(h);
			continue;
		}
        RtlSecureZeroMemory(&capabilities, sizeof(capabilities));
        HidP_GetCaps(hid_data, &capabilities);
		if ((usage_page > 0 && capabilities.UsagePage != usage_page) ||
		  (usage > 0 && capabilities.Usage != usage)) {
			HidD_FreePreparsedData(hid_data);
			CloseHandle(h);
			continue;
		}
		HidD_FreePreparsedData(hid_data);
		hid = (struct hid_struct *)calloc(1, sizeof(struct hid_struct));
		if (!hid) {
			CloseHandle(h);
			continue;
		}
		hid->handle = h;
		hid->open = 1;
        HidD_GetProductString(h, hid->product, sizeof(hid->product));
		add_hid(hid);
		count++;
		if (count >= max) return count;
	}
	return count;
}


//  rawhid_close - close a device
//
//    Inputs:
//	num = device to close (zero based)
//    Output
//	(nothing)
//
void rawhid_close(int num)
{
	hid_t *hid;

	hid = get_hid(num);
	if (!hid || !hid->open) return;
	hid_close(hid);
}



static void add_hid(hid_t *h)
{
	if (!first_hid || !last_hid) {
		first_hid = last_hid = h;
		h->next = h->prev = NULL;
		return;
	}
	last_hid->next = h;
	h->prev = last_hid;
	h->next = NULL;
	last_hid = h;
}


static hid_t * get_hid(int num)
{
	hid_t *p;
	for (p = first_hid; p && num > 0; p = p->next, num--) ;
	return p;
}


static void free_all_hid(void)
{
	hid_t *p, *q;

	for (p = first_hid; p; p = p->next) {
		hid_close(p);
	}
	p = first_hid;
	while (p) {
		q = p;
		p = p->next;
		free(q);
	}
	first_hid = last_hid = NULL;
}


static void hid_close(hid_t *hid)
{
	CloseHandle(hid->handle);
	hid->handle = NULL;
}


void print_win32_err(void)
{
	char buf[256];
	DWORD err;

	err = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
		0, buf, sizeof(buf), NULL);
	printf("err %ld: %s\n", err, buf);
}





