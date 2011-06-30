/*
 *  mpp.c - a macro preprocessor
 *
 *  Written by: Andreas Dehmel (dehmel@forwiss.tu-muenchen.de)
 *
 *  Simple, configurable macro preprocessor. Licensed under GPL.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>



#ifdef __riscos
#define DIR_SEPARATOR	'.'
#define EXT_SEPARATOR	'/'
#else
#define DIR_SEPARATOR	'/'
#define EXT_SEPARATOR	'.'
#endif

/* Reference model: Unix; don't do extension swap, however */
#define USE_DIR_SEPARATOR	'/'
#define USE_EXT_SEPARATOR	'.'



typedef struct macro_s {
  const char *ident;
  const char *value;
  unsigned int params;
  unsigned int lines;
  unsigned int idlength;
  /* for hashing */
  int hashcode, next, prev;
} macro_t;

typedef struct macrovec_s {
  macro_t *macros;
  unsigned int nummacros;
  unsigned int maxmacros;
  unsigned int firstfree;
  /* for hashing */
  int *hashtable;
} macrovec_t;

typedef struct boolstack_s {
  unsigned char *data;
  unsigned int maxbytes;
  unsigned int depth;
} boolstack_t;

typedef struct string_s {
  char *base;
  char *ptr;
  unsigned int size;
} string_t;

typedef struct environment_s {
  const char *file;
  FILE *fp;
  unsigned int line;
  int verbose;
  const char *outfile;
  FILE *outfp;
  struct parse_info_s *pinfo;
  struct binary_operation_s *binops;
  macrovec_t *mvec;
  boolstack_t *bstk;
  /* temp vars */
  char *buffer;
  unsigned int buffSize;
} environment_t;

typedef struct argdesc_s {
  char *arg;
  int len;
} argdesc_t;

typedef int (*parsecmd_f)(environment_t *, const char *);

typedef struct command_ptr_s {
  const char *ident;
  parsecmd_f handler;
} command_ptr_t;

typedef struct parse_info_s {
  char cmdchar;
  char escchar;
  char refopen;
  char refclose;
  const char *str_defined;
  const command_ptr_t *commands;
} parse_info_t;

typedef enum {
  binop_none,
  binop_equal,
  binop_unequal,
  binop_lower,
  binop_lowereq,
  binop_higher,
  binop_highereq
} binary_operation_e;

typedef struct binary_operation_s {
  const char *operation;
  binary_operation_e opid;
  int integers;
} binary_operation_t;



/* prototypes */
static char *FindAndMaterializeMacro(environment_t *env, const char **string);


#define STRINGSIZE	256
#define BUFFER_SIZE	2048
#define BLOCK_SIZE	2048
#define LD_HASH_SIZE	12

#define MACRO_GROW_BY	16
#define BVEC_GROW_BY	16
#define STRING_GROW_BY	256



static void *xmalloc(size_t size)
{
  void *ret;

  if ((ret = malloc(size)) == NULL)
  {
    fprintf(stderr, "Unable to alloc %d bytes\n", size);
    exit(-1);
  }
  return ret;
}

static void *xrealloc(void *data, size_t size)
{
  void *ret;

  if ((ret = realloc(data, size)) == NULL)
  {
    fprintf(stderr, "Unable to realloc %d bytes\n", size);
    exit(-1);
  }
  return ret;
}

static void mpp_abort(environment_t *env)
{
  if (env->outfile != NULL)
  {
    fclose(env->outfp);
    remove(env->outfile);
  }
  exit(-1);
}



static void NewBoolStack(boolstack_t *bstk)
{
  bstk->maxbytes = BVEC_GROW_BY;
  bstk->data = (unsigned char*)xmalloc(bstk->maxbytes);
  if (bstk->data != NULL)
    memset(bstk->data, 0, bstk->maxbytes);
  bstk->depth = 0;
}

static void PushBoolStack(boolstack_t *bstk, int bval)
{
  if (((bstk->depth + 7) >> 3) >= bstk->maxbytes)
  {
    bstk->maxbytes += BVEC_GROW_BY;
    bstk->data = (unsigned char*)xrealloc(bstk->data, bstk->maxbytes);
    if (bstk->data != NULL)
      memset(bstk->data + (bstk->maxbytes - BVEC_GROW_BY), 0, BVEC_GROW_BY);
  }
  if (bval == 0)
    bstk->data[bstk->depth >> 3] &= ~(1 << (bstk->depth & 7));
  else
    bstk->data[bstk->depth >> 3] |= (1 << (bstk->depth & 7));
  bstk->depth++;
}

static int PeekBoolStack(const boolstack_t *bstk)
{
  if (bstk == NULL) return 1;	/* required for -D switch */
  if ((bstk->depth == 0) || (((bstk->depth + 7) >> 3) >= bstk->maxbytes)) return 0;
  return ((bstk->data[(bstk->depth-1) >> 3] & (1 << ((bstk->depth-1) & 7))) != 0);
}

static int PopBoolStack(boolstack_t *bstk)
{
  if (bstk->depth == 0) return 0;
  else
  {
    int retval = PeekBoolStack(bstk);
    bstk->depth--;
    return retval;
  }
}

