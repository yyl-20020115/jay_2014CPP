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
static char sccsid[] = "@(#)output.c	5.7 (Berkeley) 5/24/93";
#endif /* not lint */

#include "defs.h"
#include <string.h>

Printer::Printer(GlobalInfo* info,Reader* reader, LALR* lalr, ParserGenerator* pg,ErrorReporter* erp)
  :info(info)
  ,reader(reader)
  ,lalr(lalr)
  ,pg(pg)
  ,nvectors(0)
  ,nentries(0)
  ,froms (0)
  ,tos (0)
  ,tally(0)
  ,width(0)
  ,state_count(0)
  ,order (0)
  ,base(0)
  ,pos (0)
  ,maxtable (0)
  ,table (0)
  ,check (0)
  ,lowzero (0)
  ,high(0)
  ,erp(erp)
{

}
Printer::~Printer()
{
  //if(this->froms!=0)
  //{
  //  free(this->froms);
  //  this->froms=0;
  //}
}
void Printer::Print () {
  int lno = 0;
  char buf [128];
  
  bool tflag = info->tflag;

  while (fgets(buf, sizeof buf, stdin) != NULL) {
    char * cp;
    ++ lno;
    if (buf[strlen(buf)-1] != '\n')
      fprintf(stderr, "jay: line %d is too long\n", lno), info->done(1);
    switch (buf[0]) {
    case '#':	continue;
      case 't':	if (!tflag) fputs("\t", stdout);
    case '.':	break;
    default:
      cp = strtok(buf, " \t\r\n");
      if (cp)
        if (strcmp(cp, "actions") == 0) output_semantic_actions();
        else if (strcmp(cp, "debug") == 0) output_debug(tflag);
        else if (strcmp(cp, "epilog") == 0) output_trailing_text();
        else if (strcmp(cp, "prolog") == 0)
          output_stored_text(info->prolog_file,info-> prolog_file_name);
        else if (strcmp(cp, "local") == 0)
          output_stored_text(info->local_file, info->local_file_name);
        else if (strcmp(cp, "tables") == 0)
          output_rule_data(), output_yydefred(), output_actions();
        else if (strcmp(cp, "tokens") == 0)
          output_defines(strtok(NULL, "\r\n"));
        else
          fprintf(stderr, "jay: unknown call (%s) in line %d\n", cp, lno);
        continue;
    }
    fputs(buf+1, stdout), ++ this->outline;
  }
}

void Printer::output_rule_data()
{
  register int i;
  register int j;

  printf("/*\n All more than 3 lines long rules are wrapped into a method\n*/\n");

  for (i = 0; i < reader->nmethods; ++i)
  {
    printf("%s", reader->methods[i]);
    //FREE(reader->methods[i]);
    printf("\n\n");
  }
  //FREE(reader->methods);

  printf(info->default_line_format, ++this->outline + 1);

  printf("  %s static %s short [] yyLhs  = {%16d,",
    info->csharp ? "" : " protected",
    info->csharp ? "readonly" : "final",
    reader->symbol_value[reader->start_symbol]);

  j = 10;
  for (i = 3; i < reader->nrules; i++)
  {
    if (j >= 10)
    {
      ++this->outline;
      putchar('\n');
      j = 1;
    }
    else
      ++j;

    printf("%5d,", reader->symbol_value[reader->rlhs[i]]);
  }
  this->outline += 2;
  printf("\n  };\n");

  printf("  %s static %s short [] yyLen = {%12d,",
    info->csharp ? "" : "protected",
    info->csharp ? "readonly" : "final",
    2);

  j = 10;
  for (i = 3; i < reader->nrules; i++)
  {
    if (j >= 10)
    {
      ++this->outline;
      putchar('\n');
      j = 1;
    }
    else
      j++;

    printf("%5d,", reader->rrhs[i + 1] - reader->rrhs[i] - 1);
  }
  this->outline += 2;
  printf("\n  };\n");
}


void Printer::output_yydefred()
{
  register int i, j;

  printf("  %s static %s short [] yyDefRed = {%13d,",
    info->csharp ? "" : "protected",
    info->csharp ? "readonly" : "final",	   
    (pg->GetDefred()[0] ? pg->GetDefred()[0] - 2 : 0));

  j = 10;
  for (i = 1; i < lalr->nstates; i++)
  {
    if (j < 10)
      ++j;
    else
    {
      ++this->outline;
      putchar('\n');
      j = 1;
    }

    printf("%5d,", (pg->GetDefred()[i] ? pg->GetDefred()[i] - 2 : 0));
  }

  this->outline += 2;
  printf("\n  };\n");
}


