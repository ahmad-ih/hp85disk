/**
 @file gpib/lifutils.c

 @brief LIF file utilities

 @par Edit History
 - [1.0]   [Mike Gore]  Initial revision of file.

 @par Copyright &copy; 2014-2017 Mike Gore, Inc. All rights reserved.

*/

#include "user_config.h"

#include "defines.h"
#include "drives.h"
#include <time.h>
#include "lifutils.h"


/// @brief
///  Help Menu for User invoked GPIB functions and tasks
///  See: int gpib_tests(char *str)
/// @return  void
void lif_help()
{
    printf(
		"lifhelp\n"
        "lifadd lifimage lifname file\n"
        "lifaddbin lifimage lifname file\n"
        "lifcreate lifimage label directory_sectors sectors\n"
        "lifdel\n"
        "lifdir\n"
        "lifextract lifimage lifname file\n"
        "lifextractbin lifimage lifname file\n"
        "lifrename old new\n"
        );
}

/// @brief LIF user tests
/// @return  1 matched token, 0 if not
int lif_tests(char *str)
{

    int len;
    char *ptr;

    ptr = skipspaces(str);

    if ((len = token(ptr,"lifhelp")) )
	{
		lif_help();
		return(1);
	}
    if ((len = token(ptr,"lifaddbin")) )
    {
        char name[64];
        char lifname[64];
        char user[64];

        ptr += len;

        // LIF name
        ptr = get_token(ptr, name, 63);

        // lif file name
        ptr = get_token(ptr, lifname, 63);

        // User file name
        ptr = get_token(ptr, user, 63);

		lif_add_lif_file(name, lifname, user);

        return(1);
    }
    if ((len = token(ptr,"lifadd")) )
    {
        char name[64];
        char lifname[64];
        char user[64];

        ptr += len;

        // LIF name
        ptr = get_token(ptr, name, 63);

        // lif file name
        ptr = get_token(ptr, lifname, 63);

        // User file name
        ptr = get_token(ptr, user, 63);

		lif_add_ascii_file_as_e010(name, lifname, user);

        return(1);
    }
    if ((len = token(ptr,"lifdel")) )
    {
        char name[64];
        char lifname[64];

        ptr += len;

        // LIF name
        ptr = get_token(ptr, name, 63);

        // lif file name
        ptr = get_token(ptr, lifname, 63);

		lif_del_file(name, lifname);

        return(1);
    }
    if ((len = token(ptr,"lifcreate")) )
    {
        char name[64],label[6];
        char num[12];
        long dirsectors, sectors, result;

        ptr += len;

        // LIF name
        ptr = get_token(ptr, name, 63);

        // LIF LABEL
        ptr = get_token(ptr, label, 7);

        // Directory Sectors
        ptr = get_token(ptr, num, 11);
        dirsectors = atol(num);

        // Image total Sectors
        ptr = get_token(ptr, num, 11);
        sectors= atol(num);

        ///@brief format LIF image
        result = lif_create_image(name,label,dirsectors,sectors);
        if(result != sectors)
        {
            if(debuglevel & 1)
                printf("create_format_image: failed\n");
        }
        return(1);
    }
    else if ((len = token(ptr,"lifdir")) )
    {
        char name[64];
        ptr += len;
        // LIF name
        ptr = get_token(ptr, name, 63);
        lif_dir(name);
        return(1);
    }
    if ((len = token(ptr,"lifextractbin")) )
	{

        char name[64];
        char lifname[64];
        char user[64];

        ptr += len;

        // LIF name
        ptr = get_token(ptr, name, 63);

        // lif file name
        ptr = get_token(ptr, lifname, 63);

        // User file name
        ptr = get_token(ptr, user, 63);

		lif_extract_lif_as_lif(name, lifname, user);
        return(1);
	}
    if ((len = token(ptr,"lifextract")) )
	{

        char name[64];
        char lifname[64];
        char user[64];

        ptr += len;

        // LIF name
        ptr = get_token(ptr, name, 63);

        // lif file name
        ptr = get_token(ptr, lifname, 63);

        // User file name
        ptr = get_token(ptr, user, 63);

		lif_extract_e010_as_ascii(name, lifname, user);

        return(1);
	}
    if ((len = token(ptr,"lifrename")) )
    {
        char name[64];
        char oldlifname[64];
        char newlifname[64];

        ptr += len;

        // LIF name
        ptr = get_token(ptr, name, 63);

        // old lif file name
        ptr = get_token(ptr, oldlifname, 63);

        // new lif file name
        ptr = get_token(ptr, newlifname, 63);

		lif_rename_file(name, oldlifname, newlifname);

        return(1);
    }
	return(0);
}


/// @brief Allocate and clear memory 
/// Displays message on errors
/// @param[in] size: size of memory to allocate
/// @return pointer to allocated memory
void *lif_calloc(long size)
{
	uint8_t *p = safecalloc(size,1);
	if(!p)
		printf("lif_calloc:[%ld] not enough free memory\n", size);
	return(p);
}

/// @brief Free allocated memory
/// Displays message on errors
/// @param[in] p: pointer to memory to free
/// @return pointer to allocated memory
void lif_free(uint8_t *p)
{
	if(!p)
		printf("lif_free: NULL pointer\n");
	else	
		safefree(p);
}

/// @brief Allocate and copy a string
/// Displays message on errors
/// @param[in] str: String to allocate and copy
/// @return pointer to allocated memory with string copy
void *lif_stralloc(char *str)
{
	int len = strlen(str);
	char *p = (char *)lif_calloc(len+1);
	if(!p)
		return(NULL);
	strcpy(p,str);
	return(p);
}

/// @brief Open a file that must exist
/// Displays message on errors
/// @param[in] *name: file name of LIF image
/// @param[in] *mode: open mode - see fopen
/// @return FILE * pointer
FILE *lif_open(char *name, char *mode)
{
    FILE *fp = fopen(name, mode);
    if( fp == NULL)
    {
		printf("lif_open: Can't open:[%s] mode:[%s]\n", name, mode);
        return(NULL);
    }
	return(fp);
}

/// @brief Stat a file 
/// Displays message on errors
/// @param[in] *name: file name of LIF image
/// @param[in] *p: start_t structure pointer for result
/// @return NULL on error or copy of p
stat_t *lif_stat(char *name, stat_t *p)
{
	if(stat(name, p) < 0)
	{
		printf("lif_stat: Can't stat:%s\n", name);
        return(NULL);
	}
	return(p);
}

/// @brief File seek with error message
/// Displays message on errors
/// @param[in] *fp: FILE pointer
/// @param[in] offset: file offset
/// @param[in] mesg: user string as part of error message
/// @return 1 on success, 0 on error
int lif_seek_msg(FILE *fp, long offset, char *msg)
{
	if(ftell(fp) != offset)
	{
		if(fseek(fp, offset, SEEK_SET) < 0)
		{
			printf("lif_read_msg: %s Seek error %ld\n", msg, offset);
			return(0);
		}
	}
	return(1);
}

/// @brief Read data from a LIF image 
/// Displays message on errors
/// @param[in] *LIF: lif_t structure with file pointers
/// @param[out] *buf: read buffer
/// @param[in] offset: read offset
/// @param[in] bytes: number of bytes to read
/// @return bytes read or 0 on error
long lif_read(lif_t *LIF, void *buf, long offset, int bytes)
{
	long len;

	if(!lif_seek_msg(LIF->fp,offset,LIF->name))
		return(0);

	///@brief Initial file position
	len = fread(buf, 1, bytes, LIF->fp);
	if( len != bytes)
	{
		if(debuglevel & 1)
			printf("lif_read: Read error %s @ %ld\n", LIF->name, offset);
	}
	return(len);
}

/// @brief Write data to an LIF image 
/// Displays message on errors
/// @param[in] *LIF: lif_t structure with file pointers
/// @param[in] *buf: write buffer
/// @param[in] offset: write offset
/// @param[in] bytes: number of bytes to write
/// @return bytes read or 0 on error
int lif_write(lif_t *LIF, void *buf, long offset, int bytes)
{
	long len;

	// Seek to write position
	if(!lif_seek_msg(LIF->fp, offset,LIF->name))
		return(0);

	///@brief Initial file position
	len = fwrite(buf, 1, bytes, LIF->fp);
	if( len != bytes)
	{
		if(debuglevel & 1)
			printf("lif_write: Writeerror %s @ %ld\n", LIF->name, offset);
	}
	return(len);
}



