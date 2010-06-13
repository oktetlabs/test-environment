---
-- Chunked output - chunk module.
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
co.buf      = require("co.buf")
co.chunk    = oo.class({
                        buf_size = 32768,   --- Relocation buffer size
                        manager  = nil,     --- Chunk manager
                        storage  = nil,     --- Storage: buf or file
                        size     = 0,       --- Storage contents size
                        finished = nil,     --- "Finished" flag
                        next     = nil,     --- Next chunk
                       })

---
-- Check if the storage is memory-based (a buffer).
--
-- @param storage   Storage to check.
--
-- @return True if the storage is memory-based, false otherwise.
--
local function storage_is_mem(storage)
    return getmetatable(storage) == co.buf
end


---
-- Calculate/retrieve size of a storage (file or table) contents.
--
-- @param storage   Storage to calculate/retrieve contents size of.
--
-- @return Storage contents size, bytes.
--
local function storage_size(storage)
    local size

    if storage_is_mem(storage) then
        return #storage
    else
        local result, err = storage:seek("cur", 0)
        if result == nil then
            error("failed seeking chunk storage file: " .. err)
        end
        size = result
    end

    return size
end


---
-- Create a new chunk.
--
-- @param manager   A chunk manager to which the chunk reports.
-- @param storage   Chunk storage - either a buffer or a file.
-- @param size      Storage contents size, or nil to have it determined.
--
-- @return New chunk.
--
function co.chunk:__init(manager, storage, size)
    return oo.rawnew(self,
                     {manager = manager,
                      storage = storage,
                      size = size or storage_size(storage)})
end

---
-- Check if a chunk is memory-based.
--
-- @return True if the chunk is memory-based, false otherwise.
--
function co.chunk:is_mem()
    return storage_is_mem(self.storage)
end


---
-- Check if a chunk is file-based.
--
-- @return True if the chunk is file-based, false otherwise.
--
function co.chunk:is_file()
    return not storage_is_mem(self.storage)
end


---
-- Insert a new chunk after this one.
--
-- @param size  Memory to preallocate for chunk contents, or nil.
--
-- @return New chunk inserted after this one.
--
function co.chunk:fork(size)
    local chunk

    chunk = oo.classof(self)(self.manager, co.buf(size), 0)
    chunk.next = self.next
    self.next = chunk

    return chunk
end


---
-- Write a string to an (unfinished) chunk
--
-- @param str   String to add
--
-- @return The chunk.
--
function co.chunk:write(str)
    if self:is_mem() then
        -- Request the memory
        self.manager:request_mem(self, #str)
    end

    -- NOTE: we may get displaced as a result, so check the type again
    if self:is_mem() then
        self.storage:append(str)
    else
        local ok, err = self.storage:write(str)
        if not ok then
            error("failed writing chunk storage file: " .. err)
        end
    end

    self.size = self.size + #str

    return self
end

---
-- Finish the chunk.
--
-- @return The chunk.
--
function co.chunk:finish()
    self.finished = true
    if self:is_mem() then
        self.storage:retension()
    end
    self.manager:finished(self)
    return self
end

---
-- Yield the (finished) chunk storage.
--
-- @return Storage and size
--
function co.chunk:yield()
    local storage

    storage = self.storage
    self.storage = nil

    return storage, self.size
end

---
-- Relocate chunk contents to a memory storage, appending to existing
-- contents.
--
-- @param self      Chunk to relocate.
-- @param storage   Target storage - a buffer.
--
local function relocate_to_mem(self, storage)
    local result, err

    if self:is_mem() then
        -- Merge our storage with the new storage (our storage is cleared)
        storage:merge(self.storage)
    else
        -- Request memory for the file contents
        self.manager:request_mem(self, self.size)

        -- Rewind the file
        result, err = self.storage:seek("cur", -self.size)
        if result == nil then
            error("failed rewinding chunk storage file: " .. err)
        end

        -- Read the file contents into the new storage
        result = storage:readin(self.storage)
        if result ~= self.size then
            error(("short read of chunk storage file "..
                   "(expected: %d, read: %d)"):format(self.size, result))
        end

        -- Close the file
        self.storage:close()
    end
end

---
-- Relocate chunk contents to a file storage, appending to existing
-- contents.
--
-- @param self      Chunk to relocate.
-- @param storage   Target storage - a file.
--
local function relocate_to_file(self, storage)
    local result, err

    if self:is_mem() then
        -- Write the contents to the file
        self.storage:writeout(storage)
        -- Clear the buffer
        self.storage:clear()
        -- Return the memory
        self.manager:return_mem(self, self.size)
    else
        local read_size = 0

        -- Rewind the file
        result, err = self.storage:seek("cur", -self.size)
        if result == nil then
            error("failed rewinding chunk storage file: " .. err)
        end

        -- Transfer the file via a buffer
        while true do
            local block = self.storage:read(self.buf_size)
            if block == nil then
                break
            end
            read_size = read_size + #block
            result, err = storage:write(block)
            if result == nil then
                error("failed writing chunk storage file: " .. err)
            end
        end

        -- Check transferred size
        if read_size ~= self.size then
            error(("short read of chunk storage file "..
                   "(expected: %d, read: %d)"):format(self.size, read_size))
        end

        -- Close the file
        self.storage:close()
    end
end

---
-- Relocate chunk contents to another storage, appending to existing
-- contents.
--
-- @param storage   Target storage - either buffer or file.
-- @param size      Target storage contents size or nil to have it
--                  determined.
--
-- @return The chunk.
--
function co.chunk:relocate(storage, size)
    if size == nil then
        size = storage_size(storage)
    end

    if storage_is_mem(storage) then
        relocate_to_mem(self, storage)
    else
        relocate_to_file(self, storage)
    end

    self.storage = storage
    self.size = size + self.size

    return self
end

return co.chunk
