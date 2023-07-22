/* oousb2k-test.c - a simple test program to test the oousb2k library
 * Copyright (C) 2003 Juergen "George" Sawinski sawinski@web.de
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
#include <stdio.h>
#include <usb.h>
#include "oousb2k.h"

int
main(int argc, char **argv) 
{
  int status,i;

  double lambda[USB2000_FMT_BINS] = {0};
  unsigned short buf[USB2000_FMT_BINS] = {0};

  struct usb2000_device *dev;

  usb2000_init();

  dev = usb2000_find_devices();
  if (!dev) {
    fprintf(stderr, "Couldn't find any USB2000 spectrometre\n");
    exit(-1);
  }

  if ((status = usb2000_open(dev))) {
    fprintf(stderr, "Open failed: %d - %s\n", status, strerror(status));

    usb2000_reset(dev);
    exit(-1);
  }

  usb2000_set_integration_time(dev, 10000);
  usb2000_get_wavelength(dev, lambda);

  usb2000_get_spectrum_raw(dev, buf);
  for(i=0; i<USB2000_FMT_BINS; i++) {
    printf("%.2f %d\n", lambda[i], (int) buf[i]);
  }

  fprintf(stderr, "Shutting down, please wait...\n");
  usb2000_close(dev);
}
