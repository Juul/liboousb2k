/* oousb2k.h - definitions for the Ocean Optics USB2000 spectrometer
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
#ifndef OCEANOPTICS_USB2000_LIB_H
#define OCEANOPTICS_USB2000_LIB_H

#include <usb.h>

__BEGIN_DECLS

/* acquistion format */
/** Acquisition bits (D/A) */
#define USB2000_FMT_BITS 12
/** Number of pixels per spectrum acquisition. */
#define USB2000_FMT_BINS 2048 /* number of entries */

/* trigger modes */
/** Trigger mode: acquire a spectrum each read */
#define USB2000_TRIGGER_NORMAL   0
/** Trigger mode: software trigger (see manual) */
#define USB2000_TRIGGER_SOFTWARE 2
/** Trigger mode: hardware trigger (see manual) */
#define USB2000_TRIGGER_HARDWARE 3

/* lamp */
/** Lamp on */
#define USB2000_LAMP_ENABLE  (-1)
/** Lamp off */
#define USB2000_LAMP_DISABLE   0

/** @struct usb2000_device
 *  @brief Device and device related info structure 
 */
struct usb2000_device
{
  struct usb2000_device *next;   /**< @internal Linked list of USB2000 devices attached. */

  /* data */
  char   serialno[18];           /**< Serial number string */
  double lambda[4];              /**< Wavelength coefficients: 
				    w = l[0] + l[1]*p + l[2]*p^2 + l[3]*p^3
				    with
				    w wavelength
				    l lambda array
				    p pixel number
				 */
  double stray_light;            /**< FIXME docu */
  double calib[8];               /**< Linear correction coefficients */
  int    calib_order;            /**< Polynomial order of linear correction coefficients */

  struct {
    int grating;                 /**< FIXME docu */
    int filter;                  /**< FIXME docu */
    int slit;                    /**< FIXME docu */
  } optical_bench;

  struct {
    char coating;                /**< FIXME docu */
    char wavelength;             /**< FIXME docu */
    char lense;                  /**< FIXME docu */
    char cpld_version;           /**< FIXME docu */
  } config;  
  
  /* this is stuff that is not readable from device */
  int   itime;                   /**< Integration time (value is (can) not (be) read from device) */
  int   strobe;                  /**< Strobe enable (value is (can) not (be) read from device)  */
  int   trigger;                 /**< Trigger mode (value is (can) not (be) read from device)  */

  /* private: */
  struct usb_device *device;     /**< @internal usb library device */
  usb_dev_handle *handle;        /**< @internal usb library handle */

  char *buffer;                  /**< @internal device buffer for control and data send/recv operations */
};

/** Initialize library (this also initializes the usb library) */
void                          usb2000_init();

/** Find an USB2000 device */
struct usb2000_device        *usb2000_find_devices();

/* * Reset the device connection */
/* FIXME int                           usb2000_reset(struct usb2000_device *dev);*/

/** Open device found with usb2000_find_devices() */
int                           usb2000_open(struct usb2000_device *dev);
/** Close device previously found with usb2000_find_devices() */
int                           usb2000_close(struct usb2000_device *dev);
/** Report device properties */
void                          usb2000_report(FILE *stream, struct usb2000_device *dev);

/* FIXME operations to set linearity corrections factors etc. for
   devices without EEPROM */

/** Set the integration time in ms */
int                           usb2000_set_integration_time(struct usb2000_device *dev, int ms);
/** Set strobe mode */
int                           usb2000_set_strobe_enable(struct usb2000_device *dev, int en);
/** Set trigger mode */
int                           usb2000_set_trigger_mode(struct usb2000_device *dev, int tm);

/** Get integration (if previously set) */
int                           usb2000_get_integration_time(struct usb2000_device *dev);
/** Get strobe mode (if previously set) */
int                           usb2000_get_strobe_enable(struct usb2000_device *dev);
/** Get trigger mode (if previously set) */
int                           usb2000_get_trigger_mode(struct usb2000_device *dev);

/** Get the raw spectrum from device */
void                          usb2000_get_spectrum_raw(struct usb2000_device *dev, u_int16_t *arr);

/** Get the wavelength->pixel mapping (@a arr has to of size USB2000_FMT_BINS) */
void                          usb2000_get_wavelength(struct usb2000_device *dev, double *arr);

/** Get the linearity correction mapping (@a arr has to of size 1<<USB2000_FMT_BITS) */
void                          usb2000_get_linear_correction(struct usb2000_device *dev, double *arr);

void                          usb2000_get_spectrum(struct usb2000_device *dev, double *linear_correction, double *result);

__END_DECLS

#endif
