/* oousb2k.c - see oousb2k.h
 *
 * Copyright (C) 2003 Juergen "George" Sawinski
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <endian.h>

#include "oousb2k.h"

#define USB2000_VENDOR_ID             0x2457

#define USB2000_PRODUCT_ID            0x1001
#define USB2000_PRODUCT_ID_EEPROM     0x1002
#define USB2000_PRODUCT_ID_HR2000     0x100a

/* endianness */
#if BYTE_ORDER == LITTLE_ENDIAN
# define LSB 0
# define MSB 1
# define LSB_MASK 0x00FF
# define LSB_SHIFT 0
# define MSB_MASK 0xFF00
# define MSB_SHIFT 8
#else
# if BYTE_ORDER == BIG_ENDIAN
#  define LSB 1
#  define MSB 0
#  define LSB_MASK 0xFF00
#  define LSB_SHIFT 8
#  define MSB_MASK 0x00FF
#  define MSB_SHIFT 0
# else
#  error cannot handle byte order on this system
# endif 
#endif

#if 1 /* defined(DEBUG) */
#define msg(fmt, args...) fprintf(stderr, fmt, ## args)
#else
#define msg(fmt, args...) do {} while(0)
#endif

/* some ctrl helpers */
#define USB2000_COMMAND1(dev, c1, status) {		\
    dev->buffer[0] = c1;				\
    status = (control_send(dev, 1) != 1)?EIO:0; }
#define USB2000_COMMAND2(dev, c1, c2, status) {		\
    dev->buffer[0] = c1;				\
    dev->buffer[1] = c2;				\
    status = (control_send(dev, 2) != 2)?EIO:0; }
#define USB2000_COMMAND3(dev, c1, c2, c3, status) {	\
    dev->buffer[0] = c1;				\
    dev->buffer[1] = c2;				\
    dev->buffer[2] = c3;				\
    status = (control_send(dev, 3) != 3)?EIO:0; }

/* used communication endpoints */
#define EP2 0x02
#define EP7 0x07

/* commands defined in the interface draft */
#define CMD_INIT              0x01
#define CMD_INTEGRATION_TIME  0x02
#define CMD_STROBE_ENABLE     0x03
#define CMD_QUERY_INFO        0x05
#define CMD_WRITE_INFO        0x06
#define CMD_WRITE_SN          0x07
#define CMD_GET_SN            0x08
#define CMD_GET_SPECTRA       0x09
#define CMD_TRIGGER_MODE      0x0A

/* info */
#define INFO_SERIAL_ID         0 /* unofficial */
#define INFO_WAVELEN_COEFF_0   1
#define INFO_WAVELEN_COEFF_1   2
#define INFO_WAVELEN_COEFF_2   3
#define INFO_WAVELEN_COEFF_3   4
#define INFO_STRAY_LIGHT       5
#define INFO_NONLINEAR_COEFF_0 6
#define INFO_NONLINEAR_COEFF_1 7
#define INFO_NONLINEAR_COEFF_2 8
#define INFO_NONLINEAR_COEFF_3 9
#define INFO_NONLINEAR_COEFF_4 10
#define INFO_NONLINEAR_COEFF_5 11
#define INFO_NONLINEAR_COEFF_6 12
#define INFO_NONLINEAR_COEFF_7 13
#define INFO_NONLINEAR_ORDER   14 
#define INFO_OPTICAL_BENCH     15
#define INFO_CONFIGURATION     16

#define INFO_LAST              17

/* buffer sizes */
#define INFO_SIZE     16
#define QUERY_SIZE    17
#define SYNC_SIZE      1
#define PACKET_SIZE   64 /* FIXME HR4000 usb2.0 mode has different packet size */

static struct usb2000_device *__usb2000_devices = NULL;

extern inline 
struct usb2000_device *
__usb2000_dev_create(struct usb_device *dev) 
{
  struct usb2000_device *rv = 
    (struct usb2000_device *) malloc(sizeof(struct usb2000_device));

  if (rv) {
    memset(rv, 0, sizeof(struct usb2000_device));

    rv->device = dev;
    rv->buffer = malloc(PACKET_SIZE);
  }

  return rv;
}

extern inline
void
__usb2000_dev_destroy(struct usb2000_device *ptr)
{
  if (ptr->buffer) free(ptr->buffer);
  free(ptr);
}

void
__usb2000_dev_add(struct usb2000_device *dev)
{
  if  (__usb2000_devices) {
    struct usb2000_device *ptr = __usb2000_devices;
    while (ptr->next) ptr = ptr->next;
    ptr->next = dev;
  }
  else {
    __usb2000_devices = dev;
  }
}

