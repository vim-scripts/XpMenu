let g:DllCallLibrary = 'xpmenu.dll'

" LoadLibrary : load {dllname} and return handle of library
function! LoadLibrary(dllname)
  let hModule = '0'
  silent let hModule = libcall(g:DllCallLibrary, 'dllload', a:dllname)
  return hModule
endfunction

" FreeLibrary : free {hModule}
function! FreeLibrary(hModule)
  let dwRet = 0
  silent let dwRet = libcall(g:DllCallLibrary, 'dllfree', a:hModule)
  return dwRet
endfunction

" FreeLibrary : call {pszFunction} of {hModule} with {pszArgument}
function! CallLibrary(hModule, pszFuncName, pszArgument)
  let pszRet = ''
  silent let pszRet = libcall(g:DllCallLibrary, 'callfunc', a:hModule.','.a:pszFuncName.','.a:pszArgument)
  return pszRet
endfunction

let g:hXpMenu = 0
function! XpMenuOn()
  let g:hXpMenu = LoadLibrary('xpmenu.dll')
  if g:hXpMenu != 0
    call CallLibrary(g:hXpMenu, "EnableXpMenu", "on")
  endif
endfunction

function! XpMenuOff()
  if g:hXpMenu != 0
    call CallLibrary(g:hXpMenu, "EnableXpMenu", "")
    "call FreeLibrary(g:hXpMenu)
    "let g:hXpMenu = 0
  endif
endfunction

function! XpMenuSetLeftBack(r, g, b)
  if g:hXpMenu != 0
    call CallLibrary(g:hXpMenu, "SetLeftBack", a:r.",".a:g.",".a:b)
  endif
endfunction

function! XpMenuSetRightBack(r, g, b)
  if g:hXpMenu != 0
    call CallLibrary(g:hXpMenu, "SetRightBack", a:r.",".a:g.",".a:b)
  endif
endfunction

function! XpMenuSetGrayBack(r, g, b)
  if g:hXpMenu != 0
    call CallLibrary(g:hXpMenu, "SetGrayBack", a:r.",".a:g.",".a:b)
  endif
endfunction

function! XpMenuSetFocusBack(r, g, b)
  if g:hXpMenu != 0
    call CallLibrary(g:hXpMenu, "SetFocusBack", a:r.",".a:g.",".a:b)
  endif
endfunction

function! XpMenuSetFocusFore(r, g, b)
  if g:hXpMenu != 0
    call CallLibrary(g:hXpMenu, "SetFocusFore", a:r.",".a:g.",".a:b)
  endif
endfunction

function! XpMenuSetFrameLine(r, g, b)
  if g:hXpMenu != 0
    call CallLibrary(g:hXpMenu, "SetFrameLine", a:r.",".a:g.",".a:b)
  endif
endfunction

function! XpMenuSetFont(name)
  if g:hXpMenu != 0
    call CallLibrary(g:hXpMenu, "SetFont", a:name)
  endif
endfunction

autocmd GUIEnter :call XpMenuOn()
autocmd VimLeavePre :call XpMenuOff()
call XpMenuOn()

" Sample
"call XpMenuSetFocusBack(0, 0, 0)
"call XpMenuSetFocusFore(255, 255, 255)
"call XpMenuSetFont("Tahoma:h12")
