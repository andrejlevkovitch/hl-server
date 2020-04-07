-- echo_test.lua
-- @script server test
--
-- @usage set host and port as first and second arguments. After start put some text in input and get result in standard
-- output
--
local socket_factory = require("socket")
local cjson = require("cjson")

local function main()
  local host = arg[1]
  local port = arg[2]

  if host == nil or port == nil then
    error("you must set host as first argument and port as second")
  end

  local sock = socket_factory.tcp()
  sock:connect(host, port)

  local input = io.read("*a")

  local request = cjson.encode{0, {
    id = 0,
    buf_type = "cpp",
    buf_name = "a.cpp",
    buf_length = #input,
    buf_body = input,
    additional_info = [[
-x
c++
-I/home/levkovitch/Public/git/hl/include
-I/home/levkovitch/Public/git/hl/third-party/json/single_include
-I/usr/lib/llvm-9/include
-Wall
-Wextra
-Wnon-virtual-dtor
-Wshadow
-std=gnu++17
-I/home/levkovitch/Public/git/hl/build
-I/home/levkovitch/Public/git/hl/src/hl/tokenizer/
    ]],
  }}

  local ok, err = sock:send(request)
  if not ok then
    error(err)
  end

  local str, err1 = sock:receive("*l")
  if not str then
    error(err1)
  end

  print(str)
end

main()
