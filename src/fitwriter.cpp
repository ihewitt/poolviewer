/*
 * This file is part of PoolViewer
 * Copyright (c) 2011-2017 Ivor Hewitt
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

/*
 * FIT file writer, developed using GoldenCheetah FIT save code.
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
 */

#include <QFile>
#include <QDataStream>
#include <QtEndian>

#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <ctime>
#include "stdintfwd.hpp"
#include "datastore.h"

enum ant_basetype
{
    ant_enum   = 0x00,
    ant_uint8   = 0x02,
    ant_string  = 0x07,
    ant_uint16  = 0x84,
    ant_uint32  = 0x86,
    ant_uint32z = 0x8c,
};

static const QDateTime qbase_time(QDate(1989, 12, 31), QTime(0, 0, 0), Qt::UTC);

uint16_t crc16(char *buf, int len)
{
    uint16_t crc = 0x0000;

    for (int pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos] & 0xff;

        for (int i = 8; i != 0; i--) {      // Each bit
            if ((crc & 0x0001) != 0) {      // LSB set
                crc >>= 1;                    // Shift right
                crc ^= 0xA001;                // XOR 0xA001
            }
            else
                crc >>= 1;                    // Shift right
        }
    }

    return crc;
}

typedef qint64 fit_value_t;
void write_int8(QByteArray *array, fit_value_t value) {
    array->append(value);
}

void write_int16(QByteArray *array, fit_value_t value,  bool is_big_endian=true);
void write_int16(QByteArray *array, fit_value_t value,  bool is_big_endian) {
    value = is_big_endian
            ? qFromBigEndian<qint16>( value )
            : qFromLittleEndian<qint16>( value );

    for (int i=0; i<16; i=i+8) {
        array->append(value >> i);
    }
}

void write_int32(QByteArray *array, fit_value_t value,  bool is_big_endian=true);
void write_int32(QByteArray *array, fit_value_t value,  bool is_big_endian) {
    value = is_big_endian
            ? qFromBigEndian<qint32>( value )
            : qFromLittleEndian<qint32>( value );

    for (int i=0; i<32; i=i+8) {
        array->append(value >> i);
    }
}

void write_field(QByteArray *array, int field_num, ant_basetype base_type, int field_size=-1);
void write_field(QByteArray *array, int field_num, ant_basetype base_type, int field_size)
{
    if (field_size==-1)
    {
        switch (base_type){
        case ant_string:
        case ant_enum:
        case ant_uint8:
            field_size=1;
            break;
        case ant_uint16:
            field_size=2;
            break;
        case ant_uint32:
        case ant_uint32z:
            field_size=4;
            break;
        }
    }
    write_int8(array, field_num);
    write_int8(array, field_size);
    write_int8(array, base_type);
}

void write_record(QByteArray *array, const Workout& wrk, const Set& set, int dist, int l,
                  const QDateTime& /*timestamp*/, const QDateTime& lenstart)
{
    int definition_header = 64;
    int reserved = 0;
    int is_big_endian = 1;
    int global_msg_num = 20;
    int num_fields = 3; //

    // Definition ------
    write_int8(array, definition_header);
    write_int8(array, reserved);
    write_int8(array, is_big_endian);
    write_int16(array, global_msg_num);
    write_int8(array, num_fields);

    //Fields
    write_field(array, 253, ant_uint32); // timestamp
    write_field(array, 5,   ant_uint32); // distance
    write_field(array, 6,   ant_uint16); // speed

    // Record ------
    int record_header = 0;
    write_int8(array, record_header);

    //timestamp (end of length)
    write_int32(array, lenstart.toTime_t()-qbase_time.toTime_t() + set.times[l]);

    //cumulative distance
    write_int32(array, dist*100);

    //speed
    write_int16(array, wrk.pool *1000 / set.times[l]); //speed kp/h

}