void Printer::output_actions()
{
  nvectors = 2*lalr->nstates + reader->nvars;

  froms = (short**)CALLOC(nvectors, sizeof(short *));
  tos = (short**)CALLOC(nvectors, sizeof(short *));
  tally = (short*)CALLOC(nvectors, sizeof(short ));
  width = (short*)CALLOC(nvectors, sizeof(short ));

  //ORIGINAL CODE:
  //BEGIN
  //token_actions();
  //FREE(lalr->lookaheads);
  //FREE(lalr->LA);
  //FREE(lalr->LAruleno);
  //FREE(lalr->accessing_symbol);
  //goto_actions();
  //FREE(lalr->goto_map + reader->ntokens);
  //FREE(lalr->from_state);
  //FREE(lalr->to_state);
  //END

  //CHANGED TOD:
  //BEGINE
  token_actions();
  goto_actions();
  //END

  sort_actions();
  pack_table();
  output_base();
  output_table();
  output_check();

  FREE(width);
  FREE(tally);
}


void Printer::token_actions()
{
  register int i, j;
  register int shiftcount, reducecount;
  register int max, min;
  register short *actionrow, *r, *s;
  register action *p;

  actionrow = (short*)CALLOC(2*reader->ntokens, sizeof(short));
  for (i = 0; i < lalr->nstates; ++i)
  {
    if (pg->GetParser()[i])
    {
      for (j = 0; j < 2*reader->ntokens; ++j)
        actionrow[j] = 0;

      shiftcount = 0;
      reducecount = 0;
      for (p = pg->GetParser()[i]; p; p = p->next)
      {
        if (p->suppressed == 0)
        {
          if (p->action_code == SHIFT)
          {
            ++shiftcount;
            actionrow[p->symbol] = p->number;
          }
          else if (p->action_code == REDUCE && p->number != pg->GetDefred()[i])
          {
            ++reducecount;
            actionrow[p->symbol + reader->ntokens] = p->number;
          }
        }
      }

      tally[i] = shiftcount;
      tally[lalr->nstates+i] = reducecount;
      width[i] = 0;
      width[lalr->nstates+i] = 0;
      if (shiftcount > 0)
      {
        froms[i] = r = (short*)CALLOC(shiftcount, sizeof(short));
        tos[i] = s = (short*)CALLOC(shiftcount, sizeof(short));
        min = MAXSHORT;
        max = 0;
        for (j = 0; j < reader->ntokens; ++j)
        {
          if (actionrow[j])
          {
            if (min > reader->symbol_value[j])
              min = reader->symbol_value[j];
            if (max < reader->symbol_value[j])
              max = reader->symbol_value[j];
            *r++ = reader->symbol_value[j];
            *s++ = actionrow[j];
          }
        }
        width[i] = max - min + 1;
      }
      if (reducecount > 0)
      {
        froms[lalr->nstates+i] = r = (short*)CALLOC(reducecount, sizeof(short));
        tos[lalr->nstates+i] = s = (short*)CALLOC(reducecount, sizeof(short));
        min = MAXSHORT;
        max = 0;
        for (j = 0; j < reader->ntokens; ++j)
        {
          if (actionrow[reader->ntokens+j])
          {
            if (min > reader->symbol_value[j])
              min = reader->symbol_value[j];
            if (max < reader->symbol_value[j])
              max = reader->symbol_value[j];
            *r++ = reader->symbol_value[j];
            *s++ = actionrow[reader->ntokens+j] - 2;
          }
        }
        width[lalr->nstates+i] = max - min + 1;
      }
    }
  }
  FREE(actionrow);
}

void Printer::goto_actions()
{
  register int i, j, k;

  state_count = (short*)CALLOC(lalr->nstates, sizeof(short));

  k = default_goto(reader->start_symbol + 1);
  printf("  protected static %s short [] yyDgoto  = {%14d,",info->csharp ? "readonly" : "final", k);
  save_column(reader->start_symbol + 1, k);

  j = 10;
  for (i = reader->start_symbol + 2; i < reader->nsyms; i++)
  {
    if (j >= 10)
    {
      ++this->outline;
      putchar('\n');
      j = 1;
    }
    else
      ++j;

    k = default_goto(i);
    printf("%5d,", k);
    save_column(i, k);
  }

  this->outline += 2;
  printf("\n  };\n");
  FREE(state_count);
}

