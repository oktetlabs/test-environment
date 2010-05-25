---
-- Chunked output - chunk manager module
--
-- Copyright (C) 2010 Test Environment authors (see file AUTHORS in the
-- root directory of the distribution).
-- 
-- Test Environment is free software; you can redistribute it and/or
-- modify it under the terms of the GNU General Public License as
-- published by the Free Software Foundation; either version 2 of
-- the License, or (at your option) any later version.
-- 
-- Test Environment is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
-- 
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 59 Temple Place, Suite 330, Boston,
-- MA  02111-1307  USA
-- 
-- @author Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
-- 
-- @release $Id$
--

local oo    = require("loop.base")
local co    = {}
co.manager  = oo.class({
                        max_mem  = nil, --- Maximum memory for chunks
                        used_mem = 0,   --- Memory used by in-memory chunks
                       })

---
-- Create a chunk manager.
--
-- @param max_mem   Maximum amount of memory allowed to be taken up by
--                  memory-based chunk contents.
--
-- @return New chunk manager instance.
--
function co.manager:__init(max_mem)
    assert(type(max_mem) == "number")
    assert(math.floor(max_mem) == max_mem)
    assert(max_mem >= 0)

    return oo.rawnew(self, {max_mem = max_mem})
end

---
-- Create a temporary file, raising an error in case of failure.
--
-- @return New temporary file handle
--
local function xtmpfile()
    local file, err

    file, err = io.tmpfile()
    if file == nil then
        error("failed creating a temporary file: " .. err)
    end
end

---
-- Request an amount of memory for a memory-based chunk contents
--
-- @param chunk Chunk requesting the memory.
-- @param size  Amount of memory being requested.
--
function co.manager:request_mem(chunk, size)
    assert(oo.instanceof(chunk, require("co.chunk")))
    assert(type(size) == "number")
    assert(math.floor(size) == size)
    assert(size >= 0)

    self.used_mem = self.used_mem + size
end

---
-- Return an amount of memory used by a memory-based chunk contents
--
-- @param chunk Chunk returning the memory.
-- @param size  Amount of memory being returned.
--
function co.manager:return_mem(chunk, size)
    assert(oo.instanceof(chunk, require("co.chunk")))
    assert(type(size) == "number")
    assert(math.floor(size) == size)
    assert(size >= 0)
    assert(size <= self.used_mem)

    self.used_mem = self.used_mem - size
end

---
-- Inform about a finished chunk
--
-- @param chunk Chunk being finished.
--
function co.manager:finished(chunk)
    local prev

    assert(oo.instanceof(chunk, require("co.chunk")))

    --
    -- Try to reach the first chunk via finished chunks
    --
    while chunk.prev ~= nil do
        chunk = chunk.prev
        if not chunk.finished then
            return
        end
    end

    --
    -- Collapse as much chunks as possible
    --
    while true do
        prev = chunk
        chunk = prev.next
        if chunk == nil or not chunk.finished then
            break
        end
        -- Detach the previous chunk
        chunk.prev = nil
        prev.next = nil
        -- Relocate the chunk to the previous chunk's storage
        chunk:relocate(prev:yield())
    end
end

return co.manager
