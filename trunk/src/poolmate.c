/*
 * This file is part of PoolViewer
 * Copyright (c) 2011 Ivor Hewitt
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

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <libusb-1.0/libusb.h>

#include "poolmate.h"

#ifndef _WIN32
#define LIBUSB_CALL
#endif

#ifdef VERBOSE_DEBUG
#define INFO(x,...) printf(x, ##__VA_ARGS__ )
#define DEBUG(x,...) fprintf( stderr, x, ##__VA_ARGS__ )
#else
#define INFO(x,...)
#define DEBUG(x,...)
#endif

typedef struct _context
{
    int do_exit;
    libusb_context *ctx;
    libusb_device_handle *devh;
    struct libusb_transfer *ctrl_transfer;
    struct libusb_transfer *img_transfer;
    struct libusb_transfer *irq_transfer;
    
    unsigned char imgbuf[256];
    unsigned char irqbuf[2];
} Context;
Context *c;

Exercise exercises[MAX_EXERCISES];
int num_exercises=-1;
int cur_set;

/* Status of device flags */
int initialised;
int opened;
int attached;
int allocated;

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
void dump_data()
{
    int i,j;

    DEBUG("User,LogDate,LogTime,LogType,Pool,Units,TotalDuration,Calories,TotalLengths,TotalDistance,Nset,Duration,Strokes,Distance,Speed,Efficiency,StrokeRate,HRmin,HRmax,HRavg,HRbegin,HRend,Version\n");

    for (i=0; i<=num_exercises; i++)
    {
        Exercise *e = &exercises[i];

        for (j=0;j<e->sets_t;j++)
        {
            Set *s = &exercises[i].sets[j];

            DEBUG("%d,", e->user);
            DEBUG("%02d/%02d/%04d,", e->day, e->month, e->year);
            DEBUG("%02d:%02d:00,", e->hour, e->min);

            if (s->status == 0)
            {
                int time_secs;

                DEBUG("Swim,");
                DEBUG("%d,",e->pool);
                DEBUG("m,"); //TODO units

                DEBUG("%02d:%02d:%02d,", e->dur_hr, e->dur_min, e->dur_sec);

                DEBUG("%d,", e->cal);
            
                DEBUG("%d,", e->len_t); // Total lengths
                DEBUG("%d,", e->len_t * e->pool); //Total dist

                DEBUG("%d,", j+1); // set
                DEBUG("%02d:%02d:%02d,", s->hr, s->min, s->sec);

                DEBUG("%d,", s->str);  // strokes

                DEBUG("%d,", s->len * e->pool); //distance

                time_secs = (((s->hr*60)+s->min)*60+s->sec);
                DEBUG("%d,", 100*time_secs/(s->len*e->pool));  // speed
            
                DEBUG("%d,",((25*time_secs/s->len)+(25*s->str))/e->pool); // effic

                DEBUG("%d,",
                      (60*s->str*s->len) / time_secs); //rate
            }
            else
            {
                DEBUG("Chrono,");
                DEBUG(",,");
                DEBUG("%02d:%02d:%02d,", e->dur_hr, e->dur_min, e->dur_sec);
                DEBUG(",,,");
                DEBUG("%d,", j); // set           
                DEBUG("%02d:%02d:%02d,", s->hr, s->min, s->sec);
                DEBUG(",,,,,");
            }
            DEBUG(",,,,,210\n");
        }
    }
}
#endif

static int find_poolmate_device()
{
    c->devh = libusb_open_device_with_vid_pid(c->ctx, 0x0451, 0x5051);
    return c->devh ? 0 : -EIO;
}

void decode_packet(unsigned char *buffer)
{
    static int state = 0;

    if (buffer[0] == 0 && buffer[1] == 0) //start of new exercise
    {
        Exercise* e;

        state=0;
        num_exercises++;
        cur_set=-1;

        e = &exercises[num_exercises];

#ifdef VERBOSE_DEBUG
        dump("wk",buffer);
#endif
        e->user = 1; //TODO

        e->year  = buffer[2] + 2000;
        e->month = buffer[4];
        e->day   = buffer[6];
        e->hour  = buffer[3] & 0x7f;
        e->min   = buffer[5] & 0x7f;
    }
    else if (buffer[0] == 0x55 ) //footer, think this has the userid in it.
    {
        state=-1; //just stop for now
    }
    else if (state == 0)
    {
        Exercise *e = &exercises[num_exercises];

        if (cur_set == -1) //summary
        {
#ifdef VERBOSE_DEBUG
            dump("sm",buffer);
#endif
            e->pool = buffer[0];
            e->dur_min  = buffer[1] & 0x7f;
            e->sets_t = buffer[2];
            e->dur_sec  = buffer[3] & 0x7f;
            e->cal  = buffer[4];
            e->dur_hr  = buffer[5] & 0x7f;

            e->len_t = 0;

            cur_set=0;
        }
        else //set
        {
            Set *s = &e->sets[cur_set];

#ifdef VERBOSE_DEBUG
            dump("st",buffer);
#endif
            s->len = buffer[0];
            s->min = buffer[3] & 0x7f;
            s->str = buffer[4];
            s->sec = buffer[5] & 0x7f;
            s->hr = buffer[1] & 0x7f;
            s->status = buffer[7] & 0x7f;
            
            e->len_t += s->len;

            cur_set++;
        }
    }
}

/*
static void LIBUSB_CALL cb_ctrl(struct libusb_transfer * transfer)
{
    DEBUG("Ctrl - %02x %02x\n", transfer->buffer[0], transfer->buffer[1]);
    //	libusb_submit_transfer(ctrl_transfer);
}
*/

static void LIBUSB_CALL cb_irq(struct libusb_transfer *transfer)
{
    Context *c = transfer->user_data;

    INFO("Interrupt acknowledged\n");
    libusb_submit_transfer(c->irq_transfer);
}

