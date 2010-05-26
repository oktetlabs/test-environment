---
-- Report generation tool - session node type.
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

local oo            = require("loop.multi")
local rgt           = {}
rgt.node            = {}
rgt.node.basic      = require("rgt.node.basic")
rgt.node.session    = oo.class({
                               },
                               rgt.node.internal,
                               rgt.node.parametrized,
                               rgt.node.spanning)

function rgt.node.session:__init(inst)
    rgt.node.internal:__init(inst)
    rgt.node.parametrized:__init(inst)
    rgt.node.spanning:__init(inst)

    return oo.rawnew(self, inst)
end

return rgt.node.session
