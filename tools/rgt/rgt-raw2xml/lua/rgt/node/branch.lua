---
-- Report generation tool - branch node type.
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
rgt.node.internal   = require("rgt.node.internal")
rgt.node.branch     = oo.class({}, rgt.node.internal)

function rgt.node.branch:start()
    assert(self.chunk ~= nil)

    self.chunk:start_tag("branch")
    rgt.node.internal.start(self)
end

function rgt.node.branch:finish()
    assert(self.chunk ~= nil)

    rgt.node.internal.finish(self)
    self.chunk:end_tag("branch")
end

return rgt.node.branch
