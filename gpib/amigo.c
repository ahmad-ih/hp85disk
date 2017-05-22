/**
 @file gpib/amigo.c

 @brief AMIGO disk emulator for HP85 disk emulator project for AVR8.

 @par Edit History - [1.0]   [Mike Gore]  Initial revision of file.

 @par Copyright &copy; 2014 Mike Gore, Inc. All rights reserved.

 @par Based on work by Anders Gustafsson.

 @par Copyright &copy; 2014 Anders Gustafsson All rights reserved..

*/


#include "user_config.h"

#include "defines.h"
#include "gpib.h"
#include "gpib_hal.h"
#include "gpib_task.h"
#include "amigo.h"

/// @verbatim
/// SS80 References: ("SS80" is the short form used in the project)
///  "Subset 80 from Fixed and flexible disc drives"
///   Printed November, 1985
///   HP Part# 5958-4129
///
/// CS80 References: ("CS80" is the short form used in the project)
///   "CS/80 Instruction Set Programming Manual"
///   Printed: APR 1983
///   HP Part# 5955-3442
///
/// Amigo References: ("Amigo" is the short form used in the project)
///   "Appendix A of 9895A Flexible Disc Memory Service Manual"
///   HP Part# 09895-90030
/// @endverbatim

/// @verbatim
///  Reference: Amigo Command Set Reference used in this document
///  Short Form 		Title
///  SS80 		"Subset 80 from Fixed and flexible disc drives" 5958-4129
///  HP9123 		9123 disc drive manual addendum 5957-6584
///  A 			Appendix A "HP 9895A Disc Memory Commends Set"
/// 			See Receive Status A16
///  L    Buffered Read           	0x05    2		A33		Y
///  L    Request Physical Address 	0x14    2		A21		Y 0x68
/// 			See Send Address A20
/// 
///  0x6B Command Op Codes        	OP      bytes
///  L    Buffered Read Verify    	0x05    2		A37		Y
///  L    ID Triggered Read       	0x06    2		A41		Y
/// 
///  0x6C Command Op Codes        	OP      bytes
///  L    Unbuffered Read Verify  	0x05    2		A38		Y
///  L    Request Physical Address	0x14	2		A21		Y 0x68
///  L    Format Request           	0x18    2		A50		N
///  L    Door Lock               	0x19    2		A30		N
///  L    Door Unlock             	0x1A    2		A31		N
/// 
///  0x6F Command Op Codes        	OP      bytes
///  L    Download Controller     	---     1..256	A26		N
/// 
///  0x70 Command Op Codes        	OP      bytes
///  L    HP-300 Clear            	---     1		A23		N
///  T    DSJ                     	---     1		A11		N
/// 
///  0x77 Command Op Codes        	OP      bytes
///  T/L  HP-IB CRC               	---     ---		A30		N
/// 
///  0x7E Command Op Codes        	OP      bytes
///  L    Write Loopback Record 	---     1..256	A25		N
///  T    Read Loopback Record 		---     1..256	A14		N
/// 
///  0x7F Command Op Codes        	OP      bytes
///  T    Read Self Test Results    ---     2		A13		N
///  L    Initiat Self Test       	---     2		A24		N
/// @endverbatim


/// @verbatim
/// Secondary_Commands and OP Code processing
///
/// 0x60 Command Op Codes        	OP      bytes
/// L    Receive Data            	---     ---
///			Execute of most receive data requests
///			All Disk Write
/// T    Send Data               	---     ---
///			Execute of most send data requests
///			All Disk Read
///
/// 0x68 Command Op Codes        	OP      bytes	Ref		Execute Phase
/// L    Cold Load Read          	0x00    2		A40		Y
/// L    Seek                    	0x02    6		A27		N
/// L    Request Status buffered		0x03    2		A15		Y 0x68
/// L    Unbuffered Read         	0x05    2		A35		Y
/// L    Verify                  	0x07    4		A36		N
/// L    Unbuffered Write Request	0x08   	2		A45		Y
/// L    Initialize              	0x0B    2		A46		Y
/// L    Initialize DBIT            	0x2B    2		A46		Y
/// L    Request Logical Address 	0x14    2		A20		Y 0x68
/// L	End							0x15	2		A29		N
/// T    Send Status or Address 		----    4		A20		N
///			Execute of all Address and Status Requests 
///			(Y 0x68)
///
/// 0x69 Command Op Codes        	OP      bytes
/// L    Buffered Write Request  	0x08    2		A43		Y
///
/// 0x6A Command Op Codes        	OP      bytes
/// L    Request Status Unbuffered   0x03    2		A15		Y 0x68
///			See Receive Status A16
/// L    Buffered Read           	0x05    2		A33		Y
/// L    Request Physical Address 	0x14    2		A21		Y 0x68
///			See Send Address A20
///
/// 0x6B Command Op Codes        	OP      bytes
/// L    Buffered Read Verify    	0x05    2		A37		Y
/// L    ID Triggered Read       	0x06    2		A41		Y
///
/// 0x6C Command Op Codes        	OP      bytes
/// L    Unbuffered Read Verify  	0x05    2		A38		Y
/// L    Request Physical Address	0x14	2		A21		Y 0x68
/// L    Format Request             	0x18    2		A50		N
/// L    Door Lock               	0x19    2		A30		N
/// L    Door Unlock             	0x1A    2		A31		N
///
/// 0x6F Command Op Codes        	OP      bytes
/// L    Download Controller     	---     1..256	A26		N
///
/// 0x70 Command Op Codes        	OP      bytes
/// L    HP-300 Clear            	---     1		A23		N
/// T    DSJ                     	---     1		A11		N
///
/// 0x77 Command Op Codes        	OP      bytes
/// T/L  HP-IB CRC               	---     ---		A30		N
///
/// 0x7E Command Op Codes        	OP      bytes
/// L    Write Loopback Record 		---     1..256	A25		N
/// T    Read Loopback Record 		---     1..256	A14		N
///
/// 0x7F Command Op Codes        	OP      bytes
/// T    Read Self Test Results     	---     2		A13		N
/// L    Initiat Self Test       	---     2		A24		N
/// @endverbatim

