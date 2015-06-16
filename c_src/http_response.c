/*
 * Copyright (c) 2007-2009 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include <string.h>

/*JDH #include "mfs_config.h" */
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "drp.h"
#include "xaxi_eyescan.h"

#include "webserver.h"
/* JDH #include "platform_gpio.h" */
/* JDH #include "platform_fs.h" */

#include "SysStatus.h"
#include "otcLib/uPod.h"
#include "safe_printf.h"

char *notfound_header =
    "<html> \
    <head> \
        <title>404</title> \
        <style type=\"text/css\"> \
        div#request {background: #eeeeee} \
        </style> \
    </head> \
    <body> \
    <h1>404 Page Not Found</h1> \
    <div id=\"request\">";

char *notfound_footer =
    "</div> \
    </body> \
    </html>";

/* dynamically generate 404 response:
 *  this inserts the original request string in betwween the notfound_header & footer strings
 */
int do_404(int sd, char *req, int rlen)
{
    int len, hlen;
    int BUFSIZE = 1024;
    char buf[BUFSIZE];

    len = strlen(notfound_header) + strlen(notfound_footer) + rlen;

    hlen = generate_http_header(buf, "html", len);
    if (lwip_write(sd, buf, hlen) != hlen) {
        xil_printf("error writing http header to socket\r\n");
        xil_printf("http header = %s\n\r", buf);
        return -1;
    }

    lwip_write(sd, notfound_header, strlen(notfound_header));
    lwip_write(sd, req, rlen);
    lwip_write(sd, notfound_footer, strlen(notfound_footer));

    return 0;
}

int do_http_post(int sd, char *req, int rlen)
{
    int BUFSIZE = 1024;
    char buf[BUFSIZE];
    int len = 0, n;
    char *p;

    xil_printf("http POST: unsupported command\r\n");

    if (lwip_write(sd, buf, len) != len) {
        //xil_printf("error writing http POST response to socket\r\n");
        //xil_printf("http header = %s\r\n", buf);
        return -1;
    }

    return 0;
}