int Printer::default_goto(int symbol)
{
  register int i;
  register int m;
  register int n;
  register int default_state;
  register int max;

  m = lalr->goto_map[symbol];
  n = lalr->goto_map[symbol + 1];

  if (m == n) return (0);

  for (i = 0; i < lalr->nstates; i++)
    state_count[i] = 0;

  for (i = m; i < n; i++)
    state_count[lalr->to_state[i]]++;

  max = 0;
  default_state = 0;
  for (i = 0; i < lalr->nstates; i++)
  {
    if (state_count[i] > max)
    {
      max = state_count[i];
      default_state = i;
    }
  }

  return (default_state);
}



void Printer::save_column(int symbol, int default_state)
{
  register int i;
  register int m;
  register int n;
  register short *sp;
  register short *sp1;
  register short *sp2;
  register int count;
  register int symno;

  m = lalr->goto_map[symbol];
  n = lalr->goto_map[symbol + 1];

  count = 0;
  for (i = m; i < n; i++)
  {
    if (lalr->to_state[i] != default_state)
      ++count;
  }
  if (count == 0) return;

  symno =reader-> symbol_value[symbol] + 2*lalr->nstates;

  froms[symno] = sp1 = sp = (short*)CALLOC(count, sizeof(short));
  tos[symno] = sp2 = (short*)CALLOC(count, sizeof(short));

  for (i = m; i < n; i++)
  {
    if (lalr->to_state[i] != default_state)
    {
      *sp1++ = lalr->from_state[i];
      *sp2++ = lalr->to_state[i];
    }
  }

  tally[symno] = count;
  width[symno] = sp1[-1] - sp[0] + 1;
}

void Printer::sort_actions()
{
  register int i;
  register int j;
  register int k;
  register int t;
  register int w;

  order = (short*)CALLOC(nvectors, sizeof(short));
  nentries = 0;

  for (i = 0; i < nvectors; i++)
  {
    if (tally[i] > 0)
    {
      t = tally[i];
      w = width[i];
      j = nentries - 1;

      while (j >= 0 && (width[order[j]] < w))
        j--;

      while (j >= 0 && (width[order[j]] == w) && (tally[order[j]] < t))
        j--;

      for (k = nentries - 1; k > j; k--)
        order[k + 1] = order[k];

      order[j + 1] = i;
      nentries++;
    }
  }
  FREE(order);

}


void Printer::pack_table()
{
  register int i;
  register int place;
  register int state;

  base = (short*)CALLOC(nvectors, sizeof(short));
  pos = (short*)CALLOC(nentries, sizeof(short));

  maxtable = 1000;
  table = (short*)CALLOC(maxtable, sizeof(short));
  check = (short*)CALLOC(maxtable, sizeof(short));

  lowzero = 0;
  high = 0;

  for (i = 0; i < maxtable; i++)
    check[i] = -1;

  for (i = 0; i < nentries; i++)
  {
    state = matching_vector(i);

    if (state < 0)
      place = pack_vector(i);
    else
      place = base[state];

    pos[i] = place;
    base[order[i]] = place;
  }

  for (i = 0; i < nvectors; i++)
  {
    if (froms[i])
      FREE(froms[i]);
    if (tos[i])
      FREE(tos[i]);
  }

  FREE(froms);
  FREE(tos);
  FREE(pos);
}


/*  The function matching_vector determines if the vector specified by	*/
/*  the input parameter matches a previously considered	vector.  The	*/
/*  test at the start of the function checks if the vector represents	*/
/*  a row of shifts over terminal symbols or a row of reductions, or a	*/
/*  column of shifts over a nonterminal symbol.  Berkeley Yacc does not	*/
/*  check if a column of shifts over a nonterminal symbols matches a	*/
/*  previously considered vector.  Because of the nature of LR parsing	*/
/*  tables, no two columns can match.  Therefore, the only possible	*/
/*  match would be between a row and a column.  Such matches are	*/
/*  unlikely.  Therefore, to save time, no attempt is made to see if a	*/
/*  column matches a previously considered vector.			*/
/*									*/
/*  Matching_vector is poorly designed.  The test could easily be made	*/
/*  faster.  Also, it depends on the vectors being in a specific	*/
/*  order.								*/