extern uint8_t talking;
extern uint8_t listening;


enum AMIGO_states
{
    AMIGO_IDLE = 0,
    AMIGO_REQUEST_STATUS,
    AMIGO_REQUEST_STATUS_UNBUFFERED,
    AMIGO_REQUEST_STATUS_BUFFERED,
    AMIGO_REQUEST_LOGICAL_ADDRESS,
    AMIGO_COLD_LOAD_READ,
    AMIGO_READ_UNBUFFERED,
    AMIGO_READ_BUFFERED,
    AMIGO_WRITE_UNBUFFERED,
    AMIGO_WRITE_BUFFERED,
    AMIGO_INITIALIZE,
};

/// @brief Initialize Amigo state machine, Disk Position, Error status
/// @return  void

void amigo_init()
{
    AMIGOState.state = AMIGO_IDLE;

    memset(AMIGOState.status,0,sizeof(AMIGOState.status));
    memset(AMIGOState.logical_address,0,sizeof(AMIGOState.logical_address));

    AMIGOState.unitNO = 0;

    AMIGOState.sector = 0;
    AMIGOState.head = 0;
    AMIGOState.cyl = 0;

    AMIGOState.dsj = 2;
    AMIGOState.Errors = 0;

/// @todo  verify that we always want PPR disabled
    DisablePPR(AMIGO_PPR);
}


/// @brief  Set current address in AMIGOState.logical_address[]
///
/// - Reference: A15-A19
/// - State: Command
/// @return  0

int amigo_request_logical_address()
{

    AMIGOState.logical_address[0] = 0xff & (AMIGOState.cyl >> 8);
    AMIGOState.logical_address[1] = 0xff & (AMIGOState.cyl); //LSB
    AMIGOState.logical_address[2] = 0xff & (AMIGOState.head);
    AMIGOState.logical_address[3] = 0xff & (AMIGOState.sector);
    return(0);
}


/// @brief  Set drive status in AMIGOState.status[]
///
/// - Reference: A15-A19.
/// - State: Command.
/// @return  0

int amigo_request_status()
{
/// @todo  dsj
#if SDEBUG > 1
    if(debuglevel > 1)
        printf("[AMIGO request status]\n");
#endif
    AMIGOState.status[0] = 0x00;                       // Status 1
    AMIGOState.status[1] = AMIGOState.unitNO;                 // Unit
    AMIGOState.status[2] = 0x0d;                       // Status 2 (0110 = hp format) << 1, 1=HP9121
    AMIGOState.status[3] = 0x00;                       //


    if(mmc_wp_status())
    {
        AMIGOState.status[3] |= 0x40;                  // Write protect 0x40, reserved = 0x20
        AMIGOState.status[3] |= 0x20;                  // reserved = 0x20 ???
    }

    if(AMIGOState.dsj == 2)
    {
        AMIGOState.status[0] = 0b00010011;             // S1 error power on
        AMIGOState.status[3] |= 0x08;                  // F bit, power up
    }
    else if(AMIGOState.Errors || AMIGOState.dsj == 1)
    {
        if(AMIGOState.Errors & ERR_GPIB)
            AMIGOState.status[0] = 0b00001010;         // S1 error I/O error
        else if(AMIGOState.Errors & ERR_DISK)
            AMIGOState.status[3] |= 0x03;              // Do disk in drive
        else if(AMIGOState.Errors & ERR_WRITE)
            AMIGOState.status[0] = 0b00010011;         // S1 error write error
        else if(AMIGOState.Errors & ERR_SEEK)
            AMIGOState.status[3] |= 0x04;              // Seek
        AMIGOState.status[3] |= 0x10;                  // E bit hardware failure
        AMIGOState.status[2] |= 0x80;                  // Bit 15
    }

    EnablePPR(AMIGO_PPR);
    return(0);
}


/// @brief  Send current address in AMIGOState.logical_address[]
///
/// - Reference: A15-A19.
/// - State: Execute
/// @return  0 on sucess
/// @return status GPIB error flags on fail
/// @see gpib.h ERROR_MASK defines for a full list.

int amigo_send_logical_address()
{
    uint16_t status;
    UINT len;

#if SDEBUG > 1
    if(debuglevel > 1)
        printf("[AMIGO send logical address]\n");
#endif
    status = EOI_FLAG;
    len = gpib_write_str(AMIGOState.logical_address,4,&status);
    if(status & ERROR_MASK)
        AMIGOState.Errors |= ERR_GPIB;
    if( len != 4)
    {
        AMIGOState.Errors |= ERR_GPIB;
        AMIGOState.dsj = 1;
#if SDEBUG >= 1
        if(debuglevel >= 1)
            printf("[AMIGO GPIB write error]\n");
#endif
        EnablePPR(AMIGO_PPR);
        return(status & ERROR_MASK);
    }
    EnablePPR(AMIGO_PPR);
    return(status & ERROR_MASK);
}


