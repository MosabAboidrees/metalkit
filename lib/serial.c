/* -*- Mode: C; c-basic-offset: 3 -*-
 *
 * serial.c - Simple serial driver.
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

#include "serial.h"
#include "io.h"
#include "intr.h"
#include "console.h"

/*
 * Serial hardware definitions
 */

#define SERIAL_BASE_PORT_COM1   0x3F8
#define SERIAL_IRQ_COM1         4

#define SERIAL_BASE_PORT        SERIAL_BASE_PORT_COM1

#define SERIAL_DATA_PORT        SERIAL_BASE_PORT + 0    // DLAB set to 0
#define SERIAL_IER_PORT         SERIAL_BASE_PORT + 1    // DLAB set to 0
#define SERIAL_LSB_DIV_PORT     SERIAL_BASE_PORT + 0    // DLAB set to 1
#define SERIAL_MSB_DIV_PORT     SERIAL_BASE_PORT + 1    // DLAB set to 1
#define SERIAL_IIR_PORT         SERIAL_BASE_PORT + 2    // IN -> IIR, OUT -> FIFO
#define SERIAL_LCR_PORT         SERIAL_BASE_PORT + 3
#define SERIAL_MCR_PORT         SERIAL_BASE_PORT + 4
#define SERIAL_LSR_PORT         SERIAL_BASE_PORT + 5
#define SERIAL_MSR_PORT         SERIAL_BASE_PORT + 6
#define SERIAL_SR_PORT          SERIAL_BASE_PORT + 7

#define SERIAL_STATUS_DR        (1 << 0)
#define SERIAL_STATUS_ETHR      (1 << 5)

/*
 * Global serial state
 */

SerialPrivate gSerial;

/*
 * Serial_CalcBaudDivisor --
 *
 *    Calculate the baud divisor based on the given baud rate.
 */

static uint16
Serial_CalcBaudDivisor(uint32 rate)
{
    if (rate > SERIAL_BAUD_DEFAULT || rate < SERIAL_BAUD_MIN) {
        return -1;
    }

    return SERIAL_BAUD_DEFAULT / rate;
}


/*
 * Serial_SetBaudDivisor --
 *
 *    Set the baud divisor to the given value.
 */

static void
Serial_SetBaudDivisor(uint16 divisor)
{
    uint8 tmpLCR;

    if (divisor == 0) {
        return;
    }

    tmpLCR = IO_In8(SERIAL_LCR_PORT);
    tmpLCR |= 0x80;

    IO_Out8(SERIAL_LCR_PORT, tmpLCR);
    IO_Out8(SERIAL_LSB_DIV_PORT, divisor & 0xFF);
    IO_Out8(SERIAL_MSB_DIV_PORT, divisor >> 8);

    IO_Out8(SERIAL_LCR_PORT, tmpLCR & 0x7F);
}


/*
 * Serial_SetBaudRate --
 *
 *    Set the baud rate to the given value.
 */

static void
Serial_SetBaudRate(uint32 rate)
{
    uint16 divisor = Serial_CalcBaudDivisor(rate);

    if (divisor == -1) {
        return;
    }

    Serial_SetBaudDivisor(divisor);
}


/*
 * SerialHandlerInternal --
 *
 *    Low-level serial interrupt handler. We read one byte from the
 *    serial port, and write it to our buffer. When the buffer is full
 *    we pass it on to any registered SerialIRQHandler.
 */

static void
SerialHandlerInternal(int vector)
{
    uint8 byte = Serial_ReadByte();

    if (gSerial.counter >= SERIAL_BUF_SIZE) {
        if (gSerial.handler) {
            gSerial.handler(gSerial.buffer);
        }

        gSerial.counter = 0;
    }

    gSerial.buffer[gSerial.counter++] = byte;
}


/*
 * Serial_ReadByte --
 *
 *    Reads one byte from the serial port.
 */

fastcall uint8
Serial_ReadByte(void)
{
    while (!(IO_In8(SERIAL_LSR_PORT) & SERIAL_STATUS_DR));
    return IO_In8(SERIAL_DATA_PORT);
}


/*
 * Serial_WriteByte --
 *
 *    Writes one byte to the serial port.
 */

fastcall void
Serial_WriteByte(uint8 byte)
{
    while (!(IO_In8(SERIAL_LSR_PORT) & SERIAL_STATUS_ETHR));
    IO_Out8(SERIAL_DATA_PORT, byte);
}


/*
 * Serial_Init --
 *
 *    Setup the serial driver. This initializes the serial device
 *    and installs our default IRQ handler. The IRQ module must be
 *    initialized before this is called.
 *
 *    As a side-effect, this will unmask the serial IRQ (COM1) and
 *    install a handler.
 */

fastcall void
Serial_Init(uint32 rate)
{
    IO_Out8(SERIAL_IER_PORT, FALSE);

    Serial_SetBaudRate(rate);

    // Use the 'default' 8N1 for bits, parity, stop
    uint8 serialMode = 3;

    IO_Out8(SERIAL_LCR_PORT, serialMode);

    // Setup FIFO trigger level = 8Bytes
    IO_Out8(SERIAL_IIR_PORT, 0x87);
    IO_Out8(SERIAL_MCR_PORT, 0xB);

    IO_Out8(SERIAL_IER_PORT, TRUE);

    gSerial.counter = 0;
    memset(gSerial.buffer, 0, SERIAL_BUF_SIZE);

    Intr_SetMask(SERIAL_IRQ_COM1, TRUE);
    Intr_SetHandler(IRQ_VECTOR(SERIAL_IRQ_COM1), SerialHandlerInternal);
}
