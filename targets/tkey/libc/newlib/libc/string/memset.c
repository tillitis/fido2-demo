/*
FUNCTION
	<<memset>>---set an area of memory

INDEX
	memset

SYNOPSIS
	#include <string.h>
	void *memset(void *<[dst]>, int <[c]>, size_t <[length]>);

DESCRIPTION
	This function converts the argument <[c]> into an unsigned
	char and fills the first <[length]> characters of the array
	pointed to by <[dst]> to the value.

RETURNS
	<<memset>> returns the value of <[dst]>.

PORTABILITY
<<memset>> is ANSI C.

    <<memset>> requires no supporting OS subroutines.

QUICKREF
	memset ansi pure
*/

#include <string.h>

void *
memset (void *m,
	int c,
	size_t n)
{
  char *s = (char *) m;

  while (n--)
    *s++ = (char) c;

  return m;
}
