#define	RAND_MAX	0x7fffffff
#include <efi.h>
#include <Library/UefiLib.h>
#include "Protocol/CpuIo.h"
//#include "CpuIo2.h"
#include "Protocol/GraphicsOutput.h"
#include <Protocol/UgaDraw.h>

extern EFI_CPU_IO_PROTOCOL *my_cpu_io;

static unsigned long next = 1;

static unsigned int ReadRtc(int Index)
{
	unsigned char Value;

	my_cpu_io->Io.Write(my_cpu_io,EfiCpuIoWidthUint8,0x70,1,&Index);
	my_cpu_io->Io.Read(my_cpu_io,EfiCpuIoWidthUint8,0x71,1,&Value);

	return Value;
}
int snake_time()
{
	unsigned int Year = ReadRtc(9);
	unsigned int Month = ReadRtc(8);
	unsigned int Day = ReadRtc(7);
	unsigned int Hour = ReadRtc(4);
	unsigned int Minute = ReadRtc(2);
	unsigned int Second = ReadRtc(0);
	unsigned int time = (Year<<24) + (Month<<20) + (Day<<16) + (Hour<<12) + (Minute<<8) + Second;

	return time;
}
int snake_random()
{
	next = (next * 1103515245 + 12345) % ((unsigned long)RAND_MAX + 1);
	return next;
}

void snake_srand(unsigned int seed)
{
	next = seed;
}