/* respond for a file GET request */
int do_http_get(int sd, char *req, int rlen)
{
    int BUFSIZE = 4096;
    char filename[MAX_FILENAME];
    char buf[BUFSIZE],pbuf[BUFSIZE];
    int fsize, hlen, n, w;
    int fd;
    char *fext;

#ifndef USE_XILINX_ORIGINAL
    /* write the http headers */
    hlen = generate_http_header(buf, "html", 0 /*strlen(pbuf)*/);
    if( (w = lwip_write(sd, buf, hlen)) < 0 ) {
        xil_printf("error writing http header to socket\r\n");
        xil_printf("http header = %s\r\n", buf);
        xil_printf("Expected size = %d; actual size = %d\r\n",hlen,w);
        return -1;
    }

    /* now write the web page data in two steps.  FIrst the Xilinx temp/voltages */
    char *pagefmt = "<HTML><HEAD><META HTTP-EQUIV=""refresh"" CONTENT=""10; URL=192.168.1.99""></HEAD"
            "<BODY><CENTER><B>Xilinx VC707 System Status (10 s refresh)</B></CENTER><BR><HR>"
            "Uptime: %d s<BR>"
            "Temperature = %0.1f C<BR>"
            "INT Voltage = %0.1f V<BR>"
            "AUX Voltage = %0.1f V<BR>"
            "BRAM Voltage = %0.1f V<BR><HR>";
    sprintf(pbuf,pagefmt,procStatus.uptime,procStatus.v7temp,procStatus.v7vCCINT,procStatus.v7vCCAUX,procStatus.v7vBRAM);

#if POLL_UPOD_TEMPS
    char * upod_pagefmt = "%s <CENTER><B>uPod number %d at I2C Address %p</B></CENTER><BR><HR>"
            "Status = 0x%02x<BR>"
    		"Temperature %d.%03d C<BR>"
            "3.3V = %d uV<BR>"
            "2.5V = %d uV<BR><HR>";

    int idx = 0;
    for( idx=0 ; idx<8 ; idx++ ) {

        safe_sprintf( pbuf , upod_pagefmt , pbuf , idx , upod_address(idx) , upodStatus[idx]->status, \
                upodStatus[idx]->tempWhole, upodStatus[idx]->tempFrac, \
                100*upodStatus[idx]->v33, 100*upodStatus[idx]->v25 );
    }
#endif

    // Display error counts in each channel
//    char * ber_pagefmt = "%s <CENTER><B>GTX number %d</B></CENTER><BR><HR>"
//    		"Error count = %d <BR><HR>";
//    char * ber_pagefmt = "%s %d %d\n";
//    u8 i;
//    for (i = 0; i < 48; ++i) {
//    	if (xaxi_eyescan_channel_active((u32) i)){
//			u16 error_count = drp_read(ES_ERROR_COUNT, i);
//	//	u16 sample_count = drp_read(ES_SAMPLE_COUNT, i);
//	//		u16 prescale = drp_read(ES_PRESCALE, i);
//			xil_printf("errcnt=%d\n", error_count);
//	//		double ber = ((float) error_count) / ((float) (sample_count * 32 << (1 + prescale)));
//	//		xil_printf("ber=%f\n", ber);
//			safe_sprintf(pbuf, ber_pagefmt, pbuf, i, error_count);
//    	}
//    }

//    xil_printf("%s\n", pbuf);

    n=strlen(pbuf);
    if ((w = lwip_write(sd, pbuf, n)) < 0 ) {
        xil_printf("error writing web page data (part 1) to socket\r\n");
        xil_printf("attempted to lwip_write %d bytes, actual bytes written = %d\r\n", n, w);
        return -2;
    }

    
    
#ifdef THE_ETHERNET_COUNTERS_HAVE_BEEN_FIXED
    /* Then the ethernet info */
    pagefmt = "<BR>Ethernet information:<BR>"
                "Rx bytes count = %d<BR>"
                "Tx bytes oount = %d<BR><HR>";
    sprintf(pbuf,pagefmt,ethStatus.regVal[XAE_RXBL_OFFSET],ethStatus.regVal[XAE_TXBL_OFFSET]);
    n=strlen(pbuf);
    if ((w = lwip_write(sd, pbuf, n)) < 0 ) {
        xil_printf("error writing web page data (part 2) to socket\r\n");
        xil_printf("attempted to lwip_write %d bytes, actual bytes written = %d\r\n", n, w);
        return -3;
    }
#endif
    /* and finally the end of the page */
    sprintf(pbuf,"</BODY></HTML>");
    n=strlen(pbuf);
    if ((w = lwip_write(sd, pbuf, n)) < 0 ) {
        xil_printf("error writing web page trailer to socket\r\n");
        xil_printf("attempted to lwip_write %d bytes, actual bytes written = %d\r\n", n, w);
        return -4;
    }

    return 0;
#else
    /* determine file name */
    extract_file_name(filename, req, rlen, MAX_FILENAME);

    /* respond with 404 if not present */
    if (mfs_exists_file(filename) == 0) {
        //xil_printf("requested file %s not found, returning 404\r\n", filename);
        do_404(sd, req, rlen);
        return -1;
    }

    /* respond with correct file */

    /* debug statement on UART */
        //xil_printf("http GET: %s\r\n", filename);

    /* get a pointer to file extension */
    fext = get_file_extension(filename);

    fd = mfs_file_open(filename, MFS_MODE_READ);

    /* obtain file size,
     * note that lseek with offset 0, MFS_SEEK_END does not move file pointer */
    fsize = mfs_file_lseek(fd, 0, MFS_SEEK_END);

    /* write the http headers */
    hlen = generate_http_header(buf, fext, fsize);
    if (lwip_write(sd, buf, hlen) != hlen) {
        //xil_printf("error writing http header to socket\r\n");
        //xil_printf("http header = %s\r\n", buf);
        return -1;
    }

    /* now write the file */
    while (fsize) {
        int w;
        n = mfs_file_read(fd, buf, BUFSIZE);

        if ((w = lwip_write(sd, buf, n)) != n) {
            //xil_printf("error writing file (%s) to socket, remaining unwritten bytes = %d\r\n",
                    //filename, fsize - n);
            //xil_printf("attempted to lwip_write %d bytes, actual bytes written = %d\r\n", n, w);
            break;
        }

        fsize -= n;
    }

    mfs_file_close(fd);

    return 0;
#endif
}

enum http_req_type { HTTP_GET, HTTP_POST, HTTP_UNKNOWN };
enum http_req_type decode_http_request(char *req, int l)
{
    char *get_str = "GET";
    char *post_str = "POST";

    if (!strncmp(req, get_str, strlen(get_str)))
        return HTTP_GET;

    if (!strncmp(req, post_str, strlen(post_str)))
        return HTTP_POST;

    return HTTP_UNKNOWN;
}

/* generate and write out an appropriate response for the http request */
int generate_response(int sd, char *http_req, int http_req_len)
{
    enum http_req_type request_type = decode_http_request(http_req, http_req_len);

    switch(request_type) {
    case HTTP_GET:
        return do_http_get(sd, http_req, http_req_len);
    case HTTP_POST:
        return do_http_post(sd, http_req, http_req_len);
    default:
        return do_404(sd, http_req, http_req_len);
    }
}
