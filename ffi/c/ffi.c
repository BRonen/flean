#include <lean/lean.h>
#include <stdio.h>

uint32_t myAdd(uint32_t a, uint32_t b) {
  printf("a = %d, b = %d\n", a, b);
  return a + b;
}

lean_obj_res myLeanFun() {
  return lean_io_result_mk_ok(lean_box(0));
}
