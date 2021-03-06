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
static char sccsid[] = "@(#)symtab.c	5.3 (Berkeley) 6/1/90";
#endif /* not lint */

#include "defs.h"

/* TABLE_SIZE is the number of entries in the symbol table. */
/* TABLE_SIZE must be a power of two.			    */

SymbolTable::SymbolTable(ErrorReporter* erp,unsigned int tableSize)
  :erp(erp)
  ,symbol_table(0)
  ,tableSize(tableSize)
  ,first_symbol(0)
  ,last_symbol(0)
{
  this->create_symbol_table(tableSize);
}

SymbolTable::~SymbolTable()
{
  this->free_symbol_table();
  this->free_symbols();
}
bucket* SymbolTable::append(bucket* bp)
{
  if(bp!=0)
  {
    if(this->last_symbol!=0)
    {
      this->last_symbol->next = bp;
      this->last_symbol = bp;
    }
  }
  return bp;
}

void SymbolTable::create_symbol_table(unsigned int tableSize)
{
  register int i;
  register bucket *bp;

  symbol_table = (bucket **) CALLOC(tableSize,sizeof(bucket *));
  if (symbol_table == 0) erp->no_space();
  for (i = 0; i < TABLE_SIZE; i++)
    symbol_table[i] = 0;

  bp = make_bucket("error");
  bp->index = 1;
  bp->cls = TERM;

  first_symbol = bp;
  last_symbol = bp;
  symbol_table[hash("error")] = bp;
}

int SymbolTable::hash(char* name)
{
  register char *s;
  register int c, k;

  assert(name && *name);
  s = name;
  k = *s;
  while (c = *++s)
    k = (31*k + c) & (TABLE_SIZE - 1);

  return (k);
}


bucket * SymbolTable::make_bucket(char* name)
{
  register bucket *bp;

  assert(name);
  bp = (bucket *) CALLOC(1,sizeof(bucket));
  if (bp == 0) erp->no_space();
  bp->link = 0;
  bp->next = 0;
  bp->name =(char*) CALLOC(strlen(name) + 1,1);
  if (bp->name == 0) erp->no_space();
  bp->tag = 0;
  bp->value = UNDEFINED;
  bp->index = 0;
  bp->prec = 0;
  bp->cls = UNKNOWN;
  bp->assoc = TOKEN;

  if (bp->name == 0) erp->no_space();
  strcpy(bp->name, name);

  return (bp);
}


bucket * SymbolTable::lookup(char* name)
{
  register bucket *bp, **bpp;

  bpp = symbol_table + hash(name);
  bp = *bpp;

  while (bp)
  {
    if (strcmp(name, bp->name) == 0) return (bp);
    bpp = &bp->link;
    bp = *bpp;
  }

  *bpp = bp = make_bucket(name);
  last_symbol->next = bp;
  last_symbol = bp;

  return (bp);
}



void SymbolTable::free_symbol_table()
{
  FREE(symbol_table);
  symbol_table = 0;
}


void SymbolTable::free_symbols()
{
  register bucket *p, *q;

  for (p = first_symbol; p; p = q)
  {
    q = p->next;
    FREE(p);
  }
}
