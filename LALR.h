#pragma once

#include "LR0.h"

typedef
struct shorts
{
  struct shorts *next;
  short value;
}
shorts;

class LALR : public LR0
{
protected:
  int infinity;
  int maxrhs;
  int ngotos;
  unsigned *F;
  short **includes;
  shorts **lookback;
  short **R;
  short *INDEX;
  short *VERTICES;
  int top;

public:

  int tokensetsize;
  short *lookaheads;
  short *LAruleno;

  unsigned *LA;
  short *accessing_symbol;
  core **state_table;
  shifts **shift_table;
  reductions **reduction_table;
  short *goto_map ;
  short *from_state ;
  short *to_state;

public:

  LALR(GlobalInfo* info,ErrorReporter* erp,Reader* reader);
  ~LALR();

public:
  void Process();

protected:

  void add_lookback_edge(int stateno, int ruleno, int gotono);

  void set_state_table();

  void set_accessing_symbol();
  void set_shift_table();


  void set_reduction_table();

  void set_maxrhs();

  void initialize_LA();

  void set_goto_map();

  int map_goto(int state, int symbol);


  void initialize_F();

  void build_relations();

  short ** transpose(short **R, int n);

  void compute_FOLLOWS();

  void compute_lookaheads();

  void digraph(short ** relation);

  void traverse(register int i);

};