static void FreeBoolStack(boolstack_t *bstk)
{
  free(bstk->data);
  bstk->data = NULL; bstk->maxbytes = 0;
}

static void InitString(string_t *str)
{
  str->size = STRING_GROW_BY;
  str->base = (char*)xmalloc(str->size);
  str->ptr = str->base;
}

static void ResetString(string_t *str)
{
  str->ptr = str->base;
}

static void FreeString(string_t *str)
{
  free(str->base);
  str->base = NULL; str->ptr = NULL;
  str->size = 0;
}

static void WriteString(string_t *str, const char *newstr, unsigned int size)
{
  if ((str->size - (str->ptr - str->base)) < size)
  {
    unsigned int newsize;
    char *newstring;

    newsize = (str->size + size) / STRING_GROW_BY;
    str->size = (newsize + 1) * STRING_GROW_BY;
    newstring = (char*)xrealloc(str->base, str->size);
    str->ptr = newstring + (str->ptr - str->base);
    str->base = newstring;
  }
  memcpy(str->ptr, newstr, size);
  str->ptr += size;
}

static void TerminateString(string_t *str)
{
  if ((str->ptr - str->base) >= str->size)
  {
    char *newstring;

    str->size += STRING_GROW_BY;
    newstring = (char*)xrealloc(str->base, str->size);
    str->ptr = newstring + (str->ptr - str->base);
    str->base = newstring;
  }
  *(str->ptr++) = 0;
}


static int MakeHashCode(const char *str)
{
  const char *b = str;
  int code = 0;
  int shift = 0;

  while ((isalnum((unsigned int)*b)) || (*b == '_'))
  {
    code ^= (*b++) << shift;
    /* take care not too much is shifted out */
    if ((++shift) >= LD_HASH_SIZE - 4) shift = 0;
  }
  return code & ((1 << LD_HASH_SIZE) - 1);
}


static void InitMacros(macrovec_t *mvec)
{
  int i;

  mvec->maxmacros = MACRO_GROW_BY;
  mvec->nummacros = 0;
  mvec->firstfree = 0;
  mvec->macros = (macro_t*)xmalloc(mvec->maxmacros * sizeof(macro_t));
  mvec->hashtable = (int*)xmalloc(sizeof(int) << LD_HASH_SIZE);
  for (i=0; i<(1<<LD_HASH_SIZE); i++)
    mvec->hashtable[i] = -1;
}

static void DeleteMacro(macrovec_t *mvec, unsigned int num)
{
  macro_t *m = mvec->macros + num;

  if ((num < mvec->nummacros) && (m->ident != NULL))
  {
    if (m->hashcode == -1)
    {
      fprintf(stderr, "WARNING: no hashcode for macro %s\n", m->ident);
    }
    else
    {
      if (m->prev == -1)
        mvec->hashtable[m->hashcode] = m->next;
      else
        mvec->macros[m->prev].next = m->next;

      if (m->next != -1)
	mvec->macros[m->next].prev = m->prev;
    }
    free((char*)(m->ident));
    if (m->value != NULL)
      free((char*)(m->value));
    /* mark as deleted, but don't move anything to avoid having to scan the
       entire hashtable for changes */
    m->ident = NULL; m->value = NULL;
    if (mvec->firstfree > num)
      mvec->firstfree = num;
  }
}

/* Claim a new, uninitialized macro */
static macro_t *NewMacro(macrovec_t *mvec)
{
  macro_t *m;
  unsigned int i;

  /* first try to find a deleted one */
  for (i=mvec->firstfree, m=mvec->macros + i; i<mvec->nummacros; i++, m++)
  {
    if (m->ident == NULL) break;
  }
  mvec->firstfree = i+1;
  /* none found? */
  if (i >= mvec->nummacros)
  {
    if (mvec->nummacros >= mvec->maxmacros)
    {
      mvec->maxmacros += MACRO_GROW_BY;
      mvec->macros = xrealloc(mvec->macros, mvec->maxmacros * sizeof(macro_t));
    }
    m = mvec->macros + mvec->nummacros;
    (mvec->nummacros)++;
  }
  m->value = NULL;
  m->params = 0;
  m->lines = 0;
  m->hashcode = -1; m->next = -1; m->prev = -1;

  return m;
}

/* Init a fully initialized macro (hashtable etc). Ranges assumed
   to be OK */
static void InsertMacro(macrovec_t *mvec, unsigned int num)
{
  macro_t *m = mvec->macros + num;
  int i = (int)num;

  m->hashcode = MakeHashCode(m->ident);
  m->prev = -1;	/* always insert at head of hashlist */
  m->next = mvec->hashtable[m->hashcode];
  if (m->next != -1)
    mvec->macros[m->next].prev = i;
  mvec->hashtable[m->hashcode] = i;
}

static void FreeMacros(macrovec_t *mvec)
{
  unsigned int i;

  for (i=mvec->nummacros; i>0; i--)
    DeleteMacro(mvec, i-1);
  free(mvec->macros); mvec->macros = NULL;
  free(mvec->hashtable); mvec->hashtable = NULL;
  mvec->maxmacros = 0;
  mvec->nummacros = 0;
}

