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

local co    = {}
co.chunk    = {}

---
-- File->file relocation buffer size
--
co.chunk.relocate_buf_size  = 32768

---
-- Calculate/retrieve size of a storage (file or table) contents.
--
-- @param storage   Storage to calculate/retrieve contents size of.
--
-- @return Storage contents size, bytes.
--
local function storage_size(storage)
    local size

    assert(storage ~= nil)

    if type(storage) == "table" then
        size = 0
        for i, v in ipairs(storage) do
            size = size + #v
        end
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
-- @param train     A train to which the chunk belongs.
-- @param storage   Chunk storage - either a table or a file.
-- @param size      Storage contents size, or nil to have it determined.
--
-- @return New chunk.
--
function co.chunk.new(train, storage, size)
    assert(train ~= nil)
    assert(storage ~= nil)

    return setmetatable({train = train,
                         size = size or storage_size(storage),
                         storage = storage},
                        co.chunk)
end

---
-- Add a string to an (unfinished) chunk
--
-- @param str   String to add
--
function co.chunk:add(str)
    assert(self.finished ~= true)
    assert(str ~= nil)

    if type(self.storage) == "table" then
        -- Request the memory
        self.train:chunk_request_mem(#str)
    end

    -- NOTE: we may get displaced as a result, so check the type again
    if type(self.storage) == "table" then
        table.insert(self.storage, str)
    else
        self.storage:write(str)
    end

    self.size = self.size + #str
end

---
-- Finish the chunk.
--
function co.chunk:finish()
    self.finished = true
    self.train:chunk_finished(self)
end

---
-- Yield the chunk storage.
--
-- @return Storage and size
--
function co.chunk:yield()
    local storage, size

    storage = self.storage
    size = self.size
    self.storage = nil
    self.size = 0

    return storage, size
end

---
-- Relocate chunk contents to a table storage, appending to existing
-- contents.
--
-- @param self      Chunk to relocate.
-- @param storage   Target storage - a table.
--
local function relocate_to_table(self, storage)
    local result, err

    if type(self.storage) == "table" then
        for i, v in ipars(self.storage) do
            table.insert(storage, v)
        end
    else
        -- Request memory for the file contents
        self.train:chunk_request_mem(self.size)

        -- Rewind the file
        result, err = self.storage:seek("set", 0)
        if result == nil then
            error("failed rewinding chunk storage file: " .. err)
        end

        -- Read the whole contents
        result, err = self.storage:read(self.size)
        if result == nil then
            error("failed reading chunk storage file: " .. err)
        end
        if #result ~= self.size then
            error("short read of chunk storage file")
        end

        -- Add to the new storage
        table.insert(storage, result)

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

    if type(self.storage) == "table" then
        -- Write the contents to the file
        result, err = storage:write(unpack(self.storage))
        if result == nil then
            error("failed writing chunk storage file: " .. err)
        end

        -- Return the memory
        self.train:chunk_return_mem(self.size)
    else
        local read_size = 0

        -- Transfer the file contents
        while true do
            local block = self.storage:read(co.chunk.relocate_buf_size)
            if block == nil then
                break
            end
            read_size = read_size + #block
            storage:write(block)
        end

        if read_size ~= self.size then
            error("short read of chunk storage file")
        end

        -- Close the file
        self.storage:close()
    end

    result, err = storage:seek("cur", 0)
    if result == nil then
        error("failed to seek chunk storage file: " .. err)
    end
end

---
-- Relocate chunk contents to another storage, appending to existing
-- contents.
--
-- @param storage   Target storage - either memory or file.
-- @param size      Target storage contents size or nil to have it
--                  determined.
--
function co.chunk.relocate(storage, size)
    if size == nil then
        size = storage_size(storage)
    end

    if type(storage) == "table" then
        relocate_to_table(self, storage)
    else
        relocate_to_file(self, storage)
    end

    self.storage = storage
    self.size = size + self.size
end

return co.chunk
