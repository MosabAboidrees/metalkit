/* -*- Mode: C; c-basic-offset: 3 -*- */

#include "types.h"
#include "console_vga.h"
#include "console.h"
#include "intr.h"
#include "serial.h"

fastcall char
getChar(uint32 digit)
{
    char ch = '*';

    if (digit >= 0 && digit <= 9) {
        ch = digit + '0';
    }

    if (digit >= 0xA && digit <= 0xF) {
        ch = digit + 'A' - 0xA;
    }

    return ch;
}

fastcall void
srHandler(uint8 *buffer)
{
    uint32 i;
    uint32 addr;
    uint32 digit;

    addr = (uint32) buffer;

    // Display hex dump
    for (i = 0; i < SERIAL_BUF_SIZE; i++) {
        if (i == 0) {
            Console_WriteUInt32(addr, 8, '0', 16, FALSE);
        }

        Console_WriteChar(' ');

        digit = buffer[i] / 16;
        Console_WriteChar(getChar(digit));

        digit = buffer[i] % 16;
        Console_WriteChar(getChar(digit));
    }

    Console_WriteChar(' ');
    Console_WriteChar('|');

    // Display ASCII representation
    for (i = 0; i < SERIAL_BUF_SIZE; i++) {

        if (buffer[i] >= 0x20 && buffer[i] <= 0x7E) {
            Console_WriteChar(buffer[i]);
        } else {
            Console_WriteChar('.');
        }
    }

    Console_WriteChar('|');

    Console_WriteChar('\n');
    Console_Flush();
}

int
main(void)
{
   ConsoleVGA_Init();
   Intr_Init();
   Intr_SetFaultHandlers(Console_UnhandledFault);

   Serial_Init(SERIAL_BAUD_DEFAULT);
   Serial_SetHandler(srHandler);

   while (1) {
      Intr_Halt();
   }

   return 0;
}