/// @brief Check if characters in a LIF volume or LIF file name are valid
/// @param[in] c: character to test
/// @param[in] index: index of character in volume or file name
/// @retrun c (optionally upper cased) or 0 if no match
int lif_chars(int c, int index)
{
	if(c == ' ')
		return(c);
	if(c >= 'a' && c <= 'z')
		return(c-0x20);
	if(c >= 'A' && c <= 'Z')
		return(c);
	if((index > 0) && (c >= '0' && c <= '9') )
		return(c);
	if((index > 0) && (( c == '_') || c == '-'))
		return(c);
	return(0);
}

/// @brief Convert LIF space padded string name into normal string
/// @param[in] *B: LIF name space padded
/// @param[out] *name: string result with traling spaces removed
/// @param[in] size: max size of name
/// @retrun 1 if string i ok or 0 if bad characters were found
int lif_B2S(uint8_t *B, uint8_t *name, int size)
{
    int i;
	int status = 1;
	for(i=0;i<size;++i)
	{
		if( !lif_chars(B[i],i))
			status = 0;
	}
	for(i=0;i<size;++i)
		name[i] = B[i];
	name[i] = 0;
	// remove space padding
	trim_tail((char *)name);
	return(status);
}

/// @brief Check volume if name or directory name is valid
/// @param[in] *name: name to test
/// @retrun 1 if the string is ok or 0 if invalid LIF name characters on input string
int lif_checkname(char *name)
{
	int i;
	int status = 1;
    for(i=0;name[i];++i)
	{
		if(!lif_chars(name[i],i))
			status = 0;
	}
	return(status);
}

/// @brief Convert string to LIF directory record
/// @param[out] *B: LIF result with added trailing spaces
/// @param[in] *name: string
/// @param[in] size: max size of B
/// @retrun 1 if the string is ok or 0 if invalid LIF name characters on input string
void lif_S2B(uint8_t *B, uint8_t *name, int size)
{
    int i;
    for(i=0;name[i] && i<size;++i)
	{
		B[i] = name[i];
	}
    for(;i<size;++i)
        B[i] = ' ';
}


///@brief Convert name into a valid LIF name 
/// Only use the basename() part of the string and remove any file name extentions
/// LIF names may have only these characters: [A-Z][A-Z0-09_]+
/// LIF names are converted to upper case
/// LIF names are padded at the end with spaces
/// Any invalid input characters are converted into spaces
///@param[out] *B: output LIF string
///@param[in] *name: input string
///@param[in] size: maximum size of output string
///@return length of result
int lif_fixname(uint8_t *B, char *name, int size)
{
	uint8_t c,ret;
	int i,index;
	char *ptr;
	uint8_t *save = B;

	index = 0;
	// remove any "/"
	ptr = basename(name);

	for(i=0; ptr[i] && index < size;++i)
	{
		c = ptr[i];
		// trim off extensions
		if(c == '.')
			break;
		if( (ret = lif_chars(c,i)) )
			*B++ = ret;
		else
			*B++ = ' ';
	}
	while(i < size)
	{
		*B++ = ' ';
		++i;
	};
	*B = 0;
	return(strlen((char *)save));
}


///@brief Convert LIF volume records into byte vector 
///@param[in] *LIF: LIF image structure
///@param[out] B: byte vector to pack data into
///@return void
void lif_vol2str(lif_t *LIF, uint8_t *B)
{
	V2B_MSB(B,0,2,LIF->VOL.LIFid);
	lif_S2B(B+2,LIF->VOL.Label,6);
	V2B_MSB(B,8,4,LIF->VOL.DirStartSector);
	V2B_MSB(B,12,2,LIF->VOL.System3000LIFid);
	V2B_MSB(B,14,2,0);
	V2B_MSB(B,16,4,LIF->VOL.DirSectors);
	V2B_MSB(B,20,2,LIF->VOL.LIFVersion);
	V2B_MSB(B,22,2,0);
	V2B_MSB(B,24,4,LIF->VOL.tracks_per_side);
	V2B_MSB(B,28,4,LIF->VOL.sides);
	V2B_MSB(B,32,4,LIF->VOL.sectors_per_track);
	memcpy((void *) (B+36),LIF->VOL.date,6);
}

///@brief Convert byte vector into LIF volume records 
///@param[in] B: byte vector 
///@param[out] *LIF: LIF image structure
///@return void
void lif_str2vol(uint8_t *B, lif_t *LIF)
{

	LIF->VOL.LIFid = B2V_MSB(B,0,2);
	lif_B2S(B+2,LIF->VOL.Label,6);
	LIF->VOL.DirStartSector = B2V_MSB(B,8,4);
	LIF->VOL.System3000LIFid = B2V_MSB(B,12,2);
	LIF->VOL.zero1 = B2V_MSB(B,14,2);
	LIF->VOL.DirSectors = B2V_MSB(B,16,4);
	LIF->VOL.LIFVersion = B2V_MSB(B,20,2);
	LIF->VOL.zero2 = B2V_MSB(B,22,2);
	LIF->VOL.tracks_per_side = B2V_MSB(B,24,4);
	LIF->VOL.sides = B2V_MSB(B,28,4);
	LIF->VOL.sectors_per_track = B2V_MSB(B,32,4);
	memcpy((void *) LIF->VOL.date, (B+36),6);
}

///@brief Convert LIF directory records into byte vector 
///@param[int] *LIF: LIF image pointer
///@param[out] B: byte vector to pack data into
///@return void
void lif_dir2str(lif_t *LIF, uint8_t *B)
{
	lif_S2B(B,LIF->DIR.filename,10);				// 0
	V2B_MSB(B,10,2,LIF->DIR.FileType);			// 10
	V2B_MSB(B,12,4,LIF->DIR.FileStartSector);	// 12
	V2B_MSB(B,16,4,LIF->DIR.FileSectors);	// 16
	memcpy(B+20,LIF->DIR.date,6);				// 20
	V2B_MSB(B,26,2,LIF->DIR.VolNumber);			// 26
	V2B_LSB(B,28,2,LIF->DIR.FileBytes);			// 28
	V2B_LSB(B,30,2,LIF->DIR.SectorSize);			// 30
}

///@brief Convert byte vector into byte vector 
///@param[in] B: byte vector to extract data from
///@param[int] LIF: lifdir_t structure pointer
///@return void
void lif_str2dir(uint8_t *B, lif_t *LIF)
{
	lif_B2S(B,LIF->DIR.filename,10);
	LIF->DIR.FileType = B2V_MSB(B, 10, 2);
	LIF->DIR.FileStartSector = B2V_MSB(B, 12, 4);
	LIF->DIR.FileSectors = B2V_MSB(B, 16, 4);
	memcpy(LIF->DIR.date,B+20,6);
	LIF->DIR.VolNumber = B2V_MSB(B, 26, 2); 
	LIF->DIR.FileBytes = B2V_LSB(B, 28, 2);
	LIF->DIR.SectorSize= B2V_LSB(B, 30, 2);
}

/// @brief Convert number >= 0 and <= 99 to BCD.
/// @warning we assume the number is in range.
///  BCD format: each hex nibble has a digit 0 .. 9
/// @param[in] data: number to convert.
/// @return  BCD value
uint8_t lif_BIN2BCD(uint8_t data)
{
    return(  ( (data/10U) << 4 ) | (data%10U) );
}


///@brief UNIX time to LIF time format
///@param[out] bcd: packed 6 byte BCD LIF time
///   YY,MM,DD,HH,MM,SS
///@param[in] t: UNIX time_t time value
///@see time() in time.c
///@return void
void lif_time2lif(uint8_t *bcd, time_t t)
{
	tm_t tm;
	localtime_r((time_t *) &t, (tm_t *)&tm);
	bcd[0] = lif_BIN2BCD(tm.tm_year & 100);
	bcd[1] = lif_BIN2BCD(tm.tm_mon);
	bcd[2] = lif_BIN2BCD(tm.tm_mday);
	bcd[3] = lif_BIN2BCD(tm.tm_hour);
	bcd[4] = lif_BIN2BCD(tm.tm_min);
	bcd[5] = lif_BIN2BCD(tm.tm_sec);
}

