// USBHID_Upgrade.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include "SLABHIDDevice.h"

#pragma comment (lib, "SLABHIDDevice.lib")

#define VID					0x10C4
#define PID					0xEAC9

// HID Report IDs
#define ID_IN_CONTROL		0xFE
#define ID_OUT_CONTROL		0xFD

#define REBOOT_DELAY		500
#define READ_TIMEOUT		100
#define WRITE_TIMEOUT		100

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

HID_DEVICE	m_hid;
BYTE report[512], getreport[512];

int WaitReturn( void )
{
	DWORD bytesReturned = 0;

	HidDevice_GetInputReport_Interrupt( m_hid, getreport, 60, 1, &bytesReturned );

	//printf( "\nGet (%d) - ", bytesReturned );
	for( DWORD i = 0; i < bytesReturned; i++ )
	{
		//printf( "0x%02x '%c', ", getreport[ i ], getreport[ i ] );
	//	printf( "0x%02x, ", getreport[ i ] );
	}
	return bytesReturned;
}

BYTE firmware[8192] = {	0 };
int firmware_size = 0;
size_t send_pos = 0;

int readdata(void *dst, int elementsize, int count)
{
	memcpy(dst,&firmware[ send_pos ],elementsize*count );
	send_pos += (elementsize*count);
	return elementsize*count;
}

int main(int argc, char* argv[])
{
	if( argc < 2 )
	{
		printf( "Usage: USBHID_Upgrade firmware.bin" );
		return 1;
	}

	FILE *fp;
	fp = fopen( argv[1], "rb" );
	if( fp == NULL )
	{
		printf("%s open failure", argv[1]);
		return 1;
	}

	size_t firmware_size = fread( firmware, 1, 8192, fp );
	printf( "Read file size %d(0x%x) bytes\r\n", (int)firmware_size, (int)firmware_size );

	char deviceString[MAX_PATH_LENGTH];
	int devnum = HidDevice_GetNumHidDevices(VID, PID);

	for( int i = 0; i < devnum; i++ )
	{
		if( HidDevice_GetHidString(i, VID, PID, HID_PATH_STRING, deviceString, MAX_PATH_LENGTH) == HID_DEVICE_SUCCESS )
		{
			printf( "%d = %s\r\n", i, deviceString );
		}
	}

	if( devnum == 0 )
	{
		printf( "Device not found" );
		return 1;
	}

	BYTE status = HidDevice_Open(&m_hid, 0, VID, PID, MAX_REPORT_REQUEST_XP);
	if( status != HID_DEVICE_SUCCESS )
	{
		printf( "Device open failed" );
		return 1;
	}

	HidDevice_SetTimeouts( m_hid, READ_TIMEOUT, WRITE_TIMEOUT ); // get/set timeout

	report[0] = ID_OUT_CONTROL;
	report[ 1 ] = '$'; // Start
	report[ 2 ] = 1;
	report[ 3 ] = 0x48;
	if( HidDevice_SetOutputReport_Interrupt( m_hid, report, 65 ) != HID_DEVICE_SUCCESS )
	{
		printf("Send message fail");
	}

	HidDevice_Close(m_hid);
	Sleep( REBOOT_DELAY );

	if( 0 == HidDevice_GetNumHidDevices(VID, PID))
	{
		printf( "Device not found" );
		return 1;
	}

	if( HID_DEVICE_SUCCESS != HidDevice_Open(&m_hid, 0, VID, PID, MAX_REPORT_REQUEST_XP))
	{
		printf( "Device open failed" );
		return 1;
	}

	HidDevice_SetTimeouts( m_hid, READ_TIMEOUT, WRITE_TIMEOUT ); // get/set timeout

	for( ;; )
	{
		memset( report, 0, 65 );
		report[0] = ID_OUT_CONTROL;

		if( 1 != readdata( &report[ 1 ], 1, 1 ))
		{
			printf( "File finished" );
			break;
		}

		if( report[ 1 ] != '$' )
		{
			// File format error
			if( send_pos < firmware_size )
			{
				printf( "File format error" );
			}
			break;
		}

		if( 1 != readdata( &report[ 2 ], 1, 1 ))
		{
			break;
		}

		if( report[ 2 ] > 62 + 64 )
		{
			int size = report[ 2 ];

			if( 62 != readdata( &report[ 3 ], 1, 62 ))
			{
				break;
			}

			if( HidDevice_SetOutputReport_Interrupt( m_hid, report, 65 ) != HID_DEVICE_SUCCESS )
			{
				printf("Set report error");
				goto Error;
			}

			size -= 62;
			if( 64 != readdata( &report[ 1 ], 1, 64 ))
			{
				break;
			}

			if( HidDevice_SetOutputReport_Interrupt( m_hid, report, 65 ) != HID_DEVICE_SUCCESS )
			{
				printf("Set report error");
				goto Error;
			}

			size -= 64;
			memset( &report[1], 0, 64 );
			if( size != readdata( &report[ 1 ], 1, size ))
			{
				break;
			}

			if( HidDevice_SetOutputReport_Interrupt( m_hid, report, 65 ) != HID_DEVICE_SUCCESS )
			{
				printf("Set report error");
				goto Error;
			}
		}
		else if( report[ 2 ] > 62 )
		{
			int size = report[ 2 ];

			if( 62 != readdata( &report[ 3 ], 1, 62 ))
			{
				break;
			}

			if( HidDevice_SetOutputReport_Interrupt( m_hid, report, 65 ) != HID_DEVICE_SUCCESS )
			{
				printf("Set report error");
				goto Error;
			}
			size -= 62;
			if( size != readdata( &report[ 1 ], 1, size ))
			{
				break;
			}

			if( HidDevice_SetOutputReport_Interrupt( m_hid, report, 65 ) != HID_DEVICE_SUCCESS )
			{
				printf("Set report error");
				goto Error;
			}
		}
		else
		{
			if( report[ 2 ] != readdata( &report[ 3 ], 1, report[ 2 ] ))
			{
				break;
			}

			if( HidDevice_SetOutputReport_Interrupt( m_hid, report, 65 ) != HID_DEVICE_SUCCESS )
			{
				printf("Set report error");
				goto Error;
			}
		}
		if( WaitReturn() == 0 )
		{
			printf("Error, disconnect from PC and connect again, and retry");
			break;
		}
		printf(".");
	}

Error:
	HidDevice_Close(m_hid);
	return 0;
}
