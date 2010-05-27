---
-- Report generation tool - internal node type.
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

local oo            = require("loop.simple")
local rgt           = {}
rgt.node            = {}
rgt.node.basic      = require("rgt.node.basic")
rgt.node.internal   = oo.class({
                                child       = nil   --- Child node
                                head_chunk  = nil   --- Head chunk
                                tail_chunk  = nil   --- Tail chunk
                               }, rgt.node.basic)

function rgt.node.internal:grab_chunk(chunk)
    self.head_chunk = chunk
    self.tail_chunk = chunk:fork_next()
    rgt.node.basic.grab_chunk(self, self.tail_chunk:fork_prev():descend())
end

function rgt.node.internal:yield_chunk(chunk)
    local chunk = self.tail_chunk

    self.head_chunk:finish()
    self.head_chunk = nil
    rgt.node.basic.yield_chunk(self):finish()
    self.tail_chunk = nil

    return chunk
end

function rgt.node.internal:get_child()
    return self.child
end

function rgt.node.internal:add_child(node)
    assert(oo.instanceof(node, rgt.node.basic))
    assert(self:child == nil)
    assert(self.chunk ~= nil)

    if self.logging then
        self:finish_logging()
    end

    node:grab_chunk(self.chunk)
    self.chunk = nil
    self.child = node
end

function rgt.node.internal:del_child(node)
    assert(oo.instanceof(node, rgt.node.basic))
    assert(self.child == node)
    assert(self.chunk == nil)

    self.chunk = self.child:yield_chunk()
    self.child = nil
end

function rgt.node.internal:log(ts, level, entity, user, text)
    if self.child ~= nil then
        self:get_child():log(ts, level, entity, user, text)
    else
        rgt.node.basic.log(self, ts, level, entity, user, text)
    end
end

return rgt.node.internal