/// @brief Clear LIF structure 
/// @param[in] *LIF: pointer to LIF structure
/// @return void
void lif_image_clear(lif_t *LIF)
{
	memset((void *) LIF,0,sizeof(lif_t));
}


/// @brief Clear DIR part of LIF structure 
/// @param[in] *LIF: pointer to LIF structure
/// @return void
void lif_dir_clear(lif_t *LIF)
{
	memset((void *) &LIF->DIR,0,sizeof(lifdir_t));
}

/// @brief Clear VOL part of LIF structure
/// @param[in] *LIF: pointer to LIF structure
/// @return void
void lif_vol_clear(lif_t *LIF)
{
	memset((void *) &LIF->VOL,0,sizeof(lifvol_t));
}

/// @brief Dump LIF struture data for debugging
/// @param[in] *LIF: pointer to LIF structure
/// @return void
void lif_dump_vol(lif_t *LIF)
{
   printf("LIF name:             %s\n", LIF->name);
   printf("LIF sectors:          %8lXh\n", (long)LIF->sectors);
   printf("LIF bytes:            %8lXh\n", (long)LIF->bytes);
   printf("LIF filestart:        %8lXh\n", (long)LIF->filestart);
   printf("LIF file sectors:     %8lXh\n", (long)LIF->filesectors);
   printf("LIF used:             %8lXh\n", (long)LIF->usedsectors);
   printf("LIF free:             %8lXh\n", (long)LIF->freesectors);
   printf("LIF files:            %8lXh\n",(long)LIF->files);
   printf("LIF purged:           %8lXh\n",(long)LIF->purged);

   printf("VOL Label:            %s\n", LIF->VOL.Label);
   printf("VOL LIFid:            %8Xh\n",(int)LIF->VOL.LIFid);
   printf("VOL Dir start:        %8lXh\n",(long)LIF->VOL.DirStartSector);
   printf("VOL Dir sectors:      %8lXh\n",(long)LIF->VOL.DirSectors);
   printf("VOL 3000LIFid:        %8Xh\n",(long)LIF->VOL.System3000LIFid);
   printf("VOL LIFVersion:       %8Xh\n",(long)LIF->VOL.LIFVersion);

   printf("DIR File Name:        %s\n", LIF->DIR.filename);
   printf("DIR File Type:        %8Xh\n", (int)LIF->DIR.FileType);
   printf("DIR File Volume#:     %8Xh\n", (int)LIF->DIR.VolNumber);
   printf("DIR File start:       %8lXh\n", (long)LIF->DIR.FileStartSector);
   printf("DIR File sectors:     %8lXh\n", (long)LIF->DIR.FileSectors);
   printf("DIR File bytes:       %8lXh\n", (long)LIF->DIR.FileBytes);
   printf("DIR File sector size: %8Xh\n", (int)LIF->DIR.SectorSize);
}

///@brief Check Volume Table for values in range
///@param[in] *LIF: Image structure
///@return 1 of ok, 0 on eeror
int lif_check_volume(lif_t *LIF)
{
	int status = 1;
	if( !lif_checkname((char *)LIF->VOL.Label) )
	{
		status = 0;
		if(debuglevel & 1)
			printf("LIF Volume invalid Volume Name");
	}

	if(LIF->VOL.DirStartSector < 1)
	{
		status = 0;
		if(debuglevel & 1)
			printf("LIF Volume invalid start sector:%ld\n", LIF->VOL.DirStartSector);
	}
	if(LIF->VOL.DirSectors < 1)
	{
		if(debuglevel & 1)
			printf("LIF Volume invalid Directory Sector Count < 1\n");
		status = 0;
	}

	if(LIF->VOL.System3000LIFid != 0x1000)
	{
		status = 0;
		if(debuglevel & 1)
			printf("LIF Volume invalid System3000 ID (%04XH) expected 1000H\n", LIF->VOL.System3000LIFid);
	}
	if(LIF->VOL.LIFVersion != 0)
	{
		if(debuglevel & 1)
			printf("LIF Version: %04XH != 0\n", LIF->VOL.LIFVersion);
		status = 0;
	}

	if(LIF->VOL.zero1 != 0)
	{
		if(debuglevel & 1)
			printf("LIF Volume invalid bytes at offset 14&15 should be zero\n");
		status = 0;
	}
	if(LIF->VOL.zero2 != 0)
	{
		if(debuglevel & 1)
			printf("LIF Volume invalid bytes at offset 22&23 should be zero\n");
		status = 0;
	}

	return(status);
}

///@brief Check LIF base structure value (Not: VOL, DIR or space)
///@param[in] *LIF: Image structure
///@return 1 of ok, 0 on eeror
int lif_check_lif_headers(lif_t *LIF)
{
	int status = 1;


    // File start is after the directory area
    if(LIF->filestart > LIF->sectors)
    {
		if(debuglevel & 1)
            printf("LIF Volume invalid file area start > image size\n");
        status = 0;
    }

    // File start is after the directory area
    if(LIF->filestart < LIF->VOL.DirStartSector)
    {
		if(debuglevel & 1)
            printf("LIF Volume invalid file area start < directory start\n");
        status = 0;
    }

    // Check Directory pointers
    if(LIF->filesectors < 1)
    {
		if(debuglevel & 1)
            printf("LIF Volume invalid file area size < 1\n");
        status = 0;
    }

    // Check Directory pointers
    if(LIF->freesectors < 1)
    {
		if(debuglevel & 1)
            printf("LIF Volume invalid file area size < 1\n");
        status = 0;
    }

	return(status);
}


///@brief Validate Directory record values
/// We only do basic out of bounds tests for this record
/// Purged or EOF directory records are NOT checked and always return 1
///@param[in] *LIF: Image structure
///@param[in] debug: dispaly diagostice messages
///@return 1 of ok, 0 on eeror
int lif_check_dir(lif_t *LIF)
{
	int status = 1;


	// We do not check purged or end of DIRECTORY ercordss
	if(LIF->DIR.FileType == 0)
	{
		return(1);
	}

	if(LIF->DIR.FileType == 0xffff)
	{
		return(1);
	}
	
	if( !lif_checkname((char *)LIF->DIR.filename) )
	{
		status = 0;
		if(debuglevel & 1)
			printf("LIF Directory invalid Name:[%s]\n",LIF->DIR.filename);
	}

	if(LIF->filestart)
	{
		if(LIF->DIR.FileStartSector < LIF->filestart)
		{
			status = 0;
			if(debuglevel & 1)
				printf("LIF Directory invalid start sector:%lXh\n", (long)LIF->DIR.FileStartSector);
		}
	}

	if(LIF->sectors)
	{
		if( (LIF->DIR.FileStartSector + LIF->DIR.FileSectors) > (LIF->sectors) )
		{
			status = 0;
			if(debuglevel & 1)
				printf("LIF Directory invalid end sector:%lXh\n", (long)LIF->DIR.FileStartSector + LIF->DIR.FileSectors);
		}
	}

	if(LIF->DIR.VolNumber != 0x8001)
	{
		status = 0;
		if(debuglevel & 1)
			printf("LIF Directory invalid Volume Number:%Xh\n", (int)LIF->DIR.VolNumber);
	}

//FIXME check file types!
	if( LIF->DIR.FileBytes && lif_bytes2sectors(LIF->DIR.FileBytes) != LIF->DIR.FileSectors )
	{
		status = 0;
		if(debuglevel & 1)
			printf("LIF Directory invalid FileBytes size:%ld\n", (long) LIF->DIR.FileBytes);
	}

//FIXME check file types!
	if(LIF->DIR.SectorSize != LIF_SECTOR_SIZE)
	{
		status = 0;
		if(debuglevel & 1)
			printf("LIF Directory invalid sector size:%ld\n", LIF->DIR.SectorSize);
	}

	return(status);
}