/// @brief Send drive status in AMIGOState.status[]
///
/// - Reference: A15-A19.
/// - State: Execute>
/// @return  0
/// @return status GPIB error flags on fail
/// @see gpib.h ERROR_MASK defines for a full list.

int amigo_send_status()
{
    uint16_t status;
    UINT len;

#if SDEBUG > 1
    if(debuglevel > 1)
        printf("[AMIGO send status]\n");
#endif
    status = EOI_FLAG;
    len = gpib_write_str(AMIGOState.status,4,&status);
    if(status & ERROR_MASK)
        AMIGOState.Errors |= ERR_GPIB;
    if( len != 4)
    {
        AMIGOState.Errors |= ERR_GPIB;
        AMIGOState.dsj = 1;
#if SDEBUG >= 1
        if(debuglevel >= 1)
            printf("[AMIGO GPIB write error]\n");
#endif
        EnablePPR(AMIGO_PPR);
        return(status & ERROR_MASK);
    }
    AMIGOState.Errors = 0;
    AMIGOState.dsj = 0;
    EnablePPR(AMIGO_PPR);
    return(status & ERROR_MASK);
}


/// @brief  Convert Drive CHS position to offset
///
/// - Reference: A15-A19.
/// @param[in] p: Disk Layout
/// @param[in] msg: user message on error
/// @return  0

static DWORD amigo_chs_to_logical(AMIGOStateType *p, char *msg)
{
    DWORD pos;
    pos =  (long) ( AMIGODisk.GEOMETRY.SECTORS_PER_TRACK * p->head);
    pos += (long) ( (AMIGODisk.GEOMETRY.SECTORS_PER_TRACK * AMIGODisk.GEOMETRY.HEADS) * p->cyl);
    pos += (long) p->sector;
    pos *= (long) AMIGODisk.GEOMETRY.BYTES_PER_SECTOR;

#if SDEBUG > 1
    if(debuglevel > 1)
        printf("[AMIGO %s, P:%08lx, U:%d C:%d H:%d S:%d]\n",
            msg, pos, AMIGOState.unitNO, p->cyl, p->head, p->sector);
#endif
    return(pos);
}


/// @brief  Check for CHS position overflow
///
/// - Reference: A15-A19
/// @return 0
/// @return 1 overflow

static int amigo_overflow_check(AMIGOStateType *p, char *msg)
{
    int stat = 0;
    while(p->sector >= AMIGODisk.GEOMETRY.SECTORS_PER_TRACK)
    {
        p->sector = 0;
        p->head++;
        while (p->head >= AMIGODisk.GEOMETRY.HEADS)
        {
            p->head = 0;
            p->cyl++;
            if (p->cyl >= AMIGODisk.GEOMETRY.CYLINDERS)
            {
                stat = 1;
#if SDEBUG > 1
                if(debuglevel > 1)
                    printf("[AMIGO %s pos OVERFLOW]\n", msg);
#endif
            }
        }
    }
    return(stat);
}


/// @brief Increment sector if it will not overflow.
///
/// @param[in] msg: user message on overflow.
///
/// @return 0 ok.
/// @return 1 error.

int amigo_increment(char *msg)
{
    int stat = 0;
    AMIGOStateType tmp = AMIGOState;

    ++tmp.sector;

    stat = amigo_overflow_check((AMIGOStateType *) &tmp, msg);
    if(!stat)
    {
        AMIGOState = tmp;
    }
    return(stat);
}


/// @brief Seek to the position held in the disk structure.
///
///  - Reference: A27.
///  - We should not commit bad head/sector/cylinder values until tested!..
///
/// @param[in] p:  AMIGOStateType (Current Disk Position) pointer.
///
/// @return 0 ok
/// @return 1 error 

int amigo_seek( AMIGOStateType *p)
{
    int stat = 0;

    stat = amigo_overflow_check(p, "Seek");
    if(stat)
    {
        AMIGOState.dsj = 1;
        AMIGOState.Errors |= ERR_SEEK;
    }
    else
    {
        AMIGOState.sector = p->sector;
        AMIGOState.head = p->head;
        AMIGOState.cyl = p->cyl;
    }

    EnablePPR(AMIGO_PPR);
    return(stat);
}


/// @brief  Verify Disk Sectors at current position
///
/// - Refernce: A36
/// @return 0 ok
/// @return 1 Error, Sets dsj and Amigo_errors

int amigo_verify(uint16_t sectors)
{
    int len;
    int stat = 0;
    DWORD pos;

    pos = amigo_chs_to_logical((AMIGOStateType *) &AMIGOState, "Verify Start");

#if SDEBUG > 1
    if(debuglevel > 1)
        printf("[AMIGO verify P:%08lx, sectors:%04x]\n", pos, sectors);
#endif

    while(sectors--)
    {
        pos = amigo_chs_to_logical((AMIGOStateType *) &AMIGOState, "Verfify");

#if SDEBUG > 1
        if(debuglevel > 2)
            gpib_timer_elapsed_begin();
#endif

        len = dbf_open_read(AMIGODisk.HEADER.NAME, pos, gpib_iobuff, AMIGODisk.GEOMETRY.BYTES_PER_SECTOR, &AMIGOState.Errors);

#if SDEBUG > 1
        if(debuglevel > 2)
            gpib_timer_elapsed_end("Disk Read");
#endif
        if(len != AMIGODisk.GEOMETRY.BYTES_PER_SECTOR)
        {
            AMIGOState.dsj = 1;
            AMIGOState.Errors |= ERR_READ;
            stat = 1;
            break;
        }
        if(amigo_increment("Verify"))             // address overflow
        {
            stat = 1;
            break;
        }
    }
    EnablePPR(AMIGO_PPR);
    return(stat);
}


