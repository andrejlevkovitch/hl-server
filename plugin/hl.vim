augroup hl_callbacks
  au BufWinEnter *            call hl#TryForNewWindow()

  au WinEnter *               call hl#SendRequest()

  au InsertLeave *            call hl#SendRequest()
  au TextChanged *            call hl#SendRequest()
augroup END
