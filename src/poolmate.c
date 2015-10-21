/*
 * This file is part of PoolViewer
 * Copyright (c) 2011-2012 Ivor Hewitt
 *
 * PoolViewer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PoolViewer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PoolViewer.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Note:
 * Poolmate dongle is actually a TI3410 USB serial interface,
 * so could probably use a usbserial driver instead but I didn't
 * manage to get it going with the ti_usb_3410_5052 driver
 * so lets stick with talking to it in userspace for now.
 * However this interface just presents the raw binary downloaded to
 * the application, so switching to the serial interface instead should
 * be simple.
 */

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <libusb-1.0/libusb.h>

#include "poolmate.h"
#include "logging.h"

#ifndef _WIN32
#define LIBUSB_CALL
#endif

typedef enum {
    NONE,
    READY,
    TRANSFER,
    COMPLETE,
    ERROR
} STATE;

typedef struct _context
{
    STATE state;

    libusb_context *ctx;
    libusb_device_handle *devh;

    unsigned char* data;
    unsigned long len;

    struct libusb_transfer *ctrl_transfer;
    struct libusb_transfer *img_transfer;
    struct libusb_transfer *irq_transfer;

    unsigned char imgbuf[256];
    unsigned char irqbuf[2];

    int claimed;  //remove need for claimed state?
    int transfer; //replace with cleaner cleanup.
} Context;

Context *g_c;

long poolmate_len()
{
    return g_c->len;
}

unsigned char* poolmate_data()
{
    if (g_c && g_c->len)
        return g_c->data;
    else
        return 0;
}


#ifdef VERBOSE_DEBUG
void dump(char* n, unsigned char* buf)
{
    int i;
    DEBUG("%s ",n);
    for (i=0; i<8; ++i)
    {
        DEBUG("%02x", buf[i]);
    }
    DEBUG("\n");
}
#endif

static int find_poolmate_device()
{
//Try first id
    g_c->devh = libusb_open_device_with_vid_pid(g_c->ctx, 0x0451, 0x5051);

    return g_c->devh ? 0 : -EIO;
}

/*
static void LIBUSB_CALL cb_ctrl(struct libusb_transfer * transfer)
{
    DEBUG("Ctrl - %02x %02x\n", transfer->buffer[0], transfer->buffer[1]);
    //	libusb_submit_transfer(ctrl_transfer);
}
*/

 //
 // At the moment IRQ notifications are simply ignored
 //
static void LIBUSB_CALL cb_irq(struct libusb_transfer *transfer)
{
    Context *c = transfer->user_data;
    INFO("Interrupt acknowledged\n");
    libusb_submit_transfer(c->irq_transfer);
}

// TODO detect end state somehow

static void LIBUSB_CALL cb_img(struct libusb_transfer *transfer)
{
    int len;
    Context *c = transfer->user_data;

    if (transfer->status == LIBUSB_TRANSFER_CANCELLED)
    {
        c->transfer = 0;
        return;
    }

    len = transfer->actual_length;

    if (c->state == READY)
        c->state = TRANSFER;

    if (len + c->len > 4104)
    {
        c->state = ERROR;
        return;
    }

    if (len)
    {
        int j;
        //        INFO("Receiving data: %d ",len);
        memcpy(c->data + c->len, transfer->buffer, len);
        c->len += len;

        for (j=0; j<len; j++)
        {
            INFO("%02x ", transfer->buffer[j]);
        }

        INFO("  %d\n",len);
    }

    if (c->len == 4104)
    {
        INFO("\nDone\n");

        c->state = COMPLETE;
        c->transfer=0;
        return;
    }

    libusb_submit_transfer(c->img_transfer);
    c->transfer=1;
    //detect complete
}

static int alloc_transfers()
{
    g_c->img_transfer = libusb_alloc_transfer(0);
    if (!g_c->img_transfer)
        return -ENOMEM;

    g_c->irq_transfer = libusb_alloc_transfer(0);
    if (!g_c->irq_transfer)
        return -ENOMEM;

    g_c->ctrl_transfer = libusb_alloc_transfer(0);
    if (!g_c->ctrl_transfer)
        return -ENOMEM;

    libusb_fill_bulk_transfer(g_c->img_transfer, g_c->devh, 0x81, g_c->imgbuf,
                              sizeof(g_c->imgbuf), cb_img, g_c, 5000);

    libusb_fill_interrupt_transfer(g_c->irq_transfer, g_c->devh, 0x83, g_c->irqbuf,
                                   sizeof(g_c->irqbuf), cb_irq, g_c, 0);

    return 0;
}

int set_serial(unsigned char* config )
{
    int r;
    r = libusb_control_transfer(g_c->devh, 0x40, 0x05, 0x00, 0x03, config, 10, 0);
    if (r < 0)
    {
        INFO("control error %d\n", r);
    }
    return r;
}

int send_ctrl(int bRequest, int wValue)
{
    int r;
    r = libusb_control_transfer(g_c->devh, 0x40, bRequest, wValue, 3,  0, 0, 0);

    if (r < 0)
    {
        INFO( "control error %d\n", r);
    }
    return r;
}

int open_port(int wValue )
{
    return send_ctrl(0x06, wValue);
}

int close_port()
{
    return send_ctrl(0x07, 0);
}

