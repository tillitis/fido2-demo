/*
 * string.h
 *
 * Definitions for memory and string functions.
 */

#ifndef _STRING_H_
#define	_STRING_H_


#include <stddef.h>

int 	 memcmp (const void *, const void *, size_t);
void *	 memcpy (void *__restrict, const void *__restrict, size_t);
void *	 memmove (void *, const void *, size_t);
void *	 memset (void *, int, size_t);
int	 strcmp (const char *, const char *);
size_t	 strlen (const char *);
int	 strncmp (const char *, const char *, size_t);

#endif /* _STRING_H_ */
