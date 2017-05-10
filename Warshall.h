#pragma once

class Warshall
{
public:

  static void transitive_closure(unsigned *R, int n);

  static void reflexive_transitive_closure(unsigned *R, int n);
};