int start_port()
{
    return send_ctrl(0x08, 0);
}

int purge_port(int wValue)
{
    return send_ctrl(0x0b, wValue);
}

int get_status()
{
    int r;
    uint16_t buf;

    r = libusb_control_transfer(g_c->devh, 0x80, 0x00, 0,0, (unsigned char*)&buf,2,1000);
    if (r < 0)
    {
        INFO( "control error %d\n", r);
    }
    return buf;
}

int poolmate_init()
{
    int r;
    libusb_context *ctx;

    r = libusb_init(&ctx);
    if (r)
        return r;

    g_c = malloc(sizeof(Context));
    memset( g_c, 0, sizeof(Context));
    g_c->ctx = ctx;
    g_c->data = malloc(4104);

    return r;
}

int poolmate_find()
{
    int r;
    r = find_poolmate_device();
    return r;
}

int poolmate_attach()
{
    int r;
    int config;

#ifndef _WIN32
    libusb_get_configuration(g_c->devh, &config);

    /* Actually after reading about the serial interface
       might need to check there are 2 configurations and upload
       firmware image if we are in configuration 1. */

    INFO("Configuration %d\n", config);

    if (config != 2)
        libusb_set_configuration(g_c->devh, 2);

    libusb_get_configuration(g_c->devh, &config);
    if (config != 2)
    {
        INFO( "configuration error\n");
        return -1;
    }

    r = libusb_kernel_driver_active(g_c->devh, 0);
    if (r)
        return r;
#endif

    r = libusb_claim_interface(g_c->devh, 0);
    if (r)
        return 0;

    g_c->claimed=1;

    libusb_reset_device(g_c->devh);

    DEBUG("Status %d\n", get_status());

    r = alloc_transfers();
    if (r)
        return r;

    //only start interrupt once?
    r = libusb_submit_transfer(g_c->irq_transfer);
    if (r)
        return r;

    return r;
}


int poolmate_start()
{
    int r=0;

    if (g_c->transfer)
    {
        libusb_cancel_transfer(g_c->img_transfer);
        while (g_c->transfer)
            libusb_handle_events(g_c->ctx);
    }

    g_c->state = READY;
    g_c->len = 0;

    {
//1200bps, 5data
        unsigned char config1[10]={0x03,0x01,0x60,0x00,0x00,0x00,0x00,0x11,0x13,0x00};
//115200bps, 7data
//        unsigned char config2[10]={0x00,0x08,0x60,0x00,0x02,0x00,0x00,0x11,0x13,0x00};
//115200bps, 8data
//        unsigned char config3[10]={0x00,0x08,0x60,0x00,0x03,0x00,0x00,0x11,0x13,0x00};
//115200bps, 8data,nodtr
//        unsigned char config4[10]={0x00,0x08,0x70,0x00,0x03,0x00,0x00,0x11,0x13,0x00};
//115200bps, 8data,nodtr,norts
        unsigned char config5[10]={0x00,0x08,0x70,0x02,0x03,0x00,0x00,0x11,0x13,0x00};

//Not sure if this is necessary but drops to 1200bps before opening port
        set_serial(config1);

        open_port(0x01 | 0x80 | 2<<2);
        start_port(); //irq

        set_serial(config5);
    }

    r = libusb_submit_transfer(g_c->img_transfer);
    g_c->transfer=1;

    return r;
}

// cleanup wherever we've got to
int poolmate_stop()
{
    purge_port(0x80);
    libusb_clear_halt(g_c->devh,129);

    libusb_cancel_transfer(g_c->img_transfer);
    libusb_handle_events(g_c->ctx);

    return close_port();
}

int poolmate_cleanup()
{
    Context *c = g_c;
    g_c=0;

    if (!c)
        return 0;

    int errCode = 0;

    if (c->devh)
    {
        errCode= libusb_release_interface(c->devh, 0);
    }

    if  (errCode)
    {
        return -1;
    }

    libusb_close(c->devh);
    libusb_exit(c->ctx);

    free(c->data);
    c->data=0;
    free(c);

    return 0;
}


int poolmate_run()
{
    int r;

    r = libusb_handle_events(g_c->ctx);
    if (r)
        return r; // Error

    switch (g_c->state)
    {
    case COMPLETE:
        return 0;
    case ERROR:
        return -1;
    default:
        return 1; //more to do
    }
}


#ifdef TEST
static int sig=0;
static void sighandler(int signum)
{
    sig = 1;
}

void main()
{
    printf("Test mode\n");
    struct sigaction sigact;
    sigact.sa_handler = sighandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
    sigaction(SIGQUIT, &sigact, NULL);


    if (poolmate_init())
    {
        DEBUG("unable to init\n");
        return;
    }

    if (poolmate_find())
    {
        DEBUG("unable to find\n");
        return;
    }

    if (poolmate_attach())
    {
        DEBUG("unable ot attach\n");
        return;
    }

    while (1)
    {
        sig=0;
        DEBUG ("listening\n");
        if (poolmate_start())
        {
            DEBUG("unable to start\n");
            return;
        }

        while (!sig && poolmate_run() > 0)
        { }
        DEBUG ("\n");

        if (poolmate_stop())
            DEBUG("unable to stop\n");
    }

}

#endif
