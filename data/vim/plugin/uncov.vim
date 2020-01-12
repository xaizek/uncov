" Vim plugin for querying coverage information from uncov command-line tool.

" Maintainer: xaizek <xaizek@posteo.net>
" Last Change: 2020 January 10
" License: Same terms as Vim itself (see `help license`)

""""""""""""""""""""""""""""""""""""""""""""""""""""""""

if exists('loaded_uncov')
    finish
endif
let loaded_uncov = 1

command! -nargs=? Uncov call uncov#ShowCoverage(0, <f-args>)

" vim: set tabstop=4 softtabstop=4 shiftwidth=4 expandtab :