int Printer::matching_vector(int vector)
{
  register int i;
  register int j;
  register int k;
  register int t;
  register int w;
  register int match;
  register int prev;

  i = order[vector];
  if (i >= 2*lalr->nstates)
    return (-1);

  t = tally[i];
  w = width[i];

  for (prev = vector - 1; prev >= 0; prev--)
  {
    j = order[prev];
    if (width[j] != w || tally[j] != t)
      return (-1);

    match = 1;
    for (k = 0; match && k < t; k++)
    {
      if (tos[j][k] != tos[i][k] || froms[j][k] != froms[i][k])
        match = 0;
    }

    if (match)
      return (j);
  }

  return (-1);
}



int Printer::pack_vector(int vector)
{
  register int i, j, k, l;
  register int t;
  register int loc;
  register int ok;
  register short *from;
  register short *to;
  int newmax;

  i = order[vector];
  t = tally[i];
  assert(t);

  from = froms[i];
  to = tos[i];

  j = lowzero - from[0];
  for (k = 1; k < t; ++k)
    if (lowzero - from[k] > j)
      j = lowzero - from[k];
  for (;; ++j)
  {
    if (j == 0)
      continue;
    ok = 1;
    for (k = 0; ok && k < t; k++)
    {
      loc = j + from[k];
      if (loc >= maxtable)
      {
        if (loc >= MAXTABLE)
          erp->fatal("maximum table size exceeded");

        newmax = maxtable;
        do { newmax += 200; } while (newmax <= loc);
        table = (short *) REALLOC(table, newmax*sizeof(short));
        if (table == 0) erp->no_space();
        check = (short *) REALLOC(check, newmax*sizeof(short));
        if (check == 0) erp->no_space();
        for (l  = maxtable; l < newmax; ++l)
        {
          table[l] = 0;
          check[l] = -1;
        }
        maxtable = newmax;
      }

      if (check[loc] != -1)
        ok = 0;
    }
    for (k = 0; ok && k < vector; k++)
    {
      if (pos[k] == j)
        ok = 0;
    }
    if (ok)
    {
      for (k = 0; k < t; k++)
      {
        loc = j + from[k];
        table[loc] = to[k];
        check[loc] = from[k];
        if (loc > high) high = loc;
      }

      while (check[lowzero] != -1)
        ++lowzero;

      return (j);
    }
  }
}



void Printer::output_base()
{
  register int i, j;

  printf("  protected static %s short [] yySindex = {%13d,", info->csharp? "readonly":"final", base[0]);

  j = 10;
  for (i = 1; i < lalr->nstates; i++)
  {
    if (j >= 10)
    {
      ++this->outline;
      putchar('\n');
      j = 1;
    }
    else
      ++j;

    printf("%5d,", base[i]);
  }

  this->outline += 2;
  printf("\n  };\n  protected static %s short [] yyRindex = {%13d,",
    info->csharp ? "readonly" : "final",
    base[lalr->nstates]);

  j = 10;
  for (i =lalr-> nstates + 1; i < 2*lalr->nstates; i++)
  {
    if (j >= 10)
    {
      ++this->outline;
      putchar('\n');
      j = 1;
    }
    else
      ++j;

    printf("%5d,", base[i]);
  }

  this->outline += 2;
  printf("\n  };\n  protected static %s short [] yyGindex = {%13d,",
    info->csharp ? "readonly" : "final",
    base[2*lalr->nstates]);

  j = 10;
  for (i = 2*lalr->nstates + 1; i < nvectors - 1; i++)
  {
    if (j >= 10)
    {
      ++this->outline;
      putchar('\n');
      j = 1;
    }
    else
      ++j;

    printf("%5d,", base[i]);
  }

  this->outline += 2;
  printf("\n  };\n");
  FREE(base);
}



void Printer::output_table()
{
  register int i;
  register int j;

  printf("  protected static %s short [] yyTable = {%14d,", info->csharp ? "readonly" : "final", table[0]);

  j = 10;
  for (i = 1; i <= high; i++)
  {
    if (j >= 10)
    {
      ++this->outline;
      putchar('\n');
      j = 1;
    }
    else
      ++j;

    printf("%5d,", table[i]);
  }

  this->outline += 2;
  printf("\n  };\n");
  FREE(table);
}



void Printer::output_check()
{
  register int i;
  register int j;

  printf("  protected static %s short [] yyCheck = {%14d,",
    info->csharp ? "readonly" : "final",
    check[0]);

  j = 10;
  for (i = 1; i <= high; i++)
  {
    if (j >= 10)
    {
      ++this->outline;
      putchar('\n');
      j = 1;
    }
    else
      ++j;

    printf("%5d,", check[i]);
  }

  this->outline += 2;
  printf("\n  };\n");
  FREE(check);
}


