#pragma once


typedef struct action action;
struct action
{
  struct action *next;
  short symbol;
  short number;
  short prec;
  char action_code;
  char assoc;
  char suppressed;
};

struct GlobalInfo;

class ParserGenerator
{

protected:

  LALR* lalr;
  Reader* reader;
  GlobalInfo* info;
  ErrorReporter* erp;

protected:

  int SRcount;
  int RRcount;


  int SRtotal ;
  int RRtotal ;

protected:

  action **parser ;

  short *SRconflicts ;
  short *RRconflicts ;
  short *defred ;
  short *rules_used ;
  short nunused ;
  short final_state ;

public:
  action** GetParser(){return this->parser;}
  short* GetSRconflicts(){return this->SRconflicts;}
  short* GetRRconflicts(){return this->RRconflicts;}
  short* GetDefred(){return this->defred;}
  short* GetRules_Used(){return this->rules_used;}
  short GetUnused(){return this->nunused;}
  short GetFinalState(){return this->final_state;}
  bool HasConflict(){return this->SRtotal||this->RRtotal;}

public:

  ParserGenerator(GlobalInfo* info,ErrorReporter* erp,Reader* reader,LALR* lalr);

  ~ParserGenerator();

public:

  void GenerateParser();

protected:

  action * add_reduce(register action* actions, int ruleno, int symbol);
  action * add_reductions(int stateno,register action* actions);
  action * get_shifts(int stateno);
  action * parse_actions(register int stateno);

  void free_action_row(register action* p);
  void defreds();
  void find_final_state();
  void unused_rules();
  void remove_conflicts();
  void total_conflicts();

  int sole_reduction(int stateno);
  void free_parser();
};