/// @brief Create LIF image with Volume, Directory and optional empty filespace
/// @param[in] imagename:  Image name
/// @param[in] liflabel:   Volume Label
/// @param[in] dirstart:   Directory start sector
/// @param[in] dirsectors: Directory sectors
/// @return pointer to LIF structure
lif_t *lif_create_volume(char *imagename, char *liflabel, long dirstart, long dirsectors, long filesectors)
{
	long size;
	long i;
	long offset;
	long count;
	uint8_t buffer[LIF_SECTOR_SIZE];

	lif_t *LIF = lif_calloc(sizeof(lif_t)+4);
	if(LIF == NULL)
		return(NULL);
	
	lif_image_clear(LIF);

    // Initialize volume header
    LIF->VOL.LIFid = 0x8000;
    lif_fixname(LIF->VOL.Label, liflabel, 6);
    LIF->VOL.DirStartSector = dirstart;
    LIF->VOL.DirSectors = dirsectors;
    LIF->VOL.System3000LIFid = 0x1000;
    LIF->VOL.tracks_per_side = 0;
    LIF->VOL.sides = 0;
    LIF->VOL.sectors_per_track = 0;
    ///@brief Current Date
    lif_time2lif(LIF->VOL.date, 0);

	// update LIF headers
	LIF->name = lif_stralloc(imagename);
	if(LIF->name == NULL)
	{
		lif_close_volume(LIF);
		return(NULL);
	}

	LIF->filestart = (dirstart+dirsectors);
	LIF->filesectors = filesectors;
	LIF->sectors = (LIF->filestart+LIF->filesectors);
	LIF->bytes = LIF->sectors * LIF_SECTOR_SIZE;
	LIF->freesectors = LIF->filesectors;
	LIF->usedsectors = 0;
	LIF->files = 0;
	LIF->purged = 0;
	LIF->dirindex = -1;

	memset(buffer,0,LIF_SECTOR_SIZE);

	lif_vol2str(LIF,buffer);

	// Write Volume header
	LIF->fp = lif_open(LIF->name,"w+");
	if(LIF->fp == NULL)
	{
		lif_close_volume(LIF);
		return(NULL);
	}

	offset = 0;
	count = 0;

	size = lif_write(LIF, buffer, offset, LIF_SECTOR_SIZE);

	if(size < LIF_SECTOR_SIZE)
	{
		lif_close_volume(LIF);
		return(NULL);
	}
	offset += size;
	++count;


	memset(buffer,0,LIF_SECTOR_SIZE);

	// Space BETWEEN Volume header and Directory area
	for(i=1;i<dirstart;++i)
	{
		size = lif_write(LIF, buffer, offset, LIF_SECTOR_SIZE);
		if(size < LIF_SECTOR_SIZE)
		{
			lif_close_volume(LIF);
			return(NULL);
		}
		offset += size;
		if((count % 100) == 0)
			printf("Wrote: %ld\r", count);
		++count;
	}

	// Write Directory sectors
	lif_dir_clear(LIF);
	LIF->DIR.FileType = 0xffff;

	for(i=0;i<LIF_SECTOR_SIZE;i+=LIF_DIR_SIZE)
		lif_dir2str(LIF,buffer+i);

	for(i=0;i<dirsectors;++i)
	{
		size = lif_write(LIF, buffer, offset, LIF_SECTOR_SIZE);
		if(size < LIF_SECTOR_SIZE)
		{
			lif_close_volume(LIF);
			return(NULL);
		}
		offset += size;
		if((count % 100) == 0)
			printf("Wrote: %ld\r", count);
		++count;
	}

	// File area sectors
	memset(buffer,0,LIF_SECTOR_SIZE);
	for(i=0;i<filesectors;++i)
	{
		size = lif_write(LIF, buffer, offset, LIF_SECTOR_SIZE);
		if(size < LIF_SECTOR_SIZE)
		{
			lif_close_volume(LIF);
			return(NULL);
		}
		offset += size;
		if((count % 100) == 0)
			printf("Wrote: %ld\r", count);
		++count;
	}
	if(debuglevel & 0x400)
		lif_dump_vol(LIF);
	printf("Wrote: %ld\n", count);

	return(LIF);
}

/// @brief Free LIF structure and close any files
/// @param[in] *LIF: pointer to LIF Volume/Directoy structure
/// @return void
void lif_close_volume(lif_t *LIF)
{
	if(LIF)
	{
		if(LIF->fp)
		{
			fseek(LIF->fp, 0, SEEK_END);
			fclose(LIF->fp);
			sync();
		}
		if(LIF->name)
			safefree(LIF->name);

		lif_vol_clear(LIF);
		safefree(LIF);
	}
}

/// @brief Convert bytes into used sectors
/// @param[in] bytes: size in bytes
/// @return sectors
uint32_t lif_bytes2sectors(uint32_t bytes)
{
	uint32_t sectors = (bytes/LIF_SECTOR_SIZE);
	if(bytes % LIF_SECTOR_SIZE)
		++sectors;
	return(sectors);
}

/// @brief Rewind LIF directory 
/// Note readdir pre-increments the directory pointer index so we start at -1
/// @param[in] *LIF: pointer to LIF Volume/Directoy structure
/// @return void
void lif_rewinddir(lif_t *LIF)
{
	// Directory index
	LIF->dirindex = -1;
}

/// @brief Close LIF directory 
/// clear and free lif_t structure
/// @param[in] *LIF: pointer to LIF Volume/Directoy structure
/// @return 0 on sucesss, -1 on error
void lif_closedir(lif_t *LIF)
{
	return( lif_close_volume(LIF) );
}

/// @brief Check directory index limits
/// @param[in] *LIF: LIF Volume/Diractoy structure 
/// @param[in] index: directory index
/// @return 1 inside, 0 outside
int lif_checkdirindex(lif_t * LIF, int index)
{
	if(index < 0 || lif_bytes2sectors((long) index * LIF_DIR_SIZE) > LIF->VOL.DirSectors)
	{
		printf("lif_checkdirindex:[%s] direcory index:[%d] out of bounds\n",LIF->name, index);
		if(debuglevel & 0x400)
			lif_dump_vol(LIF);
		return(0);
	}
	return(1);
}

/// @brief Read LIF directory record number N
/// @param[in] *LIF: to LIF Volume/Diractoy structure 
/// @param[in] index: director record number
/// @return 1 on success, 0 if error, bad directory record or outside of directory limits
int lif_readdirindex(lif_t *LIF, int index)
{
	uint32_t offset;
	uint8_t dir[LIF_DIR_SIZE];

	if( !lif_checkdirindex(LIF, index) )
		return(0);

	offset = (index * LIF_DIR_SIZE) + (LIF->VOL.DirStartSector * LIF_SECTOR_SIZE);

	// read raw data
	if( lif_read(LIF, dir, offset, sizeof(dir)) < (long)sizeof(dir) )
        return(0);

	// Convert into directory structure
	lif_str2dir(dir, LIF);

	if( !lif_check_dir(LIF))
	{
		if(debuglevel & 0x400)
			lif_dump_vol(LIF);
		return(0);
	}
	return(1);
}

/// @brief Write LIF drectory record number N
/// @param[in] *LIF: LIF Volume/Diractoy structure 
/// @param[in] index: director record number
/// @return 1 on success, 0 if error, bad directory record or outside of directory limits
int lif_writedirindex(lif_t *LIF, int index)
{
	long offset;
	uint8_t dir[LIF_DIR_SIZE];

	// Validate the record
	if(!lif_check_dir(LIF))
	{
		if(debuglevel & 0x400)
			lif_dump_vol(LIF);
		return(0);
	}

	// check for out of bounds
	if( !lif_checkdirindex(LIF, index))
		return(0);

	offset = (index * LIF_DIR_SIZE) + (LIF->VOL.DirStartSector * LIF_SECTOR_SIZE);

	// store LIF->DIR settings into dir
	lif_dir2str(LIF, dir);

	if( lif_write(LIF, dir, offset, sizeof(dir)) < (int ) sizeof(dir) )
        return(0);

	return(1);
}

/// @brief Write LIF drectory EOF
/// @param[in] *LIF: LIF Volume/Diractoy structure 
/// @param[in] index: director record number
/// @return 1 on success, 0 on error and outsize directory limits
int lif_writedirEOF(lif_t *LIF, int index)
{
	// Create a director EOF
	lif_dir_clear(LIF);
	LIF->DIR.FileType = 0xffff;
	return( lif_writedirindex(LIF,index));
}