int Printer::is_C_identifier(char* name)
{
  register char *s;
  register int c;

  s = name;
  c = *s;
  if (c == '"')
  {
    c = *++s;
    if (!isalpha(c) && c != '_' && c != '$')
      return (0);
    while ((c = *++s) != '"')
    {
      if (!isalnum(c) && c != '_' && c != '$')
        return (0);
    }
    return (1);
  }

  if (!isalpha(c) && c != '_' && c != '$')
    return (0);
  while (c = *++s)
  {
    if (!isalnum(c) && c != '_' && c != '$')
      return (0);
  }
  return (1);
}


void Printer::output_defines(char* prefix)
{
  register int c, i;
  register char *s;

  for (i = 2; i < reader->ntokens; ++i)
  {
    s = reader->symbol_name[i];
    if (is_C_identifier(s))
    {
      if (prefix)
        printf("  %s ", prefix);
      c = *s;
      if (c == '"')
      {
        while ((c = *++s) != '"')
        {
          putchar(c);
        }
      }
      else
      {
        do
        {
          putchar(c);
        }
        while (c = *++s);
      }
      ++this->outline;
      printf(" = %d%s\n", reader->symbol_value[i], info->csharp ? ";" : ";");
    }
  }

  ++this->outline;
  printf("  %s yyErrorCode = %d%s\n", prefix ? prefix : "", reader->symbol_value[1], info->csharp ? ";" : ";");
}


void Printer::output_stored_text(FILE* file,char* name)
{
  register int c;
  register FILE *in;

  fflush(file);
  in = fopen(name, "r");
  if (in == NULL)
    erp->open_error(name);
  if ((c = getc(in)) != EOF) {
    if (c ==  '\n')
      ++this->outline;
    putchar(c);
    while ((c = getc(in)) != EOF)
    {
      if (c == '\n')
        ++this->outline;
      putchar(c);
    }
    printf(info->default_line_format, ++this->outline + 1);
  }
  fclose(in);
}


