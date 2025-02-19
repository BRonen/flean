#include <lean/lean.h>
#include <sqlite3.h>
#include <stdint.h>
#include <stdio.h>

uint32_t myAdd(uint32_t a, uint32_t b) {
  printf("a = %d, b = %d\n", a, b);
  return a + b;
}

lean_obj_res myLeanFun() {
  return lean_io_result_mk_ok(lean_box(0));
}

typedef struct {
  sqlite3_stmt* stmt;
  int cols;
} cursor_t;

lean_external_class* g_sqlite_object_external_class = NULL;
lean_external_class* g_sqlite_cursor_external_class = NULL;

void noop_foreach(void* mod, b_lean_obj_arg fn) {}

lean_object* box_connection(sqlite3 *conn) {
  return lean_alloc_external(g_sqlite_object_external_class, conn);
}

sqlite3* unbox_connection(lean_object* o) {
  return (sqlite3*) lean_get_external_data(o);
}

lean_object* box_cursor(cursor_t* cursor) {
  return lean_alloc_external(g_sqlite_cursor_external_class, cursor);
}

cursor_t* unbox_cursor(lean_object* o) {
  return (cursor_t*) lean_get_external_data(o);
}

void connection_finalize(void* conn) {
  if (!conn) return;

  sqlite3_close(conn);
}

void cursor_finalize(void* cursor_ptr) {
  cursor_t* cursor = (cursor_t*) cursor_ptr;

  if (!cursor->stmt) return;
  sqlite3_finalize(cursor->stmt);

  if (!cursor) return;
  free(cursor);
}

lean_obj_res lean_sqlite_initialize() {
  g_sqlite_object_external_class = lean_register_external_class(connection_finalize, noop_foreach);
  g_sqlite_cursor_external_class = lean_register_external_class(cursor_finalize, noop_foreach);
  return lean_io_result_mk_ok(lean_box(0));
}

lean_obj_res lean_sqlite_open(b_lean_obj_arg path) {
  const char* path_str = lean_string_cstr(path);
  sqlite3* conn = malloc(sizeof(sqlite3*));

  int c = sqlite3_open(path_str, &conn);

  if (c == SQLITE_OK)
    return lean_io_result_mk_ok(box_connection(conn));

  lean_object *err = lean_mk_string(sqlite3_errmsg(conn));

  sqlite3_close(conn);

  return lean_io_result_mk_error(lean_mk_io_error_other_error(c, err));
}

lean_obj_res lean_sqlite_exec(b_lean_obj_arg conn_box, b_lean_obj_arg query_str) {
  sqlite3* conn = unbox_connection(conn_box);
  const char* query = lean_string_cstr(query_str);

  cursor_t* cursor = malloc(sizeof(cursor_t*));

  int c = sqlite3_prepare_v2(conn, query, -1, &cursor->stmt, NULL);

  if (c != SQLITE_OK) {
    lean_object* err = lean_mk_string(sqlite3_errmsg(conn));
    free(cursor);
    return lean_io_result_mk_error(lean_mk_io_error_other_error(c, err));
  }

  cursor->cols = (uint32_t) sqlite3_column_count(cursor->stmt);

  if (cursor->cols == 0)
    return lean_io_result_mk_ok(lean_box(0));

  lean_object *res = lean_alloc_ctor(1, 1, 0);
  lean_ctor_set(res, 0, box_cursor(cursor));
  return lean_io_result_mk_ok(res);
}

lean_obj_res lean_sqlite_column_text(b_lean_obj_arg cursor_box, uint32_t col) {
  cursor_t* cursor = unbox_cursor(cursor_box);

  printf("col: %d\n", col);

  const unsigned char* text = sqlite3_column_text(cursor->stmt, col);
  printf("text: %s\n", text);

  lean_object* s = lean_mk_string((const char*) text);

  return lean_io_result_mk_ok(s);
}

lean_obj_res lean_sqlite_column_int(b_lean_obj_arg cursor_box, uint32_t col) {
  cursor_t* cursor = unbox_cursor(cursor_box);
  
  const int integer = sqlite3_column_int(cursor->stmt, col);

  return lean_io_result_mk_ok(lean_box(integer));
}

lean_obj_res lean_sqlite_step(b_lean_obj_arg cursor_box) {
  cursor_t* cursor = unbox_cursor(cursor_box);

  int c = sqlite3_step(cursor->stmt);

  if (c == SQLITE_ROW) {
    return lean_io_result_mk_ok(lean_box(1));
  }

  if (c == SQLITE_DONE) {
    return lean_io_result_mk_ok(lean_box(0));
  }
  
  lean_object* err = lean_mk_string(sqlite3_errmsg(sqlite3_db_handle(cursor->stmt)));
  return lean_io_result_mk_error(lean_mk_io_error_other_error(c, err));
}

lean_obj_res lean_sqlite_step_row(b_lean_obj_arg cursor_box) {
  cursor_t* cursor = unbox_cursor(cursor_box);

  int c = sqlite3_step(cursor->stmt);

  if (c == SQLITE_ROW) {
    lean_object* row = lean_alloc_array(0, cursor->cols);

    for (int i = 0; i < cursor->cols; i++) {
      const unsigned char* text = sqlite3_column_text(cursor->stmt, i);
      lean_object* s = lean_mk_string((const char*) text);

      lean_array_push(row, s);
    }

    lean_object *some_row = lean_alloc_ctor(1, 1, 0);
    lean_ctor_set(some_row, 0, row);

    return lean_io_result_mk_ok(some_row);
  }

  if (c == SQLITE_DONE) {
    lean_object* none_row = lean_alloc_ctor(0, 0, 0);
    return lean_io_result_mk_ok(none_row);
  }

  lean_object* err = lean_mk_string(sqlite3_errmsg(sqlite3_db_handle(cursor->stmt)));
  return lean_io_result_mk_error(lean_mk_io_error_other_error(c, err));
}

lean_obj_res lean_sqlite_reset_cursor(b_lean_obj_arg cursor_box) {
  cursor_t* cursor = unbox_cursor(cursor_box);

  int c = sqlite3_reset(cursor->stmt);

  if (c == SQLITE_OK)
    return lean_io_result_mk_ok(lean_box(0));

  lean_object *err = lean_mk_string(sqlite3_errmsg(sqlite3_db_handle(cursor->stmt)));
  return lean_io_result_mk_error(lean_mk_io_error_other_error(c, err));
}

uint32_t lean_sqlite_count_columns_cursor(b_lean_obj_arg cursor_box) {
  cursor_t* cursor = unbox_cursor(cursor_box);

  printf("%u - %d\n", cursor->cols, cursor->cols);

  return cursor->cols;
}

int callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}

uint32_t wasd(uint32_t a) {
  printf("hello world\n");

  sqlite3 *db;
  char *zErrMsg = 0;
  int err;

  err = sqlite3_open("test.sqlite3", &db);
  if (err != SQLITE_OK) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return(1);
  }

  err = sqlite3_exec(db, "select 1 = 1;", callback, 0, &zErrMsg);
  if (err != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }

  sqlite3_close(db);

  return a;
}
