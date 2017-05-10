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
static char sccsid[] = "@(#)lr0.c	5.3 (Berkeley) 1/20/91";
#endif /* not lint */


#include "defs.h"

LR0::LR0(GlobalInfo* info,ErrorReporter* erp,Reader* reader)
: info(info),
  reader(reader),
  erp(erp),
  state_set(0),
  this_state(0),
  last_state(0),
  last_shift(0),
  last_reduction(0),
  nshifts(0),
  shift_symbol(0),
  redset(0),
  shiftset(0),
  kernel_base(0),
  kernel_end(0),
  kernel_items(0),
  nstates(0),
  first_state(0),
  first_shift(0),
  first_reduction(0),
  nullable(0),
  derives(0)
{
}
LR0::~LR0()
{
  this->free_derives();
  this->free_nullable();

  if(this->first_reduction!=0)
  {
    register reductions *rp, *next;

    for (rp = this->first_reduction; rp; rp = next)
    {
      next = rp->next;
      if(rp->rules!=0)
      {
        free(rp->rules);
      }
      free(rp);
    }

  }

  if(this->first_state!=0)
  {
      register core *cp, *next;

      for (cp = this->first_state; cp; cp = next)
      {
        next = cp->next;
        if(cp!=0 && cp->items!=0)
        {
          free(cp->items);
        }
        free(cp);
      }
      this->first_state= 0;
  }
  if(this->first_shift!=0)
  {
    register shifts *sp,*next;

    for (sp =   this->first_shift; sp; sp = next)
    {
      next = sp->next;
      if(sp!=0&& sp->shift!=0)
      {
        free(sp->shift);
      }
      free(sp);
    }
    this->first_shift = 0;
  }

}


void LR0::allocate_itemsets()
{
  register short *itemp;
  register short *item_end;
  register int symbol;
  register int i;
  register int count;
  register int max;
  register short *symbol_count;

  count = 0;
  //所有符号，每一个符号出现的次数
  symbol_count = (short*)CALLOC(reader->nsyms, sizeof(short));

  item_end = reader->ritem + reader->nitems;
  //检查右端的，记录总数，记录每一个右端符号出现的次数
  for (itemp = reader->ritem; itemp < item_end; itemp++)
  {
    symbol = *itemp;
    if (symbol >= 0)
    {
      count++;
      symbol_count[symbol]++;
    }
  }
  //所有符号
  kernel_base = (short**)CALLOC(reader->nsyms, sizeof(short *));
  //右端符号
  kernel_items = (short*)CALLOC(count, sizeof(short));

  count = 0;
  max = 0;
  for (i = 0; i < reader->nsyms; i++)
  {
    kernel_base[i] = kernel_items + count;
    count += symbol_count[i];
    if (max < symbol_count[i])
      max = symbol_count[i];
  }

  shift_symbol = symbol_count;
  kernel_end = (short**)CALLOC(reader->nsyms, sizeof(short *));
}


void LR0::allocate_storage()
{
  allocate_itemsets();
  shiftset = (short*)CALLOC(reader->nsyms, sizeof(short));
  redset = (short*)CALLOC(reader->nrules + 1, sizeof(short));
  state_set = (core**)CALLOC(reader->nitems , sizeof(core *));
}


void LR0::append_states()
{
  register int i;
  register int j;
  register int symbol;

#ifdef	TRACE
  fprintf(stderr, "Entering append_states()\n");
#endif

  for (i = 1; i < nshifts; i++)
  {
    symbol = shift_symbol[i];
    j = i;
    while (j > 0 && shift_symbol[j - 1] > symbol)
    {
      shift_symbol[j] = shift_symbol[j - 1];
      j--;
    
    }

    shift_symbol[j] = symbol;
  }

  for (i = 0; i < nshifts; i++)
  {
    symbol = shift_symbol[i];

    shiftset[i] = get_state(symbol);

  }
}


void LR0::free_storage()
{
  FREE(shift_symbol);
  FREE(redset);
  FREE(shiftset);
  FREE(kernel_base);
  FREE(kernel_end);
  FREE(kernel_items);
  FREE(state_set);
}



void LR0::generate_states()
{

  allocate_storage();
  
  Closure closure(info,this->reader,this);

  initialize_states();
  
  while (this_state)
  {

    closure.closure(this_state->items, this_state->nitems);

    save_reductions(closure.GetItemSet(),closure.GetItemSetEnd());

    new_itemsets(closure.GetItemSet(),closure.GetItemSetEnd());
        
    append_states();
   
    if (nshifts > 0)
    {
      save_shifts();


    }
    this_state = this_state->next;
  }

  free_storage();
}


