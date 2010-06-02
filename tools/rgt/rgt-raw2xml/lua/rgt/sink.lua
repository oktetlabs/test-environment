---
-- Report generation tool - sink module.
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
local co            = require("co")
local rgt           = {}
rgt.msg_ctl_text    = require("rgt.msg_ctl_text")
rgt.node            = {}
rgt.node.root       = require("rgt.node.root")
rgt.node.package    = require("rgt.node.package")
rgt.node.session    = require("rgt.node.session")
rgt.node.test       = require("rgt.node.test")
rgt.sink            = oo.class({
                                chunk   = nil,  --- The output chunk
                                map     = nil,  --- ID->node map
                                max_mem = nil,  --- Maximum memory allowed
                                                --  for output
                               })

function rgt.sink:__init(max_mem)
    assert(type(max_mem) == "number")
    assert(math.floor(max_mem) == max_mem)
    assert(max_mem >= 0)

    return oo.rawnew(self, {map = {}, max_mem = max_mem})
end

function rgt.sink:take_file(file)
    assert(self.chunk == nil)
    assert(self.map[0] == nil)
    assert(file ~= nil)
    self.chunk = co.xml_chunk(co.manager(self.max_mem), file, 0)
end

function rgt.sink:start()
    assert(self.map[0] == nil)
    inst.map[0] = rgt.node.root():take_chunk(self.chunk):start()
    self.chunk = nil
end

function rgt.sink:put(msg)
    local prm
    local parent, node

    assert(self.map[0] ~= nil)

    -- If it is not a control message
    if msg.id ~= 0 or msg.user ~= "Control" or msg.entity ~= "Tester" then
        -- log the message to its node
        node = map[msg.id]
        node:log(msg.ts, msg.level, msg.entity, msg.user, msg.text)
        return
    end

    -- parse control message text
    prm = rgt.msg_ctl_text.parse({}, msg.text)

    -- lookup parent node
    parent = self.map[prm.parent_id]

    -- If it is a node opening
    if prm.event == "package" or
       prm.event == "session" or
       prm.event == "test" then
        -- create new node
        node = rgt.node[prm.event]{
                start       = msg.ts,
                name        = prm.name,
                objective   = prm.objective,
                tin         = prm.tin,
                page        = prm.page,
                authors     = prm.authors,
                args        = prm.args,
                start       = msg.ts}

        -- add to the map
        self.map[prm.id] = node

        -- add to the parent's children list
        parent:add_child(prm.id, node)

        -- start node output
        node:start()
    else
        -- lookup node
        node = self.map[prm.id]

        -- finish the node with the result
        node:finish(msg.ts, prm.event, prm.err)

        -- remove the node from the parent
        parent:del_child(prm.id)

        -- remove the node from the map
        self.map[prm.id] = nil
    end
end

function rgt.sink:finish()
    assert(self.map[0] ~= nil)
    assert(self.chunk == nil)

    -- Finish the root node and take its chunk
    self.chunk = self.map[0]:finish():yield_chunk()

    -- Remove the root node
    self.map[0] = nil
end

function rgt.sink:yield_file()
    local file, size

    assert(self.map[0] == nil)
    assert(self.chunk ~= nil)

    -- Finish the chunk and retrieve its storage and size
    file, size = self.chunk:finish():yield()
    -- Remove the chunk
    self.chunk = nil

    return file, size
end

return rgt.sink
