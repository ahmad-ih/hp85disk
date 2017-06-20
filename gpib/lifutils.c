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
        "lifadd lifimage lifname file\n"
        "lifcreate lifimage label directory_sectors sectors\n"
        "lifdir\n"
        "lifextract lifimage lifname file\n"
        );
}

/// @brief LIFGuser tests
/// @return  1 matched token, 0 if not
int lif_tests(char *str)
{

    int len;
    char *ptr;

    ptr = skipspaces(str);

    if ((len = token(ptr,"lifadd")) )
    {
        char name[64];
        char lifname[64];
        char user[64];

        ptr += len;

        // IMAGE name
        ptr = get_token(ptr, name, 63);

        // lif file name
        ptr = get_token(ptr, lifname, 63);

        // User file name
        ptr = get_token(ptr, user, 63);

        lif_add_file ( name, lifname, user);

        return(1);
    }
    if ((len = token(ptr,"lifextract")) )
	{

        char name[64];
        char lifname[64];
        char user[64];

        ptr += len;

        // IMAGE name
        ptr = get_token(ptr, name, 63);

        // lif file name
        ptr = get_token(ptr, lifname, 63);

        // User file name
        ptr = get_token(ptr, user, 63);

		lif_extract_file(name, lifname, user);

        return(1);
	}
    else if ((len = token(ptr,"lifdir")) )
    {
        char name[64];
        ptr += len;
        // IMAGE name
        ptr = get_token(ptr, name, 63);
        lif_dir(name);
        return(1);
    }
    else if ((len = token(ptr,"lifcreate")) )
    {
        char name[64],label[6];
        char num[12];
        long dirsecs, sectors, result;

        ptr += len;

        // IMAGE name
        ptr = get_token(ptr, name, 63);

        // IMAGE LABEL
        ptr = get_token(ptr, label, 7);

        // Directory Sectors
        ptr = get_token(ptr, num, 11);
        dirsecs = atol(num);

        // Image total Sectors
        ptr = get_token(ptr, num, 11);
        sectors= atol(num);

        ///@brief format LIF image
        result = lif_create_image(name,label,dirsecs,sectors);
        if(result != sectors)
        {
            if(debuglevel & 1)
                printf("create_format_image: failed\n");
        }
        return(1);
    }
	return(0);
}

/// @brief Check if characters in a volume of file name are valid
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
/// @brief Check that a IF volume name of directory name is valid
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

/// @brief string to LIF directory entry
/// @param[out] *B: LIF result with added trailing spaces
/// @param[in] *name: string
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


///@brief Convert a file name (unix/fat32) format into a valid LIF name 
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


///@brief Pack lifvol_t data into bytes
///@param[out] B: byte vector to pack data into
///@param[int] V: lifvol_t structure pointer
///@return V
lifvol_t *lif_PackVolume(uint8_t *B, lifvol_t *V)
{
	V2B_MSB(B,0,2,V->LIFid);
	lif_S2B(B+2,V->Label,6);
	V2B_MSB(B,8,4,V->DirStartSector);
	V2B_MSB(B,12,2,V->System3000LIFid);
	V2B_MSB(B,14,2,0);
	V2B_MSB(B,16,4,V->DirSectors);
	V2B_MSB(B,20,2,V->LIFVersion);
	V2B_MSB(B,22,2,0);
	V2B_MSB(B,24,4,V->tracks_per_side);
	V2B_MSB(B,28,4,V->sides);
	V2B_MSB(B,32,4,V->sectors_per_track);
	memcpy((void *) (B+36),V->date,6);
	return(V);
}

///@brief Check Volume Table
///@param[in] V: lifvol_t structure pointer
///@param[in] debug: dispaly diagostice messages
///@return 1 of ok, 0 on eeror
int lif_ValidateVolume(lifvol_t *V, int debug)
{
	int status = 1;
	if( !lif_checkname((char *)V->Label) )
	{
		status = 0;
		if(debug)
			printf("LIF Invalid Volume Name");
	}
	if(V->DirStartSector < 1)
	{
		status = 0;
		if(debug)
			printf("LIF Invalid start sector:%ld\n", V->DirStartSector);
	}
	if(V->System3000LIFid != 0x1000)
	{
		status = 0;
		if(debug)
			printf("LIF System3000 ID (%04XH) != 1000H\n", V->System3000LIFid);
	}
	if(V->zero1 != 0)
	{
		if(debug)
			printf("LIF zero1 != 0000H\n", V->zero1);
		status = 0;
	}
	if(V->DirSectors < 1)
	{
		if(debug)
			printf("Directory Sectors < 1\n");
		status = 0;
	}
	if(V->zero2 != 0)
	{
		if(debug)
			printf("LIF zero2 != 0000H\n", V->zero2);
		status = 0;
	}
	if(V->LIFVersion != 0)
	{
		if(debug)
			printf("LIF Version != 0\n");
		status = 0;
	}
	return(status);
}