int LR0::get_state(int symbol)
{
  register int key;
  register short *isp1;
  register short *isp2;
  register short *iend;
  register core *sp;
  register int found;
  register int n;

#ifdef	TRACE
  fprintf(stderr, "Entering get_state(%d)\n", symbol);
#endif

  isp1 = kernel_base[symbol];
  iend = kernel_end[symbol];
  n = iend - isp1;

  key = *isp1;

  assert(0 <= key && key < reader->nitems);
  sp = state_set[key];
  if (sp)
  {
    found = 0;
    while (!found)
    {
      if (sp->nitems == n)
      {
        found = 1;
        isp1 = kernel_base[symbol];
        isp2 = sp->items;

        while (found && isp1 < iend)
        {
          if (*isp1++ != *isp2++)
            found = 0;
        }
      }

      if (!found)
      {
        if (sp->link)
        {
          sp = sp->link;
        }
        else
        {
          sp = sp->link = new_state(symbol);
          found = 1;
        }
      }
    }
  }
  else
  {
    state_set[key] = sp = new_state(symbol);
  }

  return (sp->number);
}



void LR0::initialize_states()
{
  register int i;
  register short *start_derives;
  register core *p;

  start_derives = this->derives[reader->start_symbol];
  for (i = 0; start_derives[i] >= 0; ++i)
    continue;

  p = (core *) CALLOC(1,sizeof(core));
  p->items = (short*) CALLOC(i+1,sizeof(short));

  if (p == 0) erp->no_space();

  p->next = 0;
  p->link = 0;
  p->number = 0;
  p->accessing_symbol = 0;
  p->nitems = i;

  for (i = 0;  start_derives[i] >= 0; ++i)
    p->items[i] = reader->rrhs[start_derives[i]];

  first_state = last_state = this_state = p;
  nstates = 1;
}


void LR0::new_itemsets(short* itemset, short* itemsetend)
{
  int t = 0;
  register int i;
  register int shiftcount;
  register short *isp;
  register short *ksp;
  register int symbol;


  for (i = 0; i < reader->nsyms; i++)
    kernel_end[i] = 0;
 
  shiftcount = 0;
  isp = itemset;

  while (isp < itemsetend)
  {
    i = *isp++;

    symbol = reader->ritem[i];

    if (symbol > 0)
    {
      ksp = kernel_end[symbol];
      if (!ksp)
      {
        shift_symbol[shiftcount++] = symbol;
        ksp = kernel_base[symbol];
      }

      *ksp++ = i + 1;
      kernel_end[symbol] = ksp;
    }
  }

  nshifts = shiftcount;

}



core * LR0::new_state(int symbol)
{
  register int n;
  register core *p;
  register short *isp1;
  register short *isp2;
  register short *iend;

#ifdef	TRACE
  fprintf(stderr, "Entering new_state(%d)\n", symbol);
#endif

  if (nstates >= MAXSHORT)
    erp->fatal("too many states");

  isp1 = kernel_base[symbol];
  iend = kernel_end[symbol];
  n = iend - isp1;

  p = (core *) CALLOC(1,sizeof(core));
  p->items= (short*) CALLOC(n,sizeof(short));

  p->accessing_symbol = symbol;
  p->number = nstates;
  p->nitems = n;

  isp2 = p->items;
  while (isp1 < iend)
    *isp2++ = *isp1++;

  last_state->next = p;
  last_state = p;

  nstates++;

  return (p);
}


/* show_cores is used for debugging */

void LR0::show_cores()
{
  core *p;
  int i, j, k, n;
  int itemno;

  k = 0;
  for (p =first_state; p; ++k, p = p->next)
  {
    if (k) printf("\n");
    printf("state %d, number = %d, accessing symbol = %s\n",
      k, p->number, reader->symbol_name[p->accessing_symbol]);
    n = p->nitems;
    for (i = 0; i < n; ++i)
    {
      itemno = p->items[i];
      printf("%4d  ", itemno);
      j = itemno;
      while (reader->ritem[j] >= 0) ++j;
      printf("%s :", reader->symbol_name[reader->rlhs[-reader->ritem[j]]]);
      j = reader->rrhs[-reader->ritem[j]];
      while (j < itemno)
        printf(" %s", reader->symbol_name[reader->ritem[j++]]);
      printf(" .");
      while (reader->ritem[j] >= 0)
        printf(" %s", reader->symbol_name[reader->ritem[j++]]);
      printf("\n");
      fflush(stdout);
    }
  }
}


/* show_ritems is used for debugging */

void LR0::show_ritems()
{
  int i;

  for (i = 0; i < reader->nitems; ++i)
    printf("ritem[%d] = %d\n", i, reader->ritem[i]);
}


/* show_rrhs is used for debugging */
void LR0::show_rrhs()
{
  int i;

  for (i = 0; i < reader->nrules; ++i)
    printf("rrhs[%d] = %d\n", i, reader->rrhs[i]);
}


