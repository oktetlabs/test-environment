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

local oo    = require("loop.simple")
local co    = {}
co.manager  = oo.class({
                        max_mem  = nil, --- Maximum memory for chunks
                        used_mem = 0,   --- Memory used by in-memory chunks
                        first    = nil  --- First chunk
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
-- Set first chunk - the chunk to whose storage all the successive chunks
-- will be relocated, once finished.
--
-- @param chunk The first chunk.
--
function co.manager:set_first(chunk)
    assert(oo.instanceof(chunk, require("co.chunk")))
    self.first = chunk
end


---
-- Get first chunk - the chunk to whose storage all the successive chunks
-- are relocated, once finished.
--
-- @return The first chunk.
--
function co.manager:get_first(chunk)
    return self.first
end


---
-- Collapse all finished memory-based chunk strips into files, until there
-- are no more of them, or enough memory is freed for the requesting chunk.
--
-- @param requesting_chunk  The chunk requesting the memory.
-- @param size              Amount of memory requested.
--
-- @return True if enough memory was freed, false otherwise; then amount of
--         memory requested (0 if the requested chunk was displaced,
--         otherwise the same).
--
function co.manager:displace_finished_strips(requesting_chunk, size)
    local prev, chunk, next

    assert(oo.instanceof(requesting_chunk, require("co.chunk")))
    assert(not requesting_chunk.finished)
    assert(type(size) == "number" and
           math.floor(size) == size and
           size >= 0)

    chunk = self.first
    while (self.used_mem + size) > ((self.max_mem * 3) / 4) do
        if not chunk.finished then
            prev = chunk
            chunk = chunk.next
        else
            if type(chunk.storage) == "table" then
                -- xtmpfile is defined in raw2xml.c
                chunk:relocate(xtmpfile(), 0)
            end
            -- Chunk is finished and file-based (now)
            while true do
                -- If the resulting memory is less than or equal to 3/4 of
                -- the maximum (also satisfies outer loop stopping
                -- condition)
                if (self.used_mem + size) <= ((self.max_mem * 3) / 4) then
                    return true, size
                end
                next = chunk.next
                -- If there is no next chunk or it is file-based
                if next == nil or type(next.storage) ~= "table" then
                    prev = chunk
                    chunk = next
                    break
                end
                -- Relocate next chunk into this chunk storage
                next:relocate(chunk:yield())
                -- Remove this chunk
                if prev == nil then
                    self.first = next
                else
                    prev.next = next
                end

                -- Move to the next chunk
                chunk = next

                if not chunk.finished then
                    -- Nullify request if the requesting chunk was displaced
                    if chunk == requesting_chunk then
                        size = 0
                    end
                    break
                end
            end
        end
        if chunk == nil then
            break
        end
    end

    return false, size
end


---
-- Displace all chunks to files (biggest first), stop when there is enough
-- memory for the requesting chunk
--
-- @param requesting_chunk  The chunk requesting the memory.
-- @param size              Amount of memory requested.
--
-- @return Amount of memory requested (0 if the requested chunk was
--         displaced, otherwise the same).
--
function co.manager:displace_all(requesting_chunk, size)
    local list
    local chunk

    assert(oo.instanceof(requesting_chunk, require("co.chunk")))
    assert(not requesting_chunk.finished)
    assert(type(size) == "number" and
           math.floor(size) == size and
           size >= 0)

    -- Build a list of non-empty memory-based chunks
    list = {}
    chunk = self.first
    repeat
        if type(chunk.storage) == "table" then
            local size = (chunk == requesting_chunk) and size or chunk.size
            if size > 0 then
                table.insert(list, {chunk, size})
            end
        end
        chunk = chunk.next
    until chunk == nil

    -- Sort it bigger chunks first
    table.sort(list, function (x, y) return x[2] >= y[2] end)

    -- Displace until half the memory is free
    for i, chunk_and_size in ipairs(list) do
        -- xtmpfile is defined in raw2xml.c
        chunk_and_size[1]:relocate(xtmpfile(), 0)
        -- Nullify request if the requesting chunk was displaced
        if chunk_and_size[1] == requesting_chunk then
            size = 0
        end
        if (self.used_mem + size) <= (self.max_mem / 2) then
            break
        end
    end

    return size
end


---
-- Collapse finished file-based chunk strips
--
function co.manager:collapse_file_strips()
    local prev, chunk, next

    chunk = self.first
    repeat
        if not chunk.finished or type(chunk.storage) == "table" then
            prev = chunk
            chunk = chunk.next
        else
            repeat
                -- Retrieve next chunk
                next = chunk.next
                -- If there is no next chunk or it is memory-based
                if next == nil or type(next.storage) == "table" then
                    prev = chunk
                    chunk = next
                    break
                end
                -- Relocate next chunk into this chunk storage
                next:relocate(chunk:yield())
                -- Remove this chunk
                if prev == nil then
                    self.first = next
                else
                    prev.next = next
                end
                -- Move to the next chunk
                chunk = next
            until not chunk.finished
        end
    until chunk == nil
end


---
-- Request an amount of memory for a memory-based chunk contents
--
-- @param chunk Chunk requesting the memory.
-- @param size  Amount of memory being requested.
--
function co.manager:request_mem(requesting_chunk, size)
    local new_mem, first_attempt

    assert(oo.instanceof(requesting_chunk, require("co.chunk")))
    assert(not requesting_chunk.finished)
    assert(type(size) == "number" and
           math.floor(size) == size and
           size >= 0)

    new_mem = self.used_mem + size

    -- If the chunk still fits
    if new_mem <= self.max_mem then
        self.used_mem = new_mem
        return
    end

    first_attempt = true
    while true do
        local ok, err

        -- Displace chunks to files until enough memory is freed
        ok, err = pcall(function ()
                            local satisfied

                            satisfied, size =
                                self:displace_finished_strips(
                                                        requesting_chunk,
                                                        size)

                            if not satisfied then
                                size = self:displace_all(requesting_chunk,
                                                         size)
                            end
                        end)

        -- If succeeded
        if ok then
            self.used_mem = self.used_mem + size
            return
        else
            -- If it was the first attempt and it is xtmpfile (defined in
            -- raw2xml.c) telling us we're out of filehandles
            if first_attempt and
               type(err) == "string" and
               err:match("too many open files$") then
                -- Try to minimize the number of open files
                self:collapse_file_strips()
                first_attempt = false
            else
                error(err)
            end
        end
    end
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
    local prev, next

    assert(self.first ~= nil)
    assert(oo.instanceof(chunk, require("co.chunk")))

    -- chunk argument is not used currently

    -- Relocate as much chunks as possible to the first chunk storage
    chunk = self.first
    while chunk.finished do
        next = chunk.next
        if next == nil then
            break
        end
        next:relocate(chunk:yield())
        chunk = next
    end
    self.first = chunk
end


---
-- Dump chunk map to a file
--
-- @param file  The file to dump to
--
function co.manager:dump(file)
    local chunk = self.first

    file:write(("%u/%u %3.0f%%\n"):
                format(self.used_mem, self.max_mem,
                       self.max_mem == 0
                           and 100
                            or self.used_mem * 100 / self.max_mem))
    while chunk ~= nil do
        file:write(
                tostring(chunk) .. " " ..
                ("%6s %10u%s\n"):
                    format(type(chunk.storage) == "table"
                            and "memory"
                             or "file",
                           chunk.size,
                           chunk.finished and " finished" or ""))
        chunk = chunk.next
    end
end

return co.manager