void
__usb2000_dev_remove(struct usb2000_device *dev)
{
  struct usb2000_device *ptr = __usb2000_devices;
  struct usb2000_device *prev = NULL;
    
  while (ptr) {
    if (ptr == dev) {
      if (prev) {
	prev->next = ptr->next;
      }
      else {
	__usb2000_devices = ptr->next;
      }
    }
	
    prev = ptr;
    ptr = ptr->next;
  }
}

extern inline
struct usb2000_device *
__usb2000_dev_find(struct usb_device *dev)
{
  struct usb2000_device *ptr = __usb2000_devices;

  while (ptr) {
    if (ptr->device == dev) break;
    ptr = ptr->next;
  }
    
  return ptr;
}

static void
usb2000_finish()
{
  struct usb2000_device *ptr = __usb2000_devices;
  struct usb2000_device *nxt;
  while (ptr) {
    nxt = ptr->next;
    if (ptr->handle) usb2000_close(ptr);
    __usb2000_dev_destroy(ptr);
    ptr = nxt;
  }    
}

void
usb2000_init()
{
  usb_init();
  atexit(usb2000_finish);
}

struct usb2000_device *
usb2000_find_devices()
{
  struct usb_bus *bus;
  struct usb_device *dev;  

  usb_find_busses();
  usb_find_devices();

  /* walk thru busses */
  bus = usb_get_busses();
  while (bus) {
    dev = bus->devices;

    while (dev) {
      if (dev->descriptor.idVendor == USB2000_VENDOR_ID) {
	int take = 0;

	switch (dev->descriptor.idProduct) {
	  /* FIXME not supported without respective config setting functions
	     case USB2000_PRODUCT_ID:
	     msg("Found USB2000 spectrometer (w/o EEPROM)\n");
	     take = 1;
	     break;
	  */
	case USB2000_PRODUCT_ID_EEPROM:
	  msg("Found USB2000 spectrometer\n");
	  take = 1;
	  break;
	case USB2000_PRODUCT_ID_HR2000:
	  msg("Found USB2000 spectrometer (HR2000)\n");
	  take = 1;
	  break;
	}
	
	if (take) {
	  if (__usb2000_dev_find(dev) == NULL) {
	    __usb2000_dev_add(__usb2000_dev_create(dev));
	  }
	}
      }

      dev = dev->next;
    }

    bus = bus->next;
  }

  return __usb2000_devices;
}  

int
usb2000_reset(struct usb2000_device *dev) {
  if (!dev->handle) {
    dev->handle = usb_open(dev->device);
    if (!dev->handle) {
      errno = ENXIO;
      return -1;
    }
  }
  else {
    usb_release_interface(dev->handle, 0);
  }

  usb_reset(dev->handle);

  /*@FIXME find the device again */

  return 0;
}

extern inline int
control_send(struct usb2000_device *dev, int len) 
{
  return usb_bulk_write(dev->handle,
			EP2,
			dev->buffer, len,
			1000);
}

extern inline int
control_recv(struct usb2000_device *dev, int len)
{
  return usb_bulk_read(dev->handle,
		       EP7,
		       dev->buffer, len,
		       1000);
}

