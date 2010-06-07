---
-- Report generation tool - message XML chunk formatted output.
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

local rgt               = {}
rgt.msg_fmt             = require("rgt.msg_fmt")
local meta              = {__index = rgt.msg_fmt}
rgt.msg_fmt_xml_chunk   = setmetatable({}, meta)

function rgt.msg_fmt_xml_chunk:apply_spec_body(output, fmt, pos, arg)
    local new_pos
    local s

    new_pos = rgt.msg_fmt.apply_spec_body(self, output, fmt, pos, arg)
    if new_pos > pos then
        return new_pos
    end

    s = fmt:sub(pos, pos + 1)

    if s == "Tf" then
        output:element("file", {{"name", "TODO"}}, arg)
        return pos + 2
    end

    if s == "Tm" then
        local rl, el, new_pos

        pos = pos + 2
        rl, el, new_pos = fmt:match("^%[%[(%d)%]%.%[(%d)%]%]()", pos)
        if rl == nil then
            rl  = 16
            el  = 1
        else
            if el == 0 then
                error(("Invalid %%%s format specifier parameters"):format(s))
            end
            if #arg % el ~= 0 then
                error(("Invalid %%%s format specifier argument"):format(s))
            end
            pos = new_pos
        end

        output:start_tag("mem-dump")
        output:start_tag("row")
        if #arg > 0 then
            local i = 1
            local e = ""

            while true do
                e = e .. ("%02X"):format(arg:byte(i))
                if i % el == 0 then
                    output:element("elem", nil, e)
                    e = ""
                end
                if i % rl == 0 then
                    output:end_tag("row")
                end
                if i >= #arg then
                    break
                end
                if i % rl == 0 then
                    output:start_tag("row")
                end
                i = i + 1
            end
        end
        output:end_tag("row")
        output:end_tag("mem-dump")

        return pos
    end

    return pos
end

function rgt.msg_fmt_xml_chunk:write(output, str)
    output:cdata(str)
end

function meta.__call(self, output, fmt, args)
    self:apply(output, fmt, args)
end

return rgt.msg_fmt_xml_chunk
