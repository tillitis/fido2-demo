/*
FUNCTION
	<<memmove>>---move possibly overlapping memory

INDEX
	memmove

SYNOPSIS
	#include <string.h>
	void *memmove(void *<[dst]>, const void *<[src]>, size_t <[length]>);

DESCRIPTION
	This function moves <[length]> characters from the block of
	memory starting at <<*<[src]>>> to the memory starting at
	<<*<[dst]>>>. <<memmove>> reproduces the characters correctly
	at <<*<[dst]>>> even if the two areas overlap.


RETURNS
	The function returns <[dst]> as passed.

PORTABILITY
<<memmove>> is ANSI C.

<<memmove>> requires no supporting OS subroutines.

QUICKREF
	memmove ansi pure
*/

#include <string.h>
#include <stddef.h>
#include <limits.h>


void *
memmove (void *dst_void,
	const void *src_void,
	size_t length)
{
  char *dst = dst_void;
  const char *src = src_void;

  if (src < dst && dst < src + length)
    {
      /* Have to copy backwards */
      src += length;
      dst += length;
      while (length--)
	{
	  *--dst = *--src;
	}
    }
  else
    {
      while (length--)
	{
	  *dst++ = *src++;
	}
    }

  return dst_void;
}
