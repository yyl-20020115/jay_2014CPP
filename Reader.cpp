/*
* Copyright (c) 1989 The Regents of the University of California.
* All rights reserved.
*
* This code is derived from software contributed to Berkeley by
* Robert Paul Corbett.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. All advertising materials mentioning features or use of this software
*    must display the following acknowledgement:
*	This product includes software developed by the University of
*	California, Berkeley and its contributors.
* 4. Neither the name of the University nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/

#ifndef lint
static char sccsid[] = "@(#)reader.c	5.7 (Berkeley) 1/20/91";
#endif /* not lint */

#include "defs.h"

/*  The line size must be a positive integer.  One hundred was chosen	*/
/*  because few lines in Yacc input grammars exceed 100 characters.	*/
/*  Note that if a line exceeds LINESIZE characters, the line buffer	*/
/*  will be expanded to accomodate it.					*/

Reader::Reader(GlobalInfo* info,ErrorReporter* erp, SymbolTable* symbolTable)
  :cache (0)
  ,cinc  (0)
  ,cache_size (0)
  ,ntags (0)
  ,tagmax (0)
  ,tag_table (0)
  ,saw_eof(0)
  ,linesize(0)
  ,goal(0)
  ,prec(0)
  ,gensym(0)
  ,last_was_action(0)
  ,maxitems(0)
  ,pitem(0)
  ,maxrules(0)
  ,name_pool_size(0)
  ,name_pool(0)
  ,info(info)
  ,erp(erp)
  ,symbolTable(symbolTable)
  ,cptr (0)
  ,ritem(0)
  ,rlhs(0)
  ,rrhs(0)
  ,rprec(0)
  ,rassoc(0)
  ,nitems(0)
  ,nrules(0)
  ,nsyms(0)
  ,ntokens(0)
  ,nvars(0)
  ,nmethods(0)
  ,line(0)
  ,lineno(0)
  ,start_symbol(0)
  ,symbol_name(0)
  ,symbol_value(0)
  ,symbol_prec(0)
  ,symbol_assoc(0)
  ,methods(0)
{

}
Reader::~Reader()
{
  if(this->line!=0)
  {
    free(this->line);
    this->line = 0;
  }
  if(this->cache!=0)
  {
    free(this->cache);
    this->cache = 0;
  }
  if(this->rrhs!=0)
  {
    free(this->rrhs);
    this->rrhs = 0;
  }
  if(this->rlhs!=0)
  {
    free(this->rlhs);
    this->rlhs = 0;
  }
  if(this->rassoc!=0)
  {
    free(this->rassoc);
    this->rassoc = 0;
  }
  if(this->rprec!=0)
  {
    free(this->rprec);
    this->rprec = 0;
  }
  if(this->ritem!=0)
  {
    free(this->ritem);
    this->ritem = 0;
  }
  if(this->name_pool!=0)
  {
    free(this->name_pool);
    this->name_pool = 0;
    this->name_pool_size = 0;
  }
  if(this->symbol_name!=0)
  {
    free(this->symbol_name);
    this->symbol_name = 0;
  }
  if(this->symbol_value!=0)
  {
    free(this->symbol_value);
    this->symbol_value = 0;
  }
  if(this->symbol_prec!=0)
  {
    free(this->symbol_prec);
    this->symbol_prec = 0;
  }
  if(this->symbol_assoc!=0)
  {
    free(this->symbol_assoc);
    this->symbol_assoc = 0;
  }
  if(this->methods!=0)
  {
    for(int i = 0;i<this->nmethods;i++)
    {
      free(this->methods[i]);
      this->methods[i]=0;
    }
    free(this->methods);
    this->methods = 0;
    this->nmethods = 0;
  }
}
void Reader::cachec(int c)
{
  assert(cinc >= 0);
  if (cinc >= cache_size)
  {
    cache_size += 256;
    cache =(char*) REALLOC(cache, cache_size);
    if (cache == 0) erp->no_space();
  }
  cache[cinc] = c;
  ++cinc;
}


void Reader::get_line()
{
  register FILE *f = info->input_file;
  register int c;
  register int i;

  if (saw_eof || (c = getc(f)) == EOF)
  {
    if (line) { FREE(line); line = 0; }
    cptr = 0;
    saw_eof = 1;
    return;
  }

  if (line == 0 || linesize != (LINESIZE + 1))
  {
    if (line) FREE(line);
    linesize = LINESIZE + 1;
    //FIXED
    line = (char*)MALLOC(linesize);
    if (line == 0) erp->no_space();
  }

  i = 0;
  ++this->lineno;
  for (;;)
  {
    line[i]  =  c;
    if (c == '\n') { cptr = line; return; }
    if (++i >= linesize)
    {
      linesize += LINESIZE;
      line = (char*)REALLOC(line, linesize);
      if (line ==  0) erp->no_space();
    }
    c = getc(f);
    if (c ==  EOF)
    {
      line[i] = '\n';
      saw_eof = 1;
      cptr = line;
      return;
    }
  }
}

//use cpp string instead!

std::string Reader::dup_line()
{
  std::string ret;

  register char *p, *s, *t;

  if (this->line == 0) return ret;
  
  s = this->line;
  while (*s != '\n') ++s;

  p = (char*)MALLOC(s - line + 1);
  if (p == 0) erp->no_space();

  s = line;
  t = p;
  while ((*t++ = *s++) != '\n') continue;
  
  ret = p;

  FREE(p);

  return ret;
}


