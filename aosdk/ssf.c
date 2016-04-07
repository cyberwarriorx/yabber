/*
	Audio Overload SDK - main driver.  for demonstration only, not user friendly!

	Copyright (c) 2007-2009 R. Belmont and Richard Bannister.

	All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
	* Neither the names of R. Belmont and Richard Bannister nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
	CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
	EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
	PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
	LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ao.h"
#include "eng_protos.h"

/* file types */
static uint32 type;

/* ao_get_lib: called to load secondary files */
int ao_get_lib(char *filename, uint8 **buffer, uint64 *length)
{
	uint8 *filebuf;
	uint32 size;
	FILE *auxfile;

	auxfile = fopen(filename, "rb");
	if (!auxfile)
	{
		printf("Unable to find auxiliary file %s\n", filename);
		return AO_FAIL;
	}

	fseek(auxfile, 0, SEEK_END);
	size = ftell(auxfile);
	fseek(auxfile, 0, SEEK_SET);

	filebuf = malloc(size);

	if (!filebuf)
	{
		fclose(auxfile);
		printf("ERROR: could not allocate %d bytes of memory\n", size);
		return AO_FAIL;
	}

	fread(filebuf, size, 1, auxfile);
	fclose(auxfile);

	*buffer = filebuf;
	*length = (uint64)size;

	return AO_SUCCESS;
}

int load_ssf(char *filename)
{
	long size;
	FILE *fp = fopen(filename, "rb");
	unsigned char *buffer;
	int ret;

	if (!fp)
		return FALSE;

	// Get file size
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	buffer = (unsigned char *)malloc(size);

	if (buffer == NULL)
	{
		fclose(fp);
		return FALSE;
	}

	fread(buffer, 1, size, fp);
	fclose(fp);

	// Read ID
	if (buffer[0] != 0x50 || buffer[1] != 0x53 ||
		buffer[2] != 0x46 || buffer[3] != 0x11)
	{
		// Can't identify file
		return FALSE;
	}

	if ((ret = ssf_start(buffer, size)) != AO_SUCCESS)
	{
		free(buffer);
		return ret;
	}

	return TRUE;
}
