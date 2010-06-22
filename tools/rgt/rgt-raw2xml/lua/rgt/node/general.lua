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
local rgt           = {}
rgt.node            = {}
rgt.node.general    = oo.class({
                                head     = nil,  --- Head output chunk
                                branches = nil,  --- {child, chunk, logging}
                                                 --  body child/chunk array
                                tail     = nil,  --- Tail output chunk
                               })

function rgt.node.general:__init(inst)
    return oo.rawnew(self, inst)
end

function rgt.node.general:take_chunk(chunk)
    local body
    self.head = chunk
    body = chunk:fork():descend()
    self.tail = body:fork()
    self.branches = {self:start_branch({chunk = body})}
    return self
end

function rgt.node.general:start()
    return self
end

function rgt.node.general:start_branch(branch)
    return branch
end

function rgt.node.general:start_branch_logging(branch)
    branch.chunk:append_start_tag("logs")
    branch.logging = true
    return branch
end

function rgt.node.general:add_child(child)
    local branch

    -- Lookup first vacant branch
    for i, b in ipairs(self.branches) do
        if b.child == nil then
            branch = b
        end
    end

    -- If there was none
    if branch == nil then
        -- Create and start new branch
        local new_tail  = self.tail:fork()
        branch = self:start_branch({chunk = self.tail})
        table.insert(self.branches, branch)
        self.tail = new_tail
    end

    if branch.logging then
        self:finish_branch_logging(branch)
    end
    branch.child = child:take_chunk(branch.chunk)
    branch.chunk = nil

    return self
end

local function write_msg(chunk, msg)
end

function rgt.node.general:log(msg)
    local branch

    -- Lookup first branch with a child
    for i, b in ipairs(self.branches) do
        if b.child ~= nil then
            -- Delegate logging to the child
            b.child:log(msg)
            return
        end
    end

    -- No children found

    branch = self.branches[1]
    if not branch.logging then
        self:start_branch_logging(branch)
    end

    branch.chunk:append_msg(msg)

    return self
end

function rgt.node.general:del_child(child)
    -- Lookup the branch holding the child
    for i, b in ipairs(self.branches) do
        if b.child == child then
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
    branch.chunk:append_end_tag("logs")
    branch.logging = nil
    return branch
end

function rgt.node.general:finish_branch(branch)
    if branch.logging then
        self:finish_branch_logging(branch)
    end
    return branch
end

function rgt.node.general:finish()
    for i, b in ipairs(self.branches) do
        self:finish_branch(b)
    end
    return self
end

function rgt.node.general:yield_chunk()
    local chunk

    if self.head ~= nil then
        self.head:finish()
        self.head = nil
    end

    for i, b in ipairs(self.branches) do
        b.chunk:finish()
        b.chunk = nil
    end

    chunk = self.tail
    self.tail = nil

    return chunk
end

return rgt.node.general
