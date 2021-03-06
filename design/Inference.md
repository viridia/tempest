# Type inference design notes

So, one could express the set of overloads as a discrimination graph of ANDs and ORs:

  * t -> i32 IF:
    * a1 AND b1
    * a1 AND b2
  * t -> bool IF:
    * a2 AND b1
    * a2 AND b2

Which can be simplified to:

  * t -> i32 IF:
    * a1 AND (b1 or b2)
  * t -> bool IF:
    * a2 AND (b1 or b2)

And, if it turns out that (b1 or b2) is always true (because there are no other b's), then:

  * t -> i32 IF:
    * a1
  * t -> bool IF:
    * a2

The question I would have is, does it require this more complex representation to be able to
do the simplification that I have outlined?

The main advantage of the more complex representation is that it is more compact. So instead of
saying:

  * a1 AND b1
  * a1 AND b2
  * a1 AND b3
  * a1 AND b4
  * a2 AND b1
  * a2 AND b2
  * a2 AND b3
  * a2 AND b4

One can instead say:

  * (a1 or a2) AND (b1 or b2 or b3 or b4)

Let's call these ALL and ANY:

  * ALL(ANY(a1, a2), ANY(b1, b2, b3, b4))

Note, however, that ANY can only contain terms from a single site, whereas ALL always contains
terms from different sites.

So this suggests a different data structure:

  * Condition:
    _all: [(site, )]

Constraints:
    sites:
        choices

ConstraintSet:
    Constraint:
        Conditions:

## Unification:

* S -> T
* [S] -> T
* S -> [T]
* [S] -> [T]
* Map[A, B] -> C
* A -> Map[B, C]

Result of a unification is usually an empty list.

However, it an also be a list of bindings:

  * the inferred variable being bound to
  * what type of binding:
    * equal
    * subtype
    * supertype
    * assignable-to
    * assignable-from
  * the overloads upon which this type binding depends

## Arithmetic operators:

trait InfixAdd[T, Result = T] {
  fn add(t: T) -> Result;
}

implement InfixAdd[i8] for i8 {
  __intrinsic__ fn add(t: i8) -> i8;
}

implement InfixAdd[i16] for i16 {
  __intrinsic__ fn add(t: i16) -> i16;
}

implement InfixAdd[i32] for i32 {
  __intrinsic__ fn add(t: i32) -> i32;
}

implement InfixAdd[i64] for i64 {
  __intrinsic__ fn add(t: i64) -> i64;
}

// This is way simpler. The only question is, what scope does it live in?
// Ans: I think we're going to have to implement a limited form of ADL.
// The problem with ADL is that it completely borks name resolution.
// i.e. infixAdd(1, 2) is meaningless until we know the type of 1 and 2.
// OK so CALL needs to handle idents specially.

// TODO:
// 1) Static methods.
// 2) Static methods of primitive types.
// 3) Core namespace
// ?) Intrinsics

static __intrinsic__ fn infixAdd[T: i64](l: T, r: T) -> T;
static __intrinsic__ fn infixAdd[T: u64](l: T, r: T) -> T;
static __intrinsic__ fn infixAdd[T: f64](l: T, r: T) -> T;

trait InfixSubtract {
  fn subtract(t: Self) -> Self;
}

trait InfixArithmetic[T] extends
    InfixAdd[T], InfixSubtract[T], InfixMultiply[T], InfixDivide[T], InfixRemainder[T];
trait InfixShift[T] extends InfixLeftShift[T], InfixRightShift[T];
trait InfixBitwise[T] extends InfixBitAnd[T], InfixBitOr[T], InfixBitXor[T], InfixComplement[T];

### Implementation

A + B looks at all InfixAdds in scope.
