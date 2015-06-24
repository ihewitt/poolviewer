#ifndef LOGGING_H
#define LOGGING_H

//#define TEST
#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
#define INFO(x,...) do { fprintf(stderr,x, ##__VA_ARGS__ );} while(0)
#define DEBUG(x,...) do { fprintf( stderr, x, ##__VA_ARGS__ );} while(0)
#else
#define INFO(x,...)
#define DEBUG(x,...)
#endif

#endif // LOGGING_H

