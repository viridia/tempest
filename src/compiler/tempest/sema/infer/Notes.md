# Type Inference Logic

Type inference operates on individual statements - that is, each statement is a separate constraint
system to be solved independently. (We could try and solve the entire function but that would be
(a) ambitious and (b) magical.)

The solver starts by creating a constraint system. This involves various factors:

## Choices

There are two kinds of choices:

* Overloaded function calls
* Overloaded type specialiations

In each case, there's an invocation of a symbol with (possibly implicit) arguments, and there
are several competing definitions of that symbols. Each definition may carry with it a set of
constraints. The goal is find a set of choices such that all constraints are maximally satisfied.

## Type Bindings

In addition to providing a set of overload choices, the type solver will also provide a set of
type parameter bindings. These are most often associated with a particular overload of a generic
function or type.

Because a given function or type specialization may be invoked multiple times within a single
statement, possibly with different type arguments, we need to rename all of the type variables
for each call site or choice point to ensure that they are distinct.

## Constraints

There are several flavors of constraints:

* Assignable, meaning that a source type must be assignable to a target type.
  * Both the source type and the target type can be expressions that are type parameters,
    or can be generic types with inferred type parameters.
* Bindable, meaning that a type can be bound to a type parameter. Type parameters have
  additional constraints, which are:
  * Subtype constraints, meaning that the type must be a subtype of a given type.
  * Supertype constraints, meaning that the type must be a supertype of a given type.
  * Trait constraints, meaning that the type must have a given trait.

Both subtype constraints and trait constraints may have free (unbound) type parameters that are
part of the constraint; these may cascade producing additional constraints.

In addition, all of the above constraints may be unconditional or consequential. Unconditional
constraints must be satisfied regardless of which overloads are selected. Consequential constraints
only come into effect when a particular overload is chosen.

For example, if there are two overloaded methods for a particular call, and one accepts an int
argument while the other accepts a float, then there will be two assignable-to constraints, each
a consequence of choosing that particular overload.

The way this is tracked is that for each constraint, a 'when' list contains the set of conditions
under which this constraint is relevant and needs to be satisfied. Part of the job of the solver
involves manipulating this list to exclude contradictory or irrelevant options.

**Question**: what about built-in infix operators? These are 'functions' in the type inference sense,
but for reasons having to do with compiler performance we don't actually treat them as such.
So how do we want to handle the return type of i32 + T?

## Unknown types

When doing inference in the body of an unexpanded template, there may be overloads that refer to
type parameters of the template. However, those template parameters are of unknown type (other
than what is known based on the parameter constraints), and it is an error to impose additional constraints on these in the body of a method, since that would prevent the template from
being instantiated.

## Examples

```
fn a(x: i32) -> i32;
fn a(x: f32) -> bool;

fn b[T](el: T) -> [T];
fn c(x: [bool]) -> void;
fn d(x: [T]) -> T;

b(a(1.0)); // Returns array of boolean
c(b(a(1.0)));
d(b(a(1.0)));
```

Constraints for this example:

* assignableTo f32 -> i32, when a#1
* assignableTo f32 -> f32, when a#2
* bindableTo: i32 -> T, when a#1
* bindableTo: bool -> T, when a#2
* assignableTo: [T] -> [bool]
  * bindableTo: T -> bool
* assignableTo: [T@b] -> [T@d]
  * bindableTo: T@b -> T@d


```
fn a(x: i32) -> i32;
fn a(x: f32) -> bool;

fn b[S, T](a1: S, a2: T) -> S | T | void;
fn c(x: [bool]) -> void;
fn d(x: [T]) -> T;

b(a(1.0)); // Returns array of boolean
c(b(a(1.0)));
d(b(a(1.0)));
```

## Data structures

AssignableConstraint
* site
* when
* valid
* srcType
* srcTypeParams
* srcTypeArgs
* dstType
* dstTypeParams
* dstTypeArgs
