# Recent thoughts on syntax.

1) We don't need the keyword 'fn' for member functions:

  ```ts
  class A {
    new() {}

    print -> String {}
  }
  ```

2) Getters and setters: use ':' rather than ->

  ```ts
  class A {
    get size: int {

    }

    // For setters, the last parameter is implicit
    set size(value): int {

    }
  }
  ```

# Type alias and type extensions.

We really need to figure out a solution here, because 'Optional' is one of the key classes in
the core library, and our strategy depends on being able to add methods to a named union.

The problem is one of scopes and member lookup.

We want to avoid "magical" lookups. A lookup is "magical" when it can be influenced by source
files that are not in scope at the time of the lookup. An example:

  * Source file A.te defines a type.
  * Source file B.te imports the type from B.te, and looks up some symbol defined in that type.
  * Later, someone adds another source file C.te, which extends the type defined in A.te. Source
    file B is not modified and has no knowledge of C.te.
  * A lookup would be "magical" if the presence of C in the compilation would affect B.te. To put
    it another way, it means that symbol lookups would be unpredictable; worse, trying to find
    all of the factors which could affect the lookup of a symbol would be an unbounded problem.

Instead, we want to make the rule such that B and only B gets to decide which scopes are searched
when compiling B.

The mechanism for introducing scopes from other modules is imports. Thus, B must explicitly import
any extension types that it wants to use.

So the code would have to look like this:

  ```ts
  import { A } from 'a';
  import { A } from 'a-ext';
  ```

This means that there are multiple definitions of A. That in itself is OK, since we do allow
overloading of types and functions. However, the two A's must be "compatible".

Normally overloaded symbols must be defined in the same scope. You can have two 'class A' definitions, so long as both of them are defined in the same module. You can't import a 'class A'
from one module and a different 'class A' from a different module.

However, extensions are a bit weird, in that there are actually two scopes: the scope in which the
original (unextended) definition was defined, and the scope in which the extension occurs. Extension
just augments the symbol, it does not *redefine* it. To put it another way, importing an extension
doesn't actually import a new symbol - rather it has a side effect of adding definitions to that
symbol.

Note that these import side effects should operate transitively - if you import a definition from
a file which exports both the original symbol and also imports the extension, you should get the
extended definition.

Now, about these side effects: they need to be local to the module importing the extension. We can't
modify the original definition, because that definition is shared between all of the modules that
imported it.

The simplest thing, probably, is to store the extensions in the import scope. However, that only
helps when doing unqualified name lookups. It doesn't help when doing memer name lookups like a.c
(where a is a variable whose type is the extended type A). The reason it doesn't help is because the way we store an expression like a.c is to reduce the type of 'a' to a singlular type expression.

Another approach would be to put a proxy in front of every extended type, kind of like a union
filesystem - but I don't like this because the proxy would have to be very heavy (with a lot
of API methods to support all the features of a type). It would affect every single operation
on type expressions (there are a lot) rather than just the operation we want: symbol lookup.

(Actually types have a smaller surface than definitions, so this might not be *quite* so bad.)

Another idea would be to have a second field on all expressions, similar to the type field,
which contains a reference to the augmentations. This is a bit painful, however, in that we would
need to specialize the augmentations. You see, you can enhance a named type only; you can't extend
a specialization.

Another approach would be to maintain a separate list of intercept scopes - so that when you
looked up a symbol in scope A, it would also look up symbols in scope A1. There are several
difficulties to overcome. One is scope lifetime: not all scopes are global.

Basically intercept scopes would work something like the following: For each imported "extend"
definition, we build a map at the module level which maps the extended symbol to the list
of known extensions. Then, whenever we look up a symbol in that scope, we consult the extension
map to see if there are additional lookup results.
