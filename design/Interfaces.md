# Notes on interfaces and traits

I've run into a serious problem with interfaces - you can't dynamic-cast from an interface to
another interface if the second interface is not explicitly mentioned in the inheritance list
of the concrete type (in other words, the second interface uses structural typing.)

The reason is as follows:

A concrete type can be converted into an interface by creating, at compile-time, a jump table which maps the methods of the type to the methods of the interface. The 'interface value' is then a tuple
which contains both the pointer to the object and the pointer to the jump table. Doing it this way
avoids the need for the class to have a jump table of all methods, which prevents linker bloat
(the tendency of object-oriented type systems to pull into the executable large amounts of code
which is never executed.)

An interface can be down-cast into a concrete type by mantaining the inheritance tree as a run-time
data structure, which can be accessed via the object pointer. By scanning this table, you can
determine whether or not the object is a given concrete type, or a given interface type.

However, this only works if the destination type is explicitly mentioned in the inheritance list
for the concrete type. It does not work for interfaces which are structurally equivalent, because
those would not be listed in the inheritance list.

The worst-case situation is one in which the down-cast is to a type which is unknown to the original
concrete type; in this case, the original concrete type is not present in the calculation, so all
we have to go on is some supertype (either class or interface) and the target interface. There isn't
enough knowledge to map all of the methods of the concrete type to the target interface type's
methods.

Note that even if we did keep a dispatch table of every method (link bloat), we would have to also
construct the interface dispatch table at runtime, since it could not be done at compile time.

An example use case is "toString" - we want to be able to cast any interface to a "Stringable"
interface, which has one method: toString():

```cs
interface Stringable {
  fn toString() -> String;
}

fn printObjects(objects: [Object]) {
  for (const obj in objects) {
    match obj {
      s: Stringable => { console.write(s.toString()) }
      default => { console.write("Cannot be converted into a string.") }
    }
  }
}
```

Now, if the object in question explicitly inherits from `Stringable`, we have no problem. The match
statement can look through the inheritance tree of the object and find the pre-built dispatch
table for that (concrete type, interface type) combination.

However, if the object does not explicitly inherit from `Stringable`, we're stuck. At this point in
the code, the compiler has no idea what concrete type `obj` is.

**So here's an idea**: what if we only build a dispatch table of the complete class when we cast a
concrete type into an interface type? It means that concrete classes that are always used concretely
don't have references to methods that are not called. Basically the interface would have two tables;
one for the interface, and one for the concrete type; the second table would be an associative array
of method descriptors and method pointers. It means we still have to do construction of interface
tables at runtime, but oh well... :( Also, do we need to construct the same table for explicitly
declared interfaces? I think it means we do, which means that there's no savings.

One problem with this is class extension: we may not know all of the methods of the class at the
point where the cast to interface occurs, so the dispatch table will be incomplete.

**Nope**: I think what we are going to do instead, for the time being, is have a dispatch table
of every public method. However, rather than embedding the string names, I will have the methods
point to linker symbols whose names are the encoded method signatures. These will of course
be sorted like in Go. Interface dispatch tables will be created on the fly if needed.

