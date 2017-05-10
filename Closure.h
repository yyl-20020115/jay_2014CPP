#pragma once

class LR0;
class Closure
{
protected:

  short *itemset;
  short *itemsetend;
  unsigned *ruleset;
  unsigned *first_derives;
  unsigned *EFF;

protected:

  GlobalInfo* info;
  LR0* lr0;
  Reader* reader;

public:

  short* GetItemSet(){return this->itemset;}
  short* GetItemSetEnd(){return this->itemsetend;}
  unsigned * GetRuleSet(){return this->ruleset;}

public:

  Closure(GlobalInfo* info,Reader* reader,LR0* lr0);

  ~Closure();

public:

  void set_first_derives();

  void closure(short* nucleus, int n);
  
protected:

  void set_EFF();

  void finalize_closure();

public:

  void print_closure(int n);

  void print_EFF();

  void print_first_derives();
};
