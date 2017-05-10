#pragma once

class Verbose
{
protected:

  GlobalInfo* info;
  LALR* lalr;
  ParserGenerator* pg;
  Reader* reader;
  ErrorReporter* erp;

public:
  Verbose(GlobalInfo* info,ErrorReporter* erp,Reader* reader,LALR* lalr,ParserGenerator* pg);
  ~Verbose();

public:
  void PrintVerbose();

protected:

  void log_conflicts();
  void log_unused();
  void print_actions(int stateno);
  void print_conflicts(int state);
  void print_core(int state);
  void print_gotos(int stateno);
  void print_reductions(register action *p, register int defred);
  void print_nulls(int state,short* null_rules);
  void print_shifts(register action* p);
  void print_state(int state,short* null_rules);

};