void Reader::skip_comment()
{
  register char *s;

  int st_lineno = this->lineno;
  //no leak
  std::string st_line = dup_line();

  const char *st_cptr = st_line.c_str() + (cptr - line);

  s = cptr + 2;
  for (;;)
  {
    if (*s == '*' && s[1] == '/')
    {
      cptr = s + 2;
      //FREE(st_line);
      return;
    }
    if (*s == '\n')
    {
      get_line();
      if (line == 0)
        erp->unterminated_comment(st_lineno, (char*)st_line.c_str(), (char*)st_cptr);
      s = cptr;
    }
    else
      ++s;
  }
}


int Reader::nextc()
{
  register char *s;

  if (line == 0)
  {
    get_line();
    if (line == 0)
      return (EOF);
  }

  s = cptr;
  for (;;)
  {
    switch (*s)
    {
    case '\n':
      get_line();
      if (line == 0) return (EOF);
      s = cptr;
      break;

    case ' ':
    case '\t':
    case '\f':
    case '\r':
    case '\v':
    case ',':
    case ';':
      ++s;
      break;

    case '\\':
      cptr = s;
      return ('%');

    case '/':
      if (s[1] == '*')
      {
        cptr = s;
        skip_comment();
        s = cptr;
        break;
      }
      else if (s[1] == '/')
      {
        get_line();
        if (line == 0) return (EOF);
        s = cptr;
        break;
      }
      /* fall through */

    default:
      cptr = s;
      return (*s);
    }
  }
}


int Reader::keyword()
{
  register int c;
  char *t_cptr = cptr;

  c = *++cptr;
  if (isalpha(c))
  {
    cinc = 0;
    for (;;)
    {
      if (isalpha(c))
      {
        if (isupper(c)) c = tolower(c);
        cachec(c);
      }
      else if (isdigit(c) || c == '_' || c == '.' || c == '$')
        cachec(c);
      else
        break;
      c = *++cptr;
    }
    cachec(NUL);

    if (strcmp(cache, "token") == 0 || strcmp(cache, "term") == 0)
      return (TOKEN);
    if (strcmp(cache, "type") == 0)
      return (TYPE);
    if (strcmp(cache, "left") == 0)
      return (LEFT);
    if (strcmp(cache, "right") == 0)
      return (RIGHT);
    if (strcmp(cache, "nonassoc") == 0 || strcmp(cache, "binary") == 0)
      return (NONASSOC);
    if (strcmp(cache, "start") == 0)
      return (START);
  }
  else
  {
    ++cptr;
    if (c == '{')
      return (TEXT);
    if (c == '%' || c == '\\')
      return (MARK);
    if (c == '<')
      return (LEFT);
    if (c == '>')
      return (RIGHT);
    if (c == '0')
      return (TOKEN);
    if (c == '2')
      return (NONASSOC);
  }
  erp->syntax_error(this->lineno, line, t_cptr);
  /*NOTREACHED*/
  return 0;
}


void Reader::copy_text(FILE *f)
{
  register int c;
  int quote;
  int need_newline = 0;
  int t_lineno = this->lineno;
  std::string t_line = dup_line();
  char *t_cptr =(char*) t_line.c_str() + (cptr - line - 2);

  if (*cptr == '\n')
  {
    get_line();
    if (line == 0)
      erp->unterminated_text(t_lineno, (char*)t_line.c_str(), (char*)t_cptr);
  }
  fprintf(f, info->line_format, this->lineno, info->input_file_name);

loop:
  c = *cptr++;
  switch (c)
  {
  case '\n':
next_line:
    putc('\n', f);
    need_newline = 0;
    get_line();
    if (line) goto loop;
    erp->unterminated_text(t_lineno, (char*)t_line.c_str(), t_cptr);

  case '\'':
  case '"':
    {
      int s_lineno = this->lineno;
      std::string s_line = dup_line();
      char *s_cptr = (char*)s_line.c_str() + (cptr - line - 1);

      quote = c;
      putc(c, f);
      for (;;)
      {
        c = *cptr++;
        putc(c, f);
        if (c == quote)
        {
          need_newline = 1;
          //FREE(s_line);
          goto loop;
        }
        if (c == '\n')
          erp->unterminated_string(s_lineno,(char*) s_line.c_str(), s_cptr);
        if (c == '\\')
        {
          c = *cptr++;
          putc(c, f);
          if (c == '\n')
          {
            get_line();
            if (line == 0)
              erp->unterminated_string(s_lineno,(char*) s_line.c_str(), s_cptr);
          }
        }
      }
    }

  case '/':
    putc(c, f);
    need_newline = 1;
    c = *cptr;
    if (c == '/')
    {
      do putc(c, f); while ((c = *++cptr) != '\n');
      goto next_line;
    }
    if (c == '*')
    {
      int c_lineno = this->lineno;
      std::string c_line = dup_line();
      char *c_cptr = (char*)c_line.c_str() + (cptr - line - 1);

      putc('*', f);
      ++cptr;
      for (;;)
      {
        c = *cptr++;
        putc(c, f);
        if (c == '*' && *cptr == '/')
        {
          putc('/', f);
          ++cptr;
          //FREE(c_line);
          goto loop;
        }
        if (c == '\n')
        {
          get_line();
          if (line == 0)
            erp->unterminated_comment(c_lineno,(char*) c_line.c_str(), c_cptr);
        }
      }
    }
    need_newline = 1;
    goto loop;

  case '%':
  case '\\':
    if (*cptr == '}')
    {
      if (need_newline) putc('\n', f);
      ++cptr;
      //FREE(t_line);
      return;
    }
    /* fall through */

  default:
    putc(c, f);
    need_newline = 1;
    goto loop;
  }
}

