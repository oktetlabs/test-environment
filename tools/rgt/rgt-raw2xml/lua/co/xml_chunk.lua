---
-- Chunked output - XML chunk module.
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

local oo        = require("loop.base")
local co        = {}
co.chunk        = require("co.chunk")
co.xml_chunk    = oo.class({
                            depth   = 0,    -- Nesting depth
                           }, co.chunk)

---
-- Create a new XML chunk.
--
-- @param manager   A chunk manager to which the chunk belongs.
-- @param storage   Chunk storage - either a table or a file.
-- @param size      Storage contents size, or nil to have it determined.
-- @param depth     Nesting depth, 0 by default.
--
-- @return New XML chunk.
--
function co.xml_chunk:__init(manager, storage, size, depth)
    local inst

    assert(oo.instanceof(manager, require("co.manager")))
    assert(storage ~= nil)
    assert(size == nil or
           type(size) == "number" and
           math.floor(size) == size and
           size >= 0)
    assert(depth == nil or
           type(depth) == "number" and
           math.floor(depth) == depth and
           depth >= 0)

    inst = co.chunk(manager, storage, size)
    inst.depth = depth
    return oo.rawnew(self, inst)
end

---
-- Insert a new XML chunk before this one.
--
-- @return New XML chunk inserted before this one.
--
function co.xml_chunk:fork_prev()
    local chunk = co.chunk.fork_prev(self)

    chunk.depth = self.depth

    return chunk
end


---
-- Insert a new XML chunk after this one.
--
-- @return New XML chunk inserted after this one.
--
function co.xml_chunk:fork_next()
    local chunk = co.chunk.fork_next(self)

    chunk.depth = self.depth

    return chunk
end


local function format_attrs(attrs)
    local str   = ""

    assert(type(attrs) == "table")

    for n, v in pairs(attrs) do
        str = str .. " " .. name .. "=\"" .. 
               v:gsub("([<>&\"])",
                      function (c)
                        return c == "<" and "&lt;" or
                               c == ">" and "&gt;" or
                               c == "&" and "&amp;" or
                               "&quot;"
                      end)
    end

    return str
end

local function format_start_tag(name, attrs)
    assert(type(name) == "string")
    assert(attrs == nil or type(attrs) == "table")

    return "<" .. name .. (attrs and format_attrs(attrs) or "") .. ">"
end

local function format_empty_tag(name, attrs)
    assert(type(name) == "string")
    assert(attrs == nil or type(attrs) == "table")

    return "<" .. name .. (attrs and format_attrs(attrs) or "") .. "/>"
end

local function format_end_tag(name)
    assert(type(name) == "string")

    return "</" .. name .. ">"
end

local function format_cdata(text)
    assert(type(text) == "string")
    return text:gsub("[<>&]",
                     function (c)
                        return c == "<" and "&lt;" or
                               c == ">" and "&gt;" or
                               "&amp;"
                     end)
end

function co.xml_chunk:start_tag(name, attrs)
    assert(type(name) == "string")
    assert(attrs == nil or type(attrs) == "table")

    self:write((" "):rep(self.depth) ..
               format_start_tag(name, attrs) .. "\n")
    self.depth = self.depth + 1
end

function co.xml_chunk:end_tag(name)
    assert(type(name) == "string")

    self:write((" "):rep(self.depth) ..
               format_end_tag(name) .. "\n")
    self.depth = self.depth - 1
end

function co.xml_chunk:empty_tag(name, attrs)
    assert(type(name) == "string")
    assert(attrs == nil or type(attrs) == "table")

    self:write((" "):rep(self.depth) ..
               format_empty_tag(name, attrs) .. "\n")
end

function co.xml_chunk:cdata(text)
    assert(type(text) == "string")

    self:write(format_cdata(text):
               -- indent
               gsub("\n", "%0" .. (" "):rep(self.depth)):
               -- add newline if necessary
               gsub("[^\n]$", "%0\n"))
end

function co.xml_chunk:element(name, attrs, text)
    local e

    assert(type(name) == "string")
    assert(attrs = nil or type(attrs) == "table")
    assert(text == nil or type(text) == "string")

    if text == nil or #text == 0 then
        e = format_empty_tag(name, attrs)
    else
        e = format_start_tag(name, attrs) ..
            format_cdata(text) ..
            format_end_tag(name)
    end

    self:write((" "):rep(self.depth) .. e .. "\n")
end

return co.xml_chunk