/// @brief  Format Disk
///
/// - Refernce: A50.
///
/// @param[in] override: not used
/// @param[in] interleave: not used
/// @param[in] db: byte to fill sector buffer with)
///
/// @return 0 ok
/// @return 1 on Error,Sets dsj and Amigo_errors

int amigo_format(uint8_t override, uint8_t interleave, uint8_t db)
{

    DWORD pos;
    int len;
    int stat = 0;

    AMIGOState.sector = 0;
    AMIGOState.head = 0;
    AMIGOState.cyl = 0;

    memset((void *) gpib_iobuff, db, AMIGODisk.GEOMETRY.BYTES_PER_SECTOR);

#if SDEBUG > 1
    if(debuglevel > 2)
        gpib_timer_elapsed_begin();
#endif
    while( 1 )
    {
        pos = amigo_chs_to_logical((AMIGOStateType *) &AMIGOState, "Format");

        len = dbf_open_write(AMIGODisk.HEADER.NAME,
            pos, gpib_iobuff,AMIGODisk.GEOMETRY.BYTES_PER_SECTOR, &AMIGOState.Errors);

        if(len != AMIGODisk.GEOMETRY.BYTES_PER_SECTOR)
        {
            AMIGOState.Errors |= ERR_WRITE;
            AMIGOState.dsj = 1;
            stat = 1;
            break;
        }

        if(amigo_increment("Format"))             // address overflow
        {
            AMIGOState.Errors |= ERR_WRITE;
            AMIGOState.dsj = 1;
            stat = 1;
            break;
        }
    }
#if SDEBUG > 1
    if(debuglevel > 2)
        gpib_timer_elapsed_end("Format");
#endif

    EnablePPR(AMIGO_PPR);
    return(stat);
}


/// @brief  Do a buffered read of the current sector.
///
/// - Reference: A33.
/// @return 0 ok
/// @return GPIB Errors status,Sets dsj and Amigo_errors

int amigo_buffered_read()
{
    uint16_t status;
    int len;
    DWORD pos;

    pos = amigo_chs_to_logical((AMIGOStateType *) &AMIGOState, "Buffered Read");

#if SDEBUG > 1
    if(debuglevel > 2)
        gpib_timer_elapsed_begin();
#endif

    len = dbf_open_read(AMIGODisk.HEADER.NAME, pos, gpib_iobuff, AMIGODisk.GEOMETRY.BYTES_PER_SECTOR, &AMIGOState.Errors);

#if SDEBUG > 1
    if(debuglevel > 2)
        gpib_timer_elapsed_end("Disk Read");
#endif
    if(len != AMIGODisk.GEOMETRY.BYTES_PER_SECTOR)
    {
        AMIGOState.dsj = 1;
        return(0);
    }

#if SDEBUG > 1
    if(debuglevel > 2)
        gpib_timer_elapsed_begin();
#endif
    status = EOI_FLAG;
    len = gpib_write_str(gpib_iobuff, AMIGODisk.GEOMETRY.BYTES_PER_SECTOR, &status);
#if SDEBUG > 1
    if(debuglevel > 2)
        gpib_timer_elapsed_end("GPIB write");
#endif
    if(status & ERROR_MASK || len != AMIGODisk.GEOMETRY.BYTES_PER_SECTOR)
    {
        AMIGOState.dsj = 1;
        AMIGOState.Errors |= ERR_GPIB;
#if SDEBUG >= 1
        if(debuglevel >= 1)
            printf("[AMIGO GPIB write error]\n");
#endif
        EnablePPR(AMIGO_PPR);
        return(status & ERROR_MASK);
    }

/// @todo  Do we fail on overflow ?
///  currently djs is set - do we want to report that now ?
    if( amigo_increment("Buffered Read") )        //overflow
    {
        AMIGOState.dsj = 1;
        AMIGOState.Errors |= ERR_SEEK;
    }

    EnablePPR(AMIGO_PPR);
    return(0);
}


/// @brief  Do a buffered write of the current sector.

/// - Reference: A43.
/// @return 0 ok
/// @return GPIB Errors status,Sets dsj and Amigo_errors

int amigo_buffered_write()
{
    uint16_t status;
    int len;

    DWORD pos;

    pos = amigo_chs_to_logical((AMIGOStateType *) &AMIGOState, "Buffered Write");

#if SDEBUG > 1
    if(debuglevel > 2)
        gpib_timer_elapsed_begin();
#endif
    status = 0;
    len = gpib_read_str(gpib_iobuff, AMIGODisk.GEOMETRY.BYTES_PER_SECTOR, &status);

#if SDEBUG > 1
    if(debuglevel > 2)
        gpib_timer_elapsed_end("GPIB read str");
#endif

    if(status & ERROR_MASK || len != AMIGODisk.GEOMETRY.BYTES_PER_SECTOR)
    {
        AMIGOState.dsj = 1;
        AMIGOState.Errors |= ERR_GPIB;
#if SDEBUG >= 1
        if(debuglevel >= 1)
            printf("[AMIGO Write GPIB read error]\n");
#endif
        EnablePPR(AMIGO_PPR);
        return(status & ERROR_MASK);
    }

#if SDEBUG > 1
    if(debuglevel > 2)
        gpib_timer_elapsed_begin();
#endif

    len = dbf_open_write(AMIGODisk.HEADER.NAME, pos, gpib_iobuff, AMIGODisk.GEOMETRY.BYTES_PER_SECTOR, &AMIGOState.Errors);

#if SDEBUG > 1
    if(debuglevel > 2)
        gpib_timer_elapsed_end("Disk Write");
#endif

    if(len != AMIGODisk.GEOMETRY.BYTES_PER_SECTOR)
    {
        AMIGOState.dsj = 1;
        return(0);
    }
    if( amigo_increment("Buffered Write") )       //overflow
    {
        AMIGOState.dsj = 1;
        AMIGOState.Errors |= ERR_SEEK;
    }
    EnablePPR(AMIGO_PPR);
    return(status & ERROR_MASK);
}


