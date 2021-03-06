#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__attribute__((noreturn))
static void help(const int ret) {
  FILE *const helpfh = ret ? stderr : stdout;
  if(ret) fprintf(stderr, "zskwproc: not enough or invalid arguments\n");
  fprintf(helpfh, "USAGE: zskwproc [--help|-o OUTPUT] TARGET INPUT\n"
    "\n"
    "TARGETs:\n"
    "  L    lists/kw.h -- KW(name, id, kwrepr)\n"
    "  D    #define LT_... I\n"
    "  S    { #S, I },\n"
    "  n    #S, #S, ...\n"
    "\n"
    "INPUT SYNTAX:\n"
    "  '. '...    set the start value, read as hex value\n"
    "  KW_name KW_repr\n"
    "\n"
    "RETURN CODE:\n"
    "  0    successful\n"
    "  1    invalid arguments\n"
    "  2    file I/O error\n"
    "  3    OOM\n"
    "zskwproc -- (C) 2019 Erik Zscheile -- License: MIT\n");
  exit(ret);
}

__attribute__((noreturn))
static void oomerr(const char *fn, const char *vartype, const size_t allocsiz) {
  fprintf(stderr, "zskwproc: OOM @ %s() while allocating '%s' with size %zu\n", fn, vartype, allocsiz);
  exit(3);
}

static void * zsmalloc_h(const char *fn, const char *vartype, const size_t allocsiz) {
  void * ret = malloc(allocsiz);
  if(!ret) oomerr(fn, vartype, allocsiz);
  return ret;
}

static char *zs_strdup(const char *fn, const char *str) {
  char * ret = strdup(str);
  if(!ret) oomerr(fn, "zstring", strlen(str));
  return ret;
}

#define ZSMALLOC(TYPE, NAME) TYPE *NAME = zsmalloc_h(__func__, #TYPE, sizeof(TYPE))

typedef struct keyword_ {
  char *name, *repr;
  int val;
  struct keyword_ *prev, *next;
} keyword;

static void new_keyword(keyword ** kws, const char * name, const char *repr, int val) {
  ZSMALLOC(keyword, ret);
  ret->name = zs_strdup(__func__, name);
  ret->repr = zs_strdup(__func__, repr);
  ret->val = val;
  ret->prev = *kws;
  ret->next = 0;
  if(*kws) (*kws)->next = ret;
  *kws = ret;
}

static void delete_keywords(keyword *kw) {
  while(kw) {
    free(kw->name);
    free(kw->repr);
    keyword *const tmp = kw->next;
    free(kw);
    kw = tmp;
  }
}

typedef void (*kwfe_fnt)(FILE *outfh, const char *name, const char *repr, int val);
static void foreach_keywords(keyword *kw, const kwfe_fnt fn, FILE *outfh) {
  for(; kw; kw = kw->next)
    fn(outfh, kw->name, kw->repr, kw->val);
}

static void gen_lists_kw_h(FILE *outfh, const char *name, const char *repr, int val) {
  fprintf(outfh, "KW(%s,\t%d,\t\"%s\")\n", name, val, repr);
}

static void gen_kw_defs_h(FILE *outfh, const char *name, const char *repr, int val) {
  fprintf(outfh, "#define LT_%s\t%d\n", name, val);
}

static void gen_kw_stmap_h(FILE *outfh, const char *name, const char *repr, int val) {
  fprintf(outfh, "  { \"%s\", %d },\n", repr, val);
}

static void gen_kw_names_h(FILE *outfh, const char *name, const char *repr, int val) {
  fprintf(outfh, "  \"%s\",\n", name);
}

static bool tr_target_is_valid(const char *s) {
  if(s[1]) return false;
  switch(s[0]) {
    case 'D': case 'L': case 'S': case 'n':
      return true;
    default:
      return false;
  }
}

int main(int argc, char **argv) {
  /* parse arguments */
  if(argc == 2 && !strcmp(argv[1], "--help"))
    help(0);
  if(argc < 3)
    help(1);

  FILE *outfh = stdout;
  if(!strcmp(argv[1], "-o")) {
    outfh = fopen(argv[2], "w");
    if(!outfh) {
      fprintf(stderr, "zskwproc: %s: file open for writing failed", argv[2]);
      return 2;
    }
    argc -= 2;
    argv += 2;
  }

  if(argc < 3)
    help(1);

  if(!tr_target_is_valid(argv[1])) {
    fprintf(stderr, "zskwproc: unknwon target '%s'\n", argv[1]);
    help(1);
  }

  FILE *infh = fopen(argv[2], "r");
  if(!infh) {
    fprintf(stderr, "zskwproc: %s: file not found", argv[2]);
    return 2;
  }

  /* main part */
  int val = 0, ret = 0;
  keyword *kws = 0;

  /* read keywords */
  {
    char *buffer = 0;
    size_t buflen = 0, linelen = 0, lineno = 0;
    while(linelen = getline(&buffer, &buflen, infh)) {
      if(linelen == -1) break;
      lineno++;
      if(linelen <= 1) continue; /* skip empty line (buf[0] == '\n') */
      if(buffer[0] == '#') continue; /* skip comment line */
      if(!strncmp(". ", buffer, 2)) {
        // we got an 'val' assignment
        sscanf(buffer + 2, "%x", &val);
      } else {
        char *bufptr = buffer, *reprptr = 0;
        const size_t imax = linelen - 2;
        buffer[linelen - 1] = 0;
        for(size_t i = 0; i < imax; ++i) {
          if(*bufptr == ' ' || *bufptr == '\n') {
            *bufptr = 0;
            if(!reprptr) {
              reprptr = bufptr + 1;
              while(reprptr != (buffer - 1) && *reprptr == ' ')
                ++reprptr;
            }
          }
          ++bufptr;
        }
        if(!reprptr || reprptr == (buffer - 1)) {
          printf("zskwproc: ignore invalid line %zu with len %zu: %.*s\n", lineno - 1, linelen, linelen - 1, buffer);
          continue;
        }
        new_keyword(&kws, buffer, reprptr, val++);
      }
    }
    free(buffer);
  }

  /* seek to start */
  while(kws && kws->prev) kws = kws->prev;

  /* transform into target */
  fprintf(outfh, "/** GENERATED BY zskwproc [generator kw_");
  switch(argv[1][0]) {
    case 'L':
      fprintf(outfh, "x.h] from '%s' **/\n", argv[2]);
      fprintf(outfh, "#ifdef KW\n/* KW(name, token id, string in source code) */\n");
      foreach_keywords(kws, gen_lists_kw_h, outfh);
      fprintf(outfh, "# undef KW\n#else\n# error \"lists/kw.h included, but KW macro is not defined\"\n#endif\n");
      break;

    case 'D':
      fprintf(outfh, "defs.h] from '%s' **/\n#pragma once\n", argv[2]);
      foreach_keywords(kws, gen_kw_defs_h, outfh);
      break;

    case 'S':
      fprintf(outfh, "stmap.h] from '%s' **/\n", argv[2]);
      foreach_keywords(kws, gen_kw_stmap_h, outfh);
      break;

    case 'n':
      fprintf(outfh, "names.h] from '%s' **/\n", argv[2]);
      foreach_keywords(kws, gen_kw_names_h, outfh);
      break;

    default:
      /* unreachable */
      ret = 1;
      fprintf(stderr, "zskwproc: unknwon target '%c'\n", argv[1][0]);
  }

  /* cleanup */
  fclose(infh);
  if(outfh != stdout)
    fclose(outfh);
  delete_keywords(kws);
  return ret;
}