int Reader::hexval(int c)
{
  if (c >= '0' && c <= '9')
    return (c - '0');
  if (c >= 'A' && c <= 'F')
    return (c - 'A' + 10);
  if (c >= 'a' && c <= 'f')
    return (c - 'a' + 10);
  return (-1);
}


bucket *Reader::get_literal()
{
  register int c, quote;
  register int i;
  register int n;
  register char *s;
  register bucket *bp;
  int s_lineno = this->lineno;
  std::string s_line = dup_line();
  char *s_cptr = (char*)s_line.c_str() + (cptr - line);

  quote = *cptr++;
  cinc = 0;
  for (;;)
  {
    c = *cptr++;
    if (c == quote) break;
    if (c == '\n') erp->unterminated_string(s_lineno, (char*)s_line.c_str(), s_cptr);
    if (c == '\\')
    {
      char *c_cptr = cptr - 1;

      c = *cptr++;
      switch (c)
      {
      case '\n':
        get_line();
        if (line == 0) erp->unterminated_string(s_lineno, (char*)s_line.c_str(), s_cptr);
        continue;

      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
        n = c - '0';
        c = *cptr;
        if (IS_OCTAL(c))
        {
          n = (n << 3) + (c - '0');
          c = *++cptr;
          if (IS_OCTAL(c))
          {
            n = (n << 3) + (c - '0');
            ++cptr;
          }
        }
        if (n > MAXCHAR) erp->illegal_character(c_cptr);
        c = n;
        break;

      case 'x':
        c = *cptr++;
        n = hexval(c);
        if (n < 0 || n >= 16)
          erp->illegal_character(c_cptr);
        for (;;)
        {
          c = *cptr;
          i = hexval(c);
          if (i < 0 || i >= 16) break;
          ++cptr;
          n = (n << 4) + i;
          if (n > MAXCHAR) erp->illegal_character(c_cptr);
        }
        c = n;
        break;

      case 'a': c = 7; break;
      case 'b': c = '\b'; break;
      case 'f': c = '\f'; break;
      case 'n': c = '\n'; break;
      case 'r': c = '\r'; break;
      case 't': c = '\t'; break;
      case 'v': c = '\v'; break;
      }
    }
    cachec(c);
  }
  //FREE(s_line);

  n = cinc;
  s = (char*)MALLOC(n);
  if (s == 0) erp->no_space();

  for (i = 0; i < n; ++i)
    s[i] = cache[i];

  cinc = 0;
  if (n == 1)
    cachec('\'');
  else
    cachec('"');

  for (i = 0; i < n; ++i)
  {
    c = ((unsigned char *)s)[i];
    if (c == '\\' || c == cache[0])
    {
      cachec('\\');
      cachec(c);
    }
    else if (isprint(c))
      cachec(c);
    else
    {
      cachec('\\');
      switch (c)
      {
      case 7: cachec('a'); break;
      case '\b': cachec('b'); break;
      case '\f': cachec('f'); break;
      case '\n': cachec('n'); break;
      case '\r': cachec('r'); break;
      case '\t': cachec('t'); break;
      case '\v': cachec('v'); break;
      default:
        cachec(((c >> 6) & 7) + '0');
        cachec(((c >> 3) & 7) + '0');
        cachec((c & 7) + '0');
        break;
      }
    }
  }

  if (n == 1)
    cachec('\'');
  else
    cachec('"');

  cachec(NUL);
  bp = symbolTable->lookup(cache);
  bp->cls = TERM;
  if (n == 1 && bp->value == UNDEFINED)
    bp->value = *(unsigned char *)s;
  FREE(s);

  return (bp);
}


int Reader::is_reserved(char* name)
{
  char *s;

  if (strcmp(name, ".") == 0 ||
    strcmp(name, "$accept") == 0 ||
    strcmp(name, "$end") == 0)
    return (1);

  if (name[0] == '$' && name[1] == '$' && isdigit(name[2]))
  {
    s = name + 3;
    while (isdigit(*s)) ++s;
    if (*s == NUL) return (1);
  }

  return (0);
}


bucket * Reader::get_name()
{
  register int c;

  cinc = 0;
  for (c = *cptr; IS_IDENT(c); c = *++cptr)
    cachec(c);
  cachec(NUL);

  if (is_reserved(cache)) erp->used_reserved(cache);

  return (symbolTable->lookup(cache));
}


int Reader::get_number()
{
  register int c;
  register int n;

  n = 0;
  for (c = *cptr; isdigit(c); c = *++cptr)
    n = 10*n + (c - '0');

  return (n);
}


