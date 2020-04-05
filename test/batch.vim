let g:hl_server_addr = "localhost:9173"

func EchoCallback(ch, msg)
  echo a:msg
endfunc

func MissedMsgCallback(ch, msg)
  lexpr "missed message"
endfunc

func ClearWinMatches(win_id)
  if exists("w:matches")
    for l:match in w:matches
      call matchdelete(l:match, a:win_id)
    endfor
  endif
  let w:matches = []
endfunc

func HighlightCallback(ch, msg)
  " check that request was processed properly
  if a:msg.return_code != 0
    lexpr a:msg.error_message

    " XXX even if we get fail, try highlight existed tokens
  endif

  let l:win_id = a:msg.id

  " at first clear all matches
  call ClearWinMatches(l:win_id)

  " and add new heighligth
  for l:item in a:msg.tokens
    let l:group = l:item.group
    let l:pos   = l:item.pos

    let l:match = matchaddpos(l:group, [l:pos], 0, -1, {"window": l:win_id})
    call add(w:matches, l:match)
  endfor
endfunc

func SendHLRequest()
  let l:buf_body = join(getline(1, "$"), "\n")

  let l:request = {} 
  let l:request["id"] =         win_getid()
  let l:request["buf_type"] =   &filetype
  let l:request["buf_name"] =   buffer_name("%")
  let l:request["buf_length"] = len(l:buf_body)
  let l:request["buf_body"] =   l:buf_body

  call ch_sendexpr(g:hl_server_channel, l:request, {"callback": "HighlightCallback"})
endfunc

call ch_logfile("log")
let g:hl_server_channel = ch_open(g:hl_server_addr, {"mode": "json", "callback": "MissedMsgCallback"})
