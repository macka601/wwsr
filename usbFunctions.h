#ifndef _USBFUNCTIONS_H
#define _USBFUNCTIONS_H

/* Usb functions as required by the weather program
 * 
 */
#include <usb.h>

extern int logType;
int usbOpen( usb_dev_handle **DeviceHandle, uint16_t vendor,uint16_t product );
int usbClose( usb_dev_handle *DeviceHandle );
int usbRead(usb_dev_handle *DeviceHandle,uint16_t address,uint8_t *data,uint16_t size);
#endif /* end header guard */