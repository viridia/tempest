# Compilation steps
export CC=clang
export CXX=clang++
cmake -G Ninja ../src/
ninja

# Architecture

The old class hierarchy was too tangled, each class had to serve the needs of several
different compilation phases.

What we want is smaller objects with shorter lifetimes, even if that means duplication
of some properties.

I think the representations we want are:

  * AST
  * Semantic Graph
  * Optimized Semantic Graph (Incuding template expansions, dispatch tables, etc.)
  * Output Symbols

Specific ties we want to break:

1) Be able to talk about envs without defns.
2) Be able to talk about expressions without ASTs.

# Traits

We want to be able to add a trait to a type after the fact.

Do we even want to declare the trait as part of the class signature? I guess.

extend Type implements Trait {

}

I think we can lose the Spark concept of having multiple implementations of class specializations.
Those can be better done with trait extensions. Yes.

## Unsolved syntactical problems:

* Type extensions and scope
* Mutable types