int
usb2000_open(struct usb2000_device *dev)
{
  int status;
  int count, ecount;
  int len;
  int i;

  dev->handle = usb_open(dev->device);
  if (!dev->handle) {
    msg("Cannot open device.\n");
    return ENXIO;
  }
  
  if (!dev->device->config) {
    msg("No valid config.\n");
    status = ENXIO;
    goto open_failure;
  }
  
  if ((status = usb_claim_interface(dev->handle, 0))) {
    msg("Claiming interface failed: %d - %s\n", status, usb_strerror());
    status = EIO;
    goto open_failure;
  }

  /* init device */
  msg("Initializing device...\n");
  USB2000_COMMAND1(dev, 
		   CMD_INIT, 
		   status);
  if (status) {
    msg("Device initialization failed: %s\n", usb_strerror());
    goto post_claim_failure;
  }

  /* wait for spectrum read to finish */
  for (count=0; count < 65; count++) {
    len = usb_bulk_read(dev->handle,
			EP2,
			dev->buffer, PACKET_SIZE,
			/*@FIXME the init command doesn't seem to clear the
			  integration time (opposed to what the hand book says, so...) */
			65535);
    if (len != PACKET_SIZE) {
      if (len == 1) {
	if (dev->buffer[0] != 0x69) {
	  msg("SYNC: packet length: %d\n", len);
	  msg("SYNC: first byte: %0X\n", (int) dev->buffer[0]);
	  status = EIO;
	  goto post_claim_failure; 
	}
	if (count != 64) {
	  msg("*** Premature sync packet.");
	  break;
	}
      }
      else {
	msg("*** Packet count %d (%db)\n", count, len);
	status = EIO;
	goto post_claim_failure;
      }
    }
    else {
      msg("Packet count %d (%db)\n", count, len);
    }
  }

  /* read config */



  /*@FIXME this is ugly. Read config at once, then convert...*/
#define READ_CONFIG(idx)						\
  USB2000_COMMAND2(dev,							\
		   CMD_QUERY_INFO,					\
		   (u_int8_t) (idx),					\
		   status);						\
  if (status) {								\
    msg("Send query command (%d) failed.\n", i);			\
    status = EIO;							\
    goto post_claim_failure;						\
  }									\
  ecount = 0;								\
  count = control_recv(dev, QUERY_SIZE);				\
  while ((count != QUERY_SIZE) && (ecount < 8)) {			\
    /* try again */							\
    msg("Retry reading config...\n");					\
    /* FIXME try clear read buffers */					\
    usleep(10000);							\
    count = control_recv(dev, QUERY_SIZE);				\
    ecount++;								\
  }									\
  if (count != QUERY_SIZE) {						\
    msg("Reading config id %d failed (recv count %d (%d)).\n", i, count, QUERY_SIZE); \
    status = EIO;							\
    goto post_claim_failure;						\
  }									\
  dev->buffer[18] = 0;							\
  msg("Query info %d: %s\n", (idx), dev->buffer+2);





  /* S/N */
  READ_CONFIG(0);
  memcpy(dev->serialno, dev->buffer+2, INFO_SIZE);
    
  /* wavelength coefficients */
  for(i=0; i<4; i++) {
    READ_CONFIG(i+INFO_WAVELEN_COEFF_0);
    dev->lambda[i] = strtod((char *) dev->buffer+2, NULL);
  }
    
  /* stray light */
  READ_CONFIG(INFO_STRAY_LIGHT);
  dev->stray_light = strtod((char *) dev->buffer+2, NULL);
    
  /* correction coefficients */
    
  for(i=0; i<8; i++) {
    READ_CONFIG(i+INFO_NONLINEAR_COEFF_0);
    dev->calib[i] = strtod((char *) dev->buffer+2, NULL);
  }
  READ_CONFIG(INFO_NONLINEAR_ORDER);
  dev->calib_order = strtod((char *) dev->buffer+2, NULL); 
    
  /* optical bench config */
  READ_CONFIG(INFO_OPTICAL_BENCH);
  dev->optical_bench.grating = strtol((char *) dev->buffer+2, NULL, 10);
  dev->optical_bench.filter = strtol((char *) dev->buffer+5, NULL, 10);
  for(i=9; (i<16) && (dev->buffer[i] == '0'); i++);
  dev->optical_bench.slit = strtol((char *) dev->buffer+i, NULL, 10);
    
  /* USB2000 configuration */
  READ_CONFIG(INFO_CONFIGURATION);
  dev->config.coating = dev->buffer[2];
  dev->config.wavelength = dev->buffer[3];
  dev->config.lense = dev->buffer[4];
  dev->config.cpld_version = dev->buffer[6];
    
  /* set defaults we cannot read */
  /*@FIXME this seems not really to be resetted on INIT command
    unlike the manual says... */
  dev->itime = 100;
  dev->strobe = USB2000_LAMP_DISABLE;
  dev->trigger = USB2000_TRIGGER_NORMAL;
    
  /* k */
  return 0;  
    
 post_claim_failure:
  usb_release_interface(dev->handle, 0 /*@FIXME really hardcode ?*/);
    
 open_failure:
  usb_close(dev->handle);
  dev->handle = NULL;
  errno = status;
  return -1;
}

void
usb2000_report(FILE *stream, struct usb2000_device *dev)
{
  int i;
  fprintf(stream, "Serial number: %s\n", dev->serialno);
  fprintf(stream, "Wavelength coefficients:\n");
  for(i=0; i<4; i++) fprintf(stream, "    [%d] %g\n", i, dev->lambda[i]);
  fprintf(stream, "Linearity correction coefficients:\n");
  for(i=0; i<dev->calib_order; i++) fprintf(stream, "    [%d] %g\n", i, dev->calib[i]);
  fprintf(stream, "\n");
}

int
usb2000_set_integration_time(struct usb2000_device *dev, int ms)
{
  int status;
  u_int16_t it = (u_int16_t) ms;
  if ((ms < 3) || (ms > 65535)) {
    errno = EINVAL;
    return -1;
  }

  USB2000_COMMAND3(dev,
		   CMD_INTEGRATION_TIME,
		   (it & LSB_MASK) >> LSB_SHIFT,
		   (it & MSB_MASK) >> MSB_SHIFT,
		   status);
  msg("setting itime %d (status=%d)\n", ms, status);
  dev->itime = ms;

  if (status) {
    errno = status;
    return -1;
  }

  return /*@FIXME */0;
}

