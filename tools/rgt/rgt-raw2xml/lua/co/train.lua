---
-- Chunked output - chunk train module
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

local co    = {}
co.chunk    = require("co.chunk")
co.train    = {}

co.train.__index = co.train

---
-- Create new instance of a chunk train.
--
-- @param file      Output file handle.
-- @param max_mem   Maximum amount of memory allowed to be taken up by
--                  memory-based chunk contents
--
-- @return New chunk train instance.
--
function co.train.new(file, max_mem)
    local self   = setmetatable({max_mem = max_mem,
                                 used_mem = 0},
                                co.train)

    local chunk  = co.chunk.new(self, file, 0)

    self.first = chunk
    self.last = chunk

    return self
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
-- Add a new based chunk.
--
-- @return New chunk
--
function co.train:new_chunk()
    local chunk = co.chunk.new(self, {}, 0)

    self.last.next = chunk
    self.last = chunk

    return chunk
end

---
-- Check if a train is finished.
--
-- @return True if the train is finished, false otherwise.
--
function co.train:finished()
    return self.first.finished and self.first.next == nil
end

---
-- Request an amount of memory for a memory-based chunk contents
--
-- @param size  Amount of memory being requested.
--
function co.train:chunk_request_mem(size)
    self.used_mem = self.used_mem + size
end

---
-- Return an amount of memory used by a memory-based chunk contents
--
-- @param size  Amount of memory being returned.
--
function co.train:chunk_return_mem(size)
    assert(size <= self.used_mem)
    self.used_mem = self.used_mem - size
end

---
-- Inform about a finished chunk
--
function co.train:chunk_finished()
    local prev, chunk

    --
    -- Try to collapse the chunks starting from the first
    --
    chunk = self.first

    if not chunk.finished then
        return
    end

    while true do
        prev = chunk
        chunk = prev.next
        if chunk == nil or not chunk.finished then
            break
        end
        self.first = chunk
        chunk:relocate(prev:yield())
    end
end

return co.train
