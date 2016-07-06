* Need to create a global object, or some way of passing items around.
  This should be idiomatic, global scope and files can get very confusing.
  That, or keep some sort of system/spec for returning from subbuilds.

* Move `Ld()` to `Program()`, this is the scons method of ldmodules.
