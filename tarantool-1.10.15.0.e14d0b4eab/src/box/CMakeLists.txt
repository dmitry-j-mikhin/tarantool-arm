file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/src/box/lua)

set(lua_sources)
lua_source(lua_sources lua/load_cfg.lua)
lua_source(lua_sources lua/schema.lua)
lua_source(lua_sources lua/tuple.lua)
lua_source(lua_sources lua/session.lua)
lua_source(lua_sources lua/checkpoint_daemon.lua)
lua_source(lua_sources lua/feedback_daemon.lua)
lua_source(lua_sources lua/net_box.lua)
lua_source(lua_sources lua/upgrade.lua)
lua_source(lua_sources lua/console.lua)
lua_source(lua_sources lua/xlog.lua)
set(bin_sources)
bin_source(bin_sources bootstrap.snap bootstrap.h)

add_custom_target(box_generate_lua_sources
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src/box
    DEPENDS ${lua_sources})
set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${lua_sources})

include_directories(${ZSTD_INCLUDE_DIRS})
include_directories(${CMAKE_BINARY_DIR}/src/box)

add_library(box_error STATIC error.cc errcode.c vclock.c)
target_link_libraries(box_error core stat)

add_library(vclock STATIC vclock.c)
target_link_libraries(vclock core)

add_library(xrow STATIC xrow.c iproto_constants.c)
target_link_libraries(xrow server core small vclock misc box_error
                      scramble ${MSGPUCK_LIBRARIES})

add_library(tuple STATIC
    tuple.c
    tuple_format.c
    tuple_update.c
    tuple_compare.cc
    tuple_extract_key.cc
    tuple_hash.cc
    tuple_bloom.c
    tuple_dictionary.c
    key_def.c
    coll_id_def.c
    coll_id.c
    coll_id_cache.c
    field_def.c
    opt_def.c
)
target_link_libraries(tuple json_path box_error core ${MSGPUCK_LIBRARIES} ${ICU_LIBRARIES} misc bit)

add_library(xlog STATIC xlog.c)
target_link_libraries(xlog core box_error crc32 ${ZSTD_LIBRARIES})

add_library(box STATIC
    iproto.cc
    error.cc
    xrow_io.cc
    tuple_convert.c
    identifier.c
    index.cc
    index_def.c
    iterator_type.c
    memtx_hash.c
    memtx_tree.c
    memtx_rtree.c
    memtx_bitset.c
    engine.c
    memtx_engine.c
    memtx_space.c
    sysview.c
    blackhole.c
    vinyl.c
    vy_stmt.c
    vy_mem.c
    vy_run.c
    vy_range.c
    vy_lsm.c
    vy_tx.c
    vy_write_iterator.c
    vy_read_iterator.c
    vy_point_lookup.c
    vy_cache.c
    vy_log.c
    vy_upsert.c
    vy_history.c
    vy_read_set.c
    vy_scheduler.c
    vy_regulator.c
    vy_quota.c
    request.c
    space.c
    space_def.c
    sequence.c
    func.c
    func_def.c
    alter.cc
    schema.cc
    schema_def.c
    session.cc
    port.c
    txn.c
    box.cc
    gc.c
    user_def.c
    user.cc
    authentication.cc
    replication.cc
    recovery.cc
    xstream.cc
    applier.cc
    relay.cc
    journal.c
    wal.c
    call.c
    ibuf.c
    ${lua_sources}
    lua/init.c
    lua/call.c
    lua/cfg.cc
    lua/console.c
    lua/tuple.c
    lua/slab.c
    lua/index.c
    lua/space.cc
    lua/sequence.c
    lua/misc.cc
    lua/info.c
    lua/stat.c
    lua/ctl.c
    lua/error.cc
    lua/session.c
    lua/net_box.c
    lua/xlog.c
    ${bin_sources})

target_link_libraries(box box_error tuple stat xrow xlog vclock crc32 scramble
                      ${common_libraries})
add_dependencies(box build_bundled_libs)
