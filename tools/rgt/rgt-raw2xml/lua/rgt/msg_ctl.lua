---
-- Report generation tool - control message parsing module.
--
-- I hate this crap.
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
rgt.msg_fmt_str = require("rgt.msg_fmt_str")
rgt.msg_ctl     = {}

local function divert_fmt(name, str, arg_fetch)
    local arg

    assert(type(name) == "string")
    assert(type(str) == "string")
    assert(type(arg_fetch) == "function")

    if not rgt.msg_fmt_str:spec_valid(str) then
        return str
    end

    arg = arg_fetch()
    if arg == nil then
        error(('Missing %s argument for control message ' ..
               'format string "%s"'):format(name, fmt))
    end

    return rgt.msg_fmt_str(str, arg)
end

local function read_quoted_string(text, pos)
    local unquoted

    if text:sub(pos, pos) ~= "\"" then
        unquoted, pos = text:match("([^%s]*)()", pos)
    else
        local chunk, char, new_pos

        pos = pos + 1
        unquoted = ""

        repeat
            chunk, char, new_pos = text:match("^(.-)([\"\\])()", pos)
            unquoted = unquoted .. chunk
            pos = new_pos + 1
        until char == "\""
    end

    return unquoted, pos
end

local token_parse = {}

function token_parse.tin(prm, fmt, pos, arg_fetch)
    local tin
    tin, pos = fmt:match("^%s*([^%s]+)()", pos)
    if tin == nil then
        error(("Failed to extract TIN from control message \"%s\""):
              format(fmt))
    end
    prm.tin = tonumber(divert_fmt("TIN", tin, arg_fetch))
    return pos
end

function token_parse.page(prm, fmt, pos, arg_fetch)
    local page
    page, pos = fmt:match("^%s*([^%s]+)()", pos)
    if page == nil then
        error(("Failed to extract test page from control message \"%s\""):
              format(fmt))
    end
    prm.page = divert_fmt("page", page, arg_fetch)
    return pos
end

function token_parse.authors(prm, fmt, pos, arg_fetch)
    local spec, arg, text
    local authors = {}
    local text, text_pos, new_pos, mail

    spec, pos = fmt:match("^%s*([^%s]+)()", pos)

    if not rgt.msg_fmt_str:spec_valid(spec) then
        error(('Invalid %s format specifier "%s" in control message ' ..
               'format string "%s"'):format("authors", spec, fmt))
    end

    arg = arg_fetch()
    if arg == nil then
        error(('Missing %s argument for control message ' ..
               'format string "%s"'):format("authors", fmt))
    end

    text = rgt.msg_fmt_str(spec, arg)

    text_pos = 1
    while true do
        mail, new_pos = text:match("^%s*mailto:([^%s]+)()", text_pos)
        if mail == nil then
            break
        end
        text_pos = new_pos
        table.insert(authors, mail)
    end

    if text_pos <= #text then
        error(('Invalid %s text "%s" in control message'):
              format("authors", text))
    end

    prm.authors = authors

    return pos
end

function token_parse.args(prm, fmt, pos, arg_fetch)
    local spec, arg, text
    local args = {}
    local text, text_pos, new_pos
    local name, value

    spec, pos = fmt:match("^%s*([^%s]+)()", pos)

    if not rgt.msg_fmt_str:spec_valid(spec) then
        error(('Invalid %s format specifier "%s" in control message ' ..
               'format string "%s"'):format("args", spec, fmt))
    end

    arg = arg_fetch()
    if arg == nil then
        error(('Missing %s argument for control message ' ..
               'format string "%s"'):format("args", fmt))
    end

    text = rgt.msg_fmt_str(spec, arg)

    text_pos = 1
    while true do
        name, new_pos = text:match("^%s*([^=]+)=()", text_pos)
        if name == nil then
            break
        end
        value, text_pos = read_quoted_string(text, new_pos)
        table.insert(args, {name, value})
    end

    if text_pos <= #text then
        error(('Invalid %s text "%s" in control message'):
              format("args", text))
    end

    prm.args = args

    return pos
end

local function parse_node_start(prm, fmt, pos, arg_fetch)
    if prm.event == "package" or prm.event == "test" then
        local name, objective

        -- Retrieve name
        name, pos = fmt:match("^%s*([^%s]+)()", pos)
        if name == nil then
            error('Failed to extract name ' ..
                  'from control message format string "' .. fmt .. '"')
        end
        prm.name = divert_fmt("name", name, arg_fetch)

        -- Retrieve objective
        objective, pos = fmt:match("^%s*\"([^\"]*)\"()", pos)
        if objective == nil then
            error('Failed to extract objective ' ..
                  'from control message format string "' .. fmt .. '"')
        end
        prm.objective = divert_fmt("objective", objective, arg_fetch)
    end

    -- Parse the message body
    repeat
        local token

        token, pos = fmt:match("^%s*([A-Za-z]+)()", pos)

        if token ~= nil then
            local parse_fn = token_parse[token:lower()]
            
            if parse_fn == nil then
                error(("Unknown token \"%s\" in control message " ..
                       "format string \"%s\""):format(token, fmt))
            end

            pos = parse_fn(prm, fmt, pos, arg_fetch)
        end
    until token == nil
end

function rgt.msg_ctl.parse(prm, fmt, arg_fetch)
    local event
    local pos       = 1

    assert(type(prm) == "table")
    assert(type(fmt) == "string")
    assert(type(arg_fetch) == "function")

    -- Parse the message header
    prm.parent_id, prm.id, event, pos =
            fmt:match("^(%d+)%s+(%d+)%s+([^%s]+)()")

    if prm.parent_id == nil then
        error(("Failed to extract header from " ..
               "control message format string \"%s\""):
              format(fmt))
    end
    prm.parent_id = tonumber(prm.parent_id)
    prm.id = tonumber(prm.id)

    event = divert_fmt("event", event, arg_fetch):lower()
    prm.event = event

    -- If it is a node start
    if event == "package" or event == "session" or event == "test" then
        parse_node_start(prm, fmt, pos, arg_fetch)
    else
        prm.err = divert_fmt("error", fmt:match("^%s*(.*)$", pos), arg_fetch)
    end

    --[[
    io.stderr:write("CONTROL TEXT: \"" .. fmt .. "\"\n")
    io.stderr:write("CONTROL PARAMETERS:\n")
    for k, v in pairs(prm) do
        io.stderr:write(k .. "=")
        if type(v) == "table" then
            io.stderr:write("{")
            for vk, vv in pairs(v) do
                io.stderr:write(vk .. "=")
                if type(vv) == "table" then
                    io.stderr:write("{")
                    for vvk, vvv in pairs(vv) do
                        io.stderr:write(vvk .. "=\"" .. vvv .. "\", ")
                    end
                    io.stderr:write("}")
                else
                    io.stderr:write("\"" .. vv .. "\"")
                end
                io.stderr:write(", ")
            end
            io.stderr:write("}")
        else
            io.stderr:write("\"" .. v .. "\"")
        end
        io.stderr:write("\n")
    end
    --]]

    return prm
end

return rgt.msg_ctl
