/*
	debanner
	Copyright (C) 2014 Joseph LoManto

	This software is provided 'as-is', without any express or implied 
	warranty. In no event will the authors be held liable for any damages 
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose, 
	including commercial applications, and to alter it and redistribute it 
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not 
	   claim that you wrote the original software. If you use this software 
	   in a product, an acknowledgment in the product documentation would be 
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be 
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define CBMD_HEADER_SIZE	0x88
#define CBMD_HEADER_SIZE_WORDS	0x22
#define MAGIC(c1,c2,c3,c4) ((c1)|(c2)<<8|(c3)<<16|(c4)<<24)

void usage()
{
	printf(
		"debanner\n"
		"Separates the CBMD and BCWAV files from a 3DS banner file.\n\n"
		"Usage:\n"
		"\tdebanner input.bnr\n"
		);
}

char* replaceFileType(const char* filename,const char* type)
{
	char* str;
	size_t len = strlen(filename);
	size_t typelen = strlen(type);

	size_t i,rootlen;
	for(i = len; i > 0; i--)
	{
		if(filename[i] == '.')
		{
			rootlen = i;
			break;
		}
	}
	/* if there is no file type
	then just append the type to
	the end */
	if(rootlen == 0) rootlen = len;

	str = (char*)malloc(rootlen + typelen + 1);
	if(str == NULL) /* better to be safe than sorry */
	{
		fprintf(stderr,"ERROR: Unable to allocate %d bytes for file name\n");
		return NULL;
	}

	memcpy(str,filename,rootlen);
	strcpy(&str[rootlen],type);

	return str;
}

int main(int argc,char* argv[])
{
	FILE *in,*out;
	const char* infilename = argv[1];
	char* outfilename;

	unsigned int cbmdheader[CBMD_HEADER_SIZE_WORDS];
	char* buffer;
	size_t filelen,cbmdlen,buflen;

	if(argc < 2)
	{
		usage();
		return 0;
	}

	/* open input banner file */
	if((in = fopen(infilename,"rb")) == NULL)
	{
		fprintf(stderr,"ERROR: Unable to open input '%s'\n",infilename);
		return 1;
	}

	fseek(in,0,SEEK_END);
	filelen = ftell(in);
	fseek(in,0,SEEK_SET);

	fread(cbmdheader,1,CBMD_HEADER_SIZE,in);
	/* integrity check */
	if(cbmdheader[0] != MAGIC('C','B','M','D'))
	{
		fprintf(stderr,"ERROR: banner file is corrupt!\n");
		fclose(in);
		return 1;
	}

	cbmdlen = cbmdheader[0x21];
	if(cbmdlen > filelen)
	{
		fprintf(stderr,"ERROR: cbmd length is longer than the file length\n");
		fclose(in);
		return 1;
	}

	buflen = cbmdlen - CBMD_HEADER_SIZE;
	buffer = (char*)malloc(buflen);

	fread(buffer,1,buflen,in);

	/* write out to CBMD file */
	if((outfilename = replaceFileType(infilename,".cbmd")) == NULL)
		return 1;

	if((out = fopen(outfilename,"wb")) == NULL)
	{
		fprintf(stderr,"ERROR: Unable to open cbmd '%s'\n",outfilename);
		free(outfilename);
		free(buffer);
		return 1;
	}

	fwrite(cbmdheader,1,CBMD_HEADER_SIZE,out);
	fwrite(buffer,1,buflen,out);

	fclose(out);
	free(outfilename);
	free(buffer);

	/* read BCWAV (directly follows CBMD in banner)*/
	buflen = filelen - cbmdlen;
	buffer = (char*)malloc(buflen);

	fread(buffer,1,buflen,in);

	fclose(in);

	/* integrity check */
	if(*(unsigned int*)buffer != MAGIC('C','W','A','V'))
	{
		fprintf(stderr,"ERROR: banner's bcwav file is corrupt!\n");
		return 1;
	}

	/* write out to BCWAV file */
	if((outfilename = replaceFileType(infilename,".bcwav")) == NULL)
		return 1;

	if((out = fopen(outfilename,"wb")) == NULL)
	{
		fprintf(stderr,"ERROR: Unable to open bcwav '%s'\n",outfilename);
		free(outfilename);
		free(buffer);
		return 1;
	}

	fwrite(buffer,1,buflen,out);

	fclose(out);
	free(outfilename);
	free(buffer);

	return 0;
}