/// @brief Read a directory records from LIF image advancind directory index
/// @see lif_opendir()
/// nOte: skip all purged LIF directory records
/// @param[in] *LIF: to LIF Volume/Diractoy structure 
/// @return directory structure or NULL 
lifdir_t *lif_readdir(lif_t *LIF)
{
	while(1)
	{
		// Advance index first 
		// We start initialized at -1 by lif_opendir() and lif_rewinddir()
		LIF->dirindex++;

		if( !lif_readdirindex(LIF, LIF->dirindex) )
			break;

		if( LIF->DIR.FileType == 0xffffUL )
			break;

		if(LIF->DIR.FileType)
			return( (lifdir_t *) &LIF->DIR );

		// Skip purged records
	}
	return( NULL );
}


/// @brief Open LIF directory for reading
/// @param[in] *name: file name of LIF image
/// @return LIF pointer on sucesses or NULL on error
lif_t *lif_opendir(char *name)
{
	lif_t *LIF;
	int index = 0;
	stat_t sb, *sp;
	uint8_t buffer[LIF_SECTOR_SIZE];


	sp = lif_stat(name, (stat_t *)&sb);
	if(sp == NULL)
        return(NULL);

	// To read LIF volume we must have at minimum two sectors
	// volume header a directory entry
	if(sp->st_size < LIF_SECTOR_SIZE*2)
	{
		if(debuglevel & 1)
			printf("lif_openimage:[%s] invalid volume header area too small:[%d]\n", name, sp->st_size);
		return(NULL);
	}


	// Allocate LIF structur
	LIF = lif_calloc(sizeof(lif_t)+4);
	if(!LIF)
		return(NULL);

	LIF->name = lif_stralloc(name);
	if(!LIF->name)
	{
		lif_closedir(LIF);
		return(NULL);
	}
		
	LIF->bytes = sp->st_size;
	LIF->sectors = lif_bytes2sectors(sp->st_size);

	// Used sectors
	LIF->usedsectors = 0;
	// Purged files
	LIF->purged= 0;
	// Files
	LIF->files = 0;

	LIF->fp = lif_open(LIF->name,"r");
		
	// Volume header must be it least one sector
 	if( lif_read(LIF, buffer, 0, LIF_SECTOR_SIZE) < LIF_SECTOR_SIZE)
	{
		lif_closedir(LIF);
        return(NULL);
	}

	// Unpack Volumes has the Directory start sector
	lif_str2vol(buffer, LIF);

	// File area start and size
    LIF->filestart = LIF->VOL.DirStartSector + LIF->VOL.DirSectors;
    LIF->filesectors = LIF->sectors - LIF->filestart;
	LIF->freesectors = LIF->filesectors;

	// Validate Volume headers and internal LIF headers
	if( !lif_check_volume(LIF) || !lif_check_lif_headers(LIF))
	{
		if(debuglevel & 1)
			printf("lif_openimage:[%s] invalid volume header\n", LIF->name);
		if(debuglevel & 0x400)
			lif_dump_vol(LIF);
		lif_closedir(LIF);
		return(NULL);
	}

	index = 0;
	/// Update free
	while(1)
	{
		if( !lif_readdirindex(LIF,index) )
		{
			lif_closedir(LIF);
			return(NULL);
		}

		if(LIF->DIR.FileType == 0xffff)
			break;

		if(LIF->DIR.FileType == 0)
		{
			LIF->purged++;
			++index;
			continue;
		}
		LIF->usedsectors += LIF->DIR.FileSectors;
		LIF->freesectors -= LIF->DIR.FileSectors;
		LIF->files++;
		++index;
	}
	// rewind
	lif_rewinddir(LIF);
	return(LIF);
}

/// @brief Display a LIF image file directory
/// @param[in] lifimagename: LIF disk image name
/// @return -1 on error or number of files found
void lif_dir(char *lifimagename)
{
	long bytes;
	lif_t *LIF;
	int index = 0;

	LIF = lif_opendir(lifimagename);
	if(LIF == NULL)
		return;
	
	printf("Volume: [%s]\n", LIF->VOL.Label);
	printf("NAME         TYPE   START SECTOR        SIZE     RECSIZE\n");
	while(1)
	{
		if(!lif_readdirindex(LIF,index))
			break;

		if(LIF->DIR.FileType == 0xffff)
			break;
;
		bytes = LIF->DIR.FileBytes;
		if(!bytes)
			bytes = (LIF->DIR.FileSectors * LIF_SECTOR_SIZE);

		// name type start size
		printf("%-10s  %04Xh      %8lXh   %9ld       %4d\n", 
			(LIF->DIR.FileType ? (char *)LIF->DIR.filename : "<PURGED>"), 
			(int)LIF->DIR.FileType, 
			(long)LIF->DIR.FileStartSector, 
			(long)bytes, 
			(int)LIF->DIR.SectorSize  );

		++index;
	}	

	printf("\n");
	printf("%8ld Files\n", (long)LIF->files);
	printf("%8ld Purged\n", (long)LIF->purged);
	printf("%8ld Used sectors\n", (long)LIF->usedsectors);
	printf("%8ld Free sectors\n", (long)LIF->freesectors);

	lif_closedir(LIF);
}


/// @brief Find a LIF image file by name
/// @param[in] *LIF: directory pointer
/// @param[in] liflabel: File name in LIF image
/// @return Directory index of record
int lif_find_file(lif_t *LIF, char *liflabel)
{
	int index;

	if( !lif_checkname(liflabel) )
	{
		if(debuglevel & 1)
			printf("lif_find_file:[%s] invalid characters\n", liflabel);
		return(-1);
	}
	if(strlen(liflabel) > 10)
	{
		if(debuglevel & 1)
			printf("lif_find_file:[%s] liflabel too big\n", liflabel);
		return(-1);
	}

	if(LIF == NULL)
		return(-1);
	
	index = 0;
	while(1)
	{
		if(!lif_readdirindex(LIF,index))
			return(-1);

		if(LIF->DIR.FileType == 0xffff)
			return(-1);

		if( LIF->DIR.FileType && (strcasecmp((char *)LIF->DIR.filename,liflabel) == 0) )
			break;
		++index;
	}
	return(index);
}

/// @brief Find free directory slot that can hold >= sectors in size
/// Unless a record is purged this will always be the last record
/// @param[in] *LIF: LIF pointer
/// @param[in] sectors: size of free space we need
/// @return Directory index of matching record or -1 on error
int lif_findfree_dirindex(lif_t *LIF, uint32_t sectors)
{
	// Directory index
	int index = 0;
	int purged = 0;

	// Master start of file area
	uint32_t start = LIF->filestart;

	// Clear free structure
	memset((void *)&LIF->space,0,sizeof(lifspace_t));

	if(LIF == NULL)
		return(-1);

	// Volume free space
	if(sectors > LIF->freesectors)
		return(-1);

	while(1)
	{
		if(!lif_readdirindex(LIF,index))
			break;

		// EOF Record
		if(LIF->DIR.FileType == 0xffff)
		{
			// Start of free space is after last non-type 0 record - if we had a last record
			LIF->space.start = start;
			LIF->space.size = sectors;
			LIF->space.index = index;
			LIF->space.eof = 1;
			LIF->dirindex = index;
			return(index);
		}

		// Purged Record, the specs say we can NOT use the file size or start
		if(LIF->DIR.FileType == 0)
		{
 			if(purged == 0)
			{
				// Start of possible free space is after last non-type 0 record - if we had a last record
				LIF->space.start = start;
				// INDEX of this record
				LIF->space.index = index;
				purged = 1;
			}
			++index;
			continue;
		}

		// Valid Record

		// We had a previous purged record
		// Is there enough free space between valid records to save a new file of sectors in size ?
		if(purged)
		{
			// Compute the gap between the last record, or start of free space, and this one
			LIF->space.size = LIF->DIR.FileStartSector - LIF->space.start;
			if(LIF->space.size >= sectors)
			{
				LIF->dirindex = LIF->space.index;
				return(LIF->dirindex);
			}
			purged = 0;
		}
		// Find the start of the NEXT record
		start = LIF->DIR.FileStartSector + LIF->DIR.FileSectors;
		++index;
	}

	return(-1);
}


