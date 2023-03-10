fiber = require('fiber')
test_run = require('test_run').new()
build_path = os.getenv("BUILDDIR")
package.cpath = build_path..'/test/box/?.so;'..build_path..'/test/box/?.dylib;'..package.cpath
errinj = box.error.injection
netbox = require('net.box')
box.schema.user.grant('guest', 'read,write,execute', 'universe')

conn = netbox.connect(box.cfg.listen)

--
-- gh-4648: box.schema.func.drop() didn't unload a .so/.dylib
-- module. Even if it was unused already. Moreover, recreation of
-- functions from the same module led to its multiple mmaping.
--

current_module_count = errinj.get("ERRINJ_DYN_MODULE_COUNT")
function check_module_count_diff(diff)                          \
    local module_count = errinj.get("ERRINJ_DYN_MODULE_COUNT")  \
    current_module_count = current_module_count + diff          \
    if current_module_count ~= module_count then                \
        return current_module_count, module_count               \
    end                                                         \
end

-- Module is not loaded until any of its functions is called first
-- time.
box.schema.func.create('function1', {language = 'C'})
check_module_count_diff(0)
box.schema.func.drop('function1')
check_module_count_diff(0)

-- Module is unloaded when its function is dropped, and there are
-- no not finished invocations of the function.
box.schema.func.create('function1', {language = 'C'})
check_module_count_diff(0)
conn:call('function1')
check_module_count_diff(1)
box.schema.func.drop('function1')
check_module_count_diff(-1)

-- A not finished invocation of a function from a module prevents
-- its unload. Until the call is finished.
box.schema.func.create('function1', {language = 'C'})
box.schema.func.create('function1.test_sleep', {language = 'C'})
check_module_count_diff(0)

sleep_cond = box.schema.create_space(                           \
    'sleep_cond', {format = {{'fiber', 'string'}}}              \
)
_ = sleep_cond:create_index('pk')

function long_call(...) conn:call('function1.test_sleep', {...}) end
f1 = fiber.create(long_call, sleep_cond.id, 'fiber1')
f2 = fiber.create(long_call, sleep_cond.id, 'fiber2')
test_run:wait_cond(function() return sleep_cond:count() == 2  end)
conn:call('function1')
check_module_count_diff(1)
box.schema.func.drop('function1')
box.schema.func.drop('function1.test_sleep')
check_module_count_diff(0)

sleep_cond:delete({'fiber1'})
test_run:wait_cond(function() return f1:status() == 'dead' end)
check_module_count_diff(0)

sleep_cond:delete({'fiber2'})
test_run:wait_cond(function() return f2:status() == 'dead' end)
check_module_count_diff(-1)

conn:close()
sleep_cond:drop()
box.schema.user.revoke('guest', 'read,write,execute', 'universe')
