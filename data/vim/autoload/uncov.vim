" Vim plugin for querying coverage information from uncov command-line tool.

" Maintainer: xaizek <xaizek@posteo.net>
" Last Change: 2020 April 24
" License: Same terms as Vim itself (see `help license`)

""""""""""""""""""""""""""""""""""""""""""""""""""""""""

let s:initialized = 0

function! uncov#ShowCoverage(reload, ...) abort
    if !s:initialized
        let s:initialized = 1
        call s:Initialize()
    endif

    let l:buildid = '@@'
    if a:0 > 0
        if a:1 !~ '^@-\?\d\+\|@@$'
            echohl ErrorMsg | echo 'Wrong argument:' a:1 | echohl None
            return
        endif
        let l:buildid = a:1
    endif

    if a:reload
        let l:repo = b:uncovRepo
        let l:relFilePath = b:uncovRelFilePath
    else
        let l:repo = fugitive#repo()
        let l:relFilePath = FugitivePath(@%, '')
    endif

    call s:MakeBuffer(l:repo, l:relFilePath, l:buildid, a:reload)
endfunction

function! s:Initialize()
    augroup Uncov
        autocmd! ColorScheme *
                    \  highlight UncovCovered ctermbg=darkgreen guibg=darkgreen
                    \| highlight UncovMissed ctermbg=darkred guibg=darkred
    augroup end
    doautocmd Uncov ColorScheme

    " mind that text is set to unbreakable space
    sign define UncovCovered text=  texthl=UncovCovered
    sign define UncovMissed text=  texthl=UncovMissed
endfunction

function! s:MakeBuffer(repo, relFilePath, buildid, reload) abort
    let l:coverageInfo = systemlist('uncov '.a:repo.dir().' get '.a:buildid.' /'
                                  \.shellescape(a:relFilePath))
    if v:shell_error != 0
        let l:errorMsg = 'uncov error: '.join(l:coverageInfo, '\n')
        echohl ErrorMsg | echo l:errorMsg | echohl None
        return
    endif
    let l:commit = l:coverageInfo[0]

    let l:gitArgs = ['show', l:commit.':'.a:relFilePath]
    let l:cmd = call(a:repo.git_command, l:gitArgs, a:repo)
    let l:fileLines = systemlist(l:cmd)
    if v:shell_error != 0
        let l:errorMsg = 'git error: '.join(l:fileLines, '\n')
        echohl ErrorMsg | echo l:errorMsg | echohl None
        return
    endif

    let l:cursPos = getcurpos()

    if a:reload
        enew
    else
        tabedit
    endif

    let b:uncovRepo = a:repo
    let b:uncovRelFilePath = a:relFilePath

    let l:coverage = l:coverageInfo[1:]
    let [l:loclist, b:uncovCovered, b:uncoRelevant] =
                \ s:ParseCoverage(l:coverage, l:fileLines)

    " XXX: insert buildid here?
    execute 'silent!' 'file' escape('uncov:'.a:relFilePath, ' \%')
    call setline(1, l:fileLines)
    setlocal buftype=nofile bufhidden=wipe noswapfile nomodified nomodifiable

    execute 'doautocmd BufRead' a:relFilePath
    execute 'doautocmd BufEnter' a:relFilePath

    call cursor(l:cursPos[1:])
    call setloclist(0, l:loclist)
    call s:FoldCovered(l:coverage)

    " make sure line under the cursor is not folded
    silent! normal! zO

    command -buffer UncovInfo call s:PrintCoverageInfo()
    command -buffer -nargs=? Uncov call uncov#ShowCoverage(1, <f-args>)

    UncovInfo
endfunction

function! s:ParseCoverage(coverage, fileLines) abort
    let l:loclist = []
    let l:bufnr = bufnr('%')

    let l:lineNo = 1
    let l:relevant = 0
    let l:covered = 0
    for l:hits in a:coverage
        let l:hits = str2nr(l:hits)
        if l:hits != -1
            let l:group = (l:hits == 0) ? 'UncovMissed' : 'UncovCovered'
            execute 'sign place' l:lineNo
                  \ 'line='.l:lineNo
                  \ 'name='.l:group
                  \ 'buffer='.l:bufnr
            if l:hits == 0
                let l:loclist += [{
                    \ 'bufnr': l:bufnr,
                    \ 'lnum': l:lineNo,
                    \ 'text': '(not covered) '.a:fileLines[l:lineNo - 1] }]
            else
                let l:covered += 1
            endif
            let l:relevant += 1
        endif

        let l:lineNo += 1
    endfor

    return [l:loclist, l:covered, l:relevant]
endfunction

function! s:FoldCovered(coverage) abort
    let l:nLines = line('$')

    let l:lineNo = 1
    let l:toFold = 0
    for l:hits in a:coverage
        if str2nr(l:hits) == 0
            if l:toFold > 3
                let l:startContext = (l:toFold == l:lineNo - 1 ? 0 : 1)
                execute (l:lineNo - (l:toFold - l:startContext)).','
                      \.(l:lineNo - 1 - 1)
                      \.'fold'
            endif

            let l:toFold = 0
        else
            let l:toFold += 1
        endif

        let l:lineNo += 1

        " handle case when coverage information contains more lines than the
        " file gracefully by adjusting line number and number of lines to fold
        " to do not go past the end of the buffer
        if l:lineNo > l:nLines + 1
            let l:lineNo -= 1
            if l:toFold > 0
                let l:toFold -= 1
            endif
        endif
    endfor

    if l:toFold > 3
        let l:startContext = (l:toFold == len(a:coverage) ? 0 : 1)
        execute (l:lineNo - (l:toFold - l:startContext)).','
              \.(l:lineNo - 1)
              \.'fold'
    endif
endfunction

function! s:PrintCoverageInfo() abort
    " redraw is to avoid message disappearing (see `:help :echo-redraw`)
    redraw

    let l:covered = b:uncovCovered
    let l:relevant = b:uncoRelevant

    let l:coverage = l:relevant != 0 ? 1.0*l:covered/l:relevant : 1.0
    echomsg printf('Coverage: %3.2f%% (%d/%d)', 100.0*l:coverage,
                 \ l:covered, l:relevant)
endfunction
