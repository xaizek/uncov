" Vim plugin for querying coverage information from uncov command-line tool.

" Maintainer: xaizek <xaizek@posteo.net>
" Last Change: 2020 January 10
" License: Same terms as Vim itself (see `help license`)

""""""""""""""""""""""""""""""""""""""""""""""""""""""""

if exists('loaded_uncov')
    finish
endif
let loaded_uncov = 1

augroup Uncov
    autocmd! ColorScheme *
                \  highlight UncovCovered ctermbg=darkgreen guibg=darkgreen
                \| highlight UncovMissed ctermbg=darkred guibg=darkred
augroup end
doautocmd Uncov ColorScheme

" mind that text is set to unbreakable space
sign define UncovCovered text=  texthl=UncovCovered
sign define UncovMissed text=  texthl=UncovMissed

command! -nargs=? Uncov call uncov#ShowCoverage(0, <f-args>)

" vim: set tabstop=4 softtabstop=4 shiftwidth=4 expandtab :
