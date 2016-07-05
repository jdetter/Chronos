* Boot tools should all be python functions.
* Boot sign should just be a python function: 
  ```python
  
  file_ = open("filename", "r+b")
  file_.seek(510)
  file_.write(0xAA55)
  file.clode()
  ```
* Need to create a global object, or some way of passing items around.
  This should be idiomatic, global scope and files can get very confusing.
  That, or keep some sort of system/spec for returning from subbuilds.

* Move `Ld()` to `Program()`, this is the scons method of ldmodules.
