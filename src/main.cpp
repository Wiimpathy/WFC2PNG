/****************************************************************************
 * main.cpp
 *
 * WFC to PNG converter
 *
 * Convert a WiiFlow cache file to a PNG image.
 * The default input format is GX compressed textures (CMPR)
 * This format is using the S3TC DXT1 algorithm, a lossy compression.
 * 
 * Wiimpathy 2019
 *
 ***************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h> 
#include <sys/stat.h> 
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <algorithm>
#include <zlib.h>
#include "pngu.h"
#include "common.h"
#include "texture.h"

// Command line return codes
#define ARGV_OK              0
#define ARGV_MISSING_OPTION  1
#define ARGV_RANGE_OPTION    2
#define ARGV_UNKNOWN_OPTION  3
#define ARGV_PATH_NOT_FOUND  4
#define ARGV_FILE_TOO_LARGE  5
#define ARGV_FILE_UNKNOWN    6

char wfc_path[MAXPATHLEN];
char png_path[MAXPATHLEN];
size_t filesize = 0;

// PNG dimensions, max 1090*1090.
enum {Width, Height};
long ImgProperties[2];

// Texture used for the image conversion.
STexture TexHandle;


bool CheckNumber(char *nbr, int which)
{
	if (nbr)
	{
		int length = strlen(nbr);

		for (int i=0; i<length; i++)
		{
			if (!isdigit(nbr[i]))
			{
				printf ("Not a number!\n");
				return false;
			}
		}
	}
	else
	{
		printf ("Missing number!\n");
		return false;
	}

	ImgProperties[which-2] = strtol(nbr, NULL, 10);

	return true;
}

int MakePath(char* path)
{
	char cwd[MAXPATHLEN];
	char out_dir[4]="OUT";

	if(getcwd(cwd, sizeof(cwd)) == NULL)
	{
		printf("\nCan't get current directory!\n");
		return ARGV_PATH_NOT_FOUND;
	}

#ifdef WIN32
	mkdir(out_dir);
#else
	mkdir(out_dir,S_IRWXU);
#endif

	if(!DirExist(out_dir))
		return ARGV_PATH_NOT_FOUND;

	char *png_file = GetFilename(path);
	
	// Strip extension
	char *ext = NULL;
	ext = strrchr(png_file, '.');

	if (strcasecmp(ext, ".wfc") )
	{
		return ARGV_FILE_UNKNOWN;
	}

	size_t ext_length = strlen(ext);
	size_t path_length = strlen(png_file);
	png_file[path_length - ext_length] = 0;

	snprintf(png_path, sizeof(png_path), "%s%c%s%c%s.png", cwd, separator, out_dir, separator, png_file);

	return ARGV_OK;
}

static void Usage(const char *exename, int error, const char *arg_error)
{
	ClearScreen();

	fprintf(stderr, "\nWFC to PNG converter %s (Wiimpathy 2019)\n", VERSION_STR);

	if(error == ARGV_MISSING_OPTION)
	{
		fprintf(stderr, "\nERROR!!! Missing option: %s\n", arg_error);
	}
	else if(error == ARGV_RANGE_OPTION)
	{
		fprintf(stderr, "\nERROR!!! Number too large : %s\n", arg_error);
	}
	else if(error == ARGV_UNKNOWN_OPTION)
	{
		fprintf(stderr, "\nERROR!!! Unknown option : %s\n", arg_error);
	}
	else if(error == ARGV_PATH_NOT_FOUND)
	{
		fprintf(stderr, "\nERROR!!! Wrong path: %s\n", arg_error);
	}
	else if(error == ARGV_FILE_TOO_LARGE)
	{
		fprintf(stderr, "\nERROR!!! File too large : %s\n", arg_error);
	}
	else if(error == ARGV_FILE_UNKNOWN)
	{
		fprintf(stderr, "\nERROR!!! File not supported : %s\n", arg_error);
	}

	fprintf(stderr, "\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: %s Path Width Height \n", exename);
	fprintf(stderr, "\n");
	fprintf(stderr, "#Path#\n");
	fprintf(stderr, "  The path to the WiiFlow cache file.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "#Width/Height#\n");
	fprintf(stderr, " The width and height of the expected png image in pixels.\n");
	fprintf(stderr, "\n");
#ifdef WIN32
	fprintf(stderr, "Examples:\n");
	fprintf(stderr, "  %s \"e:\\WiiFlow\\cache\\snes9xgx\\Mr. Nutz.wfc\" 1090 680 \n", exename);
#else
	fprintf(stderr, "Examples:\n");
	fprintf(stderr, "  %s \"/WiiFlow/cache/snes9xgx/Mr. Nutz.wfc\" 1090 680 \n", exename);
#endif
	fprintf(stderr, "\n");
	exit(0);
}

static int HandleArguments(u32 argcount, char *argv[])
{
	int option = 1;

	if(argcount < 2)
		Usage(argv[0], ARGV_OK, "");

	if(argcount > 1 && (argcount < 3 || argcount < 4))
		Usage(argv[0], ARGV_MISSING_OPTION, "Width/Height");

	while(option < argcount)
	{
		if(option == 1)
		{
			strncpy(wfc_path, argv[1], sizeof(wfc_path));
			wfc_path[MAXPATHLEN] = '\0';
		}
		else if(option == 2 || option == 3)
		{
			if(!CheckNumber(argv[option], option))
				Usage(argv[0], ARGV_UNKNOWN_OPTION, argv[option]);
				
			if(ImgProperties[option-2] > 1090)
				Usage(argv[0], ARGV_RANGE_OPTION, argv[option]);

		}
		option++;
	}

	if(!FileExist(wfc_path, &filesize))
		Usage(argv[0], ARGV_PATH_NOT_FOUND, wfc_path);

	if(filesize > 2*MB)
		Usage(argv[0], ARGV_FILE_TOO_LARGE, "Max size 2MB.");

	int PathError = MakePath(wfc_path);

	if(PathError)
		Usage(argv[0], PathError, (PathError == ARGV_PATH_NOT_FOUND) ? "OUT folder not found." : "Not a wfc file!");
}


int main (int argc, char *argv[])
{
	// Check command lines options
	HandleArguments(argc, argv);

	// Convert wfc texture to png file
	printf("\nConverting : %s\n", wfc_path);
	int TexError = 0;
	TexError = TexHandle.CacheToPNG(wfc_path, filesize, png_path, ImgProperties[Width], ImgProperties[Height]);

	if(TexError)
	{
		printf("\n Error converting texture!!! Read log_wfc2png.txt. \n");
		
		// Open log file and append any errors
		char logfile[MAXPATHLEN];
		sprintf(logfile, "log_wfc2png.txt");
		FILE *log = NULL;

		log = fopen(logfile, "a");
		if (log == NULL)
		{
			printf("CacheToPNG Error! can't open log file %s.\n", logfile);
			fclose(log);
		}
		fprintf(log, "Error converting %s\n", wfc_path);
		fprintf(log, "Error code: %d\n", TexError);
		fclose(log);
	}
	else
	{
		printf("%s saved.\n\n", png_path);
	}
	
	return 0;
}