static void PrintHashStatistics(macrovec_t *mvec, FILE *fp)
{
  int i, codes, total, max;

  codes = 0; total = 0; max = 0;
  for (i=0; i<(1 << LD_HASH_SIZE); i++)
  {
    int j = mvec->hashtable[i];

    if (j != -1)
    {
      int chain=0;

      codes++;
      while (j != -1)
      {
        chain++;
	j = mvec->macros[j].next;
      }
      total += chain;
      if (chain > max) max = chain;
    }
  }
  fprintf(fp, "Hashcodes defined: %d; macros: %d; longest chain: %d", codes, total, max);
  if (codes != 0) fprintf(fp, "; average chain %f", ((double)total)/codes);
  fprintf(fp, "\n");
}


static int CompareKeyword(const char *str1, const char *str2)
{
  int len = strlen(str2);

  if (strncmp(str1, str2, len) == 0)
  {
    if ((!isalnum((unsigned int)str1[len])) && (str1[len] != '_')) return 0;
  }
  return -1;
}


static macro_t *FindMacro(environment_t *env, const char *name)
{
  macrovec_t *mvec = env->mvec;
  int idx;

  idx = mvec->hashtable[MakeHashCode(name)];
  while (idx != -1)
  {
    macro_t *m = mvec->macros + idx;
    if ((m->ident != NULL) && (CompareKeyword(name, m->ident) == 0))
      return m;
    idx = m->next;
  }
  return NULL;
}


static char *GetMacroOrLiteral(environment_t *env, const char **start)
{
  char *value;

  if ((value = FindAndMaterializeMacro(env, start)) == NULL)
  {
    const char *first = *start;
    const char *b;

    b = first;
    if (*b == '\"')
    {
      b++;
      while ((*b != '\0') && (*b != '\"')) b++;
      if (*b == '\0')
      {
        fprintf(stderr, "%s: invalid expression at line %d\n", env->file, env->line);
        mpp_abort(env);
      }
      b++;
    }
    else
    {
      while ((isalnum((unsigned int)*b)) || (*b == '_')) b++;
    }
    value = xmalloc((b - first) + 1);
    strncpy(value, first, (b-first));
    value[b-first] = '\0';
    *start = b;
  }

  return value;
}


static int EvaluateExpression(environment_t *env, char *expr)
{
  unsigned int parantheses;
  char *b, *first;

  /* find all || not enclosed within parantheses and split them up into
     recursive calls */
  first = expr; b = first; parantheses = 0;
  while (*b != '\0')
  {
    char c = *b++;

    if (c == '(') parantheses++;
    else if (c == ')') parantheses--;

    if ((parantheses == 0) && (c == '|') && (*b == '|'))
    {
      b[-1] = '\0';
      if (EvaluateExpression(env, first) != 0) return 1;
      b++; first = b;
    }
  }

  while (*first != '\0')
  {
    int negate;
    int subexpr;

    while (isspace((unsigned int)*first)) first++;
    if (*first == '!')
    {
      negate = 1; first++;
      while (isspace((unsigned int)*first)) first++;
    }
    else
      negate = 0;
    /* first char is opening paranthesis ==> strip pair */
    if (*first == '(')
    {
      b = ++first;
      parantheses = 1;
      while (*b != '\0')
      {
        if (*b == '(') parantheses++;
        else if (*b == ')')
        {
          parantheses--;
          if (parantheses == 0)
          {
            *b++ = '\0'; break;
          }
        }
        b++;
      }
      while (isspace((unsigned int)*b)) b++;
      if ((b[0] == '&') && (b[1] == '&'))
      {
        *b = '\0';
        subexpr = EvaluateExpression(env, first);
        if (negate) subexpr = !subexpr;
        if (subexpr == 0) return 0;
        first = b+2;
      }
      else if (*b == '\0')
      {
        subexpr = EvaluateExpression(env, first);
        if (negate) subexpr = !subexpr;
        return subexpr;
      }
    }
    else
    {
      subexpr = negate;
      if (CompareKeyword(first, env->pinfo->str_defined) == 0)
      {
        macro_t *m;

        first += strlen(env->pinfo->str_defined);
        while (isspace((unsigned int)*first)) first++;
        m = FindMacro(env, first);
        subexpr = (m != NULL);
        if (negate) subexpr = !subexpr;
        while ((isalnum((unsigned int)*first)) || (*first == '_')) first++;
      }
      else
      {
        char *left;
        int j, l=0;

        left = GetMacroOrLiteral(env, (const char**)&first);
        while (isspace((unsigned int)*first)) first++;
        for (j=0; env->binops[j].operation != NULL; j++)
        {
          l = strlen(env->binops[j].operation);
          if (strncmp(first, env->binops[j].operation, l) == 0) break;
        }
        if (env->binops[j].operation == NULL)
        {
          char *rest;
          int ival;

          /* Expressions consisting of integer numbers are OK */
          ival = strtol(left, &rest, 10);
          if ((left == rest) || (*rest != '\0'))
          {
            fprintf(stderr, "%s: Bad expression at line %d\n", env->file, env->line);
            mpp_abort(env);
          }
          subexpr = (ival != 0);
        }
        else
        {
          char *right;

          first += l;
          while (isspace((unsigned int)*first)) first++;
          right = GetMacroOrLiteral(env, (const char**)&first);
          if (env->binops[j].integers == 0)
          {
            switch (env->binops[j].opid)
            {
              case binop_equal:
                subexpr = (strcmp(left, right) == 0); break;
              case binop_unequal:
                subexpr = (strcmp(left, right) != 0); break;
              default:
                break;
            }
          }
          else
          {
            int lval, rval=0;
            char *rest;

            lval = strtol(left, &rest, 10);
            if ((left == rest) || (*rest != '\0'))
              rest = NULL;
            else
            {
              rval = strtol(right, &rest, 10);
              if ((right == rest) || (*rest != '\0')) rest = NULL;
            }
            if (rest == NULL)
            {
              fprintf(stderr, "%s: Can't transform to integers at line %d\n", env->file, env->line);
              mpp_abort(env);
            }
            switch(env->binops[j].opid)
            {
              case binop_lower:
                subexpr = (lval < rval); break;
              case binop_lowereq:
                subexpr = (lval <= rval); break;
              case binop_higher:
                subexpr = (lval > rval); break;
              case binop_highereq:
                subexpr = (lval >= rval); break;
              default:
                break;
            }
          }
          free(right);
        }
        if (negate) subexpr = !subexpr;
        free(left);
      }
      while (isspace((unsigned int)*first)) first++;
      if ((first[0] == '&') && (first[1] == '&'))
      {
        if (subexpr == 0) return 0;
        first += 2;
      }
      else if (*first == 0) return subexpr;
    }
  }

  return 0;
}


