#pragma once

#include <stdio.h>
#include <stdlib.h>

#ifndef IOT_IMPL

char* iotReadFile(FILE* fp);
char* iotReadTill(FILE* fp, int delim);

#else

char* iotReadTill(FILE* fp, int delim)
{
	char* ret = malloc(1);
	int i = 0;
	char c;

	while ((c = fgetc(fp)) != delim)
	{
		ret[i++] = c;
		ret = realloc(ret, i + 1);
	}

	ret[i] = '\0';
	return ret;
}

char* iotReadFile(FILE* fp)
{
	return iotReadTill(fp, EOF);
}

#endif