///@brief UnPack lifvol_t data from bytes
///@param[in] B: byte vector to pack data into
///@param[out] V: lifvol_t structure pointer
///@return null
lifvol_t *lif_UnPackVolume(uint8_t *B, lifvol_t *V)
{

	V->LIFid = B2V_MSB(B,0,2);
	lif_B2S(B+2,V->Label,6);
	V->DirStartSector = B2V_MSB(B,8,4);
	V->System3000LIFid = B2V_MSB(B,12,2);
	V->zero1 = B2V_MSB(B,14,2);
	V->DirSectors = B2V_MSB(B,16,4);
	V->LIFVersion = B2V_MSB(B,20,2);
	V->zero2 = B2V_MSB(B,22,2);
	V->tracks_per_side = B2V_MSB(B,24,4);
	V->sides = B2V_MSB(B,28,4);
	V->sectors_per_track = B2V_MSB(B,32,4);
	memcpy((void *) V->date, (B+36),6);
	return(V);
}

///@brief Pack DireEntryType data into bytes
///@param[out] B: byte vector to pack data into
///@param[int] D: lifdirent_t structure pointer
///@return DIR
lifdir_t *lif_PackDir(uint8_t *B, lifdir_t *DIR)
{
	lif_S2B(B,DIR->DE.filename,10);				// 0
	V2B_MSB(B,10,2,DIR->DE.FileType);			// 10
	V2B_MSB(B,12,4,DIR->DE.FileStartSector);	// 12
	V2B_MSB(B,16,4,DIR->DE.FileSectors);	// 16
	memcpy(B+20,DIR->DE.date,6);				// 20
	V2B_MSB(B,26,2,DIR->DE.VolNumber);			// 26
	V2B_LSB(B,28,2,DIR->DE.FileBytes);			// 28
	V2B_LSB(B,30,2,DIR->DE.SectorSize);			// 30
	return(DIR);
}

///@brief UnPack lifdirent_t data from bytes
///@param[in] B: byte vector to extract data from
///@param[int] DIR: lifdirent_t structure pointer
///@return DIR
lifdir_t *lif_UnPackDir(uint8_t *B, lifdir_t *DIR)
{
	lif_B2S(B,DIR->DE.filename,10);
	DIR->DE.FileType = B2V_MSB(B, 10, 2);
	DIR->DE.FileStartSector = B2V_MSB(B, 12, 4);
	DIR->DE.FileSectors = B2V_MSB(B, 16, 4);
	memcpy(DIR->DE.date,B+20,6);
	DIR->DE.VolNumber = B2V_MSB(B, 26, 2); 
	DIR->DE.FileBytes = B2V_LSB(B, 28, 2);
	DIR->DE.SectorSize= B2V_LSB(B, 30, 2);
	return(DIR);
}

/// @brief Convert number >= 0 and <= 99 to BCD.
///
///  - BCD format has each hex nibble has a digit 0 .. 9
///
/// @param[in] data: number to convert.
/// @return  BCD value
/// @warning we assume the number is in range.
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

/// @brief Clear main lifdir_t DIR structure 
/// @param[in] *DIR: pointer to DIR structure
/// @return void
void lif_dir_clear(lifdir_t *DIR)
{
	memset((void *) DIR,0,sizeof(lifdir_t));
}


/// @brief Clear lifdirent_t DE structure in DIR
/// @param[in] *DIR: pointer to DIR structure
/// @return void
void lif_dirent_clear(lifdir_t *DIR)
{
	memset((void *) &DIR->DE,0,sizeof(lifdirent_t));
}

/// @brief Clear lifvol_t V structure in DIR
/// @param[in] *DIR: pointer to DIR structure
/// @return void
void lif_vol_clear(lifdir_t *DIR)
{
	memset((void *) &DIR->V,0,sizeof(lifvol_t));
}
/// @brief rewind LIF directory 
/// Modeled after Linux closedir()
/// Here we just rewind, there is nothing to unallocate
/// @param[in] *DIR: pointer to LIF Volume/Directoy structure
/// @return 0 on sucesss, -1 on error
void lif_rewinddir(lifdir_t *DIR)
{
	// Directory index
	DIR->index = 0;
	// File pointers
	DIR->current = DIR->V.DirStartSector+DIR->V.DirSectors;
	DIR->next = DIR->current;
}

/// @brief Close LIF directory 
/// Modeled after Linux closedir()
/// Here we just rewind, there is nothing to unallocate
/// @param[in] *DIR: pointer to LIF Volume/Directoy structure
/// @return 0 on sucesss, -1 on error
int lif_closedir(lifdir_t *DIR)
{
	if(DIR)
	{
		lif_rewinddir(DIR);
		return(0);
	}
	return(1);
}