/// @brief  Send DSJ
///
/// - Reference: A11.
/// @return 0 ok
/// @return GPIB send Errors status
/// - Sets dsj and Amigo_errors

int amigo_cmd_dsj()
{
    uint8_t tmp[1];
    uint16_t status;
    int len;

    tmp[0] = AMIGOState.dsj;

    status = EOI_FLAG;
    len = gpib_write_str(tmp, sizeof(tmp), &status);
    if(status & ERROR_MASK)
        AMIGOState.Errors |= ERR_GPIB;
    if(len != sizeof(tmp))
    {
        AMIGOState.dsj = 1;
        AMIGOState.Errors |= ERR_GPIB;
#if SDEBUG >= 1
        if(debuglevel >= 1)
            printf("[AIMGO: DSJ send failed]\n");
#endif
        return(status & ERROR_MASK);
    }
    else
    {
#if SDEBUG > 1
        if(debuglevel > 1)
            printf("[DSJ %02X]\n", AMIGOState.dsj);
#endif
    }
    AMIGOState.dsj = 0;
    AMIGOState.Errors = 0;
    return ( 0 );
}


/// @brief  Wakeup (undocumented), just enable PPR response.
///
/// @todo - not finished
/// - Sets dsj and Amigo_errors
///
/// @return  GPIB Errors status - or 0 OK

int amigo_cmd_wakeup()
{
    uint8_t tmp[1];
    uint16_t status;
    UINT len;

#if SDEBUG > 1
    if(debuglevel > 1)
    {
        printf("[AMIGO Wakeup]\n");
    }
#endif
    tmp[0] = AMIGOState.dsj;
    len = gpib_write_str(tmp, 1, &status);
    if(status & ERROR_MASK)
        AMIGOState.Errors |= ERR_GPIB;
    if( len != 1)
    {
        AMIGOState.dsj = 1;
        AMIGOState.Errors |= ERR_GPIB;
#if SDEBUG >= 1
        if(debuglevel >= 1)
            printf("[AMIGO GPIB write error]\n");
#endif
    }
/// @todo FIXME
    EnablePPR(AMIGO_PPR);
    AMIGOState.dsj = 0;
    return(status & ERROR_MASK);
}


/// @brief  Response to selected device clear or all clear
///
/// - Reference: A23.
/// - Select sector 0.
/// @return  0

int amigo_cmd_clear()
{
#if SDEBUG > 1
    if(debuglevel > 1)
        printf("[AMIGO Clear]\n");
#endif
    AMIGOState.sector = 0;
    AMIGOState.head = 0;
    AMIGOState.cyl = 0;
/// @todo FIXME
/// 
///  Clear the DSJ byte that might be 2 after powerup
/// 
    AMIGOState.dsj = 0;
    AMIGOState.Errors =0;

    EnablePPR(AMIGO_PPR);
    return(0);
}


/// @brief  Report any Unimplimented commands with Op Codes.
///
/// @param[in] secondary: GPIB secondary command
/// @param[in] opcode: GPIB data byte OP Code
/// @param[in] len: Length of GOIB data returned
/// @return  0

int amigo_todo_op(uint8_t secondary, uint8_t opcode, int len)
{
    if(AMIGO_is_MLA(listening))
        printf("[L   Amigo TODO secondary: %02x, state:%02x, opcode:%02x, len:%3d, listening:%02x, talking:%02x]\n",
            secondary, AMIGOState.state, opcode, len, listening, talking);
    else if(AMIGO_is_MTA(talking))
        printf("[T   Amigo TODO secondary: %02x, state:%02x, opcode:%02x, len:%3d, listening:%02x, talking:%02x]\n",
                secondary, AMIGOState.state, opcode, len, listening, talking);
    else if(talking == UNT)
        printf("[UNT Amigo TODO secondary: %02x, state:%02x, opcode:%02x, len:%3d, listening:%02x, talking:%02x]\n",
                secondary, AMIGOState.state, opcode, len, listening, talking);
    else
        printf("[U Amigo TODO secondary: %02x, state:%02x, opcode:%02x, len:%3d, listening:%02x, talking:%02x]\n",
            secondary, AMIGOState.state, opcode, len, listening, talking);
    EnablePPR(AMIGO_PPR);
    return(0);
}


/// @brief  Report any Unimplimented commands.
///
/// @param[in] secondary: GPIB seconday command
///
/// @return  0

int amigo_todo(uint8_t secondary)
{
    if(AMIGO_is_MLA(listening))
        printf("[L   Amigo TODO secondary: %02x, state:%02x, listening:%02x, talking:%02x]\n",
            secondary,AMIGOState.state,listening,talking);
    else if(AMIGO_is_MTA(talking))
        printf("[T   Amigo TODO secondary: %02x, state:%02x, listening:%02x, talking:%02x]\n",
                secondary,AMIGOState.state,listening,talking);
    else if(talking == UNT)
        printf("[UNT Amigo TODO secondary: %02x, state:%02x, listening:%02x, talking:%02x]\n",
                secondary,AMIGOState.state,listening,talking);
    else
        printf("[E   Amigo ERROR secondary: %02x, state:%02x, listening:%02x, talking:%02x]\n",
            secondary,AMIGOState.state,listening,talking);
    EnablePPR(AMIGO_PPR);
    return(0);
}



