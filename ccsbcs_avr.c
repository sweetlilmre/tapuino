#include "ff.h"
#include <avr/pgmspace.h>

WCHAR ff_convert (	/* Converted character, Returns zero on error */
	WCHAR	chr,	/* Character code to be converted */
	UINT	dir		/* 0: Unicode to OEMCP, 1: OEMCP to Unicode */
)
{
	return chr;
}


WCHAR ff_wtoupper (	/* Upper converted character */
	WCHAR chr		/* Input character */
)
{
  if (chr >= 'a' && chr <= 'z') {
    chr = 'A' + (chr - 'a');
  }
  return chr;
}
