#ifndef VERSION_H_
#define VERSION_H_

#ifdef SF2000
#define NAME "sfsd"
#else
#define NAME "spisd"
#endif

#define VERSION 2
#define REVISION 2
#define DATE "5.3.2023"

extern char device_name[];
extern char id_string[];

#endif