/// @brief  Amigo Command and OP Code Processing functions.
///
/// - We disbale PPR as soon as a valide command is decoded.
/// - ONLY process Secondary Commands in listen mode with op codes and optional data
///  - Ignore Talking.
///  - Ignore Seconday Addresses.
///  - Ignore Execute state "0x60" here.
///  - Must have OP Codes, or Data, and EOI.
/// - Read all of the Data/Opcodes/Parameters at once - while ATN is false.
/// - Last byte read should be EOI or an error.
/// - Unknown OP Code processing rules.
///  - Skip the remaining codes, Wait for Report Phase.
/// - We enable PPR on exit.
/// 
/// @param[in] secondary: command
///
/// @return  0 on sucess
/// @return or GPIB error flags on fail
/// @see  gpib.h ERROR_MASK defines for a full list)

int Amigo_Command( int secondary )
{
    uint8_t op;                                   // Current OP Code
    uint8_t *ptr;
    uint16_t status;                              // Current status
    UINT len;                                     // Size of Data/Op Codes/Parameters read in bytes

#if SDEBUG >= 1
    if(debuglevel >= 1)
        printf("[AMIGO Command(%02x): listen:%02x, talk:%02x]\n",
            secondary, listening, talking);
#endif

///  Reference: A14
    if (secondary == 0x7e && AMIGO_is_MTA(talking))
    {
        DisablePPR(AMIGO_PPR);
        status = EOI_FLAG;
        len = gpib_write_str(gpib_iobuff, GPIB_IOBUFF_LEN, &status);
        EnablePPR(AMIGO_PPR);
        if(status & ERROR_MASK)
        {
            AMIGOState.dsj = 1;
            AMIGOState.Errors |= ERR_GPIB;
#if SDEBUG >= 1
            if(debuglevel >= 1)
                printf("[AMIGO_Command:GPIB write error]\n");
#endif
        }
        return(status & ERROR_MASK);
    }

///  Reference: A25
    if (secondary == 0x7f && AMIGO_is_MLA(listening))
    {
        DisablePPR(AMIGO_PPR);
#if SDEBUG > 1
        if(debuglevel > 2)
            gpib_timer_elapsed_begin();
#endif
        status = EOI_FLAG;
        len = gpib_read_str(gpib_iobuff, GPIB_IOBUFF_LEN, &status);
#if SDEBUG > 1
        if(debuglevel > 2)
            gpib_timer_elapsed_end("GPIB read str");
#endif
        EnablePPR(AMIGO_PPR);
        if(status & ERROR_MASK)
        {
            AMIGOState.dsj = 1;
            AMIGOState.Errors |= ERR_GPIB;
#if SDEBUG >= 1
            if(debuglevel >= 1)
                printf("[AMIGO Command:GPIB read error]\n");
#endif
        }
        return(status & ERROR_MASK);
    }

    if( !AMIGO_is_MLA(listening) || AMIGO_is_MTA(talking))
        return(0);

    DisablePPR(AMIGO_PPR);


///  Note: the function will "unread" any commands and return
///  All Data MUST have EOI

    status = EOI_FLAG;
    len = gpib_read_str(gpib_iobuff, GPIB_IOBUFF_LEN, &status);
    if(status & ERROR_MASK)
    {
        AMIGOState.dsj = 1;
        AMIGOState.Errors |= ERR_GPIB;
#if SDEBUG >= 1
        if(debuglevel >= 1)
            printf("[AMIGO Command:GPIB read error]\n");
#endif
        return(status & ERROR_MASK);
    }
#if SDEBUG >= 1
    if(debuglevel >= 1)
        printf("[AMIGO Command(%02x): GPIB read bytes:%02x]\n",
            secondary, len);
#endif
    if(!len)
    {
        AMIGOState.dsj = 1;
        AMIGOState.Errors |= ERR_GPIB;
        return(status & ERROR_MASK);
    }

    ptr = gpib_iobuff;
    op = *ptr++;

    if (secondary == 0x68)
    {
        if(op == 0x00 && len == 2)
        {
///  Reference: A40

            AMIGOStateType tmp;
#if SDEBUG >= 1
            if(debuglevel >= 1)
                printf("[AMIGO Cold Load Read Command]\n");
#endif
/// @todo FIXME
            AMIGOState.unitNO = 0;
            tmp.cyl = 0;
            tmp.head = ( (0xff & *ptr) >> 6) & 0x03;
            tmp.sector = 0x3f & *ptr;
            ++ptr;
            AMIGOState.dsj = 0;
            AMIGOState.Errors = 0;
/// @todo FIXME
            amigo_seek((AMIGOStateType *) &tmp);
            AMIGOState.state = AMIGO_COLD_LOAD_READ;
            EnablePPR(AMIGO_PPR);
            return(status & ERROR_MASK);
        }
        else if(op == 0x02 && len == 5)
        {
///  Reference: A27
/// @brief 
///  Seek 1 byte cylinder

            AMIGOStateType tmp;
#if SDEBUG >= 1
            if(debuglevel >= 1)
                printf("[AMIGO Seek len=5]\n");
#endif
            AMIGOState.unitNO = 0xff & *ptr++;
            tmp.cyl = 0xff & *ptr++;
            tmp.head = 0xff & *ptr++;
            tmp.sector = 0xff & *ptr++;
            amigo_seek(&tmp);
            EnablePPR(AMIGO_PPR);
            return(status & ERROR_MASK);
        }
        else if(op == 0x02 && len == 6)
        {
///  Reference: A27
/// @brief 
///  Seek 2 byte cylinder

            AMIGOStateType tmp;
#if SDEBUG >= 1
            if(debuglevel >= 1)
                printf("[AMIGO Seek len=6]\n");
#endif
            AMIGOState.unitNO = 0xff & *ptr++;
            tmp.cyl = (0xff & *ptr++) << 8;       // MSB
            tmp.cyl |= (0xff & *ptr++);           // LSB
            tmp.head = 0xff & *ptr++;
            tmp.sector = 0xff & *ptr++;
            amigo_seek(&tmp);
            EnablePPR(AMIGO_PPR);
            return(status & ERROR_MASK);
        }
        else if(op == 0x03 && len == 2)
        {
///  Reference: A15
#if SDEBUG >= 1
            if(debuglevel >= 1)
                printf("[AMIGO Request Status Buffered Command]\n");
#endif
/// @todo  is this the unit $ ?
            AMIGOState.unitNO = 0xff & *ptr++;
            amigo_request_status();
            AMIGOState.state = AMIGO_REQUEST_STATUS_BUFFERED;
            return(status & ERROR_MASK);
        }
        else if(op == 0x05 && len == 2)
        {
///  Reference: A35
#if SDEBUG >= 1
            if(debuglevel >= 1)
                printf("[AMIGO Read Unbuffered Command]\n");
#endif
            AMIGOState.unitNO = *ptr++;
            AMIGOState.state = AMIGO_READ_UNBUFFERED;
            EnablePPR(AMIGO_PPR);
            return(status & ERROR_MASK);
        }
        else if(op == 0x07 && len == 4)
        {
            uint16_t sectors;
#if SDEBUG >= 1
            if(debuglevel >= 1)
                printf("[AMIGO Verify]\n");
#endif
            AMIGOState.unitNO = *ptr++;
            sectors = (0xff & *ptr++) << 8;
            sectors |= (0xff & *ptr++);
            return ( amigo_verify( sectors) );
        }
        else if(op == 0x08 && len == 2)
        {
#if SDEBUG >= 1
            if(debuglevel >= 1)
                printf("[AMIGO Write Unbuffered Command]\n");
#endif
            AMIGOState.unitNO = *ptr++;
            AMIGOState.state = AMIGO_WRITE_UNBUFFERED;
            EnablePPR(AMIGO_PPR);
            return(status & ERROR_MASK);
        }
        else if((op == 0x0B || op == 0x2b) && len == 2)
        {
#if SDEBUG >= 1
            if(debuglevel >= 1)
                printf("[AMIGO Initialize Command]\n");
#endif
            AMIGOState.unitNO = *ptr++;
            AMIGOState.state = AMIGO_INITIALIZE;
            EnablePPR(AMIGO_PPR);
            return(status & ERROR_MASK);
        }
        else if(op == 0x14 && len == 2)
        {
#if SDEBUG >= 1
            if(debuglevel >= 1)
                printf("[AMIGO Request Logical Address Command]\n");
#endif
/// @todo  unit ???
            amigo_request_logical_address();
            AMIGOState.state = AMIGO_REQUEST_LOGICAL_ADDRESS;
            EnablePPR(AMIGO_PPR);
            return(status & ERROR_MASK);
        }
    }
    else if (secondary == 0x69)
    {
        if(op == 0x08 && len == 2)
        {
#if SDEBUG >= 1
            if(debuglevel >= 1)
                printf("[AMIGO Write Buffered Command]\n");
#endif
            AMIGOState.unitNO = *ptr++;
            AMIGOState.state = AMIGO_WRITE_BUFFERED;
            EnablePPR(AMIGO_PPR);
            return(status & ERROR_MASK);
        }
    }
    else if (secondary == 0x6A)
    {
        if(op == 0x08 && len == 2)
        {
#if SDEBUG >= 1
            if(debuglevel >= 1)
                printf("[AMIGO Request Status Unbuffered Command]\n");
#endif
            AMIGOState.unitNO = *ptr++;
            AMIGOState.state = AMIGO_REQUEST_STATUS_UNBUFFERED;
            amigo_request_status();
            return(status & ERROR_MASK);
        }
        if(op == 0x05 && len == 2)
        {
#if SDEBUG >= 1
            if(debuglevel >= 1)
                printf("[AMIGO Read Buffered Command]\n");
#endif
            AMIGOState.unitNO = *ptr++;
            AMIGOState.state = AMIGO_READ_BUFFERED;
            EnablePPR(AMIGO_PPR);
            return(status & ERROR_MASK);
        }
    }
    else if (secondary == 0x6C)
    {
        if(op == 0x18 && len == 5)
        {
///  Reference: A48 ..  A50

            uint8_t override;
            uint8_t interleave;
            uint8_t db;
#if SDEBUG >= 1
            if(debuglevel >= 1)
                printf("[AMIGO Format]\n");
#endif
            AMIGOState.unitNO = 0xff & *ptr++;
            override = 0xff & *ptr++;
            interleave = 0xff & *ptr++;
            db = 0xff & *ptr++;
            amigo_format(override, interleave, db);
            return(status & ERROR_MASK);
        }
    }
    else if (secondary == 0x70)                   // HP-300 Clear
    {
///  Reference: A23
        ++ptr;                                    // Dummy byte
        return(status & ERROR_MASK);
    }
    return ( amigo_todo_op(secondary, op, len) );
}


