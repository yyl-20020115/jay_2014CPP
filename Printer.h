#pragma once

struct GlobalInfo;

class Printer
{
protected:
  int nvectors;
  int nentries;
  short **froms ;
  short **tos ;
  short *tally;
  short *width;
  short *state_count;
  short *order ;
  short *base;
  short *pos ;
  int maxtable ;
  short *table ;
  short *check ;
  int lowzero ;
  int high;
  int outline;

protected:

  GlobalInfo* info;
  Reader* reader;
  LALR* lalr;
  ParserGenerator* pg;
  ErrorReporter* erp;

public:

  Printer(GlobalInfo* info,Reader* reader, LALR* lalr, ParserGenerator* pg,ErrorReporter* erp);

  ~Printer();

public:

  void Print();

protected:

  void output_rule_data();
  void output_semantic_actions();
  void output_trailing_text();
  void output_yydefred();
  void output_actions();
  void output_debug(bool tflag);

  void sort_actions();
  void pack_table();
  void output_base();
  void output_table();
  void output_check();
  void token_actions();
  void goto_actions();
  void save_column(int symbol, int default_state);
  void output_defines(char* prefix);
  void output_stored_text(FILE* file,char* name);
  int default_goto(int symbol);
  int matching_vector(int vector);
  int pack_vector(int vector);
  int is_C_identifier(char* name);

};