static int StartEvaluation(environment_t *env, const char *expr)
{
  char *temp;
  int status;

  temp = xmalloc(strlen(expr)+1);
  strcpy(temp, expr);
  status = EvaluateExpression(env, temp);
  free(temp);
  return status;
}


argdesc_t *ParseParams(environment_t *env, const char **inout, unsigned int *numargs)
{
  const char *b;
  argdesc_t *args = NULL;

  *numargs = 0; b = *inout;
  while ((isspace((unsigned int)*b)) && (*b != '\n')) b++;
  if (*b == '(')
  {
    const char *start;
    char *params=NULL;
    unsigned int size=0;
    unsigned int par=0;
    int pass;

    start = b+1;
    for (pass=0; pass<2; pass++)
    {
      if (pass != 0)
      {
        args = (argdesc_t*)xmalloc(par * sizeof(argdesc_t));
        params = (char*)xmalloc(size);
      }
      size = 0; par = 0; b = start;
      while (*b != ')')
      {
        const char *ps;
        const char *nowhite=b;

        while (isspace((unsigned int)*b)) b++;
        ps = b;
        if (*b == '\"')
        {
          b++;
          while (*b != '\"')
          {
            if (*b == '\0')
            {
              fprintf(stderr, "%s: bad argument #%d at line %d\n", env->file, par+1, env->line);
              mpp_abort(env);
            }
            b++;
          }
          b++; nowhite = b;
        }
        else
        {
          unsigned int parantheses = 0;

          while ((*b != ',') || (parantheses != 0))
          {
            if (*b == '\0') break;
            else if (*b == '(') parantheses++;
            else if (*b == ')')
            {
              if (parantheses == 0) break;
              parantheses--;
            }
            else if (*b == '\\') b++;	/* escape, e.g. for commas */
            if (!isspace((unsigned int)*b)) nowhite = b+1;
            b++;
          }
          if (parantheses != 0)
          {
            fprintf(stderr, "%s: bad parantheses structure at line %d\n", env->file, env->line);
            mpp_abort(env);
          }
        }
        if (params != NULL)
        {
          const char *pptr = ps;
          unsigned int ns = size, parantheses = 0;

          /* In case of backslashes in parameters, interpret them as escaped special chars
             unless they're contained within (), in which case just preserve the escape. */
          args[par].arg = params + size;
          while (pptr != nowhite)
          {
            if (*pptr == '(') parantheses++;
            else if (*pptr == ')') parantheses--;
            else if (*pptr == '\\')
            {
              if (parantheses == 0) pptr++;
            }
            params[ns++] = *pptr++;
          }
          params[ns] = '\0';
          args[par].len = (ns - size);
        }
        size += (nowhite - ps) + 1;
        par++;
        while (isspace((unsigned int)*b)) b++;
        if ((*b != ',') && (*b != ')'))
        {
          fprintf(stderr, "%s: parse error at line %d while reading ( %s ): bad terminator %c\n", env->file, env->line, ps, *b);
          mpp_abort(env);
        }
        if (*b == ',') b++;
      }
    }
    *numargs = par;
    b++;
  }
  *inout = b;

  if (env->verbose >= 2)
  {
    printf("Parameters: (");
    if (args != NULL)
    {
      unsigned int i;

      printf("%s[%d]", args[0].arg, args[0].len);
      for (i=1; i<*numargs; i++)
      {
        printf(",%s[%d]", args[i].arg, args[i].len);
      }
    }
    printf(")\n");
    fflush(stdout);
  }

  return args;
}

static void FreeParams(argdesc_t *args)
{
  if (args != NULL)
  {
    free(args[0].arg);
    free(args);
  }
}


