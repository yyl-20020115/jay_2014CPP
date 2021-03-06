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
static char sccsid[] = "@(#)verbose.c	5.3 (Berkeley) 1/20/91";
#endif /* not lint */

#include "defs.h"

Verbose::Verbose(GlobalInfo* info,ErrorReporter* erp,Reader* reader,LALR* lalr,ParserGenerator* pg)
  : info(info)
  , erp(erp)
  , reader(reader)
  , lalr(lalr)
  , pg(pg)
{
}

Verbose::~Verbose()
{

}


void Verbose::PrintVerbose()
{
  
  if (!info->vflag) return;

  register int i;

  short* null_rules = (short *) CALLOC(reader->nrules,sizeof(short));
  
  if (null_rules == 0) erp->no_space();
  fprintf(info->verbose_file, "\f\n");
  for (i = 0; i < lalr->nstates; i++)
    print_state(i,null_rules);

  FREE(null_rules);

  if (pg->GetUnused())
    log_unused();
  if (pg->HasConflict())
    log_conflicts();

  fprintf(info->verbose_file, "\n\n%d terminals, %d nonterminals\n", reader->ntokens,reader->nvars);
  fprintf(info->verbose_file, "%d grammar rules, %d states\n", reader->nrules - 2, lalr->nstates);
}


void Verbose::log_unused()
{
  register int i;
  register short *p;

  fprintf(info->verbose_file, "\n\nRules never reduced:\n");
  for (i = 3; i < reader->nrules; ++i)
  {
    if (!pg->GetRules_Used()[i])
    {
      fprintf(info->verbose_file, "\t%s :", reader->symbol_name[reader->rlhs[i]]);
      for (p = reader->ritem + reader->rrhs[i]; *p >= 0; ++p)
        fprintf(info->verbose_file, " %s", reader->symbol_name[*p]);
      fprintf(info->verbose_file, "  (%d)\n", i - 2);
    }
  }
}


void Verbose::log_conflicts()
{
  register int i;

  fprintf(info->verbose_file, "\n\n");
  for (i = 0; i < lalr->nstates; i++)
  {
    if (pg->GetSRconflicts()[i] || pg->GetRRconflicts()[i])
    {
      fprintf(info->verbose_file, "State %d contains ", i);
      if (pg->GetSRconflicts()[i] == 1)
        fprintf(info->verbose_file, "1 shift/reduce conflict");
      else if (pg->GetSRconflicts()[i] > 1)
        fprintf(info->verbose_file, "%d shift/reduce conflicts",
        pg->GetSRconflicts()[i]);
      if (pg->GetSRconflicts()[i] && pg->GetRRconflicts()[i])
        fprintf(info->verbose_file, ", ");
      if (pg->GetRRconflicts()[i] == 1)
        fprintf(info->verbose_file, "1 reduce/reduce conflict");
      else if (pg->GetRRconflicts()[i] > 1)
        fprintf(info->verbose_file, "%d reduce/reduce conflicts",
        pg->GetRRconflicts()[i]);
      fprintf(info->verbose_file, ".\n");
    }
  }
}


void Verbose::print_state(int state,short* null_rules)
{
  if (state)
    fprintf(info->verbose_file, "\n\n");
  if (pg->GetSRconflicts()[state] || pg->GetRRconflicts()[state])
    print_conflicts(state);
  fprintf(info->verbose_file, "state %d\n", state);
  print_core(state);
  print_nulls(state,null_rules);
  print_actions(state);
}


void Verbose::print_conflicts(int state)
{
  register int symbol, act, number;
  register action *p;

  symbol = -1;
  for (p = pg->GetParser()[state]; p; p = p->next)
  {
    if (p->suppressed == 2)
      continue;

    if (p->symbol != symbol)
    {
      symbol = p->symbol;
      number = p->number;
      if (p->action_code == SHIFT)
        act = SHIFT;
      else
        act = REDUCE;
    }
    else if (p->suppressed == 1)
    {
      if (state == pg->GetFinalState() && symbol == 0)
      {
        fprintf(info->verbose_file, "%d: shift/reduce conflict \
                              (accept, reduce %d) on $end\n", state, p->number - 2);
      }
      else
      {
        if (act == SHIFT)
        {
          fprintf(info->verbose_file, "%d: shift/reduce conflict \
                                (shift %d, reduce %d) on %s\n", state, number, p->number - 2,
                                reader->symbol_name[symbol]);
        }
        else
        {
          fprintf(info->verbose_file, "%d: reduce/reduce conflict \
                                (reduce %d, reduce %d) on %s\n", state, number - 2, p->number - 2,
                                reader->symbol_name[symbol]);
        }
      }
    }
  }
}


