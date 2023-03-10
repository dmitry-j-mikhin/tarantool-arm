-- Use the default LuaJIT globals.
std = 'luajit'

-- These files are inherited from the vanilla LuaJIT or different
-- test suites and need to be coherent with the upstream.
exclude_files = {
  'dynasm/',
  'src/',
  'test/LuaJIT-tests/',
  'test/PUC-Rio-Lua-5.1-tests/',
  'test/lua-Harness-tests/',
}
