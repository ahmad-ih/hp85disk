/**
 @file gpib/printer.c

 @brief HPGL printer capture code for HP85 disk emulator project for AVR. 

 @par Edit History
 - [1.0]   [Mike Gore]  Initial revision of file.

 @par Copyright &copy; 2014-2017 Mike Gore, Inc. All rights reserved.

*/

#include "user_config.h"

#include "defines.h"

#include "drives.h"

#include "gpib_hal.h"
#include "gpib.h"
#include "gpib_task.h"
#include "printer.h"
#include "posix.h"
#include "delay.h"

///@brief Plotter file data structure used for saving plot data.
PRINTERStateType plot = { 0 };

///@brief Plotter GPIB test vector.
uint8_t plot_str[] = { 0x3f, 0x5f, 0x47, 0x21 };

/// @brief  Open a file to receive plot data using POSIX functions.
///
/// - Uses POSIX FatFs wrappers.
///
/// @see posix.c
/// @see posix.h
/// @return  void

void printer_open(char *name)
{

    char *ptr;

    if(!name)
    {
        time_t seconds;
        tm_t *tc;
        ts_t ts;
        char fname[64];

        clock_gettime(0, (ts_t *) &ts);
        seconds = ts.tv_sec;
        tc = gmtime(&seconds);
        sprintf(fname,"/plot-%02d%s%04d-%02d%02d%02d.plt",
            tc->tm_mday,
            tm_mon_to_ascii(tc->tm_mon),
            tc->tm_year + 1900,
            tc->tm_hour,
            tc->tm_min,
            tc->tm_sec);
        ptr = fname;
    }
    else
    {
        ptr = name;
    }

    if(debuglevel & 32)
        printf("Capturing plot to:%s\n", ptr);

    plot.fp = fopen(ptr,"w");
    if(plot.fp == NULL)
    {
        if(debuglevel & (1+32))
        {
            perror("open failed");
            printf("exiting...\n");
        }
        return;
    }

    plot.buf = calloc(512+1,1);
    if(plot.buf == NULL)
        printer_close();
    plot.size = 512;
}


/// @brief  Initialize plotter structures and state.
///
/// - Only called once after power on by main().
/// @return  void

void printer_init()
{
    plot.error = 0;
    plot.count = 0;
    plot.ind = 0;
    plot.fp = NULL;
    plot.buf = NULL;
}


/// @brief  Close current plot file and reset states.
///
/// - Verify that polt file is open before attempting close.
/// - Uses POSIX FatFs wrappers.
///
/// @see posix.c
/// @see posix.h
/// @return  void
/// FYI: for the HP54645D plots end with: pd;pu;pu;sp0;
/// This gets called
void printer_close()
{
    if( receive_plot_flush() < 0 )
        plot.error = 1;

    if(debuglevel & (1+32))
    {
        if(plot.error)
            printf("ERROR durring write\n");
    }

    if(plot.fp)
    {
        fclose(plot.fp);
        if(debuglevel & 32)
            printf("\nDONE: %08ld\n",plot.count);
    }

    if(plot.buf)
        safefree(plot.buf);
    printer_init();
}


/// @brief Write Plotter data and flush.
///
/// - Uses POSIX FatFs wrappers.
///
/// @see posix.c
/// @see posix.h
/// @see printer_buffer()
/// @return  file handle

int receive_plot_flush()
{
    int ret;
    int fno;

    if(plot.fp == NULL || plot.ind == 0)
        return(0);

    ret  = fwrite(plot.buf, 1, plot.ind , plot.fp);
    if(ret != plot.ind)
    {
        if(debuglevel & (1+32))
        {
            perror("receive_plot_flush");
            printf("write failed: wanted %d, got:%d\n", plot.ind, ret);
        }
        return(-1);
    }

    fno = fileno( plot.fp );
    if(fno < 0)
        return(-1);
    ///@brief sync filesystem after every write 
    syncfs( fno );
    return (ret);
}


/// @brief  Buffer Plotter data and flush when buffer is full
///
/// - Uses POSIX FatFs wrappers.
///
/// @see posix.c
/// @see posix.h
/// @return  void
void printer_buffer( uint16_t val )
{

    uint16_t ch;

    if(debuglevel & (1+32))
    {
        if( ( plot.count & 255L ) == 0)
            printf("%08ld\r",plot.count);
    }

    ch = val & 0xff;
    if(val & (0xff00 & ~REN_FLAG))
    {
        if( receive_plot_flush() )
            plot.error = 1;

        //fprintf(plot.fp,"%s\n", ptr);
        //plot.count += strlen(ptr);
    }
    else
    {
        ch  = val & 0xff;
        plot.buf[plot.ind++] = ch;
        plot.count++;

        if(plot.ind >= plot.size)
        {
            if( receive_plot_flush() < 0 )
                plot.error = 1;
            plot.ind  = 0;
        }
    }
}