/** @brief  HP85 E010 ASCII LIF records 
	ef [ff]* = no more data in this sector 
	df size [ASCII] = data must fit inside this sector
	cf size [ASCII] = split data accross sector boundry, "6f" record continues at start of next sector
		   Note: The 6f header is INSIDE the "cf" record and not included in the "cf" size value (yuck!)
	6f size [ASCII] = split continue (always starts at sector boundry)
	df 00 00 ef [ff]* = EOF (df size = 0) pad with "ef" and "ff" until sector end
	size = 16 bits LSB MSB

	Example:
	000080e0 : 4b 7c 22 0d cf 29 00 31 34 20 44 49 53 50 20 22  : K|"..).14 DISP "
	000080f0 : 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 5f  :                _
	00008100 : 6f 10 00 5f 5f 5f 5f 5f 5f 5f 5f 5f 5f 5f 5f 5f  : o.._____________
	00008110 : 5f 22 0d df 2b 00 31 35 20 44 49 53 50 20 22 20  : _"..+.15 DISP "

	cf 29 00 (19 is to sector end) (new sector start with 6F 10 00 (10 is remainder)
	So 29 = 19 and 10 (yuck!)
*/

///@brief Convert an ASCII string into HP85 E010 format 
/// @param[in] str: ASCII string to write
/// @param[in] offset: E010 data sector offset, only used in formatting wbuf with headers
/// @param[in] wbuf: E010 data result
/// @return size of E010 data
int lif_ascii_string_to_e010(char *str, long offset, uint8_t *wbuf)
{
	int ind;
	int len;
	int bytes;
	int pos,rem;
	
	// Data written to string
	bytes = 0;

	// String size
	len = strlen(str);

	// Compute the current offset in this sector
	pos = (offset % LIF_SECTOR_SIZE);

	// Number of bytes free in this sector
	rem = LIF_SECTOR_SIZE - pos;

	// Output buffer index
	ind = 0;

	// Does the string + header fit ?
	if(len && rem < (3 + len))
	{
		// If we split a string we end up writting two headers
		//  So for less then 6 bytes is no point splitting those strings
		//  We just pad and write the string in the next sector
		//  
		if(rem < 6)
		{
			// PAD
			wbuf[ind++] = 0xEF;
			while(ind<rem)
				wbuf[ind++] = 0xff;

			// NEW SECTOR
			// Debugging make sure we are at sector boundry
			if(((offset + ind)  % LIF_SECTOR_SIZE))
			{
				if(debuglevel & 1)
					printf("Expected sector boundry, offset:%d\n", (int) ((offset + ind) % LIF_SECTOR_SIZE) );
				return(-1);
			}
			// Write string in new sector
			// The full string + header will fit
			wbuf[ind++] = 0xDF;
			wbuf[ind++] = len & 0xff;
			wbuf[ind++] = (len >> 8) & 0xff;
			// Write string
			while(*str)
				wbuf[ind++] = *str++;
		}
		else
		{
			// Split string
			// String spans sector , so split the string
			// Split string Header
			wbuf[ind++] = 0xCF;
			wbuf[ind++] = len & 0xff;
			wbuf[ind++] = (len >>8) & 0xff;
			// Write as much of the string as we can in this sector
			while(*str && ind<rem)
				wbuf[ind++] = *str++;

			// NEW SECTOR
			// Debugging make sure we are at sector boundry
			if(((offset + ind)  % LIF_SECTOR_SIZE))
			{
				if(debuglevel & 1)
					printf("Expected sector boundry, offset:%d\n", (int) ((offset + ind) % LIF_SECTOR_SIZE) );
				return(-1);
			}

			// Update remining string length
			len = strlen(str);
			// Write split string continuation heaader at start of new sector
			wbuf[ind++] = 0x6F;
			wbuf[ind++] = (len & 0xff);
			wbuf[ind++] = (len>>8) & 0xff;
			// Write string
			while(*str)
				wbuf[ind++] = *str++;
		}
	}
	else 
	{
		// Note: zero length is an EOF

		// The full string + header will fit
		wbuf[ind++] = 0xdf;
		wbuf[ind++] = len & 0xff;
		wbuf[ind++] = (len >> 8) & 0xff;


		if(len)
		{
			while(*str)
				wbuf[ind++] = *str++;
		}
		else 
		{
			// EOF is a zero length sector
			wbuf[ind++]  = 0xef;
			// Pad sector
			while( (offset+ind) % LIF_SECTOR_SIZE)
				wbuf[ind++] = 0xff;
		}
	}

	offset += ind;
	bytes += ind;

	return( bytes );
}
	

/// @brief Add ASCII file as E010 data to LIF image - or compute converted data size
/// To find size of formatted result only, without writting, set LIF to NULL
/// @param[in] userfile: User ASCII file source
/// @param[in] *LIF: Where to write file if set (not NULL)
/// @return size of formatted result
long lif_add_ascii_file_as_e010_wrapper(lif_t *LIF, uint32_t offset, char *name)
{
	long bytes;
	int count;
	int size;
	int len;
	FILE *fi;

	// strings are limited to less then this
	char str[LIF_SECTOR_SIZE+1];
	// output buffer must be larger then a single sectors because of either headers or padding
	uint8_t obuf[LIF_SECTOR_SIZE*2];

	fi = lif_open(name, "r");
	if(fi == NULL)
		return(-1);

	bytes = 0;
	count = 0;

	// Read user file and write LIF records
	// reserve 3 + LIF header bytes + 1 (EOS)
	while( fgets((char *)str,(int)sizeof(str) - 4, fi) != NULL )
	{
		trim_tail((char *)str);

		strcat((char *)str,"\r"); // HP85 lines end with "\r"

		size = lif_ascii_string_to_e010(str, offset, obuf);
		// Write string
		// Now Write string
		if(LIF)
		{
			len = lif_write(LIF, obuf, offset, size);
			if(len < size)
			{
				fclose(fi);
				return(-1);
			}
		}

		offset += size;
		bytes += size;
		count += size;

		if(count > 256)
		{		
			count = 0;
			if(LIF)
				printf("Wrote: %8ld\r", (long)bytes);
		}
	}

	fclose(fi);

	str[0] = 0;
	// Write EOF string with padding
	size = lif_ascii_string_to_e010(str, offset,obuf);
	if(LIF)
	{
		printf("Wrote: %8ld\r", (long)bytes);
		len = lif_write(LIF, obuf, offset, size);
		if(len < size)
			return(-1);

	}
	offset += size;
	bytes += size;
	if(LIF)
		printf("Wrote: %8ld\r",(long)bytes);

	return(bytes);
}

/// @brief Convert and add ASCII file to the LIF image as type E010 format
/// The basename of the lifname, without extensions, is used as the LIF file name
/// @param[in] lifimagename: LIF image name
/// @param[in] lifname: LIF file name
/// @param[in] userfile: userfile name
/// @return size of data written into to LIF image, or -1 on error
long lif_add_ascii_file_as_e010(char *lifimagename, char *lifname, char *userfile)
{
	long bytes;
	long sectors;
	long offset;
	int index;
	lif_t *LIF;

	if(!*lifimagename)
	{
		printf("lif_add_ascii_file_as_e010: lifimagename is empty\n");
		return(-1);
	}
	if(!*lifname)
	{
		printf("lif_add_ascii_file_as_e010: lifname is empty\n");
		return(-1);
	}
	if(!*userfile)
	{
		printf("lif_add_ascii_file_as_e010: userfile is empty\n");
		return(-1);
	}

	if(debuglevel & 0x400)
		printf("LIF image:[%s], LIF name:[%s], user file:[%s]\n", 
			lifimagename, lifname, userfile);

	// Find out how big converted file will be
	bytes = lif_add_ascii_file_as_e010_wrapper(NULL,0,userfile);
	sectors = lif_bytes2sectors(bytes);

	LIF = lif_opendir(lifimagename);
	if(LIF == NULL)
		return(-1);	

	// Now find free record
	index = lif_findfree_dirindex(LIF, sectors);
	if(index == -1)
	{
		printf("LIF image:[%s], not enough free space for:[%s]\n", 
			lifimagename, userfile);
			lif_closedir(LIF);
			return(-1);
	}

	// Initialize the free directory entry
	lif_fixname(LIF->DIR.filename, lifname,10);
	LIF->DIR.FileType = 0xe010;  			// 10
	LIF->DIR.FileStartSector = LIF->space.start;	// 12
	LIF->DIR.FileSectors = sectors;    		// 16
	memset(LIF->DIR.date,0,6);				// 20
	LIF->DIR.VolNumber = 0x8001;			// 26
	LIF->DIR.FileBytes = 0;					// 28
	LIF->DIR.SectorSize  = 0x100;			// 30

	offset = LIF->space.start * LIF_SECTOR_SIZE;

	if(debuglevel & 0x400)
		lif_dump_vol(LIF);

	// Write converted file into free space first
	bytes = lif_add_ascii_file_as_e010_wrapper(LIF,offset,userfile);

	if(debuglevel & 0x400)
	{
		printf("New Directory Information AFTER write\n");
		printf("Name:              %s\n", LIF->DIR.filename);
		printf("Index:            %4d\n", (int)index);
		printf("First Sector:     %4lxH\n", LIF->DIR.FileStartSector);
		printf("File Sectors:     %4lxH\n", LIF->DIR.FileSectors);
	}

	// Write directory record
	if( !lif_writedirindex(LIF,index))
	{
		lif_closedir(LIF);
		return(-1);
	}

	// Write EOF is this is the last record
	if(LIF->space.eof && !lif_writedirEOF(LIF,index+1))
	{
		lif_closedir(LIF);
		return(-1);
	}
	lif_closedir(LIF);


	printf("Wrote: %8ld\n", bytes);

	// Return file size
	return(bytes);
}