void write_length(QByteArray *array, const Workout& wrk, const Set& set, int first, int l,
                  const QDateTime& timestamp, const QDateTime& lenstart
                  )
{
    int definition_header = 64;
    int reserved = 0;
    int is_big_endian = 1;
    int global_msg_num = 101;
    int num_fields = 12; //

    // Definition ------
    write_int8(array, definition_header);
    write_int8(array, reserved);
    write_int8(array, is_big_endian);
    write_int16(array, global_msg_num);
    write_int8(array, num_fields);

    //Fields
    write_field(array, 253, ant_uint32); //timestamp
    write_field(array, 254, ant_uint16); //index

    write_field(array, 2,   ant_uint32); // start_time
    write_field(array, 3,   ant_uint32); // elapsed
    write_field(array, 4,   ant_uint32); // timer
    //timertime
    write_field(array, 5,   ant_uint16); // strokes

    write_field(array, 0,   ant_enum);   // event
    write_field(array, 1,   ant_enum);   // eventtype
    write_field(array, 12,  ant_enum);   // length_type
    write_field(array, 7,   ant_enum);   // stroke

    write_field(array, 6,   ant_uint16); // speed
    write_field(array, 9,   ant_uint8);  // cadence


    // Record ------
    int record_header = 0;
    write_int8(array, record_header);

    write_int32(array, timestamp.toTime_t()-qbase_time.toTime_t()); //timestamp
    write_int16(array, first + l);

    write_int32(array, lenstart.toTime_t()-qbase_time.toTime_t());

    write_int32(array, set.times[l] * 1000); //elapsed
    write_int32(array, set.times[l] * 1000); //timer
    write_int16(array, set.strokes[l]);

    write_int8(array, 28); //length
    write_int8(array, 3); //marker
    write_int8(array, 1); //active

    //TODO   QString styl = set.styles[i];
    write_int8(array, 0); //fc

    write_int16(array, wrk.pool *1000 / set.times[l]); //speed kp/h
    write_int8(array, 60 * set.strokes[l] /set.times[l]);    //cadence
}

void write_rest(QByteArray *array, const Workout& /*wrk*/, const Set& set,int first,
                const QDateTime& timestamp, const QDateTime& lenstart )
{
    int definition_header = 64;
    int reserved = 0;
    int is_big_endian = 1;
    int global_msg_num = 101;
    int num_fields = 8;

    // Definition ------
    write_int8(array, definition_header);
    write_int8(array, reserved);
    write_int8(array, is_big_endian);
    write_int16(array, global_msg_num);
    write_int8(array, num_fields);

    //Fields
    write_field(array, 253, ant_uint32);  //timestamp
    write_field(array, 254, ant_uint16);  //index
    write_field(array, 2,   ant_uint32);  // start_time
    write_field(array, 3,   ant_uint32);  // elapsed
    write_field(array, 4,   ant_uint32);  // timer
    write_field(array, 0,   ant_enum);    // event
    write_field(array, 1,   ant_enum);    // eventtype
    write_field(array, 12,  ant_enum);   // length_type

    // Record ------
    int record_header = 0;
    write_int8(array, record_header);

    write_int32(array, timestamp.toTime_t()-qbase_time.toTime_t()); //timestamp
    write_int16(array, first); //index
    write_int32(array, lenstart.toTime_t()-qbase_time.toTime_t());

    write_int32(array, set.rest.msecsSinceStartOfDay()); //elapsed
    write_int32(array, set.rest.msecsSinceStartOfDay()); //timer

    write_int8(array, 28); //length
    write_int8(array, 1);  //stop
    write_int8(array, 0);  //inactive

    //now lap end
    definition_header = 64;
    reserved = 0;
    is_big_endian = 1;
    global_msg_num = 19;
    num_fields = 10;

    // Definition ------
    write_int8(array, definition_header);
    write_int8(array, reserved);
    write_int8(array, is_big_endian);
    write_int16(array, global_msg_num);
    write_int8(array, num_fields);

    write_field(array, 253, ant_uint32); // timestamp
    write_field(array, 2,   ant_uint32); // start_time
    write_field(array, 7,   ant_uint32); // elapsed
    write_field(array, 8,   ant_uint32); // timer
    write_field(array, 9,   ant_uint32); // distance
    write_field(array, 32,  ant_uint16); // lengths
    write_field(array, 40,  ant_uint16); // activelengths
    write_field(array, 0,   ant_enum);   // event
    write_field(array, 1,   ant_enum);   // eventtype
    write_field(array, 25,  ant_enum);   // sport

    // Record ------
    record_header = 0;
    write_int8(array, record_header);

    write_int32(array, timestamp.toTime_t()-qbase_time.toTime_t()); //timestamp
    write_int32(array, lenstart.toTime_t()-qbase_time.toTime_t());

    write_int32(array, set.rest.msecsSinceStartOfDay()); //elapsed
    write_int32(array, set.rest.msecsSinceStartOfDay()); //timer

    write_int32(array, 0);
    write_int16(array, 0); //lengths
    write_int16(array, 0);
    write_int8(array, 9); //lap
    write_int8(array, 1); //stop
    write_int8(array, 5); //swimming
}

