-- Manage Lua coroutines

local coro = {}

coro._coros = {}

-- Launch a Lua coroutine: create and resume.
-- Returns:
-- new coroutine
-- bool ok
-- if not ok: error message
-- if ok:     values yielded or returned
function coro.launch(func, ...)
    local co = coroutine.create(func)
    table.insert(coro._coros, co)
    return co, coroutine.resume(co, ...)
end

-- yield to other coroutines even if you don't know whether you're in a
-- created coroutine or the main coroutine
function coro.yield(...)
    if coroutine.running() then
        -- this is a real coroutine, yield normally
        return coroutine.yield(...)
    else
        -- This is the main coroutine: coroutine.yield() doesn't work.
        -- But we can take a spin through previously-launched coroutines.
        -- Walk a copy of coro._coros in case any of these coroutines launches
        -- another: next() forbids creating new entries during traversal.
        for co in coro._live_coros_iter, table.clone(coro._coros) do
            co.resume()
        end
    end
end

-- Walk coro._coros table, returning running or suspended coroutines.
-- Once a coroutine becomes dead, remove it from _coros and don't return it.
function coro._live_coros()
    return coro._live_coros_iter, coro._coros
end

-- iterator function for _live_coros()
function coro._live_coros_iter(t, idx)
    local k, co = next(t, idx)
    while k and coroutine.status(co) == 'dead' do
--      t[k] = nil
        -- See coro.yield(): sometimes we traverse a copy of _coros, but if we
        -- discover a dead coroutine in that copy, delete it from _coros
        -- anyway. Deleting it from a temporary copy does nothing.
        coro._coros[k] = nil
        coroutine.close(co)
        k, co = next(t, k)
    end
    return co
end

return coro
