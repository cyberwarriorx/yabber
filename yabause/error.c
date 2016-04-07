/*  Copyright 2005-2006 Theo Berkau

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

/*! \file error.c
    \brief Error handling functions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "error.h"
#include "yui.h"

//////////////////////////////////////////////////////////////////////////////

static void AllocAmendPrintString(const char *string1, const char *string2)
{
   char *string;

   if ((string = (char *)malloc(strlen(string1) + strlen(string2) + 2)) == NULL)
      return;

   sprintf(string, "%s%s\n", string1, string2);
   YuiErrorMsg(string);

   free(string);
}

//////////////////////////////////////////////////////////////////////////////

void YabSetError(int type, const void *extra)
{
   char tempstr[512];

   switch (type)
   {
      case YAB_ERR_FILENOTFOUND:
         AllocAmendPrintString(_("File not found: "), extra);
         break;
      case YAB_ERR_MEMORYALLOC:
         YuiErrorMsg(_("Error allocating memory\n"));
         break;
      case YAB_ERR_FILEREAD:
         AllocAmendPrintString(_("Error reading file: "), extra);
         break;
      case YAB_ERR_FILEWRITE:
         AllocAmendPrintString(_("Error writing file: "), extra);
         break;
      case YAB_ERR_CANNOTINIT:
         AllocAmendPrintString(_("Cannot initialize "), extra);
         break;
      case YAB_ERR_SH2READ:
         YuiErrorMsg(_("SH2 read error\n")); // fix me
         break;
      case YAB_ERR_SH2WRITE:
         YuiErrorMsg(_("SH2 write error\n")); // fix me
         break;
      case YAB_ERR_SDL:
         AllocAmendPrintString(_("SDL Error: "), extra);
         break;
      case YAB_ERR_OTHER:
         YuiErrorMsg((char *)extra);
         break;
      case YAB_ERR_UNKNOWN:
      default:
         YuiErrorMsg(_("Unknown error occurred\n"));
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void YabErrorMsg(const char * format, ...) {
    va_list l;
    int n;
    char * buffer;

    va_start(l, format);
    n = vsnprintf(NULL, 0, format, l);
    va_end(l);

    buffer = malloc(n + 1);

    va_start(l, format);
    vsprintf(buffer, format, l);
    va_end(l);

    YuiErrorMsg(buffer);
    free(buffer);
}

//////////////////////////////////////////////////////////////////////////////

void YuiErrorMsg(const char *string)
{

}