#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "hidapi.h"
#include <memory.h>
#include <stdbool.h>

#define MAX_STR 255
//#define PATH "44'/60'/0'/0/0"
#define COIN_TYPE "Ethereum"

static int HARDENED_FLAG = 1 << 31;
int main(int argc, char *argv[])
{
	int res;
	//unsigned char buf[65];

	uint32_t bip_path[5];
	uint32_t header[5];
	header[0] = 0x08;
	header[0] = 0x00;
	header[0] = 0x00;
	header[0] = 0x00;
	header[0] = 0x18;

	bip_path[0] = 2147483692; //44 | HARDENED_FLAG;
	bip_path[1] = 2147483708; //60 | HARDENED_FLAG;
	bip_path[2] = 2147483648; //0 | HARDENED_FLAG;
	bip_path[3] = 0;
	bip_path[4] = 0;

	wchar_t wstr[MAX_STR];
	char chstr[MAX_STR];
	hid_device *handle;
	int i;
	int len;
	unsigned char buf[64];

	// Initialize the hidapi library
	res = hid_init();

	printf("init response %d\n", res);
	// Open the device using the VID, PID,
	// and optionally the Serial number.
	handle = hid_open(0x2c97, 0x1001, NULL);
	struct hid_device_info *device_info = hid_enumerate(0x2c97, 0x1001);
	/// test to ping device

	buf[0] = 0x1;
	buf[1] = 0x81;
	res = hid_write(handle, buf, 17);
	if (res < 0)
		printf("Unable to write() (2)\n");

	// Read requested state. hid_read() has been set to be
	// non-blocking by the call to hid_set_nonblocking() above.
	// This loop demonstrates the non-blocking nature of hid_read().
	if (res >= 0) {
		res = hid_read(handle, buf, sizeof(buf));
		if (res > 0)
		{
		
			for (i = 0; i < res; i++)
			{
				printf("%02hhx ", buf[i]);
				
			}
			printf("\nread from device %d\n",res);
		}else{
			printf("unable to read \n");
		}
	}
	///end test 
	if (NULL != handle)
	{
		// Read the Manufacturer String
		printf("device info\n");
		printf("device vid %d pid %d\n",device_info->vendor_id,device_info->product_id);
		res = hid_get_manufacturer_string(handle, wstr, MAX_STR);

		printf("Manufacturer String: %ls\n", wstr);
	

		// Read the Product String
		res = hid_get_product_string(handle, wstr, MAX_STR);
		printf("Product String: %ls\n", wstr);

		// Read the Serial Number String
		res = hid_get_serial_number_string(handle, wstr, MAX_STR);
		printf("Serial Number String:  %ls\n", wstr);

		// Read Indexed String 1
		res = hid_get_indexed_string(handle, 1, wstr, MAX_STR);
		printf("Indexed String 1: %ls\n", wstr);

		///////hid init command
		unsigned char cmd_file[] = {0x01,0x01,0x05,0x00,0x00,0x00,0x07,0x80,0x02,0x00,0x00,0x02
									,0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
									,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
									,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
									,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
									,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
									,0x00,0x00,0x00,0x00};
		int cmd_size = 64;
		printf("writing to usb %d\n", cmd_size);
		res = hid_write(handle, cmd_file, cmd_size);
		
		//hid_set_nonblocking(handle,1);
		res = hid_read(handle, buf, 64);
		if (res > 0)
		{
		
			for (i = 0; i < res; i++)
			{
				printf("%02hhx ", buf[i]);
				
			}
			printf("\nread from device %d\n",res);
		}else{
			printf("unable to read \n");
		}


		// Close the device
		hid_close(handle);

		res = hid_exit();
	}else{
		printf("can't open device handle \n");
	}
	return 0;
}

int readBinaryFile(unsigned char **array, char *filename)
{

	FILE *pFile;
	pFile = fopen(filename, "rb");
	if (pFile != NULL)
	{
		fseek(pFile, 0L, SEEK_END);
		size_t size = ftell(pFile);
		fseek(pFile, 0L, SEEK_SET);
		//uint8_t *ByteArray;
		printf("file size in bytes %d\n", size);
		*array = malloc(size);
		if (pFile != NULL)
		{
			int counter = 0;
			counter = fread(*array, 1, size, pFile);
			printf("bytes read %d\n", counter);
			fclose(pFile);
		}
		return size;
	}
	printf("could not open file\n");
	return 0;
}