#ifndef STRING_H
#define STRING_H

/*
 * SENG 21213 — Computer Architecture & Operating Systems
 * Stage 0: Minimal string utilities
 *
 * A freestanding kernel cannot link against the C standard library.
 * This header provides the small subset of string functions needed
 * for the shell and VGA driver.
 */

#include <stdint.h>
#include <stddef.h>

/* Memory operations */
void  *memset (void *dst, int c, size_t n);
void  *memcpy (void *dst, const void *src, size_t n);
int    memcmp (const void *a, const void *b, size_t n);

/* String operations */
size_t strlen  (const char *s);
int    strcmp  (const char *a, const char *b);
int    strncmp (const char *a, const char *b, size_t n);
char  *strcpy  (char *dst, const char *src);
char  *strncpy (char *dst, const char *src, size_t n);
char  *strchr  (const char *s, int c);

#endif /* STRING_H */