char * Reader::get_tag(int emptyOk)
{
  register int c;
  register int i;
  register char *s;
  int t_lineno = this->lineno;
  std::string t_line = dup_line();
  char *t_cptr = (char*)t_line.c_str() + (cptr - line);

  ++cptr;
  c = nextc();
  if (c == EOF) erp->unexpected_EOF();
  if (emptyOk && c == '>') {
    ++cptr; return 0;	// 0 indicates empty tag if emptyOk
  }
  if (!isalpha(c) && c != '_' && c != '$')
    erp->illegal_tag(t_lineno, (char*)t_line.c_str(), t_cptr);

  cinc = 0;
  do { cachec(c); c = *++cptr; } while (IS_IDENT(c));
  cachec(NUL);

  c = nextc();
  if (c == EOF) erp->unexpected_EOF();
  if (c != '>')
    erp->illegal_tag(t_lineno, (char*)t_line.c_str(), t_cptr);
  ++cptr;

  for (i = 0; i < ntags; ++i)
  {
    if (strcmp(cache, tag_table[i]) == 0)
      return (tag_table[i]);
  }

  if (ntags >= tagmax)
  {
    tagmax += 16;
    tag_table = (char **)
      (tag_table ? REALLOC(tag_table, tagmax*sizeof(char *))
      : MALLOC(tagmax*sizeof(char *)));
    if (tag_table == 0) erp->no_space();
  }

  s = (char*)MALLOC(cinc);
  if  (s == 0) erp->no_space();
  strcpy(s, cache);
  tag_table[ntags] = s;
  ++ntags;
  //FREE(t_line);
  return (s);
}


void Reader::declare_tokens(int assoc)
{
  register int c;
  register bucket *bp;
  int value;
  char *tag = 0;

  if (assoc != TOKEN) ++prec;

  c = nextc();
  if (c == EOF) erp->unexpected_EOF();
  if (c == '<')
  {
    tag = get_tag(0);
    c = nextc();
    if (c == EOF) erp->unexpected_EOF();
  }

  for (;;)
  {
    if (isalpha(c) || c == '_' || c == '.' || c == '$')
      bp = get_name();
    else if (c == '\'' || c == '"')
      bp = get_literal();
    else
      return;

    if (bp == goal) erp->tokenized_start(bp->name);
    bp->cls = TERM;

    if (tag)
    {
      if (bp->tag && tag != bp->tag)
        erp->retyped_warning(bp->name);
      bp->tag = tag;
    }

    if (assoc != TOKEN)
    {
      if (bp->prec && prec != bp->prec)
        erp->reprec_warning(bp->name);
      bp->assoc = assoc;
      bp->prec = prec;
    }

    c = nextc();
    if (c == EOF) erp->unexpected_EOF();
    value = UNDEFINED;
    if (isdigit(c))
    {
      value = get_number();
      if (bp->value != UNDEFINED && value != bp->value)
        erp->revalued_warning(bp->name);
      bp->value = value;
      c = nextc();
      if (c == EOF) erp->unexpected_EOF();
    }
  }
}


void Reader::declare_types()
{
  register int c;
  register bucket *bp;
  char *tag;

  c = nextc();
  if (c == EOF) erp->unexpected_EOF();
  if (c != '<') erp->syntax_error(this->lineno, line, cptr);
  tag = get_tag(0);

  for (;;)
  {
    c = nextc();
    if (isalpha(c) || c == '_' || c == '.' || c == '$')
      bp = get_name();
    else if (c == '\'' || c == '"')
      bp = get_literal();
    else
      return;

    if (bp->tag && tag != bp->tag)
      erp->retyped_warning(bp->name);
    bp->tag = tag;
  }
}


void Reader::declare_start()
{
  register int c;
  register bucket *bp;

  c = nextc();
  if (c == EOF) erp->unexpected_EOF();
  if (!isalpha(c) && c != '_' && c != '.' && c != '$')
    erp->syntax_error(this->lineno, line, cptr);
  bp = get_name();
  if (bp->cls == TERM)
    erp->terminal_start(bp->name);
  if (goal && goal != bp)
    erp->restarted_warning();
  goal = bp;
}


void Reader::read_declarations()
{
  register int c, k;

  cache_size = 256;
  //FIXED:
  cache = (char*)MALLOC(cache_size);
  if (cache == 0) erp->no_space();

  for (;;)
  {
    c = nextc();
    if (c == EOF) erp->unexpected_EOF();
    if (c != '%') erp->syntax_error(this->lineno, line, cptr);
    switch (k = keyword())
    {
    case MARK:
      return;

    case TEXT:
      copy_text(info->prolog_file);
      break;

    case TOKEN:
    case LEFT:
    case RIGHT:
    case NONASSOC:
      declare_tokens(k);
      break;

    case TYPE:
      declare_types();
      break;

    case START:
      declare_start();
      break;
    }
  }
}


void Reader::initialize_grammar()
{
  this->nitems = 4;
  maxitems = 300;
  pitem = (bucket **) CALLOC(maxitems,sizeof(bucket *));
  if (pitem == 0) erp->no_space();
  pitem[0] = 0;
  pitem[1] = 0;
  pitem[2] = 0;
  pitem[3] = 0;

  this->nmethods = 0;
  this->nrules = 3;
  maxrules = 100;
  plhs = (bucket **) CALLOC(maxrules,sizeof(bucket *));
  if (plhs == 0) erp->no_space();
  plhs[0] = 0;
  plhs[1] = 0;
  plhs[2] = 0;
  this->rprec = (short *) CALLOC(maxrules,sizeof(short));
  if (this->rprec == 0) erp->no_space();
  this->rprec[0] = 0;
  this->rprec[1] = 0;
  this->rprec[2] = 0;
  this->rassoc = (char *) CALLOC(maxrules,sizeof(char));
  if (this->rassoc == 0) erp->no_space();
  this->rassoc[0] = TOKEN;
  this->rassoc[1] = TOKEN;
  this->rassoc[2] = TOKEN;
}


void Reader::expand_items()
{
  maxitems += 300;
  pitem = (bucket **) REALLOC(pitem, maxitems*sizeof(bucket *));
  if (pitem == 0) erp->no_space();
}