static void EnsureFilePointer(environment_t *env)
{
  if (env->fp == NULL)
  {
    fprintf(stderr, "Bad macro definition at commandline!\n");
    mpp_abort(env);
  }
}

static int ParseMacro(environment_t *env, const char *name)
{
  static char *block=NULL;
  static unsigned int bsize=0;
  macro_t *m=NULL;
  unsigned int numargs, k;
  argdesc_t *args;
  const char *b;
  char *d;
  int i, j=0;
  char *bptr;

  b = name;
  while (isalnum((unsigned int)*b) || (*b == '_')) b++;

  /* We have to parse the macro even if we're in a false block!
     But we mustn't create a new macro. */
  if (PeekBoolStack(env->bstk))
  {
    k = (b - name);
    d = xmalloc(k + 1); strncpy(d, name, k); d[k] = '\0';
    if ((m = FindMacro(env, d)) != NULL)
    {
      fprintf(stderr, "%s: Warning, redefining macro %s at line %d\n", env->file, d, env->line);
      DeleteMacro(env->mvec, m - env->mvec->macros);
    }
    m = NewMacro(env->mvec);
    m->ident = d; m->idlength = k;
    m->lines = 0;
    if (env->verbose >= 2)
    {
      printf("%s: NEW MACRO %s at line %d\n", env->file, m->ident, env->line);
      fflush(stdout);
    }
  }
  args = ParseParams(env, &b, &numargs);
  if (m != NULL)
    m->params = numargs;

  if (block == NULL)
  {
    block = xmalloc(BLOCK_SIZE);
    bsize = BLOCK_SIZE;
  }
  bptr = block;
  while ((*b != '\n') && (isspace((unsigned int)*b))) b++;
  if (*b == '=')
  {
    b++;
    while (isspace((unsigned int)*b)) b++;
    while (*b == '\0')
    {
      EnsureFilePointer(env);
      (env->line)++;
      if ((b = fgets(env->buffer, env->buffSize, env->fp)) == NULL)
      {
        fprintf(stderr, "%s: unexpected EOF\n", env->file);
        mpp_abort(env);
      }
      while (isspace((unsigned int)*b)) b++;
    }
    if (*b == '{')
    {
      b++; i = 0;
      while (b[i] != '\n')
      {
        if (b[i] == '}') break;
        /* escape? */
        if (b[i] == env->pinfo->escchar) i++;
        i++;
      }
      if (bptr + i >= block + bsize)
      {
        bsize += BLOCK_SIZE; j = bptr - block;
        block = xrealloc(block, bsize);
        bptr = block + j;
      }
      strncpy(bptr, b, i); bptr += i;
      if (b[i] != '}')
      {
        EnsureFilePointer(env);
        while (!feof(env->fp))
        {
          (env->line)++;
          if ((b = fgets(env->buffer, env->buffSize, env->fp)) == NULL)
          {
            fprintf(stderr, "%s: unexpected EOF\n", env->file);
            mpp_abort(env);
          }
          while (*b != '\0')
          {
            if (*b == '}') break;
            if (*b == env->pinfo->escchar) b++;
            b++;
          }
          if (bptr + (b-env->buffer) >= block + bsize)
          {
            bsize += BLOCK_SIZE; j = bptr - block;
            block = xrealloc(block, bsize);
            bptr = block + j;
          }
          strncpy(bptr, env->buffer, (b-env->buffer));
          bptr += (b-env->buffer);
          if (*b == '}') break;
        }
      }
    }
    else
    {
      int parantheses=0;
      for (i=0; (parantheses!=0) || (!isspace((unsigned int)b[i])); i++)
      {
        if (b[i] == '\0') break;
        else if (b[i] == '(') parantheses++;
        else if (b[i] == ')') parantheses--;
      }
      if (bptr + i >= block + bsize)
      {
        bsize += BLOCK_SIZE; j = bptr - block;
        block = xrealloc(block, bsize);
        bptr = block + j;
      }
      strncpy(bptr, b, i); bptr += i;
    }
  }
  *bptr = 0;

  /* false block? ==> just return */
  if (m == NULL)
  {
    FreeParams(args);
    return 0;
  }

  if (strlen(block) == 0)
  {
    m->value = NULL;
  }
  else
  {
    d = NULL;
    for (i=0; i<2; i++)
    {
      if (i != 0)
      {
        d = (char*)xmalloc(strlen(block) + 1 + j);
        m->value = d;
      }
      j = 0;
      b = block;
      while (b < bptr)
      {
        if (*b == env->pinfo->refopen)
        {
          if (b[1] == env->pinfo->refopen)
          {
            if (d != NULL) d += sprintf(d, "%c%c", env->pinfo->refopen, env->pinfo->refopen);
            b+=2;
          }
          else
          {
            char symbol[STRINGSIZE];

            k=0; b++;
            while (b[k] != env->pinfo->refclose)
            {
              if ((b[k] == '\0') || (b[k] == '\n'))
              {
                fprintf(stderr, "%s: parse error in macro %s\n", env->file, m->ident);
                mpp_abort(env);
              }
              k++;
            }
            strncpy(symbol, b, k); symbol[k] = '\0';
            b += k+1;
            for (k=0; k<numargs; k++)
            {
              if (strcmp(args[k].arg, symbol) == 0) break;
            }
            if (k >= numargs)
            {
              fprintf(stderr, "%s: unknown parameter $%s$ referenced in macro %s\n", env->file, symbol, m->ident);
              mpp_abort(env);
            }
            if (d != NULL)
            {
              d += sprintf(d, "%c%d%c", env->pinfo->refopen, k, env->pinfo->refclose);
            }
            else
            {
              char snum[16];

              sprintf(snum, "%d", k);
              j += strlen(snum) - strlen(symbol);
            }
          }
        }
        else
        {
          if (*b == env->pinfo->escchar) b++;
          if (d != NULL)
          {
            if ((*d++ = *b) == '\n') (m->lines)++;
          }
          b++;
        }
      }
    }
    *d = '\0';
    /* remove trailing newlines */
    d--;
    while ((d > m->value) && (isspace((unsigned int)*d)))
    {
      if (*d == '\n') *d = '\0';
      d--;
    }
  }

  InsertMacro(env->mvec, (m - env->mvec->macros));

  FreeParams(args);

  return 0;
}


