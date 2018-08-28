# Type inference design notes

One problem I haven't figured out yet is how to deal with a combinatorial explosion of overloads.

Take for example the following:

```
fn a[T](input: i32) -> i32;
fn a[T](input: T) -> T;

let x: i32 = a(a(a(a(a(a(a(a(a(a(1))))))))));
```

There are 10 nested calls to `a`, and each one has two overloads, so there are 2 ** 10 or 1024
possible combinations of overloads that have to be examined.

Also, the type of the outermost `a` depends on the return types of all the inner calls, so we
can't solve that call site in isolation.

If `a` had more overloads, it would be quite simple to create more combinations than could fit
in RAM.

So what is needed is a way to prune overloads before we expand all of the possibilities.

A different approach would be to trade time for space - rather than expanding all the possibilities,
to generate the permutations one at a time.

There are two problems with this approach. First, it may take a very long time to go through
all of the possibilities.

Second, there could be a large number of permutations that are equally good. We would need a way
other than conversion rankings to decide which permutations are better than others.

## Idea:

One thing that would help if we could partition the condition sets, such that no assignment or
type binding constraint crossed a partion boundary. Then each partition could be solved
independently from the others. However, note that the pessimal example above is not partitionable
in this way. However, it would help in real-world cases.

## Another Idea:

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