/// @brief Open a file that must exist
/// @param[in] *name: file name of LIF image
/// @param[in] *mode: open mode - see fopen
/// @return FILE * pointer
FILE *lif_open(char *name, char *mode)
{
    FILE *fp = fopen(name, mode);
    if( fp == NULL)
    {
		if(debuglevel & 1)
			printf("lif_open: Can't open:[%s] mode:[%s]\n", name, mode);
        return(NULL);
    }
	return(fp);
}
/// @brief Stat a file 
/// @param[in] *name: file name of LIF image
/// @return struct stat *
stat_t *lif_stat(char *name)
{
	static stat_t _sb;
	stat_t *p = (stat_t *) &_sb;

	if(stat(name, p) < 0)
	{
		if(debuglevel & 1)
			printf("lif_stat: Can't stat:%s\n", name);
        return(NULL);
	}
	return(p);
}



/// @brief Read data from a LIF image 
/// File is closed after read
/// WHY? We want time minimize file open to to avoid corruption on the SDCARD
/// @param[in] *name: file name of LIF image
/// @param[in] *buf: read buffer
/// @param[in] offset: read offset
/// @param[in] bytes: number of bytes to read
/// @return number of bytes read - OR - -1 on error
long lif_read(char *name, void *buf, long offset, int bytes)
{
	FILE *fp;
	long len;

	fp = lif_open(name, "r");
    if( fp == NULL)
        return(-1);

	if(fseek(fp, offset, SEEK_SET) < 0)
	{
		if(debuglevel & 1)
			printf("lif_read: Seek error %s @ %ld\n", name, offset);
		fclose(fp);
		return(-1);
	}

	///@brief Initial file position
	len = fread(buf, 1, bytes, fp);
	if( len != bytes)
	{
		if(debuglevel & 1)
			printf("lif_read: Read error %s @ %ld\n", name, offset);
		fclose(fp);
		return(-1);
	}
	fclose(fp);
	return(len);
}

/// @brief Write data to an LIF image 
/// File is closed, positioned to the end of file, after write
/// Why? We want time minimize file open to to avoid corruption on the SDCARD
/// @param[in] *name: file name of LIF image
/// @param[in] *buf: write buffer
/// @param[in] offset: write offset
/// @param[in] bytes: number of bytes to write
/// @return -1 on error or number of bytes written 
int lif_write(char *name, void *buf, long offset, int bytes)
{
	FILE *fp;
	long len;

	fp = lif_open(name, "r+");
    if( fp == NULL)
        return(-1);

	// Seek to write position
	if(fseek(fp, offset, SEEK_SET) < 0)
	{
		if(debuglevel & 1)
			printf("lif_write: Seek error %s @ %ld\n", name, offset);

		// try to seek to the end of file anyway
		fseek(fp, 0, SEEK_END);
		fclose(fp);
		return(-1);

	}

	///@brief Initial file position
	len = fwrite(buf, 1, bytes, fp);
	if( len != bytes)
	{
		if(debuglevel & 1)
			printf("lif_write: Write error %s @ %ld\n", name, offset);

		// seek to the end of file before close!
		fseek(fp, 0, SEEK_END);
		fclose(fp);
		return(len);
	}

	// seek to the end of file before close!
	fseek(fp, 0, SEEK_END);
	fclose(fp);
	return(len);
}


/// @brief Convert bytes into used sectors
/// @param[in] bytes: size in bytes
/// @return sectors
uint32_t lif_bytes2sectors(long bytes)
{
	uint32_t sectors = (bytes/LIF_SECTOR_SIZE);
	if(bytes % LIF_SECTOR_SIZE)
		++sectors;
	return(sectors);
}

/// @brief Open LIF directory for reading
/// Modeled after Linux opendir()
/// @param[in] *name: file name of LIF image
/// @param[in] debug: Display extending error messages
/// @return NULL on error, lifdir_t pointer to LIF Volume/Directoy structure on sucesss
static lifdir_t _lifdir;
lifdir_t *lif_opendir(char *name, int debug)
{
	int len;
	stat_t *sb;
	lifdir_t *DIR = (lifdir_t *) &_lifdir;
	lifvol_t *V;
	uint8_t buffer[LIF_SECTOR_SIZE];

	lif_dir_clear(DIR);

	sb = lif_stat(name);
	if(sb == NULL)
        return(NULL);

	DIR->imagebytes = sb->st_size;
	DIR->imagesectors = lif_bytes2sectors(sb->st_size);

	strncpy(DIR->filename,name,LIF_IMAGE_NAME_SIZE-1);

	len = lif_read(DIR->filename, buffer, 0, LIF_SECTOR_SIZE);
	if( len < LIF_SECTOR_SIZE)
        return(NULL);

	// Get Volume header and Directory start sector
	V = lif_UnPackVolume(buffer, (lifvol_t *)&DIR->V);

	if(!lif_ValidateVolume(V, debug))
		return(NULL);

	lif_updatefree(DIR);

	// File pointers
	DIR->current = DIR->V.DirStartSector+DIR->V.DirSectors;
	DIR->next = DIR->current;


	// FIXME return NULL on invalid Volume ???
	return(DIR);
}

