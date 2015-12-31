/* Functions that are related to USB 
 */

#include <usb.h>
#include <string.h>	
#include <stdio.h>
#include "logger.h"
#include "usbFunctions.h"

int usbOpen(usb_dev_handle **DeviceHandle,uint16_t vendor,uint16_t product)
{
	// Will return 1 if the usb was successfully opened
	int state = 0;
	
	struct usb_bus *bus;	
	
	*DeviceHandle=NULL;
	
	if(log_sort.usb || log_sort.all) logger(LOG_DEBUG, logType, "ws_open","Initialise usb");
	
	usb_init();
	
	usb_set_debug(0);
	
	usb_find_busses();
	
	usb_find_devices();
	
	if(log_sort.usb || log_sort.all) logger(LOG_DEBUG, logType, "usbOpen", "Scan for device %04X:%04X", vendor, product);
	
	for (bus=usb_get_busses(); bus && *DeviceHandle==NULL; bus=bus->next)
	{
		struct usb_device *device;
		
		for (device=bus->devices; device && *DeviceHandle==NULL; device=device->next)
		{
			if (device->descriptor.idVendor == vendor && device->descriptor.idProduct == product)
			{
				if(log_sort.usb || log_sort.all) logger(LOG_DEBUG, logType, "usbOpen", "Found device %04X:%04X", vendor, product);
				
				*DeviceHandle=usb_open(device);
			}
		}
	}
	
	if (state==0 && *DeviceHandle)
	{
		char buf[1000];		
		switch (usb_get_driver_np(*DeviceHandle,0,buf,sizeof(buf)))
		{
			case 0:
				if(log_sort.usb || log_sort.all) logger(LOG_DEBUG, logType, "usbOpen", "Interface 0 already claimed by driver \"%s\", attempting to detach it", buf);
				state=usb_detach_kernel_driver_np(*DeviceHandle,0);
		}
		
		if (state==0)
		{
		  // Claimed the device from the system, set up the file handler
			if(log_sort.usb || log_sort.all) logger(LOG_DEBUG, logType, "usbOpen", "Claimed device from system");
			state=usb_claim_interface(*DeviceHandle,0);
		}
		
		if (state==0)
		{
			if(log_sort.usb || log_sort.all) logger(LOG_DEBUG, logType, "usbOpen", "Set alt interface");
			state=usb_set_altinterface(*DeviceHandle,0);
		}
	}
	else
	{
		if(log_sort.usb || log_sort.all) logger(LOG_ERROR, logType, "ws_open","Device %04X:%04X not found",vendor,product);
		state=1;
	}
	
	if (state==0)
	{
		if(log_sort.usb || log_sort.all) logger(LOG_USB, logType, "ws_open", "Device %04X:%04X opened, code:%d",vendor,product,state);
	}
	else
	{
		if(log_sort.usb || log_sort.all) logger(LOG_ERROR, logType, "ws_open","Device %04X:%04X: could not open, code:%d",vendor,product,state);
	}
	
	return state;
}

int usbClose(usb_dev_handle *DeviceHandle)
{
	int state;

	if (DeviceHandle)
	{
		state=usb_release_interface(DeviceHandle, 0);
		if (state!=0) {
			if(log_sort.usb || log_sort.all) logger(LOG_ERROR, logType, "usbClose", "Could not release interface, code:%d", state);
		}
		
		state=usb_close(DeviceHandle);
		if (state!=0) {
			if(log_sort.usb || log_sort.all) logger(LOG_ERROR, logType, "usbClose" , "Error closing interface, code:%d", state);
		}
	}
	
	return state;
}

int usbRead(usb_dev_handle *DeviceHandle, uint16_t address, uint8_t *data, uint16_t size)
{
	// Initiliaze local values
	uint16_t i,c;
	
	// Set the state to 0
	int state = 0;
	
	// setup a buffer
	uint8_t s, tmp[0x20];
	
	// Set the block of data to 0
	memset(data, 0, size);
	
	// Send debug message if enabled
	if(log_sort.usb || log_sort.all) logger(LOG_USB, logType, "usbRead", "Reading %d bytes from 0x%04X", size, address);
	
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
		if(log_sort.usb || log_sort.all) logger(LOG_USB, logType, "usbRead", "Send read command: Addr=0x%04X Size=%u", a, s);
		
		// Format the command
		sprintf(cmd,"\xA1%c%c%c\xA1%c%c%c", a>>8, a, c, a>>8, a, c);
		
		// Send the command to the usb device
		state = usb_control_msg(DeviceHandle, USB_TYPE_CLASS+USB_RECIP_INTERFACE, 0x0000009, 0x0000200, 0, cmd, sizeof(cmd) - 1, 1000);
		
		// Send debug message if enabled
		if(log_sort.usb || log_sort.all) logger(LOG_USB, logType, "usbRead", "Sent %d of %d bytes", state, sizeof(cmd) - 1); 
		
		// initiate a read request to the device
		state = usb_interrupt_read(DeviceHandle, 0x00000081, tmp, c, 1000);
		
		// Send debug message if enable
		if(log_sort.usb || log_sort.all) logger(LOG_USB, logType, "usbRead", "Read %d of %d bytes", state, c); 
		
		// Copy the data to tmp
		memcpy(data+i, tmp, s);
	}
	
	return 0;
}
