/*  Copyright 2005 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

/*! \file memory.c
    \brief Memory access functions.
*/

#ifdef PSP  // see FIXME in T1MemoryInit()
# include <stdint.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>

#include "memory.h"
#include "debug.h"
#include "error.h"
#include "m68kcore.h"
#include "scsp.h"
#include "scspdsp.h"
#include "yabause.h"
#include "yui.h"

#ifdef HAVE_LIBGL
#define USE_OPENGL
#endif

#ifdef USE_OPENGL
#include "ygl.h"
#endif


//////////////////////////////////////////////////////////////////////////////

writebytefunc WriteByteList[0x1000];
writewordfunc WriteWordList[0x1000];
writelongfunc WriteLongList[0x1000];

readbytefunc ReadByteList[0x1000];
readwordfunc ReadWordList[0x1000];
readlongfunc ReadLongList[0x1000];

u8 *HighWram;
u8 *LowWram;
u8 *BiosRom;
u8 *BupRam;

/* This flag is set to 1 on every write to backup RAM.  Ports can freely
 * check or clear this flag to determine when backup RAM has been written,
 * e.g. for implementing autosave of backup RAM. */
u8 BupRamWritten;

M68K_struct * M68KCoreList[] = {
	&M68KDummy,
	&M68KC68K,
	NULL
};

SoundInterface_struct *SNDCoreList[] = {
	&SNDDummy,
	NULL
};

ScspDsp scsp_dsp;

//////////////////////////////////////////////////////////////////////////////

u8 * T1MemoryInit(u32 size)
{
#ifdef PSP  // FIXME: could be ported to all arches, but requires stdint.h
            //        for uintptr_t
   u8 * base;
   u8 * mem;

   if ((base = calloc((size * sizeof(u8)) + sizeof(u8 *) + 64, 1)) == NULL)
      return NULL;

   mem = base + sizeof(u8 *);
   mem = mem + (64 - ((uintptr_t) mem & 63));
   *(u8 **)(mem - sizeof(u8 *)) = base; // Save base pointer below memory block

   return mem;
#else
   return calloc(size, sizeof(u8));
#endif
}

//////////////////////////////////////////////////////////////////////////////

void T1MemoryDeInit(u8 * mem)
{
#ifdef PSP
   if (mem)
      free(*(u8 **)(mem - sizeof(u8 *)));
#else
   free(mem);
#endif
}

//////////////////////////////////////////////////////////////////////////////

T3Memory * T3MemoryInit(u32 size)
{
   T3Memory * mem;

   if ((mem = (T3Memory *) calloc(1, sizeof(T3Memory))) == NULL)
      return NULL;

   if ((mem->base_mem = (u8 *) calloc(size, sizeof(u8))) == NULL)
   {
      free(mem);
      return NULL;
   }

   mem->mem = mem->base_mem + size;

   return mem;
}

//////////////////////////////////////////////////////////////////////////////

void T3MemoryDeInit(T3Memory * mem)
{
   free(mem->base_mem);
   free(mem);
}

//////////////////////////////////////////////////////////////////////////////

Dummy * DummyInit(UNUSED u32 s)
{
   return NULL;
}

//////////////////////////////////////////////////////////////////////////////