/// @brief Open LIF directory for reading
/// Modeled after Linux readdir()
/// @param[in] *DIR: to LIF Volume/Diractoy structure 
/// @return lifdirent_t filled with directory structure, or NULL on error of end of Directory
lifdirent_t *lif_readdir(lifdir_t *DIR)
{
	int len;
	long offset,end;
	uint8_t dirent[LIF_DIR_SIZE];

	offset = (DIR->index * LIF_DIR_SIZE) + (DIR->V.DirStartSector * LIF_SECTOR_SIZE);
	end = (DIR->V.DirStartSector +  DIR->V.DirSectors) * LIF_SECTOR_SIZE;

	if(offset >= end)
		return(NULL);

	len = lif_read(DIR->filename, dirent, offset, sizeof(dirent));
	if( len < (long)sizeof(dirent))
        return(NULL);

	// extract DIR->DE settings from dirent
	lif_UnPackDir(dirent, DIR);

	if(DIR->DE.VolNumber != 0x8001)
		return(NULL);

	if(DIR->DE.FileType == 0xffff)
		return(NULL);

	DIR->current = DIR->DE.FileStartSector;
	DIR->next = DIR->DE.FileStartSector + DIR->DE.FileSectors;	
	DIR->index++;
	return((lifdirent_t *) &DIR->DE);
}

/// @brief Write a director entry
/// Modeled after Linux readdir()
/// @param[in] *DIR: to LIF Volume/Diractoy structure 
/// @return number of bytes written
long lif_writedir(lifdir_t *DIR)
{
	int len;
	long offset,end;
	uint8_t dirent[LIF_DIR_SIZE];

	offset = (DIR->index * LIF_DIR_SIZE) + (DIR->V.DirStartSector * LIF_SECTOR_SIZE);
	end = (DIR->V.DirStartSector +  DIR->V.DirSectors) * LIF_SECTOR_SIZE;

	if(offset >= end)
		return(-1);

	// store DIR->DE settings into dirent
	lif_PackDir(dirent, DIR);

	len = lif_write(DIR->filename, dirent, offset, sizeof(dirent));
	if( len < (long)sizeof(dirent))
        return(-1);

	// Update used sectors
	DIR->used += DIR->DE.FileSectors;
	DIR->free -= DIR->DE.FileSectors;
	// Update first free sector as we read
	DIR->current = DIR->DE.FileStartSector;
	DIR->next = DIR->DE.FileStartSector + DIR->DE.FileSectors;	
	DIR->index++;
	return(len);
}


/// @brief get LIF file size from directory entry
/// @param[in] name: LIF disk image name
/// @param[in] DE: LIF directory entry
/// @retrun void
long lif_filelength(lifdirent_t *DE)
{
	long bytes;
	bytes = DE->FileBytes;
	if(!bytes)
		bytes = (DE->FileSectors * LIF_SECTOR_SIZE);
	return(bytes);
}


/// @brief Find offset to first free sector that is big enough
/// Modeled after Linux readdir()
/// @param[in] *DIR: DIR pointer
/// @return Directory pointer
lifdir_t *lif_find_free(lifdir_t *DIR, uint32_t sectors)
{
	lifdirent_t *DE;

	if(DIR == NULL)
		return(NULL);

	if(sectors > DIR->free)
		return(NULL);

	lif_rewinddir(DIR);
	while(1)
	{
		DE = lif_readdir(DIR);
		if(DE == NULL)
			break;
		// We can reuse this purged sector
		if(DE->FileType == 0 && DE->FileSectors >= sectors)
			break;
	}

	// DIR->next will point to free space
	// DIR->current points to current file
	return(DIR);		// Last valid directory entry
}

/// @brief Find free space
/// @param[in] *DIR: to LIF Volume/Diractoy structure 
/// @return Directory pointer
lifdir_t *lif_updatefree(lifdir_t *DIR)
{
	lifdirent_t *DE;

	if(DIR == NULL)
		return(NULL);

	lif_rewinddir(DIR);
	DIR->free = DIR->imagesectors  - (DIR->V.DirSectors + DIR->V.DirStartSector);
	DIR->used = 0;
	while(1)
	{
		DE = lif_readdir(DIR);
		if(DE == NULL)
			break;
		if(DE->FileType)
			DIR->used += DIR->DE.FileSectors;
	}
	DIR->free -= DIR->used;
	lif_rewinddir(DIR);
	return(DIR);		// Last valid directory entry
}
	
/// @brief Display a LIF image file directory
/// @param[in] lifimagename: LIF disk image name
/// @return -1 on error or number of files found
int lif_dir(char *lifimagename)
{
	lifdirent_t *DE;
	long bytes;
	int files = 0;
	int purged = 0;
	lifdir_t *DIR;

	DIR = lif_opendir(lifimagename,1);

	if(DIR == NULL)
		return(-1);
	
	printf("Volume: [%s]\n", DIR->V.Label);
	printf("NAME         TYPE   START SECTOR        SIZE     RECSIZE\n");
	while(1)
	{
		DE = lif_readdir(DIR);
		if(DE == NULL)
			break;

		bytes = lif_filelength(DE);

		// name type start size
		printf("%-10s  %04Xh      %8lXh   %9ld       %4d\n", 
			DE->filename, 
			(int)DE->FileType, 
			(long)DE->FileStartSector, 
			(long)bytes, 
			(int)DE->SectorSize  );

		if(DE->FileType != 0)
			++files;
		else
			++purged;
	}	
	printf("\n");
	printf("%8ld Files\n", (long)files);
	printf("%8ld Purged\n", (long)purged);
	printf("%8ld Used sectors\n", (long)DIR->used);
	printf("%8ld Free sectors\n", (long)DIR->free);
	printf("%8ld First free sector (%lXh)\n", (long)DIR->next,(long)DIR->next);
	lif_closedir(DIR);

	return(files);
}

