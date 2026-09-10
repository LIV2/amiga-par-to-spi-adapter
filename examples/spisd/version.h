#ifndef VERSION_H_
#define VERSION_H_

#ifdef SF2000
#define NAME "sfsd"
#else
#define NAME "spisd"
#endif

#define VERSION 2
#define REVISION 4
#define DATE "10.9.2026"

extern char device_name[];
extern char id_string[];

#endif
