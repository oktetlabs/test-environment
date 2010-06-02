---
-- Report generation tool - general node type.
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
local co            = {}
co.xml_chunk        = require("co.xml_chunk")
local rgt           = {}
rgt.msg             = require("rgt.msg")
rgt.node            = {}
rgt.node.general    = oo.class({
                                head     = nil,  --- Head output chunk
                                branches = nil,  --- {child, chunk, logging}
                                                 --  body child/chunk array
                                tail     = nil,  --- Tail output chunk
                               })

function rgt.node.general:__init(inst)
    assert(type(inst) == "table")
    return oo.rawnew(self, inst)
end

function rgt.node.general:take_chunk(chunk)
    self.head = chunk
    self.tail = chunk:fork_next()
    self.branches = {self:start_branch({chunk = chunk:fork_next()})}
    return self
end

function rgt.node.general:start()
    return self
end

function rgt.node.general:start_branch(branch)
    assert(type(branch) == "table")
    return branch
end

function rgt.node.general:start_branch_logging(branch)
    assert(type(branch) == "table")
    assert(not branch.logging)
    branch.chunk:start_tag("logs")
    branch.logging = true
    return branch
end

function rgt.node.general:add_child(child)
    local branch

    assert(oo.instanceof(child, rgt.node.general))

    -- Lookup first vacant branch
    for i, b in ipairs(self.branches) do
        assert(b.child ~= child)
        if b.child == nil then
            assert(b.chunk ~= nil)
            branch = b
        end
    end

    -- If there was none
    if branch == nil then
        -- Create and start new branch
        branch = self:start_branch({chunk = self.tail:fork_prev()})
        table.insert(self.branches, branch)
    end

    if branch.logging then
        self:stop_branch_logging(branch)
    end
    branch.child = child:take_chunk(branch.chunk)
    branch.chunk = nil

    return self
end

function rgt.node.general:log(ts, level, entity, user, text)

    -- Lookup first branch with a child
    for i, b in ipairs(self.branches) do
        if b.child ~= nil then
            -- Delegate logging to the child
            b.child:log(ts, level, entity, user, text)
            return
        end
    end

    -- No children found

    if not self.branches[1].logging then
        self:start_branch_logging(self.branches[1])
    end
    self.branches[1].chunk:element("msg",
                                   {ts      = ts:format_short_abs(),
                                    level   = level,
                                    entity  = entity,
                                    user    = user})
    return self
end

function rgt.node.general:del_child(child)
    assert(oo.instanceof(child, rgt.node.general))

    -- Lookup the branch holding the child
    for i, b in ipairs(self.branches) do
        if b.child == child then
            assert(b.chunk == nil)
            -- Get back the chunk
            b.chunk = b.child:yield_chunk()
            -- Remove the child
            b.child = nil
            return self
        end
    end

    assert(false)
end

function rgt.node.general:finish_branch_logging(branch)
    assert(type(branch) == "table")
    assert(branch.logging)
    branch.chunk:end_tag("logs")
    branch.logging = nil
    return branch
end

function rgt.node.general:finish_branch(branch)
    assert(type(branch) == "table")
    if branch.logging then
        self:finish_branch_logging(branch)
    end
    return branch
end

function rgt.node.general:finish()
    for i, b in ipairs(self.branches) do
        assert(b.child == nil)
        assert(b.chunk ~= nil)
        self:finish_branch(b)
    end
    return self
end

function rgt.node.general:yield_chunk()
    local chunk

    self.head:finish()
    self.head = nil

    for i, b in ipairs(self.branches) do
        assert(b.child == nil)
        assert(b.chunk ~= nil)
        b.chunk:finish()
        b.chunk = nil
    end

    chunk = self.tail
    self.tail = nil

    return chunk
end

return rgt.node.general
