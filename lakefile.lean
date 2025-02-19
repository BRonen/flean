import Lake
open Lake DSL

package flean

lean_lib Flean

@[default_target]
lean_exe flean where
  root := `Main
  moreLinkArgs := #["-lsqlite3"]

require ffi from "ffi"
require sqlite from git "https://github.com/bronen/sqlite3-lean4" @ "master"
