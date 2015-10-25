#ifndef LOGGING_H
#define LOGGING_H

#if (QT_NO_DEBUG)
#define INFO(x,...)
#define DEBUG(x,...)
#else
//for excessive logging
//#define VERBOSE_DEBUG
#define INFO(x,...) do { fprintf(stderr,x, ##__VA_ARGS__ );} while(0)
#define DEBUG(x,...) do { fprintf( stderr, x, ##__VA_ARGS__ );} while(0)
#endif

#endif // LOGGING_H

