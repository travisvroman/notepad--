let SessionLoad = 1
let s:so_save = &g:so | let s:siso_save = &g:siso | setg so=0 siso=0 | setl so=-1 siso=-1
let v:this_session=expand("<sfile>:p")
silent only
silent tabonly
cd ~/development/notepad--
if expand('%') == '' && !&modified && line('$') <= 1 && getline(1) == ''
  let s:wipebuf = bufnr('%')
endif
let s:shortmess_save = &shortmess
if &shortmess =~ 'A'
  set shortmess=aoOA
else
  set shortmess=aoO
endif
badd +104 notepadmm/src/application.c
badd +446 notepadmm/src/systems/font_system.c
badd +22 ~/development/notepad--/notepadmm/src/renderer/opengl/gl_types.h
badd +22 ~/development/notepad--/notepadmm/src/renderer/opengl/gl_backend.h
badd +239 ~/development/notepad--/notepadmm/src/renderer/opengl/gl_backend.c
badd +13 ~/development/notepad--/notepadmm/src/platform/platform.h
badd +80 ~/development/notepad--/notepadmm/src/platform/platform.c
badd +198 ~/development/notepad--/notepadmm/src/math/math_types.h
argglobal
%argdel
edit notepadmm/src/application.c
let s:save_splitbelow = &splitbelow
let s:save_splitright = &splitright
set splitbelow splitright
wincmd _ | wincmd |
vsplit
1wincmd h
wincmd w
let &splitbelow = s:save_splitbelow
let &splitright = s:save_splitright
wincmd t
let s:save_winminheight = &winminheight
let s:save_winminwidth = &winminwidth
set winminheight=0
set winheight=1
set winminwidth=0
set winwidth=1
exe 'vert 1resize ' . ((&columns * 99 + 100) / 200)
exe 'vert 2resize ' . ((&columns * 100 + 100) / 200)
argglobal
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let &fdl = &fdl
let s:l = 65 - ((18 * winheight(0) + 18) / 37)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 65
normal! 0
wincmd w
argglobal
if bufexists(fnamemodify("~/development/notepad--/notepadmm/src/renderer/opengl/gl_backend.c", ":p")) | buffer ~/development/notepad--/notepadmm/src/renderer/opengl/gl_backend.c | else | edit ~/development/notepad--/notepadmm/src/renderer/opengl/gl_backend.c | endif
if &buftype ==# 'terminal'
  silent file ~/development/notepad--/notepadmm/src/renderer/opengl/gl_backend.c
endif
balt ~/development/notepad--/notepadmm/src/renderer/opengl/gl_backend.h
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let &fdl = &fdl
let s:l = 122 - ((13 * winheight(0) + 18) / 37)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 122
normal! 069|
wincmd w
2wincmd w
exe 'vert 1resize ' . ((&columns * 99 + 100) / 200)
exe 'vert 2resize ' . ((&columns * 100 + 100) / 200)
tabnext 1
if exists('s:wipebuf') && len(win_findbuf(s:wipebuf)) == 0 && getbufvar(s:wipebuf, '&buftype') isnot# 'terminal'
  silent exe 'bwipe ' . s:wipebuf
endif
unlet! s:wipebuf
set winheight=1 winwidth=20
let &shortmess = s:shortmess_save
let &winminheight = s:save_winminheight
let &winminwidth = s:save_winminwidth
let s:sx = expand("<sfile>:p:r")."x.vim"
if filereadable(s:sx)
  exe "source " . fnameescape(s:sx)
endif
let &g:so = s:so_save | let &g:siso = s:siso_save
set hlsearch
doautoall SessionLoadPost
unlet SessionLoad
" vim: set ft=vim :