/// @brief  GPIB Secondary Command Printer commands.
///
/// @todo  Fully emulated plotter response.
/// @return  0

int PRINTER_COMMANDS(uint8_t ch)
{

    // We could, for example, use secondaries to set file names, etc
    // We don not use them yet
    if(PRINTER_is_MLA(listening))
    {
#if SDEBUG
        if(debuglevel & 32)
            printf("[SC PRINTER Listen: %02XH]\n",  0xff & ch );
#endif
        return(0);
    }

    if(PRINTER_is_MTA(talking))
    {
#if SDEBUG
        if(debuglevel & 32)
            printf("[SC PRINTER Talk: %02XH]\n",  0xff & ch );
#endif
        return(0);
    }
    return(0);
}

int gpib_ascii_request(char *str, uint16_t *status)
{
    int len = strlen(str);
    return(gpib_write_str( (uint8_t *)str,len,status) );
}
   

int printer_send(char *str)
{
    uint16_t status = 0;
    int len;
    gpib_write_byte(0x5f | ATN_FLAG);   // untalk
    gpib_write_byte(0x3f | ATN_FLAG);   // unlisten

    gpib_write_byte(0x20 | 8 | ATN_FLAG);   // SCOPE listen
    gpib_write_byte(0x40 | 5 | ATN_FLAG);   // PRINTER talk
    status = 0;
    printf("sending: %s\n", str);
    len = gpib_ascii_request((char *)str,&status);
    printf("sent: %d bytes\n",len);

    gpib_write_byte(0x5f | ATN_FLAG);   // untalk
    gpib_write_byte(0x3f | ATN_FLAG);   // unlisten
    return(len);
}

int printer_read_str(char *str, int len)
{
    uint16_t status = 0;

    GPIB_IO_LOW(IFC);
    delayms(200);
    GPIB_PIN_FLOAT(IFC);
    delayms(200);
    gpib_write_byte(DCL | ATN_FLAG);

    gpib_write_byte(0x5f | ATN_FLAG);   // untalk
    gpib_write_byte(0x3f | ATN_FLAG);   // unlisten

    gpib_write_byte(0x40 | 8 | ATN_FLAG);   // SCOPE talk
    gpib_write_byte(0x20 | 5 | ATN_FLAG);   // PRINTER listen

    status = 0;
    len = gpib_read_str((uint8_t *)str,len, &status);
    printf("Recieved %d\n",(int)len);
    printf("Reply:%s",str);

    gpib_write_byte(0x5f | ATN_FLAG);   // untalk
    gpib_write_byte(0x3f | ATN_FLAG);   // unlisten
    return(len);
}

int printer_read()
{
    uint16_t ch;
    long len =0;

    gpib_write_byte(0x5f | ATN_FLAG);   // untalk
    gpib_write_byte(0x3f | ATN_FLAG);   // unlisten

    gpib_write_byte(0x40 | 8 | ATN_FLAG);   // SCOPE talk
    gpib_write_byte(0x20 | 5 | ATN_FLAG);   // PRINTER listen

    while(1)                                      // Main loop, forever
    {
        if(uart_keyhit(0))
            break;

        ch = gpib_read_byte(0);
        gpib_decode(ch);
       if(ch & EOI_FLAG)
            break;
        ++len;
    }

    gpib_write_byte(0x5f | ATN_FLAG);   // untalk
    gpib_write_byte(0x3f | ATN_FLAG);   // unlisten
    return(len);
}

/// @brief  Instruct Instrument to send Plot data.
/// - Not finished or working yet - barely started work in progress,
/// @return  void
void plot_echo( int gpib_address)
{
    int ind,len;

    printer_close();

    while(uart_keyhit(0) )
        putchar( uart_rx_byte(0) );
        
    ind = find_type(PRINTER_TYPE);

    if(ind == -1)
    {
        printf("printer not defined\n");
        return;
    }
// DEBUGGING
    gpib_decode_header(stdout);


    len = printer_send("*idn?\n");
    len = printer_read();
    printf("received:%d\n", len);

    len = printer_send(":HARDcopy:DEVice?\n");
    len = printer_read();
    printf("received:%d\n", len);

    //len = printer_send(":PRINt?\n");
    len = printer_send(":wav:data?\n");
    len = printer_read();
    printf("received:%d\n", len);
}
