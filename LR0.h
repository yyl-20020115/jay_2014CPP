#pragma once

typedef struct core core;
struct core
{
  struct core *next;
  struct core *link;
  short *items;
  short number;
  short accessing_symbol;
  short nitems;
};

/*  the structure used to record shifts  */

typedef struct shifts shifts;
struct shifts
{
  struct shifts *next;
  short *shift;
  short number;
  short nshifts;
};


/*  the structure used to store reductions  */

typedef struct reductions reductions;
struct reductions
{
  struct reductions *next;
  short* rules;
  short number;
  short nreds;
};

class LR0
{
private:

  core **state_set;
  core *this_state;
  core *last_state;
  shifts *last_shift;
  reductions *last_reduction;

  int nshifts;
  short *shift_symbol;

  short *redset;
  short *shiftset;

  short **kernel_base;
  short **kernel_end;
  short *kernel_items;

protected:

  char *nullable;
  short **derives;

  GlobalInfo* info;

  Reader* reader;
  ErrorReporter* erp;

public:

  int nstates;
  core *first_state;
  shifts *first_shift;
  reductions *first_reduction;

public:

  short** GetDeriveds(){return this->derives;}

public:

  LR0(GlobalInfo* info,ErrorReporter* erp,Reader* reader);
  ~LR0();

public:
  virtual void Process();

protected:

  int get_state(int symbol);
  void initialize_states();
  void new_itemsets(short* itemset, short* itemsetend);
  void save_reductions(short* itemset, short* itemsetend);
  void save_shifts();
  core * new_state(int symbol);
  void allocate_itemsets();
  void allocate_storage();
  void append_states();
  void free_storage();
  void generate_states();
  void show_cores();
  void show_ritems();
  void show_rrhs();
  void show_shifts();
  void set_derives();
  void free_derives();
  void print_derives();
  void set_nullable();
  void free_nullable();
};

