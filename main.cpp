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
char copyright[] =
  "@(#) Copyright (c) 1989 The Regents of the University of California.\n\
  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)main.c	5.5 (Berkeley) 5/24/93";
#endif /* not lint */

#include <signal.h>
#include "defs.h"


void onintr(int signo)
{
  exit(1);
}
void set_signals()
{
#ifdef SIGINT
  if (signal(SIGINT, SIG_IGN) != SIG_IGN)
    signal(SIGINT, onintr);
#endif
#ifdef SIGTERM
  if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
    signal(SIGTERM, onintr);
#endif
#ifdef SIGHUP
  if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
    signal(SIGHUP, onintr);
#endif
}

GlobalInfo info;

#if defined(_WIN32) && defined(_DEBUG)
inline void EnableMemLeakCheck()
{
   _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
}
#endif

int main(int argc, char* argv[])
{
#if defined(_WIN32) && defined(_DEBUG)
  EnableMemLeakCheck();
#endif

  try
  {
    set_signals();

    info.getargs(argc, argv);

    ErrorReporter erp(&info,0);

    info.erp= &erp;

    SymbolTable table(&erp);

    Reader reader(&info,&erp,&table);

    erp.SetReader(&reader);

    LALR lalr(&info,&erp,&reader);

    ParserGenerator pg(&info,&erp,&reader,&lalr);

    Verbose verbose(&info,&erp,&reader,&lalr,&pg);

    Printer printer(&info,&reader,&lalr,&pg,&erp);

    info.open_files();
    {
      reader.Read();

      lalr.Process();

      pg.GenerateParser();

      verbose.PrintVerbose();

      printer.Print();
    }
  }
  catch(int errCode)
  {
    return errCode;
  }
  return 0;
}
