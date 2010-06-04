---
-- Report generation tool - message string formatted output.
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

local rgt       = {}
rgt.msg_fmt     = require("rgt.msg_fmt")
local meta      = {__index = rgt.msg_fmt}
rgt.msg_fmt_str = setmetatable({}, meta)

function rgt.msg_fmt_str:write(output, str)
    output[1] = output[1] .. str
end

function meta.__call(self, fmt, ...)
    local output    = {""}

    self:apply(output, fmt, {...})

    return output[1]
end

return rgt.msg_fmt_str