/* show_shifts is used for debugging */

void LR0::show_shifts()
{
  shifts *p;
  int i, j, k;

  k = 0;
  for (p = first_shift; p; ++k, p = p->next)
  {
    if (k) printf("\n");
    printf("shift %d, number = %d, nshifts = %d\n", k, p->number,
      p->nshifts);
    j = p->nshifts;
    for (i = 0; i < j; ++i)
      printf("\t%d\n", p->shift[i]);
  }
}


void LR0::save_shifts()
{
  register shifts *p;
  register short *sp1;
  register short *sp2;
  register short *send;

  p = (shifts *) CALLOC(1,(unsigned) (sizeof(shifts)));

  p->shift = (short*) CALLOC(nshifts, sizeof(short));

  p->number = this_state->number;
  p->nshifts = nshifts;
  p->next = 0;

  sp1 = shiftset;
  sp2 = p->shift;
  send = shiftset + nshifts;

  while (sp1 < send)
    *sp2++ = *sp1++;

  if (last_shift)
  {
    last_shift->next = p;
    last_shift = p;
  }
  else
  {
    first_shift = p;
    last_shift = p;
  }
}



void LR0::save_reductions(short* itemset, short* itemsetend)
{
  register short *isp;
  register short *rp1;
  register short *rp2;
  register int item;
  register int count;
  register reductions *p;
  register short *rend;

  count = 0;
  for (isp = itemset; isp < itemsetend; isp++)
  {
    item = reader->ritem[*isp];
    if (item < 0)
    {
      redset[count++] = -item;
    }
  }

  if (count)
  {
    p = (reductions *) CALLOC(1,sizeof(reductions));
    p->rules = (short*) CALLOC(count, sizeof(short));

    p->next = 0;
    p->number = this_state->number;
    p->nreds = count;

    rp1 = redset;
    rp2 = p->rules;
    rend = rp1 + count;

    while (rp1 < rend)
      *rp2++ = *rp1++;

    if (last_reduction)
    {
      last_reduction->next = p;
      last_reduction = p;
    }
    else
    {
      first_reduction = p;
      last_reduction = p;
    }
  }
}


void LR0::set_derives()
{
  register int i, k;
  register int lhs;
  register short *rules;

  //FIXED: 
  this->derives = (short**)CALLOC(reader->nsyms,sizeof( short *));
  //FIXED: 
  rules = (short*)CALLOC((reader->nvars + reader->nrules),sizeof( short));

  k = 0;
  for (lhs = reader->start_symbol; lhs < reader->nsyms; lhs++)
  {
    this->derives[lhs] = rules + k;
    for (i = 0; i < reader->nrules; i++)
    {
      if (reader->rlhs[i] == lhs)
      {
        rules[k] = i;
        k++;
      }
    }
    rules[k] = -1;
    k++;

  }

#ifdef DEBUG
  print_derives();
#endif
}

void LR0::free_derives()
{
  FREE(this->derives[reader->start_symbol]);
  FREE(this->derives);
}

void LR0::print_derives()
{
  register int i;
  register short *sp;

  printf("\nDERIVES\n\n");

  for (i = reader->start_symbol; i < reader->nsyms; i++)
  {
    printf("%s derives ", reader->symbol_name[i]);
    for (sp = this->derives[i]; *sp >= 0; sp++)
    {
      printf("  %d", *sp);
    }
    putchar('\n');
  }

  putchar('\n');
}

void LR0::set_nullable()
{
  register int i, j;
  register int empty;
  int done;

  //FIXED: 
  this->nullable = (char*)CALLOC(reader->nsyms,1);

  if (this->nullable == 0) erp->no_space();

  for (i = 0; i < reader->nsyms; ++i)
    this->nullable[i] = 0;

  done = 0;
  while (!done)
  {
    done = 1;
    for (i = 1; i <reader-> nitems; i++)
    {
      empty = 1;
      while ((j = reader->ritem[i]) >= 0)
      {
        if (!this->nullable[j])
          empty = 0;
        ++i;
      }
      if (empty)
      {
        j = reader->rlhs[-j];
        if (!this->nullable[j])
        {
          this->nullable[j] = 1;

          done = 0;
        }
      }
    }
  }

#ifdef DEBUG
  for (i = 0; i < reader-> nsyms; i++)
  {
    if (nullable[i])
      printf("%s is nullable\n",reader-> symbol_name[i]);
    else
      printf("%s is not nullable\n", reader-> symbol_name[i]);
  }
#endif
}


void LR0::free_nullable()
{
  FREE(this->nullable);
}


void LR0::Process()
{
  set_derives();
  set_nullable();
  generate_states();
}

