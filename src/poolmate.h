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

#ifndef POOLMATE_H
#define POOLMATE_H

//#define TEST
////#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
#define INFO(x,...) do { fprintf(stderr,x, ##__VA_ARGS__ );} while(0)
#define DEBUG(x,...) do { fprintf( stderr, x, ##__VA_ARGS__ );} while(0)
#else
#define INFO(x,...)
#define DEBUG(x,...)
#endif


/*
#define MAX_SETS 255
#define MAX_EXERCISES 255

typedef struct
{
    int len;
    int hr;
    int min;
    int str;
    int sec;
    int status;
} Set;

typedef struct
{
    int user;

    int year;
    int month;
    int day;
    int hour;
    int min;

    int pool;
    int dur_hr;
    int dur_min;
    int dur_sec;
    int sets_t;
    int cal;

    int len_t;

    Set sets[MAX_SETS];
} Exercise;
*/

int poolmate_init();
int poolmate_find();
int poolmate_attach();
int poolmate_start();
int poolmate_run();
int poolmate_stop();
int poolmate_cleanup();

long poolmate_len();
unsigned char* poolmate_data();

#endif /* POOLMATE_H */