void Reader::expand_rules()
{
  maxrules += 100;
  plhs = (bucket **) REALLOC(plhs, maxrules*sizeof(bucket *));
  if (plhs == 0) erp->no_space();
  this->rprec = (short *) REALLOC(this->rprec, maxrules*sizeof(short));
  if (this->rprec == 0) erp->no_space();
  this->rassoc = (char *) REALLOC(this->rassoc, maxrules*sizeof(char));
  if (this->rassoc == 0) erp->no_space();
}


void Reader::advance_to_start()
{
  register int c;
  register bucket *bp;
  char *s_cptr;
  int s_lineno;

  for (;;)
  {
    c = nextc();
    if (c != '%') break;
    s_cptr = cptr;
    switch (keyword())
    {
    case MARK:
      erp->no_grammar();

    case TEXT:
      copy_text(info->local_file);
      break;

    case START:
      declare_start();
      break;

    default:
      erp->syntax_error(this->lineno, line, s_cptr);
    }
  }

  c = nextc();
  if (!isalpha(c) && c != '_' && c != '.' && c != '_')
    erp->syntax_error(this->lineno, line, cptr);
  bp = get_name();
  if (goal == 0)
  {
    if (bp->cls == TERM)
      erp->terminal_start(bp->name);
    goal = bp;
  }

  s_lineno = this->lineno;
  c = nextc();
  if (c == EOF) erp->unexpected_EOF();
  if (c != ':') erp->syntax_error(this->lineno, line, cptr);
  start_rule(bp, s_lineno);
  ++cptr;
}


void Reader::start_rule(register bucket *bp, int s_lineno)
{
  if (bp->cls == TERM)
    erp->terminal_lhs(s_lineno);
  bp->cls = NONTERM;
  if (this->nrules >= maxrules)
    expand_rules();
  plhs[this->nrules] = bp;
  this->rprec[this->nrules] = UNDEFINED;
  this->rassoc[this->nrules] = TOKEN;
}


void Reader::end_rule()
{
  register int i;

  if (!last_was_action && plhs[this->nrules]->tag)
  {
    for (i = this->nitems - 1; pitem[i]; --i) continue;
    if (pitem[i+1] == 0 || pitem[i+1]->tag != plhs[this->nrules]->tag)
      erp->default_action_warning();	/** if classes don't match exactly **/
  }					/** bug: could be superclass... **/

  last_was_action = 0;
  if (this->nitems >= maxitems) expand_items();
  pitem[this->nitems] = 0;
  ++this->nitems;
  ++this->nrules;
}


void Reader::insert_empty_rule()
{
  register bucket *bp, **bpp;

  assert(cache);
  sprintf(cache, "$$%d", ++gensym);
  bp = symbolTable->make_bucket(cache);

  symbolTable->append(bp);

  bp->tag = plhs[this->nrules]->tag;
  bp->cls = NONTERM;

  if ((this->nitems += 2) > maxitems)
    expand_items();
  bpp = pitem + this->nitems - 1;
  *bpp-- = bp;
  while (bpp[0] = bpp[-1]) --bpp;

  if (++this->nrules >= maxrules)
    expand_rules();
  plhs[this->nrules] = plhs[this->nrules-1];
  plhs[this->nrules-1] = bp;
  this->rprec[this->nrules] = this->rprec[this->nrules-1];
  this->rprec[this->nrules-1] = 0;
  this->rassoc[this->nrules] =this-> rassoc[this->nrules-1];
  this->rassoc[this->nrules-1] = TOKEN;
}


void Reader::add_symbol()
{
  register int c;
  register bucket *bp;
  int s_lineno = this->lineno;

  c = *cptr;
  if (c == '\'' || c == '"')
    bp = get_literal();
  else
    bp = get_name();

  c = nextc();
  if (c == ':')
  {
    end_rule();
    start_rule(bp, s_lineno);
    ++cptr;
    return;
  }

  if (last_was_action)
    insert_empty_rule();
  last_was_action = 0;

  if (++this->nitems > maxitems)
    expand_items();
  pitem[this->nitems-1] = bp;
}