void Printer::output_debug(bool tflag)
{
  register int i, j, k, max;
  char **symnam, *s;
  const char * prefix = tflag ? "" : "//t";

  ++this->outline;
  printf("  protected %s int yyFinal = %d;\n", info->csharp ? "const" : "static final",pg->GetFinalState());

  ++this->outline;
  printf ("%s // Put this array into a separate class so it is only initialized if debugging is actually used\n", prefix);
  printf ("%s // Use MarshalByRefObject to disable inlining\n", prefix);
  printf("%s class YYRules %s {\n", prefix, info->csharp ? ": MarshalByRefObject" : "");
  printf("%s  public static %s string [] yyRule = {\n", prefix, info->csharp ? "readonly" : "final");
  for (i = 2; i < reader->nrules; ++i)
  {
    printf("%s    \"%s :", prefix, reader->symbol_name[reader->rlhs[i]]);
    for (j = reader->rrhs[i]; reader->ritem[j] > 0; ++j)
    {
      s = reader->symbol_name[reader->ritem[j]];
      if (s[0] == '"')
      {
        printf(" \\\"");
        while (*++s != '"')
        {
          if (*s == '\\')
          {
            if (s[1] == '\\')
              printf("\\\\\\\\");
            else
              printf("\\\\%c", s[1]);
            ++s;
          }
          else
            putchar(*s);
        }
        printf("\\\"");
      }
      else if (s[0] == '\'')
      {
        if (s[1] == '"')
          printf(" '\\\"'");
        else if (s[1] == '\\')
        {
          if (s[2] == '\\')
            printf(" '\\\\\\\\");
          else
            printf(" '\\\\%c", s[2]);
          s += 2;
          while (*++s != '\'')
            putchar(*s);
          putchar('\'');
        }
        else
          printf(" '%c'", s[1]);
      }
      else
        printf(" %s", s);
    }
    ++this->outline;
    printf("\",\n");
  }
  ++ this->outline;
  printf("%s  };\n", prefix);
  printf ("%s public static string getRule (int index) {\n", prefix);
  printf ("%s    return yyRule [index];\n", prefix);
  printf ("%s }\n", prefix);
  printf ("%s}\n", prefix);

  max = 0;
  for (i = 2; i < reader->ntokens; ++i)
    if (reader->symbol_value[i] > max)
      max = reader->symbol_value[i];

  /* need yyNames for yyExpecting() */

  printf("  protected static %s string [] yyNames = {", info->csharp ? "readonly" : "final");
  symnam = (char **) CALLOC((max+1),sizeof(char *));
  if (symnam == 0) erp->no_space();

  /* Note that it is  not necessary to initialize the element	*/
  /* symnam[max].							*/
  for (i = 0; i < max; ++i)
    symnam[i] = 0;
  for (i = reader->ntokens - 1; i >= 2; --i)
    symnam[reader->symbol_value[i]] = reader->symbol_name[i];
  symnam[0] = "end-of-file";

  j = 70; fputs("    ", stdout);
  for (i = 0; i <= max; ++i)
  {
    //FIXED:
    if ((s = symnam[i])!=0)
    {
      if (s[0] == '"')
      {
        k = 7;
        while (*++s != '"')
        {
          ++k;
          if (*s == '\\')
          {
            k += 2;
            if (*++s == '\\')
              ++k;
          }
        }
        j += k;
        if (j > 70)
        {
          ++this->outline;
          printf("\n    ");
          j = k;
        }
        printf("\"\\\"");
        s = symnam[i];
        while (*++s != '"')
        {
          if (*s == '\\')
          {
            printf("\\\\");
            if (*++s == '\\')
              printf("\\\\");
            else
              putchar(*s);
          }
          else
            putchar(*s);
        }
        printf("\\\"\",");
      }
      else if (s[0] == '\'')
      {
        if (s[1] == '"')
        {
          j += 7;
          if (j > 70)
          {
            ++this->outline;
            printf("\n    ");
            j = 7;
          }
          printf("\"'\\\"'\",");
        }
        else
        {
          k = 5;
          while (*++s != '\'')
          {
            ++k;
            if (*s == '\\')
            {
              k += 2;
              if (*++s == '\\')
                ++k;
            }
          }
          j += k;
          if (j > 70)
          {
            ++this->outline;
            printf("\n    ");
            j = k;
          }
          printf("\"'");
          s = symnam[i];
          while (*++s != '\'')
          {
            if (*s == '\\')
            {
              printf("\\\\");
              if (*++s == '\\')
                printf("\\\\");
              else
                putchar(*s);
            }
            else
              putchar(*s);
          }
          printf("'\",");
        }
      }
      else
      {
        k = strlen(s) + 3;
        j += k;
        if (j > 70)
        {
          ++this->outline;
          printf("\n    ");
          j = k;
        }
        putchar('"');
        do { putchar(*s); } while (*++s);
        printf("\",");
      }
    }
    else
    {
      j += 5;
      if (j > 70)
      {
        ++this->outline;
        printf("\n    ");
        j = 5;
      }
      printf("null,");
    }
  }
  this->outline += 2;
  printf("\n  };\n");
  FREE(symnam);
}

void Printer::output_trailing_text()
{
  register int c, last;
  register FILE *in;

  if (reader->line == 0)
    return;

  in = info->input_file;
  c = *reader->cptr;
  if (c == '\n')
  {
    ++reader->lineno;
    if ((c = getc(in)) == EOF)
      return;
    ++this->outline;
    printf(info->line_format, reader->lineno, info->input_file_name);
    if (c == '\n')
      ++this->outline;
    putchar(c);
    last = c;
  }
  else
  {
    ++this->outline;
    printf(info->line_format, reader->lineno, info->input_file_name);
    do { putchar(c); } while ((c = *++reader->cptr) != '\n');
    ++this->outline;
    putchar('\n');
    last = '\n';
  }

  while ((c = getc(in)) != EOF)
  {
    if (c == '\n')
      ++this->outline;
    putchar(c);
    last = c;
  }

  if (last != '\n')
  {
    ++this->outline;
    putchar('\n');
  }
  printf(info->default_line_format, ++this->outline + 1);
}


void Printer::output_semantic_actions()
{
  register int c, last;

  fclose(info->action_file);
  info->action_file = fopen(info->action_file_name, "r");
  if (info->action_file == NULL)
    erp->open_error(info->action_file_name);

  if ((c = getc(info->action_file)) == EOF)
    return;

  last = c;
  if (c == '\n')
    ++this->outline;
  putchar(c);
  while ((c = getc(info->action_file)) != EOF)
  {
    if (c == '\n')
      ++this->outline;
    putchar(c);
    last = c;
  }

  if (last != '\n')
  {
    ++this->outline;
    putchar('\n');
  }

  printf(info->default_line_format, ++this->outline + 1);
}


