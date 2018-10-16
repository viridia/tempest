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
