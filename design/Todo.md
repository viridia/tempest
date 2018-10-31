# TODOs

* Array and String
  * Needs Slice
    * Needs Iterator
      * Needs Interfaces
    * Needs Iterable
      * Needs Trait method calling
      * Needs ensure requirements satisfied
* Break out visitors into separate files.
* Implement const members
* Implement Slice and Iterable
* Finish infix operators
* Constant infix operators
  * String
  * Boolean
* Super calls
* Unary operators
  * NOT
* Boolean coercion
  * isTruthy function
  * NOT / if()
* ADL lookup
* Member access operator
  * Method overloads
* isNarrower, equal, assignable have lots of unimplemented cases. Need tests.
  * inferred source types
  * structural typing in unification
  * structural typing for field definitions
  * narrower doesn't handle type vars (except inferred)
* Support for "core" libraries
  * Multiple compilation units
  * Extern symbols
* Statements:
  * For
  * Match
* Traits
* Type inference for constructor calls
* Unify structural typing
* Unify function types
* Expression callables
* Private / Protected visibility tests
* Class method overload tests
* Finish Support for 'self' and 'super'
* Type parameter defaults
* Get rid of classes we don't need
* More statement types
* Extend unification to not need isAssignable
* Methods returning 'self'
* Self as a template parameter.
* Downcasting
* Destructured assignment (arrays)
* Destructured assignment (objects)
* Pre/post increment/decrement
* throw
* Immutability
* Range expressions
* Anonymous functions
* Closures
* Array literals
* Set literals
* Local variables containing unions of ref and non-ref types, needs smart GC.

## TODOs from code:

* format() should escape control chars.
* decode std::error codes from LLVM fs methods.
* cgTypeBuilder doesn't handle varargs
* integer literal suffixes
* varargs in function type expressions
* handle aliases in isAssignable
* finish structural typing in isAssignable
* implements.cpp uses isAssignable in a way that may be incorrect
* narrower.cpp compares function signatures in a way that may be incorrect
* subtype.cpp structural typing
* evalconst.cpp add cyclic checks for variable references
* defn.cpp implement 'friend'
* constraintsolver needs tests for error cases
* constraintsolver "more specific" should not consider return types in some cases.
* constraintsolver support other types of constraints
* paramassignments should include argIndex in rejection
* check for circular imports
* make sure we don't load a module twice in one file.
* support for 'int' and 'uint' types.
* support for 'char' type.
* nameresolution.cpp: various TODOs
* keyword argument support
* variadic params should be slice type
* transform default args
* coerce empty collections to boolean type.
* Don't allow specialization of specializations - flatten first
