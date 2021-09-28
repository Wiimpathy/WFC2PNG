/********************************************************************************
 * texture.cpp
 *
 * WFC to PNG converter
 *
 * This file initially came from WiiFlow. 
 * The DXT1 decoding is adapted from different c# projects.
 *
 *******************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <algorithm>
#include <malloc.h>
#include <cmath>
#include <sys/stat.h> 
#include <sys/time.h> 
#include <stdbool.h>
#include <zlib.h>
#include "pngu.h"
#include "common.h"
#include "texture.h"

using namespace std;


static u16 Read16Swap(u8* data, unsigned int offset)
{
	return  (u16)( *(data + (int)offset + 1) << 8 | *(data + (int)offset ) );
}

static unsigned int Read32Swap(u8* data, unsigned int offset)
{
	return  (u32)( *(data + (int)offset + 3) << 24 | *(data + (int)offset + 2) << 16 | *(data + (int)offset + 1) << 8 |  *(data + (int)offset ) );
}

static u8 S3TC1ReverseByte(u8 b)
{
	u8 b1 = (u8)(b & 0x3);
	u8 b2 = (u8)(b & 0xC);
	u8 b3 = (u8)(b & 0x30);
	u8 b4 = (u8)(b & 0xC0);

	return (u8)((b1 << 6) | (b2 << 2) | (b3 >> 2) | (b4 >> 6));
}

static u32 upperPower(u32 width)
{
	u32 i = 8;
	u32 maxWidth = 1024;
	while (i < width && i < maxWidth)
		i <<= 1;
	return i;
}

void STexture::_resize(u8 *dst, u32 dstWidth, u32 dstHeight, const u8 *src, u32 srcWidth, u32 srcHeight)
{
	float wc = (float)srcWidth / (float)dstWidth;
	float hc = (float)srcHeight / (float)dstHeight;
	float ax1;
	float ay1;
	
	for (u32 y = 0; y < dstHeight; ++y)
	{
		for (u32 x = 0; x < dstWidth; ++x)
		{
			float xf = ((float)x + 0.5f) * wc - 0.5f;
			float yf = ((float)y + 0.5f) * hc - 0.5f;
			u32 x0 = (int)xf;
			u32 y0 = (int)yf;
			if (x0 >= srcWidth - 1)
			{
				x0 = srcWidth - 2;
				ax1 = 1.f;
			}
			else
				ax1 = xf - (float)x0;
			float ax0 = 1.f - ax1;
			if (y0 >= srcHeight - 1)
			{
				y0 = srcHeight - 2;
				ay1 = 1.f;
			}
			else
				ay1 = yf - (float)y0;
			float ay0 = 1.f - ay1;
			u8 *pdst = dst + (x + y * dstWidth) * 4;
			const u8 *psrc0 = src + (x0 + y0 * srcWidth) * 4;
			const u8 *psrc1 = psrc0 + srcWidth * 4;
			for (int c = 0; c < 3; ++c)
				pdst[c] = (u8)((((float)psrc0[c] * ax0) + ((float)psrc0[4 + c] * ax1)) * ay0 + (((float)psrc1[c] * ax0) + ((float)psrc1[4 + c] * ax1)) * ay1 + 0.5f);
			pdst[3] = 0xFF;	// Alpha not handled, it would require using it in the weights for color channels, easy but slower and useless so far.
		}
	}
}

static void RGB565ToRGBA8(u16 sourcePixel, u8* dest, int destOffset)
{
	u8 r, g, b;
	r = (u8)((sourcePixel & 0xF100) >> 11);
	g = (u8)((sourcePixel & 0x7E0) >> 5);
	b = (u8)((sourcePixel & 0x1F));

	r = (u8)((r << (8 - 5)) | (r >> (10 - 8)));
	g = (u8)((g << (8 - 6)) | (g >> (12 - 8)));
	b = (u8)((b << (8 - 5)) | (b >> (10 - 8)));

	dest[destOffset] = b;
	dest[destOffset + 1] = g;
	dest[destOffset + 2] = r;
	dest[destOffset + 3] = 0xFF; //Set alpha to 1
}

static u8* DecompressDxt1(u8* src, u32 width, u32 height)
{
	u32 dataOffset = 0;
	u8* finalData = (u8*)malloc(width * height * 4);

	if(finalData == NULL)
		return NULL;

	for (int y = 0; y < height; y += 4)
	{
		for (int x = 0; x < width; x += 4)
		{
			u16 color1 = Read16Swap(src, dataOffset);
			u16 color2 = Read16Swap(src, dataOffset + 2);
			u32 bits = Read32Swap(src, dataOffset + 4);
			dataOffset += 8;
			unsigned char ColorTable[4][4];

			RGB565ToRGBA8(color1, ColorTable[0], 0);
			RGB565ToRGBA8(color2, ColorTable[1], 0);
			
			if (color1 > color2)
			{
				ColorTable[2][0] = (u8)((2 * ColorTable[0][0] + ColorTable[1][0] + 1) / 3);
				ColorTable[2][1] = (u8)((2 * ColorTable[0][1] + ColorTable[1][1] + 1) / 3);
				ColorTable[2][2] = (u8)((2 * ColorTable[0][2] + ColorTable[1][2] + 1) / 3);
				ColorTable[2][3] = 0xFF;
				ColorTable[3][0] = (u8)((ColorTable[0][0] + 2 * ColorTable[1][0] + 1) / 3);
				ColorTable[3][1] = (u8)((ColorTable[0][1] + 2 * ColorTable[1][1] + 1) / 3);
				ColorTable[3][2] = (u8)((ColorTable[0][2] + 2 * ColorTable[1][2] + 1) / 3);
				ColorTable[3][3] = 0xFF;
			}
			else
			{
				ColorTable[2][0] = (u8)((ColorTable[0][0] + ColorTable[1][0] + 1) / 2);
				ColorTable[2][1] = (u8)((ColorTable[0][1] + ColorTable[1][1] + 1) / 2);
				ColorTable[2][2] = (u8)((ColorTable[0][2] + ColorTable[1][2] + 1) / 2);
				ColorTable[2][3] = 0xFF;

				ColorTable[3][0] = (u8)((ColorTable[0][0] + 2 * ColorTable[1][0] + 1) / 3);
				ColorTable[3][1] = (u8)((ColorTable[0][1] + 2 * ColorTable[1][1] + 1) / 3);
				ColorTable[3][2] = (u8)((ColorTable[0][2] + 2 * ColorTable[1][2] + 1) / 3);
				ColorTable[3][3] = 0x00;
			}

			for (int iy = 0; iy < 4; ++iy)
			{
				for (int ix = 0; ix < 4; ++ix)
				{
					if (((x + ix) < width) && ((y + iy) < height))
					{
						int di = (int)(4 * ((y + iy) * width + x + ix));
						int si = (int)(bits & 0x3);
						finalData[di + 0] = ColorTable[si][0];
						finalData[di + 1] = ColorTable[si][1];
						finalData[di + 2] = ColorTable[si][2];
						finalData[di + 3] = ColorTable[si][3];
					}
					bits >>= 2;
				}
			}
		}
	}

	return finalData;
}

u8* DecodeCmpr(u8* stream, u32 width, u32 height)
{
	u8 *buffer = (u8*)malloc(width * height * 4);

	for (int y = 0; y < height / 4; y += 2)
	{
		for (int x = 0; x < width / 4; x += 2)
		{
			for (int dy = 0; dy < 2; ++dy)
			{
				for (int dx = 0; dx < 2; ++dx)
				{
					if (4 * (x + dx) < width && 4 * (y + dy) < height)
					{
						memcpy(&buffer[ (8 * ((y + dy) * width / 4 + x + dx)) ], stream, 8);
						stream += 8;
					}
				}
			}
		}
	}

	for (int i = 0; i < width * height / 2; i += 8)
	{
		// Micro swap routine needed
		std::swap( buffer[i], buffer[i + 1] );
		std::swap( buffer[i + 2], buffer[i + 3] );

		buffer[i + 4] = S3TC1ReverseByte(buffer[i + 4]);
		buffer[i + 5] = S3TC1ReverseByte(buffer[i + 5]);
		buffer[i + 6] = S3TC1ReverseByte(buffer[i + 6]);
		buffer[i + 7] = S3TC1ReverseByte(buffer[i + 7]);
	}

	//Now decompress the DXT1 data within it.
	return DecompressDxt1(buffer, width, height);
}

int STexture::CacheToPNG(const char *wfcpath, size_t wfcsize, const char *png_file, int width, int height)
{
	int ret = 0;
	
	// Texture max dimensions used for decoding/resize.
	// The main cases here are 512*512 LQ and 1024*1024 HQ.
	u16 maxwidth = upperPower(width);
	u16 maxheight = upperPower(height);

	FILE *wfcfile = NULL;
	wfcfile = fopen(wfcpath, "rb");

	if(!wfcfile)
	{
		printf("\nCan't open %s !\n", wfcpath);
		return TEX_OPEN_ERROR;
	}

	// Read the raw wfc data image, skip the 14 bytes header.
	u8 *buffertex = (u8*)malloc(wfcsize);
	if(buffertex == NULL)
	{
		printf("\nError allocating texture!\n");
		return TEX_BUFFER_ALLOC;
	}
	fseek(wfcfile, 14, SEEK_SET);
	size_t result = fread(buffertex, wfcsize, 1, wfcfile);
	if (result < 0)
	{
		return TEX_OPEN_ERROR;	
	}
	fclose(wfcfile);

	IMGCTX ctx1 = PNGU_SelectImageFromDevice(png_file);

	// Decode GX CMPR to an RGB image
	u8* tmptexture = DecodeCmpr(buffertex, maxwidth, maxheight);

	if(tmptexture == NULL)
	{
		printf("\nError decoding CMPR texture!\n");
		return TEX_DECODE_ERROR;
	}

	// Resize and save the RGB image to PNG. 4MB should be more than enough.
	// Note that wfc higher LOD resolution is 1024*1024. 1090*680 will degrade quality.
	u32 pngsize = 4*MB;
	u8 *resizedtexture = (u8*)malloc(pngsize);
	if(resizedtexture == NULL)
	{
		printf("\nError allocating resized texture!\n");
		return TEX_RESIZE_ALLOC;
	}
	_resize(resizedtexture, width, height, tmptexture, maxwidth, maxheight);

	ret = PNGU_EncodeFromYCbYCr(ctx1, width, height, resizedtexture, 0);

	PNGU_ReleaseImageContext (ctx1);
	free(buffertex);
	free(resizedtexture);

	return ret;
}