void Reader::copy_action()
{
  register int c;
  register int i, n;
  int depth;
  int quote;
  char *tag;
  FILE *f =info-> action_file;
  int a_lineno = this->lineno;
  std::string a_line = dup_line();
  char *a_cptr = (char*)a_line.c_str() + (cptr - line);
  char buffer [10000];
  int len = 0;
  int comment_lines = 0;
  char *mbody;
  memset (buffer, 0, 10000);

  if (last_was_action)
    insert_empty_rule();
  last_was_action = 1;

  fprintf(f, "case %d:\n",this-> nrules - 2);
  if (*cptr == '=') ++cptr;

  n = 0;

  for (i = this->nitems - 1; pitem[i]; --i) ++n;

  depth = 0;
loop:
  c = *cptr;
  if (c == '$')
  {
    if (cptr[1] == '<')
    {
      int d_lineno = this->lineno;
      std::string d_line = dup_line();
      char *d_cptr = (char*)d_line.c_str() + (cptr - line);

      ++cptr;
      tag = get_tag(1);
      c = *cptr;
      if (c == '$')
      {   
        if (tag && strcmp(tag, "Object")) {
          len += sprintf(buffer + len, "((%s)yyVal)", tag);
        } else {
          strcat (buffer + len, "yyVal");
          len += 5;
        }
        ++cptr;
        //FREE(d_line);
        goto loop;
      }
      else if (isdigit(c))
      {
        i = get_number();
        if (i > n) erp->dollar_warning(d_lineno, i);
        if (tag && strcmp(tag, "Object"))
          len += sprintf(buffer + len, "((%s)yyVals[%d+yyTop])", tag, i - n);
        else
          len += sprintf(buffer + len, "yyVals[%d+yyTop]", i - n);
        //FREE(d_line);
        goto loop;
      }
      else if (c == '-' && isdigit(cptr[1]))
      {
        ++cptr;
        i = -get_number() - n;
        if (tag && strcmp(tag, "Object"))
          len += sprintf(buffer + len, "((%s)yyVals[%d+yyTop])", tag, i);
        else
          len += sprintf(buffer + len, "yyVals[%d+yyTop]", i);
        //FREE(d_line);
        goto loop;
      }
      else
        erp->dollar_error(d_lineno, (char*)d_line.c_str(), d_cptr);
    }
    else if (cptr[1] == '$')
    {
      if (ntags && plhs[this->nrules]->tag == 0)
        erp->untyped_lhs();
      strcat (buffer, "yyVal");
      len += 5;
      cptr += 2;
      goto loop;
    }
    else if (isdigit(cptr[1]))
    {
      ++cptr;
      i = get_number();
      if (ntags)
      {
        if (i <= 0 || i > n)
          erp->unknown_rhs(i);
        tag = pitem[this->nitems + i - n - 1]->tag;
        if (tag == 0)
          erp->untyped_rhs(i, pitem[this->nitems + i - n - 1]->name),
          len += sprintf(buffer + len, "yyVals[%d+yyTop]", i - n);
        else if (strcmp(tag, "Object"))
          len += sprintf(buffer + len, "((%s)yyVals[%d+yyTop])", tag, i - n);
        else
          len += sprintf(buffer + len, "yyVals[%d+yyTop]", i - n);
      }
      else
      {
        if (i > n)
          erp->dollar_warning(this->lineno, i);

        len += sprintf(buffer + len,"yyVals[%d+yyTop]", i - n);
      }
      goto loop;
    }
    else if (cptr[1] == '-')
    {
      cptr += 2;
      i = get_number();
      if (ntags)
        erp->unknown_rhs(-i);
      len += sprintf(buffer + len, "yyVals[%d+yyTop]", -i - n);
      goto loop;
    }
  }
  if (isalpha(c) || c == '_' || c == '$')
  {
    do
    {
      buffer[len++] = c;
      c = *++cptr;
    } while (isalnum(c) || c == '_' || c == '$');
    goto loop;
  }
  buffer[len++] = c;
  ++cptr;
  switch (c)
  {
  case '\n':
next_line:
    get_line();
    if (line) goto loop;
    erp->unterminated_action(a_lineno,(char*) a_line.c_str(), a_cptr);

  case ';':
    if (depth > 0) goto loop;
    break;

  case '{':
    ++depth;
    goto loop;

  case '}':
    if (--depth > 0) goto loop;
    break;

  case '\'':
  case '"':
    {
      int s_lineno = this->lineno;
      std::string s_line = dup_line();
      char *s_cptr = (char*)s_line.c_str() + (cptr - line - 1);

      quote = c;
      for (;;)
      {
        c = *cptr++;
        buffer[len++] = c;
        if (c == quote)
        {
          //FREE(s_line);
          goto loop;
        }
        if (c == '\n')
          erp->unterminated_string(s_lineno, (char*)s_line.c_str(), s_cptr);
        if (c == '\\')
        {
          c = *cptr++;
          buffer[len++] = c;
          if (c == '\n')
          {
            get_line();
            if (line == 0)
              erp->unterminated_string(s_lineno, (char*)s_line.c_str(), s_cptr);
          }
        }
      }
    }

  case '/':
    c = *cptr;
    if (c == '/')
    {
      buffer[len++] = '*';
      while ((c = *++cptr) != '\n')
      {
        if (c == '*' && cptr[1] == '/'){
          buffer[len++] = '*';
          buffer[len++] = ' ';
        } else {
          buffer[len++] = c;
        }
      }
      buffer[len++] = '*';
      buffer[len++] = '/';
      buffer[len++] = '\n';
      goto next_line;
    }
    if (c == '*')
    {
      int c_lineno = this->lineno;
      std::string c_line = dup_line();
      char *c_cptr = (char*)c_line.c_str() + (cptr - line - 1);

      buffer[len++] = '*';
      ++cptr;
      for (;;)
      {
        c = *cptr++;
        buffer[len++] = c;
        if (c == '*' && *cptr == '/')
        {
          buffer[len++] = '/';
          ++cptr;
          //FREE(c_line);
          goto loop;
        }
        if (c == '\n')
        {
          ++comment_lines;
          get_line();
          if (line == 0)
            erp->unterminated_comment(c_lineno,(char*) c_line.c_str(), c_cptr);
        }
      }
    }
    goto loop;

  default:
    goto loop;
  }

  if (comment_lines > 0)
    comment_lines++;

  if ((this->lineno - (a_lineno + comment_lines)) > 2)
  {
    char mname[20];
    char line_define[256];

    sprintf(mname, "case_%d()", this->nrules - 2);

    putc(' ', f); putc(' ', f);
    fputs(mname, f);
    fprintf(f, ";");
    if (this->nmethods == 0)
    {
      maxmethods = 100;
      this-> methods = (char**)CALLOC(maxmethods,sizeof( char *));
    }
    else if (this->nmethods == maxmethods)
    {
      maxmethods += 500;
      this->methods =(char**) REALLOC (this->methods, maxmethods*sizeof(char *));
    }

    sprintf(line_define, info->line_format, a_lineno, info->input_file_name);

    //FIXED:
    mbody = (char*)MALLOC((5+strlen(line_define)+1+strlen(mname)+strlen(buffer)+1)*sizeof(char));
    strcpy(mbody, "void ");
    strcat(mbody, mname);
    strcat(mbody, "\n");
    strcat(mbody, line_define);
    strcat(mbody, buffer);
    this->methods[this->nmethods++] = mbody;
  }
  else
  {
    fprintf(f, info->line_format, this->lineno, info->input_file_name);
    putc(' ', f); putc(' ', f);
    fwrite(buffer, 1, len, f);
  }

  fprintf(f, "\n  break;\n");
}