/// @brief Extract E010 type file from LIF image and save as user ASCII file
/// @param[in] lifimagename: LIF disk image name
/// @param[in] lifname:  name of file in LIF image
/// @param[in] username: name to call the extracted image
/// @return 1 on sucess or 0 on error
int lif_extract_e010_as_ascii(char *lifimagename, char *lifname, char *username)
{
	lif_t *LIF;
	uint32_t start, end;	// sectors
	long offset, bytes;		// bytes
	int index;
	int i, len,size;
	int status = 1;
	int done = 0;

	int ind,wind;
	FILE *fo;

	// read buffer
	uint8_t buf[LIF_SECTOR_SIZE+4];
	// Write buffer, FYI: will ALWAYS be smaller then the read data buffer
	uint8_t wbuf[LIF_SECTOR_SIZE+4];


	LIF = lif_opendir(lifimagename);
	if(LIF == NULL)
	{
		printf("LIF image not found:%s\n", lifimagename);
		return(0);
	}

	index = lif_find_file(LIF, lifname);
	if(index == -1)
	{
		printf("File not found:%s\n", username);
		lif_closedir(LIF);
		return(0);
	}

	if((LIF->DIR.FileType & 0xFFFC) != 0xE010)
	{
		printf("File %s has wrong type:[%04XH] expected 0xE010..0xE013\n", username, (int) LIF->DIR.FileType);
		lif_closedir(LIF);
		return(0);
	}

	start = LIF->DIR.FileStartSector;
	end = start + LIF->DIR.FileSectors;

	offset = start * LIF_SECTOR_SIZE;

	fo = lif_open(username,"w");
	if(fo == NULL)
	{
		lif_closedir(LIF);
		return(0);
	}

	printf("Extracting: %s\n", username);

	bytes = 0;
	wind = 0;
	ind = 0;

	while(lif_bytes2sectors(offset) <= end)
	{
		// LIF images are always multiples of LIF_SECTOR_SIZE
		size = lif_read(LIF, buf, offset, LIF_SECTOR_SIZE);
		if(size < LIF_SECTOR_SIZE)
		{
			status = 0;
			break;
		}

		ind = 0;
		while(ind < LIF_SECTOR_SIZE && !done)
		{
			if(buf[ind] == 0xDF || buf[ind] == 0xCF || buf[ind] == 0x6F)
			{
				++ind;
				len = buf[ind++] & 0xff;
				len |= ((buf[ind++] & 0xff) <<8); 
				// EOF ?
				if(len == 0)
				{
					done = 1;
					break;
				}
				if(len >= LIF_SECTOR_SIZE)
				{
					printf("lif_extract_e010_as_ascii: string too big size = %d\n", (int)len);
					status = 0;
					done = 1;
					break;
				}
			}
			else if(buf[ind] == 0xEF)
			{
				// skip remaining bytes in sector
				ind = 0;
				break;
			}
			else
			{
				printf("lif_extract_e010_as_ascii: unexpected control byte:[%02XH] @ offset: %8lx, ind:%02XH\n", (int)buf[ind], offset, (int)ind);
				status = 0;
				done = 1;
				break;
			}
			// write string
			for(i=0;i <len && ind < LIF_SECTOR_SIZE;++i)
			{
				if(buf[ind] == '\r' && i == len-1)
				{
					wbuf[wind++] = '\n';
					++ind;
					break;
				}
				else 
				{
					wbuf[wind++] = buf[ind++];
				}

				if(wind >= LIF_SECTOR_SIZE)
				{
					size = fwrite(wbuf,1,wind,fo);
					if(size < wind)
					{
						printf("lif_extract_e010_as_ascii: write error\n");
						status = 0;
						done = 1;
						break;
					}
					bytes += size;
					printf("Wrote: %8ld\r", bytes);
					wind = 0;
				}

			}   // for(i=0;i <len && ind < LIF_SECTOR_SIZE;++i)

		}  	// while(ind < LIF_SECTOR_SIZE && status)

		offset += LIF_SECTOR_SIZE;

	} 	// while(offset <= end)

	lif_closedir(LIF);
	// Flush any remaining bytes
	if(wind)
	{
		size = fwrite(wbuf,1,wind,fo);
		if(size < wind)
		{
			printf("lif_extract_e010_as_ascii: write error\n");
			status = 0;
		}
		bytes += size;
	}
	fclose(fo);
	sync();
	printf("Wrote: %8ld\n", bytes);
	return(status);
}

	
/// @brief Extract a file from LIF image entry as standalone LIF image
/// @param[in] lifimagename: LIF disk image name to extract file from
/// @param[in] lifname:  name of file in LIF image we want to extract
/// @param[in] username: new LIF file to create
/// @return 1 on sucess or 0 on error
int lif_extract_lif_as_lif(char *lifimagename, char *lifname, char *username)
{
	// Master image lif_t structure
	lif_t *LIF;
	lif_t *ULIF;

	long offset, uoffset, bytes;	
	int index;
	int i, size;

	uint8_t buf[LIF_SECTOR_SIZE+4];

	LIF = lif_opendir(lifimagename);
	if(LIF == NULL)
	{
		printf("LIF image not found:%s\n", lifimagename);
		return(0);
	}

	index = lif_find_file(LIF, lifname);
	if(index == -1)
	{
		printf("File not found:%s\n", lifname);
		lif_closedir(LIF);
		return(0);
	}

	//Initialize the user file lif_t structure
	ULIF = lif_create_volume(username, "HFSLIF",1,1,0);
	if(ULIF == NULL)
	{
		lif_closedir(LIF);
		return(0);
	}

	// Only the start sector changes

	// Copy directory record
	ULIF->DIR = LIF->DIR;
	ULIF->DIR.FileStartSector = 2;
	ULIF->filesectors = LIF->DIR.FileSectors;

	if( !lif_writedirindex(ULIF,0))
	{
		lif_closedir(LIF);
		lif_closedir(ULIF);
		return(0);
	}
	if( !lif_writedirEOF(ULIF,1) )
	{
		lif_closedir(LIF);
		lif_closedir(ULIF);
		return(0);
	}

	uoffset =  ULIF->filestart * LIF_SECTOR_SIZE;

	offset = LIF->DIR.FileStartSector * LIF_SECTOR_SIZE;

	bytes = uoffset;

	for(i=0;i<(int)LIF->DIR.FileSectors;++i)
	{
		size = lif_read(LIF, buf, offset,LIF_SECTOR_SIZE);
		if(size < LIF_SECTOR_SIZE)
		{
			lif_closedir(LIF);
			lif_closedir(ULIF);
			return(0);
		}

		lif_write(ULIF,buf,uoffset,LIF_SECTOR_SIZE);
		if(size < LIF_SECTOR_SIZE)
		{
			lif_closedir(LIF);
			lif_closedir(ULIF);
			return(0);
		}
		bytes += size;
		offset += size;
		uoffset += size;
		printf("Wrote: %8ld\r", bytes);
	}
	lif_closedir(LIF);
	lif_closedir(ULIF);
	printf("Wrote: %8ld\n", bytes);
	return(1);
}
	