static char *FindAndMaterializeMacroLevel(environment_t *env, const char **string)
{
  macro_t *m;
  const char *b = *string;
  argdesc_t *args;
  unsigned int numargs;
  char *bptr;
  int j=0;
  unsigned int i;
  char *result=NULL;

  if ((m = FindMacro(env, b)) == NULL) return NULL;

  if (env->verbose >= 2)
  {
    printf("%s: read parameters for macro %s at line %d\n", env->file, m->ident, env->line);
    fflush(stdout);
  }

  b += m->idlength;
  args = ParseParams(env, &b, &numargs);
  if (numargs != m->params)
  {
    fprintf(stderr, "%s: bad number of parameters (%d) for macro %s at line %d\n", env->file, numargs, m->ident, env->line);
    mpp_abort(env);
  }
  *string = b;
  bptr = NULL;
  if (m->value != NULL)
  {
    const char *mptr=NULL;

    for (i=0; i<2; i++)
    {
      if (i != 0)
      {
        result = (char*)xmalloc((mptr - m->value) + j + 1);
        bptr = result;
      }
      j = 0; mptr = m->value;
      while (*mptr != '\0')
      {
        if (*mptr == env->pinfo->escchar)
	{
	  if (bptr != NULL)
	  {
	    *bptr++ = mptr[1];
	  }
	  mptr+=2;
	}
        else if (*mptr == env->pinfo->refopen)
        {
	  if (mptr[1] == '\0')
	  {
	    printf("Misplaced %c char at line %d\n", env->pinfo->refopen, env->line);
	    mpp_abort(env);
	  }
          else if (mptr[1] == env->pinfo->refopen)
          {
            if (bptr != NULL)
            {
              *bptr++ = env->pinfo->refopen; *bptr++ = env->pinfo->refopen;
            }
            mptr+=2;
          }
          else
          {
            const char *argbase;

            argbase = ++mptr; numargs=0;
            while ((*mptr != '\0') && (*mptr != env->pinfo->refclose))
            {
              numargs = 10*numargs + (*mptr - '0');
              mptr++;
            }
            j += args[numargs].len - (mptr - argbase);
            if (*mptr != '\0') mptr++;
            if (bptr != 0)
            {
              strcpy(bptr, args[numargs].arg); bptr += args[numargs].len;
            }
          }
        }
        else
        {
          if (bptr != NULL) *bptr++ = *mptr;
          mptr++;
        }
      }
    }
    *bptr++ = '\0';
  }
  else
  {
    /* inefficient but easier to handle */
    result = xmalloc(1); result[0] = '\0';
  }

  FreeParams(args);

  return result;
}

static char *FindAndMaterializeMacro(environment_t *env, const char **str)
{
  char *material;

  if ((material = FindAndMaterializeMacroLevel(env, str)) != NULL)
  {
    const char *b;
    const char *base;
    string_t string;
    char *newmat;

    if (env->verbose >= 2)
    {
      printf("*** MAT( %s ) ***\n", material);
      fflush(stdout);
    }

    InitString(&string);
    base = material; b = base;
    while (*b != '\0')
    {
      const char *old;

      while ((*b != '\0') && (!isalnum((unsigned int)*b)) && (*b != '_')) b++;
      old = b;
      if ((newmat = FindAndMaterializeMacro(env, &b)) != NULL)
      {
        WriteString(&string, base, old-base);
        WriteString(&string, newmat, strlen(newmat));
        free(newmat);
        base = b;
      }
      else
      {
        while ((isalnum((unsigned int)*b)) || (*b == '_')) b++;
      }
    }
    if (b != base)
    {
      WriteString(&string, base, b-base);
    }
    TerminateString(&string);
    free(material);
    /* just use the pointer directly */
    return string.base;
  }
  return NULL;
}


