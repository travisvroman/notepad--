let SessionLoad = 1
let s:so_save = &g:so | let s:siso_save = &g:siso | setg so=0 siso=0 | setl so=-1 siso=-1
let v:this_session=expand("<sfile>:p")
silent only
silent tabonly
cd ~/development/notepadmm
if expand('%') == '' && !&modified && line('$') <= 1 && getline(1) == ''
  let s:wipebuf = bufnr('%')
endif
let s:shortmess_save = &shortmess
if &shortmess =~ 'A'
  set shortmess=aoOA
else
  set shortmess=aoO
endif
badd +154 notepadmm/src/application.c
badd +1 ~/development/notepadmm/.clangd
badd +3 ~/development/notepadmm/vendor/src/glad.c
badd +15 ~/development/notepadmm/notepadmm/src/resources/resource_types.h
badd +2 ~/development/notepadmm/notepadmm/compile_flags.txt
badd +42 notepadmm/src/application.h
badd +66 ~/development/notepadmm/notepadmm/src/renderer/opengl/gl_backend.c
badd +13 ~/development/notepadmm/notepadmm/src/main.c
badd +1 ~/development/notepadmm/notepadmm/src/defines.h
badd +496 notepadmm/src/ui/sui_label.c
argglobal
%argdel
edit notepadmm/src/application.h
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
exe 'vert 1resize ' . ((&columns * 121 + 122) / 244)
exe 'vert 2resize ' . ((&columns * 122 + 122) / 244)
argglobal
balt ~/development/notepadmm/notepadmm/src/defines.h
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
let s:l = 42 - ((41 * winheight(0) + 28) / 57)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 42
normal! 018|
wincmd w
argglobal
if bufexists(fnamemodify("notepadmm/src/ui/sui_label.c", ":p")) | buffer notepadmm/src/ui/sui_label.c | else | edit notepadmm/src/ui/sui_label.c | endif
if &buftype ==# 'terminal'
  silent file notepadmm/src/ui/sui_label.c
endif
balt notepadmm/src/application.h
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
let s:l = 496 - ((56 * winheight(0) + 28) / 57)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 496
normal! 02|
wincmd w
2wincmd w
exe 'vert 1resize ' . ((&columns * 121 + 122) / 244)
exe 'vert 2resize ' . ((&columns * 122 + 122) / 244)
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
