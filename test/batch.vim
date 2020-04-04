func EchoCallback(ch, msg)
  echo a:msg
endfunc

func MissedMsgCallback(ch, msg)
  lexpr "missed message"
  lwindow
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
  let l:return_code = a:msg.return_code
  if l:return_code != 0
    let l:error_message = a:msg.error_message
    lexpr l:error_message
    lwindow
    return
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

func FormRequest(ch)
  let l:buf_body =    join(getline(1, "$"), "\n")

  let l:request = {} 
  let l:request["id"] =         win_getid()
  let l:request["buf_name"] =   buffer_name("%")
  let l:request["buf_length"] = len(l:buf_body)
  let l:request["buf_body"] =   l:buf_body

  call ch_sendexpr(a:ch, l:request, {"callback": "HighlightCallback"})
endfunc

call ch_logfile("log")
let channel = ch_open("localhost:9173", {"mode": "json", "callback": "MissedMsgCallback"})