static int TransformString(environment_t *env, char *string)
{
  const char *b;
  const char *base;

  base = string; b = base;
  while (*b != '\0')
  {
    char *material;
    const char *old;

    while ((b[0] == '\\') && (b[1] == '\\'))
    {
      fwrite(base, 1, (b-base)+1, env->outfp);
      b += 2; base = b;
    }

    while ((*b != '\0') && (!isalnum((unsigned int)*b)) && (*b != '_')) b++;
    old = b;
    if ((material = FindAndMaterializeMacro(env, &b)) != NULL)
    {
      fwrite(base, 1, old-base, env->outfp);
      fwrite(material, 1, strlen(material), env->outfp);
      free(material);
      base = b;
    }
    else
    {
      while ((isalnum((unsigned int)*b)) || (*b == '_')) b++;
    }
  }
  if (b != base)
  {
    fwrite(base, 1, b-base, env->outfp);
  }
  return 0;
}


static int ParseFileIntern(const char *file, const environment_t *srcEnv, FILE *fp)
{
  char buffer[BUFFER_SIZE];
  boolstack_t bstk;
  environment_t env;

  memcpy(&env, srcEnv, sizeof(environment_t));
  env.fp = fp;
  env.file = (file == NULL) ? "stdin" : file;

  env.line = 0; env.buffer = buffer; env.buffSize = BUFFER_SIZE; env.bstk = &bstk;
  NewBoolStack(&bstk);
  PushBoolStack(&bstk, 1);

  while (!feof(env.fp))
  {
    char *b;

    if (fgets(buffer, BUFFER_SIZE, env.fp) == NULL) break;
    b = buffer; (env.line)++;
    while (isspace((unsigned int)*b)) b++;
    if (*b == env.pinfo->cmdchar)	/* commando? */
    {
      b++;
      if (*b != env.pinfo->cmdchar)
      {
        const command_ptr_t *cmds = env.pinfo->commands;

        while (isspace((unsigned int)*b)) b++;
        while (cmds->ident != NULL)
        {
          if (CompareKeyword(b, cmds->ident) == 0) break;
          cmds++;
        }
        if (cmds->ident == NULL)
        {
          fprintf(stderr, "%s: Unknown command at line %d\n", env.file, env.line);
          mpp_abort(&env);
        }
        b += strlen(cmds->ident);
        while (isspace((unsigned int)*b)) b++;
        if (cmds->handler != NULL)
          cmds->handler(&env, b);
      }
    }
    else if (PeekBoolStack(env.bstk))
    {
      TransformString(&env, buffer);
    }
  }

  fclose(env.fp);

  FreeBoolStack(&bstk);

  return 0;
}


static int ParseFile(const char *file, const environment_t *srcEnv)
{
  FILE *fp;
  int status = 0;

  if ((fp = fopen(file, "r")) == NULL)
  {
    fprintf(stderr, "Unable to open file \"%s\"\n", file);
    status = -1;
  }
  else
  {
    status = ParseFileIntern(file, srcEnv, fp);
  }

  return status;
}


static int ParseInclude(environment_t *env, const char *name)
{
  char filename[STRINGSIZE] = "";
  const char *b = name;
  char *d;

  if (PeekBoolStack(env->bstk) == 0) return 0;

  while (isspace((unsigned int)*b)) b++;
  if (*b == '\"')
  {
    b++; d = filename;
    while ((*b != '\"') && (*b != '\0'))
    {
      if (*b == USE_DIR_SEPARATOR) *d++ = DIR_SEPARATOR;
      else if (*b == USE_EXT_SEPARATOR) *d++ = EXT_SEPARATOR;
      else *d++ = *b;
      b++;
    }
    *d++ = '\0';
    if (*b != '\"') filename[0] = '\0';
  }
  if (filename[0] == '\0')
  {
    fprintf(stderr, "%s: bad include at line %d\n", env->file, env->line);
    mpp_abort(env);
  }
  return ParseFile(filename, env);
}


static int ParseIf(environment_t *env, const char *name)
{
  /* If we're in a false block, this will become a false block by default.
     otherwise we have to evaluate the expression. */
  if (PeekBoolStack(env->bstk) == 0)
    PushBoolStack(env->bstk, 0);
  else
    PushBoolStack(env->bstk, StartEvaluation(env, name));

  return 0;
}


static int ParseElse(environment_t *env, const char *name)
{
  int currentMode;

  /* else maintains the stack level but inverts the block mode */
  currentMode = PopBoolStack(env->bstk);
  /* if the previous block was false, this one must be too. */
  currentMode = PeekBoolStack(env->bstk) & !currentMode;
  PushBoolStack(env->bstk, currentMode);

  return 0;
}


static int ParseElif(environment_t *env, const char *name)
{
  int currentMode;

  /* FIXME: elif is broken; this is a conceptual problem and can't be resolved
     with the current bool stack (e.g. \if [true] \elif ... \else ... \endif
     will result in the following states: true, false, true (because else inverts
     the previous state). Don't use elif ATM. */
  currentMode = PopBoolStack(env->bstk);
  /* if the previous block was false, this one must be too. */
  if ((currentMode != 0) || (PeekBoolStack(env->bstk) == 0))
    PushBoolStack(env->bstk, 0);
  else
    PushBoolStack(env->bstk, StartEvaluation(env, name));

  return 0;
}


static int ParseEndif(environment_t *env, const char *name)
{
  PopBoolStack(env->bstk);

  return 0;
}


