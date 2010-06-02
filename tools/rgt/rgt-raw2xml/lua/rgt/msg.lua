---
-- Report generation tool - message module.
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

local oo    = require("loop.base")
local rgt   = {}
rgt.ts      = require("rgt.ts")
rgt.msg     = oo.class({
                        version = nil,  --- Version
                        ts      = nil,  --- Timestamp
                        level   = nil,  --- Level
                        id      = nil,  --- Node ID
                        entity  = nil,  --- Entity name
                        user    = nil,  --- User name
                        text    = nil,  --- Text
                       })

function rgt.msg.id_valid(id)
    return type(id) == "number" and
           math.floor(id) == id and
           id >= 0
end

function rgt.msg:__init(version, ts, level, id, entity, user, text)
    assert(version == 1)
    assert(oo.instanceof(ts, rgt.ts))
    assert(type(level) == "string")
    assert(rgt.msg.id_valid(id))
    assert(type(entity) == "string")
    assert(type(user) == "string")
    assert(type(text) == "string")

    return oo.rawnew(self,
                     {version   = version,
                      ts        = ts,
                      level     = level,
                      id        = id,
                      entity    = entity,
                      user      = user,
                      text      = text})
end

