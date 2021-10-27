#ifndef _USBFUNCTIONS_H
#define _USBFUNCTIONS_H

/* Usb functions as required by the weather program
 * 
 */
#include <usb.h>

int wwsr_usb_open (void);
int wwsr_usb_close (void);
int wwsr_usb_read_value (uint16_t address, uint16_t *data);
#endif /* end header guard */