void write_lens(QByteArray *array, const Workout& wrk, int snum, int lnum, const Set& set, const QDateTime& lap_start )
{
    QDateTime len_start = lap_start;

    int dist=0;
    int s;

    for (s=0; s<snum; ++s)
    {
        dist=dist+set.dist;
    }

    QDateTime timestamp = lap_start;

    int i;
    for (i=0; i< set.lens; ++i)
    {
        dist += wrk.pool;

        timestamp=timestamp.addMSecs(set.times[i]*1000);

        write_length(array, wrk, set, lnum, i, timestamp, len_start);
        write_record(array, wrk, set, dist, i, timestamp, len_start);

        len_start=timestamp;
    }
}

void write_start(QByteArray *array, const Workout &/*workout*/, const QDateTime& start)
{
    int definition_header = 64;
    int reserved = 0;
    int is_big_endian = 1;
    int global_msg_num = 21;
    int num_fields = 4;

    // Definition ------
    write_int8(array, definition_header);
    write_int8(array, reserved);
    write_int8(array, is_big_endian);
    write_int16(array, global_msg_num);
    write_int8(array, num_fields);

    write_field(array, 253, ant_uint32);  // timestamp
    write_field(array, 0,   ant_enum);    // event
    write_field(array, 1,   ant_enum);    // eventtype
    write_field(array, 4,   ant_uint8);   // eventgroup

    // Record ------
    int record_header = 0;
    write_int8(array, record_header);

    write_int32(array, start.toTime_t()-qbase_time.toTime_t()); //timestamp
    write_int8(array, 0); //timer
    write_int8(array, 0); //start
    write_int8(array, 0); //group
}

void write_stop(QByteArray *array, const Workout &/*workout*/, const QDateTime& stop)
{
    int definition_header = 64;
    int reserved = 0;
    int is_big_endian = 1;
    int global_msg_num = 21;
    int num_fields = 4;

    // Definition ------
    write_int8(array, definition_header);
    write_int8(array, reserved);
    write_int8(array, is_big_endian);
    write_int16(array, global_msg_num);
    write_int8(array, num_fields);

    write_field(array, 253, ant_uint32); // timestamp
    write_field(array, 0,   ant_enum);   // event
    write_field(array, 1,   ant_enum);   // eventtype
    write_field(array, 4,   ant_uint8);   // eventgroup

    // Record ------
    int record_header = 0;
    write_int8(array, record_header);

    write_int32(array, stop.toTime_t()-qbase_time.toTime_t()); //timestamp
    write_int8(array, 0); //timer
    write_int8(array, 4); //stop
    write_int8(array, 0); //group
}

void write_laps(QByteArray *array, const Workout &workout ) {
    std::vector<Set>::const_iterator j;

    QDateTime lap_end, lap_start;
    lap_start = QDateTime(workout.date, workout.time);
    int snum=0,lnum=0;

    write_start(array, workout, lap_start);
    for (j = workout.sets.begin(); j!= workout.sets.end(); ++j, ++snum)
    {
        const Set& s = *j;

        lap_end=lap_start.addMSecs(s.duration.msecsSinceStartOfDay()); //for timestamp

        write_lens(array, workout, snum, lnum, *j, lap_start);
        write_stop(array, workout, lap_end);

        int definition_header = 64;
        int reserved = 0;
        int is_big_endian = 1;
        int global_msg_num = 19;
        int num_fields = 12;

        // Definition ------
        write_int8(array, definition_header);
        write_int8(array, reserved);
        write_int8(array, is_big_endian);
        write_int16(array, global_msg_num);
        write_int8(array, num_fields);

        write_field(array, 253, ant_uint32); // timestamp
        write_field(array, 2,   ant_uint32); // start_time
        write_field(array, 7,   ant_uint32); // elapsed
        write_field(array, 8,   ant_uint32); // timer
        write_field(array, 9,   ant_uint32); // distance
        write_field(array, 32,  ant_uint16); // lengths
        write_field(array, 40,  ant_uint16); // active lengths
        write_field(array, 0,   ant_enum);   // event
        write_field(array, 1,   ant_enum);   // eventtype
        write_field(array, 25,  ant_enum);   // sport
        write_field(array, 39,  ant_enum);   // subsport
        write_field(array, 35,  ant_uint16); // first length index

        // Record ------
        int record_header = 0;
        write_int8(array, record_header);

        write_int32(array, lap_end.toTime_t()-qbase_time.toTime_t()); //timestamp
        write_int32(array, lap_start.toTime_t()-qbase_time.toTime_t()); //lap start
        write_int32(array, s.duration.msecsSinceStartOfDay()); //elapsed
        write_int32(array, s.duration.msecsSinceStartOfDay()); //timer //
        write_int32(array, s.dist*100);
        write_int16(array, s.lens);
        write_int16(array, s.lens);
        write_int8(array, 9); //lap
        write_int8(array, 1); //stop
        write_int8(array, 5); //swimming
        write_int8(array,17); //lap swim
        write_int16(array, lnum);

        lap_start = lap_end;
        lnum += s.lens;

        // Add rest as a set
        //start event, stop length, stop lap, stop event.
        lap_end = lap_start.addMSecs(s.rest.msecsSinceStartOfDay()); //for timestamp

        write_start(array,workout,lap_start);
        write_rest(array,workout, *j, lnum++, lap_end, lap_start);

        lap_start = lap_end;
    }
    write_stop(array,workout,lap_end);
}

