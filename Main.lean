import Sqlite.FFI

open IO

def double (f : Nat -> Nat) n := f (f n)
def inc := fun n => n + 1

def printUser (fuel : Nat) (cursor : Sqlite.FFI.Cursor) : IO Unit := do
  if fuel = 0 then
    pure ()
  else
    match ← Sqlite.FFI.step cursor with
    | false => pure ()
    | true => do
      println s!"id  : {← Sqlite.FFI.columnInt cursor 0}"
      println s!"name: {← Sqlite.FFI.columnText cursor 1}"
      printUser (fuel - 1) cursor

def main : IO Unit := do
  let conn ← Sqlite.FFI.connect "test.sqlite3"
  match ← Sqlite.FFI.exec conn "select count(1) from users;" with
  | Sqlite.FFI.Result.rows c =>
    println s!"{Sqlite.FFI.colsCount c}"
    match ← Sqlite.FFI.step c with
    | false => println "error"
    | true => println "step"
    let count ← Sqlite.FFI.columnInt c 0
    println s!"count: {count}"
    match ← Sqlite.FFI.exec conn "select * from users;" with
     | Sqlite.FFI.Result.error e => println e
     | Sqlite.FFI.Result.ok => println "ok"
     | Sqlite.FFI.Result.rows c =>
       printUser count.toNat c
  | _ => println "error"
  println s!"Hello, {conn}"
