/* -*- Mode: C; c-basic-offset: 3 -*-
 *
 * serial.h - Simple serial driver.
 *
 * This file is part of Metalkit, a simple collection of modules for
 * writing software that runs on the bare metal. Get the latest code
 * at http://svn.navi.cx/misc/trunk/metalkit/
 *
 * Copyright (c) 2008-2009 Micah Elizabeth Scott
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "types.h"

/*
 * Serial baud rate stuff
 */

#define SERIAL_BAUD_DEFAULT 115200
#define SERIAL_BAUD_MIN     50


/*
 * Private data
 */

#define SERIAL_BUF_SIZE 8

typedef fastcall void (*SerialIRQHandler)(uint8 *buffer);

typedef struct SerialPrivate {
    SerialIRQHandler handler;
    uint8 counter;
    uint8 buffer[SERIAL_BUF_SIZE];
} SerialPrivate;

extern SerialPrivate gSerial;


/*
 * Public Functions
 */

fastcall void Serial_Init(uint32);

fastcall uint8 Serial_ReadByte(void);

fastcall void Serial_WriteByte(uint8);


/*
 * Serial_SetHandler --
 *
 *    Set a handler that will receive bytes from the serial.
 *    This handler runs withing the IRQ handler, so it must complete
 *    quickly and use minimal stack space.
 */

static inline void
Serial_SetHandler(SerialIRQHandler handler)
{
    gSerial.handler = handler;
}


#endif /* __SERIAL_H__ */
