#ifndef TEXTURE_H
#define TEXTURE_H

#include "common.h"

#define TEX_OPEN_ERROR        11
#define TEX_BUFFER_ALLOC      12
#define TEX_DECODE_ERROR      13
#define TEX_RESIZE_ALLOC      14



enum TexErr
{
	TE_OK = 0,
	TE_ERROR,
	TE_NOMEM
};

struct TexData
{
	TexData() : data(NULL), dataSize(0), width(0), height(0), format(-1), maxLOD(0), thread(false) { }
	u8 *data;
	u32 dataSize;
	u32 width;
	u32 height;
	u8 format;
	u8 maxLOD;
	bool thread;
//} ATTRIBUTE_PACKED; // FIXME
};


class STexture
{
public:
	int CacheToPNG(const char *wfc_file, size_t wfcsize, const char *png_file, int width, int height);
private:
	void _resize(u8 *dst, u32 dstWidth, u32 dstHeight, const u8 *src, u32 srcWidth, u32 srcHeight);
};


extern STexture TexHandle;

#endif
