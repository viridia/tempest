# Compilation steps
export CC=clang
export CXX=clang++
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ../src/
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
  * Output Symbols - Code Flow Graph

Specific ties we want to break:

1) Be able to talk about envs without defns.
2) Be able to talk about expressions without ASTs.

## Memory management

We still need some way to deal with the fact that temporary objects such as specializations
and environments are often created during analysis.

There's an ownership problem in that the name lookup function can return either temporary or
permanent objects.

Memory management is made much harder by the idea that we might want to run all of this as a server
for syntax checking and parsing. Basically any stage up to type inference need to be able to reset
memory.

I think the general strategy should be:

Allocations:

* AST: per module
* Semantic Graph: per module
* Type Store: per compilation unit
* Synthetic definitions: per compilation unit, with possible caching.

## Output symbol graph

* OutputSymbol
  * StringLiteral
  * ObjectLiteral
  * ArrayLiteral
  * EnumValue
  * Method
  * ExternMethod
  * MethodTable
    * ClassMethodTable
    * InterfaceMethodTable
  * StaticArray
    * TypeDescriptorArray
    * InterfaceValueDescriptorArray
    * MethodDescriptorArray
  * TraceTable
  * TypeDescriptor
    * ClassDescriptor
    * FunctionTypeDescriptor
    * UnionTypeDescriptor
    * TupleTypeDescriptor
    * PrimitiveTypeDescriptor
    * InterfaceValueTypeDescriptor
  * TypeKey
  * GlobalVariable
  * NullReference

We need a way to register output types and ensure they are unique. This includes anonymous types.

What aspects of a type can be used as keys?

  * For nominal types:
    * Fully-qualified name.
    * List of all template parameters, in sequence order, including inheritance.
  * For union and intersection types:
    * type kind
    * List of all member types, in canonical order
  * For tuple types:
    * type kind
    * List of all member types, in sequence order
  * For function types:
    * Fully-qualified name.
    * List of all template parameters, in sequence order, including inheritance.
    * List of all parameter types, in sequence order
    * Note that keywords are *not* part of the type.

  CGType
    * toString
    * irType
    * irFieldType - when used as a member field
  CGCompositeType
    * private / anonymous
    * CGQualifiedName
  CGUnionType
  CGTupleType
  CGBasicType

Where can an intersection type be used?

  * As constraint on a type variable

  -- You can't use an intersection type as an actual type, because you can't use traits as
     actual types either.

# Language Design

## Traits

We want to be able to add a trait to a type after the fact.

Do we even want to declare the trait as part of the class signature? I guess.

extend Type implements Trait {

}

I think we can lose the Spark concept of having multiple implementations of class specializations.
Those can be better done with trait extensions. Yes.

## Operator overloading

Via traits:

* Addition
* Subtraction
* Multiplication
* Division
* Modulus
* BitwiseAnd
* BitwiseOr
* BitwiseXor
* PartialOrder

One problem with using traits for operator overloading is that operators have two arguments which
can be of different types (path / string for example).

// I think the operator traits will have to be simple markers.
trait Addition {}

class String implements Addition {
  static fn add(s0: String, s1: String): String;
}

## Unsolved syntactical problems:

* Type extensions and scope
* Mutable types
