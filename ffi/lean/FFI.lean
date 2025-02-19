@[extern "myAdd"]
opaque myAdd : UInt32 → UInt32 → UInt32

@[extern "wasd"]
opaque wasd : UInt32 → UInt32

namespace Sqlite

private opaque Nonempty : NonemptyType

private def RawConn : Type := Sqlite.Nonempty.type
def Cursor : Type := Sqlite.Nonempty.type

structure Connection where
  path : String
  conn : RawConn

instance : ToString Connection where
  toString (conn : Connection) := s!"Connection({conn.path})"

inductive Result where
  | ok : Result
  | rows (c : Cursor) : Result
  | error (e : String) : Result

structure Value where
  value : String

@[extern "lean_sqlite_initialize"]
private opaque initSqlite : IO Unit
builtin_initialize initSqlite

@[extern "lean_sqlite_open"]
private opaque openSqlite : String → IO RawConn

@[extern "lean_sqlite_exec"]
private opaque execSqlite : @&RawConn → String → IO Result

@[extern "lean_sqlite_step"]
private opaque stepSqlite : @&Cursor → IO (Bool)

@[extern "lean_sqlite_step_row"]
private opaque stepRowSqlite : @&Cursor → IO (Option (Array String))

@[extern "lean_sqlite_reset_cursor"]
private opaque resetCursorSqlite : @&Cursor → IO Unit

@[extern "lean_sqlite_count_columns_cursor"]
private opaque colsCursorSqlite : @&Cursor → UInt32

@[extern "lean_sqlite_column_text"]
private opaque columnTextSqlite : @&Cursor → UInt32 → IO String

@[extern "lean_sqlite_column_int"]
private opaque columnIntSqlite : @&Cursor → UInt32 → IO Int

def connect (s : String) : IO Connection := do
  let rawconn ← openSqlite s
  pure { path := s, conn := rawconn }

def exec (c : Connection) (query : String) : IO Result :=
  execSqlite c.conn query

def step (c : Cursor) : IO Bool :=
  stepSqlite c

def stepRow (c : Cursor) : IO (Option (Array String)) :=
  stepRowSqlite c

def reset (c : Cursor) : IO Unit :=
  resetCursorSqlite c

def columnText (c : Cursor) (i : UInt32) : IO String :=
  columnTextSqlite c i

def columnInt (c : Cursor) (i : UInt32) : IO Int :=
  columnIntSqlite c i

def colsCount (c : Cursor) : UInt32 :=
  colsCursorSqlite c

def getAllCols' (c : Cursor) (acc : Array (Array String)) (fuel : Nat) : IO (Array (Array String)) := do
  if fuel = 0
  then pure acc
  else let fuel' := fuel - 1
       match ← stepRow c with
       | none => pure acc
       | some cols => getAllCols' c (acc.push cols) fuel'

def getAllCols (c : Cursor) : IO (Array (Array String)) := do
  resetCursorSqlite c
  getAllCols' c Array.empty (colsCount c).toNat

end Sqlite
