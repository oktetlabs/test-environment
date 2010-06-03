---
-- Report generation tool - root node type.
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
rgt.msg             = require("rgt.msg")
rgt.node            = {}
rgt.node.general    = require("rgt.node.general")
rgt.node.root       = oo.class({}, rgt.node.general)

function rgt.node.root:start()
    assert(self.head ~= nil)

    self.head:write("<?xml version=\"1.0\"?>\n")
    self.head:start_tag("proteos:log_report",
                        {{"xmlns:proteos",
                          "http://www.oktetlabs.ru/proteos"}})
    rgt.node.general.start(self)

    return self
end

function rgt.node.root:add_child(child)
    assert(oo.instanceof(child, rgt.node.general))
    -- One branch at most
    assert(self.branches[1].child == nil)
    rgt.node.general.add_child(self, child)
    return self
end

function rgt.node.root:finish()
    assert(self.tail ~= nil)

    rgt.node.general.finish(self)
    self.tail:end_tag("proteos:log_report")
    return self
end

return rgt.node.root