void write_session(QByteArray *array, const Workout &workout )
{
    int definition_header = 64;
    int reserved = 0;
    int is_big_endian = 1;
    int global_msg_num = 18;
    int num_fields = 15;

    // Definition ------
    write_int8(array, definition_header);
    write_int8(array, reserved);
    write_int8(array, is_big_endian);
    write_int16(array, global_msg_num);
    write_int8(array, num_fields);

    write_field(array, 253, ant_uint32); // timestamp
    write_field(array, 2,   ant_uint32);   // start_time
    write_field(array, 7,   ant_uint32);   // elapsed_time
    write_field(array, 8,   ant_uint32);   // timer_time
    write_field(array, 5,   ant_enum);     // sport
    write_field(array, 6,   ant_enum);     // subsport
    write_field(array, 26,  ant_uint16);   // laps sets
    write_field(array, 33,  ant_uint16);   // lengths
    write_field(array, 9,   ant_uint32);   // distance
    write_field(array, 46,  ant_enum);     // unit
    write_field(array, 44,  ant_uint16);   // pool len
    write_field(array, 10,  ant_uint32);  // strokes
    write_field(array, 11,  ant_uint16);   // calories
    write_field(array, 14,  ant_uint16);   // avgspeed
    write_field(array, 18,  ant_uint8);    // avgcad

    //    unknown110 (110-16-STRING): "Pool Swim"

    // Record ------
    int record_header = 0;
    write_int8(array, record_header);

    //stop time
    QDateTime end=QDateTime(workout.date, workout.time).addMSecs(workout.totalduration.msecsSinceStartOfDay());
    fit_value_t value = end.toTime_t();
    write_int32(array, value - qbase_time.toTime_t()); //timestamp

    value = QDateTime(workout.date, workout.time).toTime_t();
    write_int32(array, value - qbase_time.toTime_t()); //.starttime

    write_int32(array, workout.totalduration.msecsSinceStartOfDay() + workout.rest.msecsSinceStartOfDay() ); //.elapsed time
    write_int32(array, workout.totalduration.msecsSinceStartOfDay());  //.timer time

    write_int8(array, 5);                                  //.sport - swim
    write_int8(array, 17);                                 //. subsport - lap swim
    write_int16(array, workout.sets.size());         //. laps
    write_int16(array, workout.lengths);             //. lengths
    write_int32(array, workout.totaldistance * 100); //. distance

    // 9. units //TODO
    //  if (workout.unit == "m")
    write_int8(array, 0 ); //Metric

    //10. pool len
    write_int16(array, workout.pool * 100);

    int strokes=0;
    double time=0;
    std::vector<Set>::const_iterator j;
    for (j = workout.sets.begin(); j!= workout.sets.end(); ++j)
    {

        std::vector<double>::const_iterator t;
        for (t = j->times.begin(); t != j->times.end(); ++t)
        {
            time += *t;
        }
        std::vector<int>::const_iterator k;
        for (k = j->strokes.begin(); k != j->strokes.end(); ++k)
        {
            strokes += *k;
        }
    }
    write_int32(array, strokes); //. strokes
    write_int16(array, workout.cal); //. calories
    write_int16(array, workout.totaldistance * 1000 / time);

    write_int8(array, strokes*60 / time );

    write_laps(array, workout);
}