static int ParseCommentary(environment_t *env, const char *name)
{
  return 0;
}


static int ParseUndef(environment_t *env, const char *name)
{
  macro_t *m;

  if ((m = FindMacro(env, name)) == NULL)
  {
    fprintf(stderr, "%s: Warning, undef unknown macro %s at line %d\n", env->file, name, env->line);
  }
  else
  {
    DeleteMacro(env->mvec, (m - env->mvec->macros));
  }
  return 0;
}



static void DumpMacros(macrovec_t *mvec, FILE *fp)
{
  unsigned int i;
  macro_t *m = mvec->macros;

  for (i=0; i<mvec->nummacros; i++, m++)
  {
    if (m->ident != NULL)
    {
      fprintf(fp, "*** MACRO %3d: <%s>", i, m->ident);
      if (m->params != 0) fprintf(fp, "(%d)", m->params);
      fprintf(fp, " [hash %x]", m->hashcode);
      if (m->value != NULL)
        fprintf(fp, " = {\n%s\n}", m->value);
      fprintf(fp, "\n");
    }
  }
}



static command_ptr_t command_ptrs[] = {
  { "co", ParseCommentary },
  { "include", ParseInclude },
  { "define", ParseMacro },
  { "undef", ParseUndef },
  { "if", ParseIf },
  { "else", ParseElse },
  { "elif", ParseElif },
  { "endif", ParseEndif },
  { NULL, NULL }
};

static parse_info_t default_parse_info = {
  '\\', '\\', '$', '$',
  "defined",
  command_ptrs
};

static binary_operation_t default_binary_ops[] = {
  { "==", binop_equal, 0 },
  { "!=", binop_unequal, 0 },
  { "<",  binop_lower, 1 },
  { "<=", binop_lowereq, 1 },
  { ">",  binop_higher, 1 },
  { ">=", binop_highereq, 1 },
  { NULL, binop_none, 1 }
};


static void ShowUsage(const char *name)
{
  printf("MPP 1.00 (Macro PreProcessor)\n");
  printf("Written by Andreas Dehmel, 1999. Licensed under GPL\n");
  printf("Usage: %s [-o file -Dmacro=value -rXX -eXX -m -s -v # -h]\n", name);
  printf("\t-o: output to file, otherwise stdout\n");
  printf("\t-D: explicitly define a macro\n");
  printf("\t-r: define opening and closing chars for derefs (default $$)\n");
  printf("\t-e: define escape chars for commands and within macros (default \\\\)\n");
  printf("\t-m: dump macros before exit\n");
  printf("\t-s: print statistics before exit\n");
  printf("\t-v: set verbose level to x\n");
  printf("\t--: read from stdin\n");
  printf("\t-h: this help\n");
  exit(0);
}



int main(int argc, char *argv[])
{
  macrovec_t mvec;
  environment_t env;
  char infile[STRINGSIZE] = "";
  int i;
  int doprint=0;
  int dostdin=0;

  memset(&env, 0, sizeof(environment_t));

  InitMacros(&mvec);
  env.mvec = &mvec;
  env.pinfo = &default_parse_info;
  env.binops = default_binary_ops;
  env.verbose = 0;
  env.outfile = NULL;
  env.outfp = stdout;
  env.fp = NULL; env.bstk = NULL;

  i=1;
  while (i < argc)
  {
    if (argv[i][0] == '-')
    {
      switch (argv[i][1])
      {
        case 'o':	/* output file */
          if ((env.outfp = fopen(argv[++i], "w")) == NULL)
          {
            fprintf(stderr, "Unable to open %s for output\n", argv[i]);
            mpp_abort(&env);
          }
          env.outfile = argv[i];
          break;
        case 'D':	/* define macro */
          ParseMacro(&env, argv[i]+2);
          break;
        case 'r':
          env.pinfo->refopen = argv[i][2];
          env.pinfo->refclose = argv[i][3];
          break;
        case 'e':
          env.pinfo->cmdchar = argv[i][2];
          env.pinfo->escchar = argv[i][3];
          break;
        case 'm':	/* do macro dump afterwards */
          doprint |= 1;
          break;
        case 's':	/* print statistics afterwards */
          doprint |= 2;
          break;
        case 'h':
          ShowUsage(argv[0]);
          break;
        case 'v':
          env.verbose = atoi(argv[++i]);
          break;
        case '-':
          dostdin = 1;
          break;
        default:
          fprintf(stderr, "Bad switch %s\n", argv[i]);
          mpp_abort(&env);
      }
    }
    else
    {
      if (infile[0] == '\0')
        strcpy(infile, argv[i]);
      else
      {
        fprintf(stderr, "More than one input file given: %s\n", argv[i]);
        mpp_abort(&env);
      }
    }
    i++;
  }

  if (infile[0] != '\0')
    ParseFile(infile, &env);
  else if (dostdin)
    ParseFileIntern(NULL, &env, stdin);

  if (env.outfile != NULL)
    fclose(env.outfp);

  if ((doprint & 1) != 0)
    DumpMacros(&mvec, stdout);
  if ((doprint & 2) != 0)
    PrintHashStatistics(&mvec, stdout);

  FreeMacros(&mvec);

  return 0;
}