static int count=0;

static void LIBUSB_CALL cb_img(struct libusb_transfer *transfer)
{
    int len;
    Context *c = transfer->user_data;

    len = transfer->actual_length;
    if (len)
    {
        INFO("Receiving data: %d ",len);

        if (transfer->buffer[0] != 0xff)
            decode_packet(transfer->buffer);
        
        INFO("%d\r",(count*100/512));
        count++;
    }

    if (len == 16)
    {
        INFO("len 16\n");
        c->do_exit=1;
        return;
    }

    libusb_submit_transfer(c->img_transfer);
}

static int alloc_transfers()
{
    c->img_transfer = libusb_alloc_transfer(0);
    if (!c->img_transfer)
        return -ENOMEM;
	
    c->irq_transfer = libusb_alloc_transfer(0);
    if (!c->irq_transfer)
        return -ENOMEM;

    c->ctrl_transfer = libusb_alloc_transfer(0);
    if (!c->ctrl_transfer)
        return -ENOMEM;

    libusb_fill_bulk_transfer(c->img_transfer, c->devh, 0x81, c->imgbuf,
                              sizeof(c->imgbuf), cb_img, c, 0);

    libusb_fill_interrupt_transfer(c->irq_transfer, c->devh, 0x83, c->irqbuf,
                                   sizeof(c->irqbuf), cb_irq, c, 0);
    return 0;
}

int send_command(unsigned char* buf, int len )
{
    int r;
    
    r = libusb_control_transfer(c->devh, 0x40, 0x05, 0x00, 0x03, buf, len, 0);	
    if (r < 0)
    {
        INFO("control error %d\n", r);
    }
    return r;
}

int send_ctrl(int bRequest, int wValue)
{
    int r;
    r = libusb_control_transfer(c->devh, 0x40, bRequest, wValue, 3,  0, 0, 0);	

    if (r < 0)
    {
        INFO( "control error %d\n", r);
    }
    return r;
}

int get_status()
{
    int r;
    unsigned char buf[2];

    r = libusb_control_transfer(c->devh, 0x80, 0x00, 0,0,  buf,2,1000);
    if (r < 0)
    {
        INFO( "control error %d\n", r);
    }
    return *(uint16_t*)buf;
}

int poolmate_init()
{
    int r;
    libusb_context *ctx;

    initialised=0;
    opened=0;
    attached=0;
    allocated=0;

    r = libusb_init(&ctx);
    if (r)
        return r;
    
    /* Context * */ 
    c = malloc(sizeof(Context));
    memset(c,0,sizeof(Context));
    c->ctx = ctx;
    initialised = 1;
    
    return r;
}

int poolmate_find()
{
    int r;
    r = find_poolmate_device();
    if (r == 0)
        opened=1;
    return r;
}

int poolmate_attach()
{
    int r;
    int config;

#ifndef _WIN32
    libusb_get_configuration(c->devh, &config);
    if (config != 2)
        libusb_set_configuration(c->devh, 2);
    
    r = libusb_kernel_driver_active(c->devh, 0);
    if (r)
        return r;

#endif    
    r = libusb_claim_interface(c->devh, 0);
    if (r)
        return 0;

    attached=1;
    return r;
}


int poolmate_start()
{
    int r;
    int status;

    libusb_reset_device(c->devh);    
    status = get_status();
    if (status != 1)
        return 0;

    r = alloc_transfers();
    if (r)
        return r;

    allocated=1;

    r = libusb_submit_transfer(c->irq_transfer);
    if (r)
        return r;
    
    {
        unsigned char cmd1[10]={0x03,0x01,0x60,0x00,0x00,0x00,0x00,0x11,0x13,0x00};
        unsigned char cmd2[10]={0x00,0x08,0x60,0x00,0x02,0x00,0x00,0x11,0x13,0x00};
        unsigned char cmd3[10]={0x00,0x08,0x60,0x00,0x03,0x00,0x00,0x11,0x13,0x00};
        unsigned char cmd4[10]={0x00,0x08,0x70,0x00,0x03,0x00,0x00,0x11,0x13,0x00};
        unsigned char cmd5[10]={0x00,0x08,0x70,0x02,0x03,0x00,0x00,0x11,0x13,0x00};

        send_command( cmd1, sizeof(cmd1));
        send_ctrl(0x06, 0x89);
        send_ctrl(0x08, 0x00);
        send_command( cmd2, sizeof(cmd2));
        send_command( cmd3, sizeof(cmd3));
        send_command( cmd4, sizeof(cmd4));
        send_command(cmd5, sizeof(cmd5));
    }
    
    count = 0;
    r = libusb_submit_transfer(c->img_transfer);

    return r;
}

// cleanup wherever we've got to
int poolmate_stop()
{
    if (allocated)
    {
        libusb_free_transfer(c->img_transfer);
        libusb_free_transfer(c->irq_transfer);
        allocated=0;
    }
    return -1;
}

int poolmate_cleanup()
{
    if (allocated)
    {
        libusb_free_transfer(c->img_transfer);
        libusb_free_transfer(c->irq_transfer);
        allocated=0;
    }

    if (attached)
    {
        libusb_release_interface(c->devh, 0);
        attached=0;
    }

    if (opened)
    {
        libusb_close(c->devh);
        opened=0;
    }

    if (initialised)
    {
        libusb_exit(c->ctx);
        free(c);
        initialised=0;
    }
    return -1;
}

int poolmate_progress()
{
    return count;
}

int poolmate_run()
{
    int r;
    
    r = libusb_handle_events(c->ctx);
    if (r)
        return r; // Error
    
    if (c->do_exit)
        return 0; // Done

    return 1;     // More to do
}