int Reader::mark_symbol()
{
  register int c;
  register bucket *bp;

  c = cptr[1];
  if (c == '%' || c == '\\')
  {
    cptr += 2;
    return (1);
  }

  if (c == '=')
    cptr += 2;
  else if ((c == 'p' || c == 'P') &&
    ((c = cptr[2]) == 'r' || c == 'R') &&
    ((c = cptr[3]) == 'e' || c == 'E') &&
    ((c = cptr[4]) == 'c' || c == 'C') &&
    ((c = cptr[5], !IS_IDENT(c))))
    cptr += 5;
  else
    erp->syntax_error(this->lineno, line, cptr);

  c = nextc();
  if (isalpha(c) || c == '_' || c == '.' || c == '$')
    bp = get_name();
  else if (c == '\'' || c == '"')
    bp = get_literal();
  else
  {
    erp->syntax_error(this->lineno, line, cptr);
    /*NOTREACHED*/
  }

  if (this->rprec[this->nrules] != UNDEFINED && bp->prec != this->rprec[this->nrules])
    erp->prec_redeclared();

  this->rprec[this->nrules] = bp->prec;
  this->rassoc[this->nrules] = bp->assoc;
  return (0);
}


void Reader::read_grammar()
{
  register int c;

  initialize_grammar();
  advance_to_start();

  for (;;)
  {
    c = nextc();
    if (c == EOF) break;
    if (isalpha(c) || c == '_' || c == '.' || c == '$' || c == '\'' ||
      c == '"')
      add_symbol();
    else if (c == '{' || c == '=')
      copy_action();
    else if (c == '|')
    {
      end_rule();
      start_rule(plhs[this->nrules-1], 0);
      ++cptr;
    }
    else if (c == '%')
    {
      if (mark_symbol()) break;
    }
    else
      erp->syntax_error(this->lineno, line, cptr);
  }
  end_rule();
}


void Reader::free_tags()
{
  register int i;

  if (tag_table == 0) return;

  for (i = 0; i < ntags; ++i)
  {
    assert(tag_table[i]);
    FREE(tag_table[i]);
  }
  FREE(tag_table);
}


void Reader::pack_names()
{
  register bucket *bp;
  register char *p, *s, *t;

  name_pool_size = 13;  /* 13 == sizeof("$end") + sizeof("$accept") */
  for (bp = symbolTable->GetFirstSymbol(); bp; bp = bp->next)
    name_pool_size += strlen(bp->name) + 1;
  //FIXED
  name_pool = (char*)MALLOC(name_pool_size);
  if (name_pool == 0) erp->no_space();

  strcpy(name_pool, "$accept");
  strcpy(name_pool+8, "$end");
  t = name_pool + 13;
  for (bp = symbolTable->GetFirstSymbol(); bp; bp = bp->next)
  {
    p = t;
    s = bp->name;
    while (*t++ = *s++) continue;
    FREE(bp->name);
    bp->name = p;
  }
}


void Reader::check_symbols()
{
  register bucket *bp;

  if (goal->cls == UNKNOWN)
    erp->undefined_goal(goal->name);

  for (bp = symbolTable->GetFirstSymbol(); bp; bp = bp->next)
  {
    if (bp->cls == UNKNOWN)
    {
      erp->undefined_symbol_warning(bp->name);
      bp->cls = TERM;
    }
  }
}