void Verbose::print_core(int state)
{
  register int i;
  register int k;
  register int rule;
  register core *statep;
  register short *sp;
  register short *sp1;

  statep = lalr->state_table[state];
  k = statep->nitems;

  for (i = 0; i < k; i++)
  {
    sp1 = sp = reader->ritem + statep->items[i];

    while (*sp >= 0) ++sp;
    rule = -(*sp);
    fprintf(info->verbose_file, "\t%s : ", reader->symbol_name[reader->rlhs[rule]]);

    for (sp = reader->ritem + reader->rrhs[rule]; sp < sp1; sp++)
      fprintf(info->verbose_file, "%s ", reader->symbol_name[*sp]);

    putc('.', info->verbose_file);

    while (*sp >= 0)
    {
      fprintf(info->verbose_file, " %s", reader->symbol_name[*sp]);
      sp++;
    }
    fprintf(info->verbose_file, "  (%d)\n", -2 - *sp);
  }
}


void Verbose::print_nulls(int state,short* null_rules)
{
  register action *p;
  register int i, j, k, nnulls;

  nnulls = 0;
  for (p = pg->GetParser()[state]; p; p = p->next)
  {
    if (p->action_code == REDUCE &&
      (p->suppressed == 0 || p->suppressed == 1))
    {
      i = p->number;
      if (reader->rrhs[i] + 1 == reader->rrhs[i+1])
      {
        for (j = 0; j < nnulls && i > null_rules[j]; ++j)
          continue;

        if (j == nnulls)
        {
          ++nnulls;
          null_rules[j] = i;
        }
        else if (i != null_rules[j])
        {
          ++nnulls;
          for (k = nnulls - 1; k > j; --k)
            null_rules[k] = null_rules[k-1];
          null_rules[j] = i;
        }
      }
    }
  }

  for (i = 0; i < nnulls; ++i)
  {
    j = null_rules[i];
    fprintf(info->verbose_file, "\t%s : .  (%d)\n", reader->symbol_name[reader->rlhs[j]],
      j - 2);
  }
  fprintf(info->verbose_file, "\n");
}


void Verbose::print_actions(int stateno)
{
  register action *p;
  register shifts *sp;
  register int as;

  if (stateno == pg->GetFinalState())
    fprintf(info->verbose_file, "\t$end  accept\n");

  p = pg->GetParser()[stateno];
  if (p)
  {
    print_shifts(p);
    print_reductions(p, pg->GetDefred()[stateno]);
  }

  sp = lalr->shift_table[stateno];
  if (sp && sp->nshifts > 0)
  {
    as = lalr->accessing_symbol[sp->shift[sp->nshifts - 1]];
    if (as>=reader->start_symbol)
      print_gotos(stateno);
  }
}


void Verbose::print_shifts(register action* p)
{
  register int count;
  register action *q;

  count = 0;
  for (q = p; q; q = q->next)
  {
    if (q->suppressed < 2 && q->action_code == SHIFT)
      ++count;
  }

  if (count > 0)
  {
    for (; p; p = p->next)
    {
      if (p->action_code == SHIFT && p->suppressed == 0)
        fprintf(info->verbose_file, "\t%s  shift %d\n",
        reader->symbol_name[p->symbol], p->number);
    }
  }
}


void Verbose::print_reductions(register action *p, register int defred)
{
  register int k, anyreds;
  register action *q;

  anyreds = 0;
  for (q = p; q ; q = q->next)
  {
    if (q->action_code == REDUCE && q->suppressed < 2)
    {
      anyreds = 1;
      break;
    }
  }

  if (anyreds == 0)
    fprintf(info->verbose_file, "\t.  error\n");
  else
  {
    for (; p; p = p->next)
    {
      if (p->action_code == REDUCE && p->number != defred)
      {
        k = p->number - 2;
        if (p->suppressed == 0)
          fprintf(info->verbose_file, "\t%s  reduce %d\n",
          reader->symbol_name[p->symbol], k);
      }
    }

    if (defred > 0)
      fprintf(info->verbose_file, "\t.  reduce %d\n", defred - 2);
  }
}


void Verbose::print_gotos(int stateno)
{
  register int i, k;
  register int as;
  register short *to_state;
  register shifts *sp;

  putc('\n', info->verbose_file);
  sp = lalr->shift_table[stateno];
  to_state = sp->shift;
  for (i = 0; i < sp->nshifts; ++i)
  {
    k = to_state[i];
    as = lalr->accessing_symbol[k];
    if (as>=reader->start_symbol)
      fprintf(info->verbose_file, "\t%s  goto %d\n", reader->symbol_name[as], k);
  }
}
