/* ------------------------------------------------------------------
 * Copyright (C) 2011 Martin Storsjo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>

#if defined(_MSC_VER)
#include <getopt.h>
#else
#include <unistd.h>
#endif

#include <stdlib.h>
#include "libAACdec/include/aacdecoder_lib.h"
#include "wav_file.h"

void usage(const char* name) {
	fprintf(stderr, "%s [-r bitrate] [-t aot] [-a afterburner] [-s sbr] [-v vbr] in.wav out.aac\n", name);
	fprintf(stderr, "Supported AOTs:\n");
	fprintf(stderr, "\t2\tAAC-LC\n");
	fprintf(stderr, "\t5\tHE-AAC\n");
	fprintf(stderr, "\t29\tHE-AAC v2\n");
	fprintf(stderr, "\t23\tAAC-LD\n");
	fprintf(stderr, "\t39\tAAC-ELD\n");
}

uint32_t get_data(uint8_t *sample_buffer, uint32_t frame_size, FILE *file)
{
	uint32_t samples_read = 0;
	
	samples_read = fread(sample_buffer, sizeof(char), frame_size, file);
	
	if(samples_read != frame_size)
	{
		printf("frame_size = %d\n", frame_size);
		printf("samples_read = %d\n", samples_read);
	}
	
	return samples_read;
}

int main(int argc, char *argv[]) 
{
	printf("fdk-aac_dabplus!\n");

	FILE *In_file;

	HANDLE_AACDECODER aacDec;
	CStreamInfo *aac_stream_info = NULL;  
	AAC_DECODER_ERROR error_code;
	TRANSPORT_TYPE transportFmt = TT_MP4_ADTS;
	const UINT nrOfLayers = 1;
	uint32_t isInFileEnd = 0;
	
	UINT pBytesValid = 8192;
	UINT input_size[1] = {8192};  // input buf size
	UCHAR *input_buf[1];   
	
	input_buf[0] = (UCHAR *)malloc(input_size[0]);  //heap input buf
	if(input_buf[0] == NULL)
	{
		printf("malloc input buffer fail!\n");
		return;
	}

	char *output_buf = (UCHAR *)malloc(8192); //heap output buf
	if(output_buf == NULL)
	{
		printf("malloc output buffer fail!\n");
		free(input_buf[0]);
		return;
	}

	INT_PCM *convert_buf = output_buf; //output buf

	#define IN_FILE_NAME  "E:\\wav\\out.aac"
	In_file = fopen(IN_FILE_NAME, "rb"); // input file
	if(!In_file)
	{
		printf("open in file fail!\n");	
		return;
	}

	#define OUT_FILE_NAME  "E:\\wav\\1.wav"
	
	HANDLE_WAV *pWav = (HANDLE_WAV *)malloc(sizeof(struct WAV));  //output file
	if(WAV_OutputOpen(pWav, OUT_FILE_NAME, 44100, 2, 16) == -1)
	{
		printf("open wav file fail!\n");
		return;
	}
	aacDec = aacDecoder_Open(transportFmt, nrOfLayers);
	if(aacDec == NULL)
	{
		printf("init decoder fail!\n");
		return -1;
	}

	/* 1- update external input buffer first */	
	isInFileEnd = get_data(input_buf[0], 8192, In_file);
	printf("first read bytes = %d\n",isInFileEnd);
	printf("begin to decode!\n");

	/* 2- decode loop */
    while(isInFileEnd)
    {
    	/* 3- fill the decoder internal buffer by the external buffer */
		error_code = aacDecoder_Fill(aacDec, input_buf, input_size, &pBytesValid);
//		printf("fill_error_code = %#4x\n", error_code);
		if(error_code == 0)
		{
			/* 4- decode aac frame */
			error_code = aacDecoder_DecodeFrame(aacDec, convert_buf, 8192, 0);	
//			printf("dec_error_code = %#4x\n", error_code);
		}
		else
		{
			printf("fill buffer err %d\n", error_code);
			break;
		}

		if(error_code == 0) /* 5- write pcm data to a file */
		{
			aac_stream_info = aacDecoder_GetStreamInfo(aacDec);  
			uint32_t convert_size = aac_stream_info->numChannels*aac_stream_info->frameSize*sizeof(INT_PCM);
			short *pcmBuf = convert_buf;
			WAV_OutputWrite(*pWav, pcmBuf, 2048, 16, 0);
		}
		else
		{
			printf("dec_error_code = %#x\n", error_code);
		}

		if(pBytesValid == 0) /* 6- update the external input buffer */
		{
//			printf("update input buf!\n");
			isInFileEnd = get_data(input_buf[0], 8192, In_file);
			pBytesValid = isInFileEnd;
		}	
	}


	/* 7- close the decoder and files */
	if(input_buf[0] != NULL)
	{
		free(input_buf[0]);
	}
	
	aacDecoder_Close(aacDec);
	fclose(In_file);
	WAV_OutputClose(pWav);

	printf("decode end!\n");
	return 0;

}


