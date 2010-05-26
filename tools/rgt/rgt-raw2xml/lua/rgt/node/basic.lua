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

local oo            = require("loop.base")
local co            = {}
local co.xml_chunk  = require("co.chunk")
local rgt           = {}
rgt.msg             = require("rgt.msg")
rgt.node            = {}
rgt.node.basic      = oo.class({
                                logging     = nil,  --- "Logging" flag
                                chunk       = nil,  --- Output chunk
                               })

---
-- Initialize a basic node instance.
--
-- @param inst  Instance parameter table
--
function rgt.node.basic:__init(inst)
    assert(type(inst) == "table")

    inst.logging = nil

    return oo.rawnew(self, inst)
end

---
-- Take (possession of) the output chunk.
--
-- @param chunk Output chunk
--
function rgt.node.basic:grab_chunk(chunk)
    assert(self.chunk == nil)
    assert(oo.instanceof(chunk, co.xml_chunk))

    self.chunk = chunk
end

---
-- Start node output.
--
function rgt.node.basic:start()
    assert(self.chunk ~= nil)
end

function rgt.node.basic:start_logging()
    assert(self.chunk ~= nil)
    assert(not self.logging)

    self.chunk:start_tag("log")
    self.logging = true
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

    self.chunk:element("msg",
                       {ts      = msg.ts:format_short_abs(),
                        level   = msg.level,
                        entity  = msg.entity,
                        user    = msg.user},
                       msg.text)
end

function rgt.node.basic:finish_logging()
    assert(self.chunk ~= nil)
    assert(self.logging)

    self.chunk:end_tag("log")
end

---
-- Finish node output.
--
function rgt.node.basic:finish()
    assert(self.chunk ~= nil)

    if self.logging then
        self:finish_logging()
    end
end

---
-- Yield (surrender) the (last) output chunk.
--
-- @return The output chunk
--
function rgt.node.basic:yield_chunk()
    local chunk

    assert(self.chunk ~= nil)

    chunk = self.chunk
    self.chunk = nil

    return chunk
end

return rgt.node.basic