/// @brief Find a directory entry for a file in a LIF image file
/// @param[in] *DIR: directory pointer
/// @param[in] username: File name in LIF image
/// @return lifdirent_t * directory entry, or NULL if no match
lifdirent_t *lif_find_file(lifdir_t *DIR, char *username)
{
	lifdirent_t *DE;

	if(DIR == NULL)
		return(NULL);
	
	DE = NULL;
	while(1)
	{
		DE = lif_readdir(DIR);
		if(DE == NULL)
			break;

		if(strcasecmp((char *)DE->filename,username) == 0)
			break;
	}
	lif_rewinddir(DIR);
	return(DE);
}

/// @brief Extarct an 0xe010 ascii file from a LIF image
/// @param[in] lifimagename: LIF disk image name
/// @param[in] lifname:  name of file in LIF image
/// @param[in] username: name to call the extracted image
/// @return -1 on error or number of files found
int lif_extract_file(char *lifimagename, char *lifname, char *username)
{
	lifdir_t *DIR;
	lifdirent_t *DE;
	uint32_t start, end;	// sectors
	long offset, bytes;		// bytes
	int i, len,size;
	int status = 1;
	int done = 0;

	int ind,wind;
	FILE *fp;

	uint8_t buf[LIF_SECTOR_SIZE+4];
	uint8_t wbuf[LIF_SECTOR_SIZE+4];


	DIR = lif_opendir(lifimagename, 1);
	if(DIR == NULL)
	{
		printf("LIF image not found:%s\n", lifimagename);
		return(0);
	}

	DE = lif_find_file(DIR, lifname);
	if(DE == NULL)
	{
		printf("File not found:%s\n", username);
		return(0);
	}
	if((DE->FileType & 0xFFFC) != 0xE010)
	{
		printf("File %s has wrong type:[%04XH] expected 0xE010..0xE013\n", username, (int) DE->FileType);
		return(0);
	}

	start = DE->FileStartSector;
	end = start + DE->FileSectors;

	offset = start * LIF_SECTOR_SIZE;

	fp = lif_open(username,"w");
	if(fp == NULL)
		return(0);

	printf("Extracting: %s\n", username);

	bytes = 0;
	wind = 0;
	ind = 0;

	while(lif_bytes2sectors(offset) <= end)
	{
		size = lif_read(lifimagename, buf, offset, LIF_SECTOR_SIZE);
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
					printf("lif_extract_file: string too big size = %d\n", (int)len);
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
				printf("lif_extract_file: unexpected control byte:[%02XH] @ offset: %8lx, ind:%02XH\n", (int)buf[ind], offset, (int)ind);
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
					size = fwrite(wbuf,1,wind,fp);
					if(size < wind)
					{
						printf("lif_extract_file: write error\n");
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
	// Flush any remaining bytes
	if(wind)
	{
		size = fwrite(wbuf,1,wind,fp);
		if(size < wind)
		{
			printf("lif_extract_file: write error\n");
			status = 0;
		}
		bytes += size;
	}
	fclose(fp);
	sync();
	printf("Wrote: %8ld\n", bytes);
	return(status);
}
	


///@brief Pad the last sector to the end
/// When DIR is null we do not write, just compute the write size
/// @param[in] DIR: LIF image file sructure
/// @param[in] offset: offset to write to
/// @return number of bytes written ,  -1 on error
int lif_write_pad(lifdir_t *DIR, long offset)
{
	int i, len, pos, rem;
	uint8_t buf[LIF_SECTOR_SIZE+1];

	pos = (offset % LIF_SECTOR_SIZE);
	rem = LIF_SECTOR_SIZE - pos;

	if(DIR && rem < LIF_SECTOR_SIZE)
	{
		buf[0]  = 0xef;
		for(i=1;i<rem;++i)
			buf[i]  = 0xff;

		len = lif_write(DIR->filename, buf, offset, rem);
		if(len < rem)
			return(-1);

		if(debuglevel & 0x400)
			printf("Write Offset:   %4lxH\n", (long)offset/LIF_SECTOR_SIZE);
	}

	return(rem);
}

/** @brief  HP85 ASCII LIF records have a 3 byte header 
	ef [ff]* = end of data in this sector (no size) , pad with ff's optionally if there is room
	df size = string
	cf size = split accross sector end "6f size" at start of next sector 
		   but the 6f size bytes are not included in cf size! (yuck!)
	6f size = split continue (always starts at sector boundry)
	df 00 00 ef [ff]* = EOF (df size = 0) ef send of sector and optional padding
	size = 16 bits LSB MSB

	Example:
	000080e0 : 4b 7c 22 0d cf 29 00 31 34 20 44 49 53 50 20 22  : K|"..).14 DISP "
	000080f0 : 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 5f  :                _
	00008100 : 6f 10 00 5f 5f 5f 5f 5f 5f 5f 5f 5f 5f 5f 5f 5f  : o.._____________
	00008110 : 5f 22 0d df 2b 00 31 35 20 44 49 53 50 20 22 20  : _"..+.15 DISP "

	cf 29 00 (19 is to sector end) (new sector start with 6F 10 00 (10 is remainder)
	So 29 = 19 and 10 (yuck!)
*/


///@brief Write a string with header to a lif file
/// When DIR is null we do not write, just compute the write size
/// @param[in] DIR: LIF image file sructure
/// @param[in] offset: offset to write to
/// @param[in] str: string to write
/// @return wtite size in bytes,  -1 on error
int lif_write_string(lifdir_t *DIR, long offset, void *vptr)
{
	int ind;
	int len;
	int bytes;
	int pos,rem;
	
	char *str = vptr;

	uint8_t buf[LIF_SECTOR_SIZE+6+1];

	bytes = 0;

	// String size
	len = strlen(str);

	// Compute the current offset in this sector
	pos = (offset % LIF_SECTOR_SIZE);

	// Number of bytes free in this sector
	rem = LIF_SECTOR_SIZE - pos;

	// buffer index
	ind = 0;

	// Does the string + header fit ?
	if(rem < (3 + len))
	{
		// If we split a string we end up writting two headers
		//  So for less then 6 bytes is no point splitting those strings
		//  We just pad and write the string in the next sector
		//  
		if(rem < 6)
		{
			// PAD
			buf[ind++] = 0xEF;
			while(ind<rem)
				buf[ind++] = 0xff;

			// NEW SECTOR
			// Debugging make sure we are at sector boundry
			if(((offset + ind)  % LIF_SECTOR_SIZE))
			{
				printf("Expected sector boundry, offset:%d\n", (int) ((offset + ind) % LIF_SECTOR_SIZE) );
				return(-1);
			}
			// Write string in new sector
			// The full string + header will fit
			buf[ind++] = 0xDF;
			buf[ind++] = len & 0xff;
			buf[ind++] = (len >> 8) & 0xff;
			// Write string
			while(*str)
				buf[ind++] = *str++;
		}
		else
		{
			// Split string
			// String spans sector , so split the string
			// Split string Header
			buf[ind++] = 0xCF;
			buf[ind++] = len & 0xff;
			buf[ind++] = (len >>8) & 0xff;
			// Write as much of the string as we can in this sector
			while(*str && ind<rem)
				buf[ind++] = *str++;

			// NEW SECTOR
			// Debugging make sure we are at sector boundry
			if(((offset + ind)  % LIF_SECTOR_SIZE))
			{
				printf("Expected sector boundry, offset:%d\n", (int) ((offset + ind) % LIF_SECTOR_SIZE) );
				return(-1);
			}

			// Update remining string length
			len = strlen(str);
			// Write split string continuation heaader at start of new sector
			buf[ind++] = 0x6F;
			buf[ind++] = (len & 0xff);
			buf[ind++] = (len>>8) & 0xff;
			// Write string
			while(*str)
				buf[ind++] = *str++;
		}
	}
	else 
	{
		// The full string + header will fit
		buf[ind++] = 0xdf;
		buf[ind++] = len & 0xff;
		buf[ind++] = (len >> 8) & 0xff;
		while(*str)
			buf[ind++] = *str++;

	}
	// Now Write string
	if(DIR)
	{

		len = lif_write(DIR->filename, buf, offset, ind);
		if(len < ind)
			return(-1);

		if(debuglevel & 0x400)
			printf("Write Offset:   %4lxH\n", (long)offset/LIF_SECTOR_SIZE);
	}

	offset += ind;
	bytes += ind;
	return( bytes );
}
	

/// @brief Convert a user ASCII file into HP85 0xE010 LIF format 
/// We must know the convered file size BEFORE writting
/// So we must call this function TWICE
///   1) find out how big the converted file is (DIR is NULL)
///   2) Specify where to put it (DIR is set)
///
/// @param[in] userfile: User ASCII file source
/// @param[in] *DIR: Where to write file if set (not NULL)
/// @return size of LIF image in bytes, or -1 on error
long lif_ascii2lif(char *name, lifdir_t *DIR)
{
	long offset;
	long bytes;
	int count;
	int ind;
	FILE *fi;

	uint8_t str[LIF_SECTOR_SIZE+1];

	fi = lif_open(name, "r");
	if(fi == NULL)
		return(-1);

	offset = 0;
	if(DIR)
	{
		// DIR->next should be pointing at first free sector
		offset  = DIR->next * LIF_SECTOR_SIZE;
	}

	bytes = 0;

	count = 0;
	// Read user file and write LIF records
	// reserve 3 + LIF header bytes + 1 (EOS)
	while( fgets((char *)str,(int)sizeof(str) - 4, fi) != NULL )
	{
		trim_tail((char *)str);

		strcat((char *)str,"\r"); // HP85 lines end with "\r"

		// Write string
		ind = lif_write_string(DIR, offset, str);
		if(ind < 0)
		{
			fclose(fi);
			return(-1);
		}

		offset += ind;
		bytes += ind;
		count += ind;

		if(DIR)
		{
			if(count > 256)
			{		
				count = 0;
				printf("Wrote: %8ld\r",(long)bytes);
			}
		}
	}

	fclose(fi);

	str[0] = 0;
	// Write EOF string
	ind = lif_write_string(DIR, offset, str);
	if(ind < 0)
		return(-1);

	offset += ind;
	bytes += ind;

	// PAD the end of this last sector IF any bytes have been written to it
	// Note: we do not add the pad to the file size!
	ind = lif_write_pad(DIR, offset);
	if(ind < 0)
		return(-1);

	// Padding at then end does not count toward its size

	if(DIR)
		printf("Wrote: %8ld\r",(long)bytes);

	return(bytes);
}



/// @brief Add a user file to the LIF image
/// The basename of the lifname, without extensions, is used as the LIF file name
/// @param[in] lifimagename: LIF image name
/// @param[in] lifname: LIF file name
/// @param[in] userfile: userfile name
/// @return size of data written into to LIF image, or -1 on error
long lif_add_file(char *lifimagename, char *lifname, char *userfile)
{
	long bytes;
	long sectors;
	long len;
	lifdir_t *DIR;

	if(!*lifimagename)
	{
		if(debuglevel & 1)
			printf("lif_add_file: lifimagename is empty\n");
		return(-1);
	}
	if(!*lifname)
	{
			if(debuglevel & 1)
				printf("lif_add_file: lifname is empty\n");
		return(-1);
	}
	if(!*userfile)
	{
		if(debuglevel & 1)
			printf("lif_add_file: userfile is empty\n");
		return(-1);
	}

	if(debuglevel & 0x400)
		printf("LIF image:[%s], LIF name:[%s], user file:[%s]\n", 
			lifimagename, lifname, userfile);

	// Find out how big converted file will be
	bytes = lif_ascii2lif(userfile, NULL);
	if(bytes < 0)
		return(-1);

	DIR = lif_opendir(lifimagename,1);
	if(DIR == NULL)
		return(-1);	

	sectors = lif_bytes2sectors(bytes);

	// Now find free entry
	DIR = lif_find_free(DIR, sectors);
	if(!DIR)
	{
		printf("LIF image:[%s], not enough free space for:[%s]\n", 
			lifimagename, userfile);
			return(-1);
	}

	// Write converted file into free space
	bytes = lif_ascii2lif(userfile, DIR);

	// Setup directory entry
	lif_fixname(DIR->DE.filename, lifname,10);
	DIR->DE.FileType = 0xe010;  			// 10
	DIR->DE.FileStartSector = DIR->next;	// 12
	DIR->DE.FileSectors = sectors;    // 16
	memset(DIR->DE.date,0,6);				// 20
	DIR->DE.VolNumber = 0x8001;				// 26
	DIR->DE.FileBytes = 0;					// 28
	DIR->DE.SectorSize  = 0x100;			// 30

	if(debuglevel & 0x400)
	{
		printf("New Directory Information BEFORE write\n");
		printf("Name:              %s\n", DIR->DE.filename);
		printf("Index:            %4d\n", (int)DIR->index);
		printf("First Sector:    %4lxH\n", DIR->DE.FileStartSector);
		printf("Length Sectors:  %4lxH\n", DIR->DE.FileSectors);
		printf("Used Sectors:    %4lxH\n", (long)DIR->used);
	}

	// Write directory entry
	len = lif_writedir(DIR);
	if(len < 0)
		return(-1);
	if(debuglevel & 0x400)
	{
		printf("New Directory Information AFTER write\n");
		printf("Index:            %4d\n", (int)DIR->index);
		printf("First Sector:    %4lxH\n", DIR->DE.FileStartSector);
		printf("Length Sectors:  %4lxH\n", DIR->DE.FileSectors);
		printf("Used Sectors:    %4lxH\n", (long)DIR->used);
	}
	printf("Wrote: %8ld\n", bytes);

	// Return file size
	return(bytes);
}


/// @brief Create/Format a LIF disk image
/// This can take a while to run, about 1 min for 10,000,000 bytes
/// @param[in] lifimagename: LIF disk image name
/// @param[in] liflabel: LIF Volume Label name
/// @param[in] dirsecs: Number of LIF directory sectors
/// @param[in] sectors: total disk image size in sectors
///@return bytes writting to disk image
long lif_create_image(char *lifimagename, char *liflabel, uint32_t dirsecs, uint32_t sectors)
{
    FILE *fp;
	long remainder, sector;
	long li;
	int len;
	int i;
	uint8_t buffer[LIF_SECTOR_SIZE];
	
	static lifdir_t _DIR;
	lifdir_t *DIR = (lifdir_t *) &_DIR;

	if(!*lifimagename)
	{
		if(debuglevel & 1)
			printf("lif_create_image: lifimagename is empty\n");
		return(-1);
	}
	if(!*liflabel)
	{
		if(debuglevel & 1)
			printf("lif_create_image: liflabel is empty\n");
		return(-1);
	}
	if(!dirsecs)
	{
		if(debuglevel & 1)
			printf("lif_create_image: dirsecs is 0\n");
		return(-1);
	}
	if(!sectors)
	{
		if(debuglevel & 1)
			printf("lif_create_image: sectors is 0\n");
		return(-1);
	}


	lif_vol_clear(DIR);

	// Initialize volume header
	DIR->V.LIFid = 0x8000;
	lif_fixname(DIR->V.Label, liflabel, 6);
	DIR->V.DirStartSector = 2;
	DIR->V.DirSectors = dirsecs;
	DIR->V.System3000LIFid = 0x1000;
	DIR->V.tracks_per_side = 0;
	DIR->V.sides = 0;
	DIR->V.sectors_per_track = 0;
	///@brief Current Date
	lif_time2lif(DIR->V.date, time(NULL));

	printf("Formating LIF image:[%s], Label:[%s], Dir Sectors:[%ld], sectors:[%ld]\n", 
		lifimagename, DIR->V.Label, (long)DIR->V.DirSectors, (long)sectors);

	// Size of of disk after volume start and directory sectors hae been written
	remainder = sectors - (DIR->V.DirSectors + DIR->V.DirStartSector);
	if(remainder < 0)
	{
		if(debuglevel & 1)
			printf("lif_create_image: Too few sectors specified in image fil\n");
		return(-1);
	}

	///@brief Open LIF disk image for writting
    fp = fopen(lifimagename, "w");
    if( fp == NULL)
    {
		if(debuglevel & 1)
			printf("lif_create_image: Can't open:%s\n", lifimagename);
        return( -1 );
    }

	///@brief Initial file position
	sector = 0;

	lif_PackVolume(buffer, (lifvol_t *) &DIR->V);

	// Write Volume Header
	len = fwrite(buffer, 1, LIF_SECTOR_SIZE, fp);
	if( len < LIF_SECTOR_SIZE)
	{
		if(debuglevel & 1)
			printf("lif_create_image: Write error %s @ %ld\n", lifimagename, sector);
		fclose(fp);
		sync();
        return( -1 );
	}
	++sector;

	// Write Empty Sector
	memset(buffer,0,LIF_SECTOR_SIZE);
	for(li=1; (uint32_t) li < DIR->V.DirStartSector; ++li)
	{
		len = fwrite(buffer, 1, LIF_SECTOR_SIZE, fp);
		if( len < LIF_SECTOR_SIZE)
		{
			if(debuglevel & 1)
				printf("lif_create_image: Write error %s @ %ld\n", lifimagename, sector);
			fclose(fp);
			sync();
			return( -1 );
		}
		++sector;
	}

	lif_dirent_clear(DIR);
	// Fill in Directory Entry
	///@brief File type of 0xffff is last directory entry
	DIR->DE.FileType = 0xffff;
	///FIXME this is the default that the HPdrive project uses, not sure of this
	DIR->DE.FileSectors = 0x7fffUL;

	// Fill sector with Directory entries
	for(i=0; i<LIF_SECTOR_SIZE; i+= LIF_DIR_SIZE)
	{
		// store settings into buffer
		lif_PackDir(buffer+i, DIR);
	}

	// Write Directory sectors
	for(li=0;(uint32_t) li< DIR->V.DirSectors;++li)
	{
		len = fwrite(buffer, 1, LIF_SECTOR_SIZE, fp);
		if( len < LIF_SECTOR_SIZE)
		{
			if(debuglevel & 1)
				printf("lif_create_image: Write error %s @ %ld\n", lifimagename, sector);
			fclose(fp);
			sync();
			return( -1 );
		}
		++sector;
		printf("sector: %ld\r", sector);
	}

	///@brief Zero out remining disk image
	memset(buffer,0xff,LIF_SECTOR_SIZE);

	// Write remaining in large chunks for speed
	for(li=0;li<remainder;++li)
	{
		len = fwrite(buffer, 1, LIF_SECTOR_SIZE, fp);
		if( len < LIF_SECTOR_SIZE)
		{
			if(debuglevel & 1)
				printf("lif_create_image: Write error %s @ %ld\n", lifimagename, sector);
			fclose(fp);
			sync();
			return( -1 );
		}
		++sector;
		printf("sector: %ld\r", sector);
	}

	fclose(fp);
	sync();

	printf("Formating: wrote:[%ld] sectors\n", sectors);
    return(sector);
}