/// @brief  Amigo Execute command processing
///
/// @param[in] secondary: command
///
/// @return  0 on sucess
/// @return or GPIB error flags on fail
/// @see  gpib.h ERROR_MASK defines for a full list)


int Amigo_Execute( int secondary )
{
#if SDEBUG >= 1
    if(debuglevel >= 1)
        printf("[AMIGO Execute(%02x): listen:%02x, talk:%02x]\n",
            secondary, listening, talking);
#endif

    if(talking == UNT)
        return(0);

    if(!AMIGO_is_MTA(talking) && !AMIGO_is_MLA(listening))
        return(0);

    if(secondary != 0x60 && secondary != 0x68)
        return(0);

    DisablePPR(AMIGO_PPR);

    if(secondary == 0x60)
    {
        switch(AMIGOState.state)
        {
            case AMIGO_IDLE:
                return(0);
            case AMIGO_COLD_LOAD_READ:
#if SDEBUG >= 1
                if(debuglevel >= 1)
                    printf("[AMIGO Execute Cold Load Read]\n");
#endif
                return ( amigo_buffered_read() );
            case AMIGO_READ_UNBUFFERED:
#if SDEBUG >= 1
                if(debuglevel >= 1)
                    printf("[AMIGO Execute Read Unbuffered]\n");
#endif
                return ( amigo_buffered_read() );
            case AMIGO_READ_BUFFERED:
#if SDEBUG >= 1
                if(debuglevel >= 1)
                    printf("[AMIGO Execute Read Buffered]\n");
#endif
                return ( amigo_buffered_read() );
            case AMIGO_WRITE_UNBUFFERED:
#if SDEBUG >= 1
                if(debuglevel >= 1)
                    printf("[AMIGO Execute Write Unbuffered]\n");
#endif
                return ( amigo_buffered_write() );
            case AMIGO_INITIALIZE:
#if SDEBUG >= 1
                if(debuglevel >= 1)
                    printf("[AMIGO Execute Initialize]\n");
#endif
                return ( amigo_buffered_write() );
            case AMIGO_WRITE_BUFFERED:
#if SDEBUG >= 1
                if(debuglevel >= 1)
                    printf("[AMIGO Execute Write Buffered]\n");
#endif
                return ( amigo_buffered_write() );
            default:
                return ( amigo_todo(secondary) );
        }
        AMIGOState.state = AMIGO_IDLE;
    }
    if(secondary == 0x68)
    {
        switch(AMIGOState.state)
        {
            case AMIGO_IDLE:
                return(0);
            case AMIGO_REQUEST_STATUS_BUFFERED:
#if SDEBUG >= 1
                if(debuglevel >= 1)
                    printf("[AMIGO Execute Request Status Buffered]\n");
#endif
                return ( amigo_send_status() );
            case AMIGO_REQUEST_STATUS_UNBUFFERED:
#if SDEBUG >= 1
                if(debuglevel >= 1)
                    printf("[AMIGO Exicute Request Status Unbuffered]\n");
#endif
                return ( amigo_send_status() );
            case AMIGO_REQUEST_LOGICAL_ADDRESS:
#if SDEBUG >= 1
                if(debuglevel >= 1)
                    printf("[AMIGO Execute Request Logical Address]\n");
#endif
                return ( amigo_send_logical_address() );
            default:
                return ( amigo_todo(secondary) );
        }
        AMIGOState.state = AMIGO_IDLE;
    }
    return(0);
}


