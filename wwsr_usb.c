/* Functions that are related to USB 
 */

#include <usb.h>
#include <string.h>	
#include <stdio.h>
#include "logger.h"
#include "config.h"
#include "wwsr_usb.h"

// If you have a different usb device, change the codes here
#define VENDOR_ID    0x1941
#define PRODUCT_ID   0x8021

static usb_dev_handle *device_handle;

int wwsr_usb_open (void)
{
	// Will return 1 if the usb was successfully opened
	int state = 0;

	log_event log_level = config_get_log_level ();

	struct usb_bus *bus;

	if (device_handle != NULL)
	{
		logger(LOG_ERROR, log_level, __func__,"Device already opened, please close first");
		return -1;
	}

	device_handle = NULL;

	logger(LOG_USB, log_level, __func__,"Initialise usb");

	usb_init();
	
	usb_set_debug(0);
	
	usb_find_busses();
	
	usb_find_devices();
	
	logger(LOG_USB, log_level, __func__, "Scan for device %04X:%04X", VENDOR_ID, PRODUCT_ID);
	
	for (bus = usb_get_busses(); bus && device_handle == NULL; bus = bus->next)
	{
		struct usb_device *device;
		
		for (device = bus->devices; device && device_handle == NULL; device=device->next)
		{
			if (device->descriptor.idVendor == VENDOR_ID && device->descriptor.idProduct == PRODUCT_ID)
			{
				logger(LOG_USB, log_level, __func__, "Found device %04X:%04X", VENDOR_ID, PRODUCT_ID);
				device_handle = usb_open(device);
			}
		}
	}
	
	if (state==0 && device_handle)
	{
		char buf[1000];		
		switch (usb_get_driver_np (device_handle,0,buf,sizeof(buf)))
		{
			case 0:
				logger(LOG_USB, log_level, __func__, "Interface 0 already claimed by driver \"%s\", attempting to detach it", buf);
				state = usb_detach_kernel_driver_np (device_handle, 0);
		}
		
		if (state==0)
		{
			// Claimed the device from the system, set up the file handler
			logger(LOG_USB, log_level, __func__, "Claimed device from system");
			state=usb_claim_interface(device_handle,0);
		}
		
		if (state==0)
		{
			logger(LOG_USB, log_level, __func__, "Set alt interface");
			state=usb_set_altinterface(device_handle,0);
		}
	}
	else
	{
		logger(LOG_ERROR, log_level, __func__,"Device %04X:%04X not found", VENDOR_ID, PRODUCT_ID);
		state=1;
	}
	
	if (state==0)
	{
		logger(LOG_USB, log_level, __func__, "Device %04X:%04X opened, code:%d", VENDOR_ID, PRODUCT_ID, state);
	}
	else
	{
		logger(LOG_ERROR, log_level, __func__,"Device %04X:%04X: could not open, code:%d", VENDOR_ID, PRODUCT_ID, state);
	}
	
	return state;
}

int wwsr_usb_close(void)
{
	int state;
	log_event log_level = config_get_log_level ();

	if (device_handle)
	{
		state=usb_release_interface(device_handle, 0);
		if (state!=0) {
			logger(LOG_ERROR, log_level, __func__, "Could not release interface, code:%d", state);
		}
		
		state=usb_close(device_handle);
		if (state!=0) {
			logger(LOG_ERROR, log_level, __func__, "Error closing interface, code:%d", state);
		}
	}
	logger(LOG_USB, log_level, __func__, "Closing interface, state is:%d", state);

	return state;
}

int wwsr_usb_read(uint16_t address, uint8_t *data, uint16_t size)
{
	// Initiliaze local values
	uint16_t i,c;
	log_event log_level = config_get_log_level ();

	// Set the state to 0
	int state = 0;
	
	// setup a buffer
	uint8_t s, tmp[0x20];
	
	// Set the block of data to 0
	memset(data, 0, size);
	
	// Send debug message if enabled
	logger(LOG_USB, log_level, __func__, "Reading %d bytes from 0x%04X", size, address);
	
	// set i to 0
	i = 0;
	
	// put buffer size of tmp into c
	c = sizeof(tmp);
	
	// if size - 0 is less than c, then size - 0 should equal c
	s = size - i < c ? size - i:c;
	
	for (;i < size; i += s, s = size - i < c ? size - i:c)
	{
		uint16_t a;
		
		char cmd[9];
		
		// Increment the address to read
		a = address + i;
		
		// Send debug message
		logger(LOG_USB, log_level, __func__, "Send read command: Addr=0x%04X Size=%u", a, s);
		
		// Format the command
		sprintf(cmd,"\xA1%c%c%c\xA1%c%c%c", a>>8, a, c, a>>8, a, c);
		
		// Send the command to the usb device
		state = usb_control_msg(device_handle, USB_TYPE_CLASS+USB_RECIP_INTERFACE, 0x0000009, 0x0000200, 0, cmd, sizeof(cmd) - 1, 1000);
		
		// Send debug message if enabled
		logger(LOG_USB, log_level, __func__, "Sent %d of %d bytes", state, sizeof(cmd) - 1); 
		
		// initiate a read request to the device
		state = usb_interrupt_read(device_handle, 0x00000081, tmp, c, 1000);
		
		// Send debug message if enable
		logger(LOG_USB, log_level, __func__, "Read %d of %d bytes", state, c); 
		
		// Copy the data to tmp
		memcpy(data+i, tmp, s);
	}
	
	return 0;
}