/// @brief Add LIF file from another LIF image 
/// @param[in] lifimagename: LIF image name
/// @param[in] lifname: LIF file name to copy file to
/// @param[in] userfile: LIF mage name copy file from 
/// @return size of data written into to LIF image, or -1 on error
long lif_add_lif_file(char *lifimagename, char *lifname, char *userfile)
{
	// Master image lif_t structure
	lif_t *LIF;
	lif_t *ULIF;
	int index = 0;
	long offset, uoffset, bytes;
	int i, size;

	uint8_t buf[LIF_SECTOR_SIZE+4];

	if(!*lifimagename)
	{
		printf("lif_add: lifimagename is empty\n");
		return(-1);
	}
	if(!*lifname)
	{
		printf("lif_add: lifname is empty\n");
		return(-1);
	}
	if(!*userfile)
	{
		printf("lif_add: userfile is empty\n");
		return(-1);
	}

	if(debuglevel & 0x400)
		printf("LIF image:[%s], LIF name:[%s], user file:[%s]\n", 
			lifimagename, lifname, userfile);

	// open  userfile as LIF image
	ULIF = lif_opendir(userfile);
	if(ULIF == NULL)
		return(-1);	

	// find lif file in user image
	index = lif_find_file(ULIF, lifname);
	if(index == -1)
	{
		printf("File not found:%s\n", lifname);
		lif_closedir(ULIF);
		return(0);
	}

	LIF = lif_opendir(lifimagename);
	if(LIF == NULL)
		return(-1);	

	// Now find a new free record
	index = lif_findfree_dirindex(LIF, ULIF->DIR.FileSectors);
	if(index == -1)
	{
		printf("LIF image:[%s], not enough free space for:[%s]\n", 
			lifimagename, userfile);
			lif_closedir(LIF);
			lif_closedir(ULIF);
		return(-1);
	}

	// Copy user image directory record to master image directory record
	LIF->DIR = ULIF->DIR;

	// Adjust starting sector to point here
	LIF->DIR.FileStartSector = LIF->space.start;

	// Master lif image file start in bytes
	offset  = LIF->DIR.FileStartSector * LIF_SECTOR_SIZE;
	// User lif image file start in bytes
	uoffset = ULIF->DIR.FileStartSector * LIF_SECTOR_SIZE;
	bytes = 0;
	for(i=0;i<(int)LIF->DIR.FileSectors;++i)
	{
		// Read
		size = lif_read(ULIF, buf, uoffset, LIF_SECTOR_SIZE);
		if(size < LIF_SECTOR_SIZE)
		{
			lif_closedir(LIF);
			lif_closedir(ULIF);
			return(-1);
		}

		// Write
		size = lif_write(LIF, buf, offset, LIF_SECTOR_SIZE);
		if(size < LIF_SECTOR_SIZE)
		{
			lif_closedir(LIF);
			lif_closedir(ULIF);
			return(-1);
		}
		offset += LIF_SECTOR_SIZE;
		uoffset += LIF_SECTOR_SIZE;
		bytes += LIF_SECTOR_SIZE;
		printf("Wrote: %8ld\r", bytes);
	}
	lif_closedir(ULIF);

	// Write directory record
	if( !lif_writedirindex(LIF,index))
	{
		lif_closedir(LIF);
		return(-1);
	}
	if(LIF->space.eof && !lif_writedirEOF(LIF,index+1))
	{
		lif_closedir(LIF);
		return(-1);
	}
	lif_closedir(LIF);
	printf("Wrote: %8ld\n", bytes);
	return(bytes);
}





/// @brief Delete LIF file in LIF image
/// @param[in] lifimagename: LIF image name
/// @param[in] lifname: LIF file name
/// @return 1 if deleted, 0 if not found, -1 error
int lif_del_file(char *lifimagename, char *lifname)
{
	lif_t *LIF;
	int index;

	if(!*lifimagename)
	{
		printf("lif_del_file: lifimagename is empty\n");
		return(-1);
	}
	if(!*lifname)
	{
		printf("lif_del_file: lifname is empty\n");
		return(-1);
	}
	if(debuglevel & 0x400)
		printf("LIF image:[%s], LIF name:[%s]\n", 
			lifimagename, lifname);


	LIF = lif_opendir(lifimagename);
	if(LIF == NULL)
		return(-1);	

	// Now find file record
	index = lif_find_file(LIF, lifname);
	if(index == -1)
	{
		lif_closedir(LIF);
		printf("LIF image:[%s] lif name:[%s] not found\n", lifimagename, lifname);
		return(0);
	}
	LIF->DIR.FileType = 0;

	// re-Write directory record
	if( !lif_writedirindex(LIF,index) )
	{
		lif_closedir(LIF);
		return(-1);
	}
	lif_closedir(LIF);
	printf("Deleted: %10s\n", lifname);

	return(1);
}

/// @brief Rename LIF file in LIF image
/// @param[in] lifimagename: LIF image name
/// @param[in] oldlifname: old LIF file name
/// @param[in] newlifname: new LIF file name
/// @return 1 if renamed, 0 if not found, -1 error
int lif_rename_file(char *lifimagename, char *oldlifname, char *newlifname)
{
	int index;
	lif_t *LIF;

	if(!*lifimagename)
	{
		printf("lif_rename_file: lifimagename is empty\n");
		return(-1);
	}
	if(!*oldlifname)
	{
		printf("lif_rename_file: old lifname is empty\n");
		return(-1);
	}
	if(!*newlifname)
	{
		printf("lif_rename_file: new lifname is empty\n");
		return(-1);
	}

	if(!lif_checkname(newlifname))
	{
		printf("lif_rename_file: new lifname contains bad characters\n");
		return(-1);

	}

	LIF = lif_opendir(lifimagename);
	if(LIF == NULL)
		return(-1);	

	// Now find file record
	index = lif_find_file(LIF, oldlifname);
	if(index == -1)
	{
		printf("LIF image:[%s] lif name:[%s] not found\n", lifimagename, oldlifname);
		lif_closedir(LIF);
		return(0);
	}
    lif_fixname(LIF->DIR.filename, newlifname, 10);

	// re-Write directory record
	if( !lif_writedirindex(LIF,index))
	{
		lif_closedir(LIF);
		return(-1);
	}
	lif_closedir(LIF);
	printf("renamed: %10s to %10s\n", oldlifname,newlifname);

	return(1);
}



/// @brief Create/Format a LIF new disk image
/// This can take a while to run, about 1 min for 10,000,000 bytes
/// @param[in] lifimagename: LIF disk image name
/// @param[in] liflabel: LIF Volume Label name
/// @param[in] dirsectors: Number of LIF directory sectors
/// @param[in] sectors: total disk image size in sectors
///@return bytes writting to disk image
long lif_create_image(char *lifimagename, char *liflabel, uint32_t dirsectors, uint32_t sectors)
{
	uint32_t dirstart,filestart,filesectors,end;
	lif_t *LIF;

	if(!*lifimagename)
	{
		printf("lif_create_image: lifimagename is empty\n");
		return(-1);
	}
	if(!*liflabel)
	{
		printf("lif_create_image: liflabel is empty\n");
		return(-1);
	}
	if(!dirsectors)
	{
		printf("lif_create_image: dirsectors is 0\n");
		return(-1);
	}
	if(!sectors)
	{
		printf("lif_create_image: sectors is 0\n");
		return(-1);
	}

	dirstart = 2;
	filestart = dirstart + dirsectors;
	filesectors = sectors - filestart;
	end = filestart + filesectors;

	LIF = lif_create_volume(lifimagename, liflabel, dirstart, dirsectors, filesectors);
	if(LIF == NULL)
		return(-1);
	lif_close_volume(LIF);

	printf("Formating: wrote:[%ld] sectors\n", end);
    return(end);
}