void DummyDeInit(UNUSED Dummy * d)
{
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL UnhandledMemoryReadByte(USED_IF_DEBUG u32 addr)
{
   LOG("Unhandled byte read %08X\n", (unsigned int)addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL UnhandledMemoryReadWord(USED_IF_DEBUG u32 addr)
{
   LOG("Unhandled word read %08X\n", (unsigned int)addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL UnhandledMemoryReadLong(USED_IF_DEBUG u32 addr)
{
   LOG("Unhandled long read %08X\n", (unsigned int)addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL UnhandledMemoryWriteByte(USED_IF_DEBUG u32 addr, UNUSED u8 val)
{
   LOG("Unhandled byte write %08X\n", (unsigned int)addr);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL UnhandledMemoryWriteWord(USED_IF_DEBUG u32 addr, UNUSED u16 val)
{
   LOG("Unhandled word write %08X\n", (unsigned int)addr);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL UnhandledMemoryWriteLong(USED_IF_DEBUG u32 addr, UNUSED u32 val)
{
   LOG("Unhandled long write %08X\n", (unsigned int)addr);
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL HighWramMemoryReadByte(u32 addr)
{
   return T2ReadByte(HighWram, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL HighWramMemoryReadWord(u32 addr)
{
   return T2ReadWord(HighWram, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL HighWramMemoryReadLong(u32 addr)
{
   return T2ReadLong(HighWram, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL HighWramMemoryWriteByte(u32 addr, u8 val)
{
   T2WriteByte(HighWram, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL HighWramMemoryWriteWord(u32 addr, u16 val)
{
   T2WriteWord(HighWram, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL HighWramMemoryWriteLong(u32 addr, u32 val)
{
   T2WriteLong(HighWram, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL LowWramMemoryReadByte(u32 addr)
{
   return T2ReadByte(LowWram, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL LowWramMemoryReadWord(u32 addr)
{
   return T2ReadWord(LowWram, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL LowWramMemoryReadLong(u32 addr)
{
   return T2ReadLong(LowWram, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL LowWramMemoryWriteByte(u32 addr, u8 val)
{
   T2WriteByte(LowWram, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL LowWramMemoryWriteWord(u32 addr, u16 val)
{
   T2WriteWord(LowWram, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL LowWramMemoryWriteLong(u32 addr, u32 val)
{
   T2WriteLong(LowWram, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL BiosRomMemoryReadByte(u32 addr)
{
   return T2ReadByte(BiosRom, addr & 0x7FFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL BiosRomMemoryReadWord(u32 addr)
{
   return T2ReadWord(BiosRom, addr & 0x7FFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL BiosRomMemoryReadLong(u32 addr)
{
   return T2ReadLong(BiosRom, addr & 0x7FFFF);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL BiosRomMemoryWriteByte(UNUSED u32 addr, UNUSED u8 val)
{
   // read-only
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL BiosRomMemoryWriteWord(UNUSED u32 addr, UNUSED u16 val)
{
   // read-only
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL BiosRomMemoryWriteLong(UNUSED u32 addr, UNUSED u32 val)
{
   // read-only
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL BupRamMemoryReadByte(u32 addr)
{
   return T1ReadByte(BupRam, addr & 0xFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL BupRamMemoryReadWord(USED_IF_DEBUG u32 addr)
{
   LOG("bup\t: BackupRam read word - %08X\n", addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL BupRamMemoryReadLong(USED_IF_DEBUG u32 addr)
{
   LOG("bup\t: BackupRam read long - %08X\n", addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL BupRamMemoryWriteByte(u32 addr, u8 val)
{
   T1WriteByte(BupRam, (addr & 0xFFFF) | 0x1, val);
   BupRamWritten = 1;
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL BupRamMemoryWriteWord(USED_IF_DEBUG u32 addr, UNUSED u16 val)
{
   LOG("bup\t: BackupRam write word - %08X\n", addr);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL BupRamMemoryWriteLong(USED_IF_DEBUG u32 addr, UNUSED u32 val)
{
   LOG("bup\t: BackupRam write long - %08X\n", addr);
}

//////////////////////////////////////////////////////////////////////////////

static void FillMemoryArea(unsigned short start, unsigned short end,
                           readbytefunc r8func, readwordfunc r16func,
                           readlongfunc r32func, writebytefunc w8func,
                           writewordfunc w16func, writelongfunc w32func)
{
   int i;

   for (i=start; i < (end+1); i++)
   {
      ReadByteList[i] = r8func;
      ReadWordList[i] = r16func;
      ReadLongList[i] = r32func;
      WriteByteList[i] = w8func;
      WriteWordList[i] = w16func;
      WriteLongList[i] = w32func;
   }
}

//////////////////////////////////////////////////////////////////////////////

static int MappedMemoryAddMatch(u32 addr, u32 val, int searchtype, result_struct *result, u32 *numresults)
{
   result[numresults[0]].addr = addr;
   result[numresults[0]].val = val;
   numresults[0]++;
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE int SearchIncrementAndCheckBounds(result_struct *prevresults,
                                                u32 *maxresults,
                                                u32 numresults, u32 *i,
                                                u32 inc, u32 *newaddr,
                                                u32 endaddr)
{
   if (prevresults)
   {
      if (i[0] >= maxresults[0])
      {
         maxresults[0] = numresults;
         return 1;
      }
      newaddr[0] = prevresults[i[0]].addr;
      i[0]++;
   }
   else
   {
      newaddr[0] = inc;

      if (newaddr[0] > endaddr || numresults >= maxresults[0])
      {
         maxresults[0] = numresults;
         return 1;
      }
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static int SearchString(u32 startaddr, u32 endaddr, int searchtype,
                        const char *searchstr, result_struct *results,
                        u32 *maxresults)
{
   u8 *buf=NULL;
   u32 *buf32=NULL;
   u32 buflen=0;
   u32 counter;
   u32 addr;
   u32 numresults=0;

   buflen=(u32)strlen(searchstr);

   if ((buf32=(u32 *)malloc(buflen*sizeof(u32))) == NULL)
      return 0;

   buf = (u8 *)buf32;

   // Copy string to buffer
   switch (searchtype & 0x70)
   {
      case SEARCHSTRING:
         strcpy((char *)buf, searchstr);
         break;
      case SEARCHREL8BIT:
      case SEARCHREL16BIT:
      {
         char *text;
         char *searchtext=strdup(searchstr);

         // Calculate buffer length and read values into table
         buflen = 0;
         for (text=strtok((char *)searchtext, " ,"); text != NULL; text=strtok(NULL, " ,"))
         {            
            buf32[buflen] = strtoul(text, NULL, 0);
            buflen++;
         }
         free(searchtext);

         break;
      }
   }    

   addr = startaddr;
   counter = 0;

   for (;;)
   {
      // Fetch byte/word/etc.
      switch (searchtype & 0x70)
      {
         case SEARCHSTRING:
         {
            u8 val = MappedMemoryReadByte(addr);
            addr++;

            if (val == buf[counter])
            {
               counter++;
               if (counter == buflen)
                  MappedMemoryAddMatch(addr-buflen, val, searchtype, results, &numresults);
            }
            else
               counter = 0;
            break;
         }
         case SEARCHREL8BIT:
         {
            int diff;
            u32 j;
            u8 val2;
            u8 val = MappedMemoryReadByte(addr);

            for (j = 1; j < buflen; j++)
            {
               // grab the next value
               val2 = MappedMemoryReadByte(addr+j);

               // figure out the diff
               diff = (int)val2 - (int)val;

               // see if there's a match             
               if (((int)buf32[j] - (int)buf32[j-1]) != diff)
                  break;

               if (j == (buflen - 1))
                  MappedMemoryAddMatch(addr, val, searchtype, results, &numresults);

               val = val2;
            }

            addr++;

            break;
         }
         case SEARCHREL16BIT:
         {
            int diff;
            u32 j;
            u16 val2;
            u16 val = MappedMemoryReadWord(addr);

            for (j = 1; j < buflen; j++)
            {
               // grab the next value
               val2 = MappedMemoryReadWord(addr+(j*2));

               // figure out the diff
               diff = (int)val2 - (int)val;

               // see if there's a match             
               if (((int)buf32[j] - (int)buf32[j-1]) != diff)
                  break;

               if (j == (buflen - 1))
                  MappedMemoryAddMatch(addr, val, searchtype, results, &numresults);

               val = val2;
            }

            addr+=2;
            break;
         }
      }    

      if (addr > endaddr || numresults >= maxresults[0])
         break;
   }

   free(buf);
   maxresults[0] = numresults;
   return 1;
}

//////////////////////////////////////////////////////////////////////////////