int
usb2000_set_strobe_enable(struct usb2000_device *dev, int en)
{
  /*@FIXME*/
}

int
usb2000_set_trigger_mode(struct usb2000_device *dev, int tm) 
{
  int status;
  if ((tm<0) || (tm>3) || (tm == 1)) {
    errno = EINVAL;
    return -1;
  }

  USB2000_COMMAND2(dev,
		   CMD_TRIGGER_MODE, (u_int8_t) tm,
		   status);
  dev->trigger = tm;

  if (status) {
    errno = status;
    return -1;
  }

  return /*@FIXME */0;
}

int
usb2000_get_integration_time(struct usb2000_device *dev)
{
  if (!dev->handle) {
    errno = ENXIO;
    return -1;
  }

  return dev->itime;
}

int
usb2000_get_strobe_enable(struct usb2000_device *dev)
{
  if (!dev->handle) {
    errno = ENXIO;
    return -1;
  }

  /*@FIXME*/
  return 0;
}

int
usb2000_get_trigger_mode(struct usb2000_device *dev)
{
  if (!dev->handle) {
    errno = ENXIO;
    return -1;
  }

  /*@FIXME*/
  return dev->trigger;
}

int
usb2000_close(struct usb2000_device *dev)
{
  usb_resetep(dev->handle, EP2);
  usb_resetep(dev->handle, EP7);

  usb_release_interface(dev->handle, 0/*@FIXME really hardcode ?*/);
  usb_close(dev->handle);
  
  dev->handle = NULL;
  return 0;
}

void
usb2000_get_wavelength(struct usb2000_device *dev, double *arr)
{
  int i;

#define D(n) ((double) n)
  for(i=0; i<USB2000_FMT_BINS; i++) {
    arr[i] = dev->lambda[0] 
      + D(i)*dev->lambda[1] 
      + pow(D(i), 2)*dev->lambda[2] 
      + pow(D(i), 3)*dev->lambda[3];
  }
}

void
usb2000_get_linear_correction(struct usb2000_device *dev, double *arr)
{
  int i,j;

  for(i=0; i<(1<<USB2000_FMT_BITS); i++) {
    double p = 1.0;

    for(j=0; j<dev->calib_order; j++) {
      arr[i] += dev->calib[j]*p;
      p *= D(i);
    }
  }
}

void
usb2000_get_spectrum_raw(struct usb2000_device *dev, u_int16_t *arr)
{ 
  int count;
  int i,n,off;
  u_int8_t *out = (u_int8_t *) arr;

  USB2000_COMMAND1(dev, 
		   CMD_GET_SPECTRA, 
		   count);
  /*@FIXME error check*/

  /* we expect 64 data packets and a sync packet */
  for(i=0; i<=64; i++) {
    if (i<64) {
      count = usb_bulk_read(dev->handle,
			    EP2,
			    dev->buffer, PACKET_SIZE,
			    dev->itime+500);
      msg("Finished package %d with count=%d\n", i, count);
      if (count != PACKET_SIZE) {
	if ((count == 1) && (dev->buffer[0] == 0x69)) {
	  msg("*** received sync packet???");
	  return /*@FIXME*/;
	}

	msg("*** PACKET ERROR (%s)", usb_strerror());
	/*@FIXME this may loop forever!!!!*/
	i--;
	continue;
      }

      /* convert */
      off = (i & ~1)*64;
      if (i & 0x01) {
	for(n=0; n<PACKET_SIZE; n++) 
	  out[off + 2*n + MSB] = dev->buffer[n];
      }
      else {
	for(n=0; n<PACKET_SIZE; n++) 
	  out[off + 2*n + LSB] = dev->buffer[n];
      }
    }
    else {
      count = usb_bulk_read(dev->handle,
			    EP2,
			    dev->buffer, SYNC_SIZE,
			    dev->itime+100);
      msg("Finished sync packet with count=%d\n", i, count);
      if (dev->buffer[0] != 0x69) {
	msg("Sync packet missed.\n");
	return /*@FIXME*/;
      }
    }
  }
}

void
usb2000_get_spectrum(struct usb2000_device *dev, double *linear_correction, double *result)
{
  int i;
  u_int16_t buf[USB2000_FMT_BINS];

  double maxval = D((1<<USB2000_FMT_BITS)-1);

  /* get raw spectrum */
  usb2000_get_spectrum_raw(dev, buf);

  /* FIXME correct maxval if linear_correction present */

  /* calculate */
  for(i=0; i<USB2000_FMT_BINS; i++) {
    result[i] = D(buf[i])/maxval;
    if (linear_correction)
      result[i] *= linear_correction[(int) buf[i]];
  }
}