void write_activity(QByteArray *array, const Workout& wrk) {

    int definition_header = 64;
    int reserved = 0;
    int is_big_endian = 1;
    int global_msg_num = 34;
    int num_fields = 3;

    // Definition ------
    write_int8(array, definition_header);
    write_int8(array, reserved);
    write_int8(array, is_big_endian);
    write_int16(array, global_msg_num);
    write_int8(array, num_fields);

    write_field(array, 253, ant_uint32); //timestamp
    write_field(array, 3,   ant_enum); //event
    write_field(array, 4,   ant_enum); //event_type

    // Record ------
    int record_header = 0;
    write_int8(array, record_header);

    //stop time
    QDateTime end=QDateTime(wrk.date, wrk.time).addMSecs(wrk.totalduration.msecsSinceStartOfDay());
    fit_value_t value = end.toTime_t();
    write_int32(array, value-qbase_time.toTime_t());

    write_int8(array, 26); //activity
    write_int8(array, 1); //stop

    write_session(array, wrk);

}

void write_header(QByteArray *array, quint32 data_size) {
    quint8 header_size = 14;
    quint8 protocol_version = 16;
    quint16 profile_version = 1320; // always littleEndian

    write_int8(array, header_size);
    write_int8(array, protocol_version);
    write_int16(array, profile_version, false);
    write_int32(array, data_size, false);
    array->append(".FIT");

    uint16_t header_crc = crc16(array->data(), array->length());
    write_int16(array, header_crc, false);
}

/*
 * Just in case we want to emulate a known device
 */
void write_device(QByteArray *array, const Workout &workout )
{
    int definition_header = 64;
    int reserved = 0;
    int is_big_endian = 1;
    int global_msg_num = 23;
    int num_fields = 1;

    // Definition ------
    write_int8(array, definition_header);
    write_int8(array, reserved);
    write_int8(array, is_big_endian);
    write_int16(array, global_msg_num);
    write_int8(array, num_fields);

    write_field(array, 253, ant_uint32); // timestamp
    //write_field(array, 0, ant_uint8);
    //write_field(array, 2, ant_uint16);
    //write_field(array, 4, ant_uint16);
    //    write_field(array, 3, ant_uint32z);

    int record_header = 0;
    write_int8(array, record_header);

    QDateTime end=QDateTime(workout.date, workout.time).addMSecs(workout.totalduration.msecsSinceStartOfDay());
    fit_value_t value = end.toTime_t();
    write_int32(array, value - qbase_time.toTime_t());

    //write_int8(array, 0);
    //write_int16(array, 1);       //Garmin
    //write_int16(array, 1499);    //Swim
    //    write_int32(array, 12345678,true); //Serial
}

void write_file_id(QByteArray *array, const Workout &workout)
{
    int definition_header = 64;
    int reserved = 0;
    int is_big_endian = 1;
    int global_msg_num = 0;
    int num_fields = 2;

    // Definition ------
    write_int8(array, definition_header);
    write_int8(array, reserved);
    write_int8(array, is_big_endian);
    write_int16(array, global_msg_num);
    write_int8(array, num_fields);

    write_field(array, 0, ant_enum);     // field 1: type
    write_field(array, 4, ant_uint32);   // field 2: time_created

    //    write_field(array, 1, ant_uint16);
    //    write_field(array, 2, ant_uint16);
    //    write_field(array, 5, ant_uint16);

    // Record ------
    int record_header = 0;
    write_int8(array, record_header);
    write_int8(array, 4); //activity

    QDateTime t(workout.date, workout.time);
    int value = t.toTime_t();  // time_created
    write_int32(array, value - qbase_time.toTime_t());

    //    write_int16(array, 1);      //Garmin
    //    write_int16(array, 1499);   //Swim
    //    write_int16(array, 123456); //Serial
}


bool fit_write(const QString& filename, const Workout& workout)
{
    QByteArray content;
    QByteArray data;

    write_file_id(&data, workout);
    write_activity(&data, workout);
    write_device(&data,workout);

    write_header(&content, data.size());
    content += data;

    uint16_t crc = crc16(content.data(), content.length());
    write_int16(&content, crc, false);

    //array is FIT file
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.resize(0);

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    file.write(content);
    file.close();

    return true;
}
