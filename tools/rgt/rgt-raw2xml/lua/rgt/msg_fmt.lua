---
-- Report generation tool - message abstract formatting.
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

local rgt           = {}
rgt.msg_fmt         = {}
rgt.msg_fmt.handler = {}

function rgt.msg_fmt.handler.s(self, output, c, fmt, pos, arg)
    self:write(output, arg)
    return pos
end

local function handler_uint32(self, output, c, fmt, pos, arg)
    if #arg ~= 4 then
        error(("Invalid %%%s format argument"):format(c))
    end

    self:write(output,
               ("%" .. c):format(arg:byte(1) * 2^24 +
                                 arg:byte(2) * 2^16 +
                                 arg:byte(3) * 2^8 +
                                 arg:byte(4)))
    return pos
end

rgt.msg_fmt.handler.c   = handler_uint32
rgt.msg_fmt.handler.d   = handler_uint32
rgt.msg_fmt.handler.u   = handler_uint32
rgt.msg_fmt.handler.o   = handler_uint32
rgt.msg_fmt.handler.x   = handler_uint32
rgt.msg_fmt.handler.X   = handler_uint32

function rgt.msg_fmt.handler.p(self, output, c, fmt, pos, arg)
    local zero_run  = true

    if #arg == 0 or #arg % 4 ~= 0 then
        error(("Invalid %%%s format argument"):format(c))
    end

    self:write(output, "0x")
    for i = 1, #arg, 4 do
        local v = arg:byte(i) * 2^24 +
                  arg:byte(i + 1) * 2^16 +
                  arg:byte(i + 2) * 2^8 +
                  arg:byte(i + 3)
        if zero_run and v ~= 0 then
            zero_run = false
        end
        if not zero_run or (i + 4) < #arg then
            self:write(output, ("%08X"):format(v))
        end
    end

    return pos
end

function rgt.msg_fmt.handler.r(self, output, c, fmt, pos, arg)
    local v

    if #arg ~= 4 then
        error(("Invalid %%%s format argument"):format(c))
    end

    v = arg:byte(1) * 2^24 +
        arg:byte(2) * 2^16 +
        arg:byte(3) * 2^8 +
        arg:byte(4)

    if v >= 2^31 then
        v = 2^31 - v - 1
    end

    self:write(output,
               -- This function is defined in raw2xml.c
               te_rc_to_str(v))
    return pos
end

function rgt.msg_fmt:write(output, str)
    assert(false, "Abstract function called")
end

function rgt.msg_fmt:spec_valid(spec)
    assert(type(spec) == "string")
    return #spec == 2 and
           spec:sub(1, 1) == "%" and
           self.handler[spec:sub(2, 2)] ~= nil
end

function rgt.msg_fmt:apply(output, fmt, arg_list)
    local pos       = 1
    local arg_pos   = 1
    local str, c, arg
    local handler

    while true do
        str, pos = fmt:match("^([^%%]*)()", pos)
        self:write(output, str)
        if pos > #fmt then
            break
        end
        pos = pos + 1
        c = fmt:sub(pos, pos)
        pos = pos + 1
        -- If it is an escaped '%'
        if c == "%" then
            chunk:cdata("%")
        else
            arg = arg_list[arg_pos]

            -- If we're out of arguments
            if arg == nil then
                -- Write the rest of the string with this format specifier
                self:write(fmt:sub(pos - 2))
                break
            end

            arg_pos = arg_pos + 1

            -- Lookup handler
            handler = self.handler[c]

            -- If handler is found
            if handler ~= nil then
                -- Handle the format specifier
                pos = handler(self, output, c, fmt, pos, arg)
            else
                -- Write format specifier as is
                self:write(output, fmt:sub(pos - 2, pos - 1))
            end
        end
    end
end

return rgt.msg_fmt