void Reader::pack_symbols()
{
  register bucket *bp;
  register bucket **v;
  register int i, j, k, n;

  this->nsyms = 2;
  this->ntokens = 1;
  for (bp = symbolTable->GetFirstSymbol(); bp; bp = bp->next)
  {
    ++this->nsyms;
    if (bp->cls == TERM) ++this->ntokens;
  }
  this->start_symbol = this->ntokens;
  this->nvars =this-> nsyms - this->ntokens;

  //FIXED:
  this->symbol_name = (char **) CALLOC(this->nsyms,sizeof(char *));
  if (this->symbol_name == 0) erp->no_space();
  //FIXED:
  this->symbol_value = (short *) CALLOC(this->nsyms,sizeof(short));
  if (this->symbol_value == 0) erp->no_space();
  //FIXED:
  this->symbol_prec = (short *) CALLOC(this->nsyms,sizeof(short));
  if (this->symbol_prec == 0) erp->no_space();
  //FIXED:
  this->symbol_assoc = (char*)MALLOC(this->nsyms);
  if (this->symbol_assoc == 0) erp->no_space();

  v = (bucket **) CALLOC(this->nsyms,sizeof(bucket *));
  if (v == 0) erp->no_space();

  v[0] = 0;
  v[this->start_symbol] = 0;

  i = 1;
  j = this->start_symbol + 1;
  for (bp = symbolTable->GetFirstSymbol(); bp; bp = bp->next)
  {
    if (bp->cls == TERM)
      v[i++] = bp;
    else
      v[j++] = bp;
  }
  assert(i == this->ntokens && j == this->nsyms);

  for (i = 1; i < this->ntokens; ++i)
    v[i]->index = i;

  goal->index = this->start_symbol + 1;
  k = this->start_symbol + 2;
  while (++i < this->nsyms)
    if (v[i] != goal)
    {
      v[i]->index = k;
      ++k;
    }

    goal->value = 0;
    k = 1;
    for (i = this->start_symbol + 1; i < this->nsyms; ++i)
    {
      if (v[i] != goal)
      {
        v[i]->value = k;
        ++k;
      }
    }

    k = 0;
    for (i = 1; i < this->ntokens; ++i)
    {
      n = v[i]->value;
      if (n > 256)
      {
        for (j = k++; j > 0 && this->symbol_value[j-1] > n; --j)
          this->symbol_value[j] = this->symbol_value[j-1];
        this->symbol_value[j] = n;
      }
    }

    if (v[1]->value == UNDEFINED)
      v[1]->value = 256;

    j = 0;
    n = 257;
    for (i = 2; i < this->ntokens; ++i)
    {
      if (v[i]->value == UNDEFINED)
      {
        while (j < k && n == this->symbol_value[j])
        {
          while (++j < k && n == this->symbol_value[j]) continue;
          ++n;
        }
        v[i]->value = n;
        ++n;
      }
    }

    this-> symbol_name[0] = name_pool + 8;
    this->symbol_value[0] = 0;
    this->symbol_prec[0] = 0;
    this->symbol_assoc[0] = TOKEN;
    for (i = 1; i <this-> ntokens; ++i)
    {
      this->symbol_name[i] = v[i]->name;
      this->symbol_value[i] = v[i]->value;
      this->symbol_prec[i] = v[i]->prec;
      this->symbol_assoc[i] = v[i]->assoc;
    }
    this->symbol_name[this->start_symbol] = name_pool;
    this->symbol_value[this->start_symbol] = -1;
    this->symbol_prec[this->start_symbol] = 0;
    this->symbol_assoc[this->start_symbol] = TOKEN;
    for (++i; i < this->nsyms; ++i)
    {
      k = v[i]->index;
      this->symbol_name[k] = v[i]->name;
      this->symbol_value[k] = v[i]->value;
      this->symbol_prec[k] = v[i]->prec;
      this->symbol_assoc[k] = v[i]->assoc;
    }

    FREE(v);
}


void Reader::pack_grammar()
{
  register int i, j;
  int assoc, prec;

  //FIXED:
  this->ritem = (short *) CALLOC(this->nitems,sizeof(short));
  if (this->ritem == 0) erp->no_space();
  //FIXED:
  this->rlhs = (short *) CALLOC(this->nrules,sizeof(short));
  if (this->rlhs == 0) erp->no_space();
  //FIXED:
  this->rrhs = (short *) CALLOC((this->nrules+1),sizeof(short));
  if (this->rrhs == 0) erp->no_space();
  this->rprec = (short *) REALLOC(this->rprec,this-> nrules*sizeof(short));
  if (this->rprec == 0) erp->no_space();
  this->rassoc = (char*)REALLOC(this->rassoc, this->nrules);
  if (this->rassoc == 0) erp->no_space();

  this->ritem[0] = -1;
  this->ritem[1] = goal->index;
  this->ritem[2] = 0;
  this->ritem[3] = -2;
  this->rlhs[0] = 0;
  this->rlhs[1] = 0;
  this->rlhs[2] =this-> start_symbol;
  this->rrhs[0] = 0;
  this->rrhs[1] = 0;
  this->rrhs[2] = 1;

  j = 4;
  for (i = 3; i <this-> nrules; ++i)
  {
    this->rlhs[i] = plhs[i]->index;
    this->rrhs[i] = j;
    assoc = TOKEN;
    prec = 0;
    while (pitem[j])
    {
      this->ritem[j] = pitem[j]->index;
      if (pitem[j]->cls == TERM)
      {
        prec = pitem[j]->prec;
        assoc = pitem[j]->assoc;
      }
      ++j;
    }
    this->ritem[j] = -i;
    ++j;
    if (this->rprec[i] == UNDEFINED)
    {
      this->rprec[i] = prec;
      this->rassoc[i] = assoc;
    }
  }
  this->rrhs[i] = j;

  FREE(plhs);
  FREE(pitem);
}


void Reader::print_grammar()
{
  register int i, j, k;
  int spacing;
  register FILE *f = info->verbose_file;

  if (!info->vflag) return;

  k = 1;
  for (i = 2; i < this->nrules; ++i)
  {
    if (this->rlhs[i] !=this-> rlhs[i-1])
    {
      if (i != 2) fprintf(f, "\n");
      fprintf(f, "%4d  %s :", i - 2, this->symbol_name[this->rlhs[i]]);
      spacing = strlen(this->symbol_name[this->rlhs[i]]) + 1;
    }
    else
    {
      fprintf(f, "%4d  ", i - 2);
      j = spacing;
      while (--j >= 0) putc(' ', f);
      putc('|', f);
    }

    while (this->ritem[k] >= 0)
    {
      fprintf(f, " %s", this->symbol_name[this->ritem[k]]);
      ++k;
    }
    ++k;
    putc('\n', f);
  }
}


void Reader::Read()
{
  read_declarations();
  read_grammar();
  free_tags();
  pack_names();
  check_symbols();
  pack_symbols();
  pack_grammar();
  print_grammar();
}
