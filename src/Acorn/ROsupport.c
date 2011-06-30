#include "ROsupport.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Returns 0 if access mode is OK, 1 otherwise */
int access (const char *file, int mode)
{
  unsigned int block[4];
  int type, ret;

  if ((type = ReadFileInfo (file, block)) != 0)
  {
    /* block[0] = load, block[1] = execute, block[2] = length, block[3] = attrib */
    ret = 0;
    if ((mode & R_OK) != 0)
    {
      if ((block[3] & 1) == 0) ret = 1;
    }
    if ((mode & W_OK) != 0)
    {
      if ((block[3] & 10) != 2) ret = 1;
    }
    if ((mode & X_OK) != 0)
    {
      /* Directories are usually marked executable on UNIX */
      if (type != 2)
      {
        /* Does file have load/execution entries? */
        if ((block[0] & 0xfff00000) == 0xfff00000)
        {
          /* It's a filetyped file. Check type. */
          if (((block[0] & 0xfff00) != 0xff800) || ((block[0] & 0xfff00) != 0xffa00))
          {
            ret = 1;
          }
        }
      }
    }
    return ret;
  }
  else
  {
    return 1;
  }
}


/*
 *  The same for WADs. Checks for a file with /WAD extension or a file with
 *  the same base name and the correct filetype (&16c). The filename has to
 *  have enough free space at the end for a /wad extension!
 */
int accessWAD(char *file)
{
  unsigned int block[4];
  int type;

  /* Check for name + filetype (this is what it's called with */
  if ((type = ReadFileInfo(file, block)) != 0)
  {
    if ((block[3] & 1) != 0)
    {
      if ((block[0] & 0xffffff00) == 0xfff16c00)
      {
        return 0;
      }
    }
  }
  /* Otherwise try without the extension and check for the type */
  strcat(file, "/WAD");
  if ((type = ReadFileInfo(file, block)) != 0)
  {
    if ((block[3] & 1) != 0) return 0;
  }
  return 1;
}



/*
 *  Compile these POSIX functions when using SCL or UnixLib 3.7, but don't
 *  when using UnixLib 3.8. UnixLib 3.7 defines them, but they're buggy and
 *  can be overloaded. In UnixLib 3.8 they're OK (and can't be overloaded,
 *  don't ask me why...)
 */

#ifdef __UNIXLIB_TYPES_H
#include <unistd.h>
#ifndef __UNAME_NO_PROCESS
#define __ROSUPPORT_NO_STRCASE
#endif
#endif

#ifndef __ROSUPPORT_NO_STRCASE

static unsigned char *lower_case_table = NULL;


int strcasecmp (const char *str1, const char *str2)
{
  unsigned char *a, *b;

  a = (unsigned char *)str1; b = (unsigned char *)str2;
  if (lower_case_table == NULL) {lower_case_table = LowerCaseTable();}

  while (*a != 0)
  {
    if (lower_case_table[*a] != lower_case_table[*b]) break;
    a++; b++;
  }
  if ((*a == 0) && (*b == 0)) return 0;
  if (lower_case_table[*a] < lower_case_table[*b]) return -1;
  else return 1;
}



int strncasecmp (const char *str1, const char *str2, size_t size)
{
  unsigned char *a, *b;
  int i = size;

  a = (unsigned char *)str1; b = (unsigned char *)str2;
  if (lower_case_table == NULL) {lower_case_table = LowerCaseTable();}

  while ((*a != 0) && (i > 0))
  {
    if (lower_case_table[*a] != lower_case_table[*b]) break;
    a++; b++; i--;
  }
  if ((lower_case_table[*a] == lower_case_table[*b]) || (i <= 0)) return 0;
  if (lower_case_table[*a] < lower_case_table[*b]) return -1;
  else return 1;
}

#endif

#ifdef __cplusplus
}
#endif
