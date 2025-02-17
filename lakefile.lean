import Lake
open Lake DSL

package flean

lean_lib Flean

@[default_target]
lean_exe flean where
  root := `Main
  moreLinkArgs := #["-lsqlite3"]

require ffi from "ffi"
require batteries from git "https://github.com/leanprover-community/batteries" @ "main"
