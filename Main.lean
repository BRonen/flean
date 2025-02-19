import FFI

open IO

def double (f : Nat -> Nat) n := f (f n)
def inc := fun n => n + 1

def printUser (fuel : Nat) (cursor : Sqlite.Cursor) : IO Unit := do
  if fuel = 0 then
    pure ()
  else
    match ← Sqlite.step cursor with
    | false => pure ()
    | true => do
      println s!"id  : {← Sqlite.columnInt cursor 0}"
      println s!"name: {← Sqlite.columnText cursor 1}"
      printUser (fuel - 1) cursor

def main : IO Unit := do
  let conn ← Sqlite.connect "test.sqlite3"
  match ← Sqlite.exec conn "select count(1) from users;" with
  | Sqlite.Result.rows c =>
    println s!"{Sqlite.colsCount c}"
    match ← Sqlite.step c with
    | false => println "error"
    | true => println "step"
    let count ← Sqlite.columnInt c 0
    println s!"count: {count}"
    match ← Sqlite.exec conn "select * from users;" with
     | Sqlite.Result.error e => println e
     | Sqlite.Result.ok => println "ok"
     | Sqlite.Result.rows c =>
       printUser count.toNat c
  | _ => println "error"
  println s!"Hello, {conn}"
