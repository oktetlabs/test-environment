---
-- Report generation tool - basic node type.
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
local rgt       = {}
rgt.msg         = require("rgt.msg")
rgt.node        = {}
rgt.node.basic  = oo.class({
                            depth       = nil,  --- depth in the tree
                            logging     = nil,  --- "logging" flag
                            chunk       = nil   --- log output chunk
                           })

---
-- Check if a node depth is valid.
--
-- @param depth Depth to check
--
-- @return True if the depth is valid, false otherwise.
--
function rgt.node.basic.valid_depth(depth)
    return type(depth) == "number" and
           math.floor(depth) == depth and
           depth >= 0
end

---
-- Initialize a basic node instance.
--
-- @param inst  Instance parameters:
--              depth   - node depth.
--
function rgt.node.basic:__init(inst)
    assert(type(inst) == "table")
    assert(rgt.node.basic.valid_depth(inst.depth))
    assert(chunk == nil or oo.instanceof(chunk, require("co.chunk")))

    inst.logging = nil

    return oo.rawnew(self, inst)
end

---
-- Start node output.
--
-- @param chunk The chunk to start output to.
--
function rgt.node.basic:start(chunk)
    assert(self.chunk == nil)
    assert(chunk ~= nil)

    self.chunk = chunk
end

function rgt.node.basic:start_logging()
    assert(self.chunk ~= nil)
    assert(not self.logging)

    self.chunk:write(string.rep(" ", self.depth))
    self.chunk:write("<log>\n")
    self.logging = true
    self.depth = self.depth + 1
end

function rgt.node.basic:finish_logging()
    assert(self.chunk ~= nil)
    assert(self.logging)

    self.chunk:write(string.rep(" ", self.depth))
    self.chunk:write("</log>\n")
    self.logging = nil
    self.depth = self.depth - 1
end

---
-- Log a message to the node.
--
-- @param msg   Message to log.
--
function rgt.node.basic:log(msg)
    assert(self.chunk ~= nil)
    assert(oo.instanceof(msg, rgt.msg))

    if not self.logging then
        self:start_logging()
    end

    self.chunk:write(string.rep(" ", self.depth))
    self.chunk:write(
        ("<msg ts=\"%s\" level=\"%s\" entity=\"%s\" user=\"%s\">%s</msg>\n"):
            format(msg.ts:format_short_abs(), msg.level,
                   msg.entity, msg.user, msg.text))
end

---
-- Finish node output.
--
-- @return The chunk to which the end of the node was written.
--
function rgt.node.basic:finish()
    local chunk

    assert(self.chunk ~= nil)

    if self.logging then
        self:finish_logging()
    end

    chunk = self.chunk
    self.chunk = nil

    return chunk
end

return rgt.node.basic