/// @brief AMIGO Command state processing.
///
/// - Perform Command, Execute or Reporting Phase functions.
///   - Amigo_Command().
///   - Amigo_Execute().
///   - amigo_cmd_dsj().
///
/// @param[in] ch: command.
///
/// @return  0 on sucess
/// @return or GPIB error flags on fail
/// @see  gpib.h ERROR_MASK defines for a full list.

int AMIGO_COMMANDS(uint8_t ch)
{

    if(AMIGO_is_MTA(talking) || AMIGO_is_MLA(listening))
    {

        if(talking == UNT && AMIGO_is_MLA(listening))
        {
            printf("AMIGO COMMANDS %02x NO TALK ADDRESS!\n", ch);
        }
        if(listening == 0 && AMIGO_is_MTA(talking))
        {
            printf("AMIGO COMMANDS %02x NO LISTEN ADDRESS!\n", ch);
        }

        if(ch == 0x60 && (AMIGO_is_MTA(talking) || AMIGO_is_MLA(listening)) )
        {
            return (Amigo_Execute(ch) );
        }

        if(ch == 0x68 && AMIGO_is_MTA(talking) )
        {
            return (Amigo_Execute(ch) );
        }

        if(ch == 0x68 && AMIGO_is_MLA(listening) ) // Single byte command
        {
            return (Amigo_Command(ch) );
        }
        if(ch == 0x69 && AMIGO_is_MLA(listening) ) // Single byte command
        {
            return (Amigo_Command(ch) );
        }
        if(ch == 0x6a && AMIGO_is_MLA(listening) ) // Single byte command
        {
            return (Amigo_Command(ch) );
        }
        if(ch == 0x6c && AMIGO_is_MLA(listening) ) // Single byte command
        {
            return (Amigo_Command(ch) );
        }
        if(ch == 0x70 && AMIGO_is_MTA(talking))
        {
            DisablePPR(AMIGO_PPR);
            return( amigo_cmd_dsj() );
        }
        if(ch == 0x7e && ch == 0x7f)
        {
            return (Amigo_Command(ch) );
        }
        if(ch == 0x70 && AMIGO_is_MLA(listening))
        {
        }
    }
    return(0);
}
