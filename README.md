# xsh

A minimal Unix shell written from scratch in C. Built to learn OS
fundamentals: processes, exec, file descriptors, pipes, and redirection.

## Build

gcc -Wall -Wextra -g shell.c -o myshell
./myshell

## Features

### Done
* [x] Read–eval loop with prompt (`xsh> `)
* [x] Run external commands via fork + execvp + wait
* [x] Argument parsing (split on spaces with strtok)
* [x] Built-in: `exit`
* [x] Built-in: `cd`
* [x] Empty-line guard (no segfault on blank Enter)
* [x] Clean exit on Ctrl-D (EOF)
* [x] Single pipe (`ls | grep c`)
* [x] Output redirection (`ls > out.txt`)
* [x] Input redirection (`wc < in.txt`)
* [x] N-pipes / arbitrary pipeline (`a | b | c | d`)
* [x] Combine pipe + redirection (`ls | grep c > out.txt`)
* [x] Append redirection (`>>`)
* [x] Prompt shows current working directory (getcwd), with `~` for $HOME
* [x] Colored prompt + red errors (ANSI escape codes)
* [x] Handle bare `cd` / `cd ~` (go to $HOME) and report cd errors

* [x] Signal handling: Ctrl-C interrupts child, not the shell
* [x] Split tokens on tabs too, not just spaces
* [x] Alias table — `ls`/`grep`/`diff` get `--color=auto` automatically

### In progress
* [ ] Split code into multiple files + Makefile

### Todo
* [ ] Redirection is "first one wins": only one `>` and one `<` per stage, and quoting is ignored (`echo "a > b"` still redirects)
* [ ] Swap `signal()` for `sigaction()` (signal's semantics vary across systems)
* [ ] Read aliases from a config file instead of a hardcoded table
* [ ] (stretch) Command history
* [ ] (stretch) Tab completion
* [ ] (stretch) Background jobs with `&`

## Notes / learnings
* strtok is destructive; it mutates the string, so check the raw input (e.g. for `|` or `>`) BEFORE tokenizing.
* dup2 doesn't care what's on the other end; pipe or file, it's all just file descriptors. "Everything is a file."
* Built-ins like cd/exit can't be forked; a child can't change the parent's directory or kill the parent.
* Unclosed pipe write-ends cause the reader to hang forever (no EOF).
* N-pipes is a sliding window: carry the previous pipe's read end in a variable (`in`, starting at 0 = stdin). Each stage reads from it and writes to the next new pipe. The parent must close its copy of both ends every iteration, or the pipeline hangs.
* Order of checks matters. Splitting on `>` before `|` breaks `ls | grep c > out.txt`, because the whole pipeline gets handed to the first command as arguments. Branch on `|` first, then let each stage handle its own redirection.
* ANSI colors are just bytes the terminal intercepts: `ESC [ 1;32 m` = bold green. Always emit RESET after, or the color bleeds into every later line.
* `~` is expanded by the shell, not the kernel. `chdir("~")` looks for a directory literally named `~` and fails — you have to swap in $HOME yourself.
* Ignoring a return value hides bugs: `chdir()` failed silently for ages. It only became visible once the prompt displayed the cwd.
* Signals are inherited across fork, exactly like fds are. The shell sets `SIG_IGN` for SIGINT so Ctrl-C doesn't kill it; but then every child inherits that ignore and becomes uninterruptible, so each child must `SIG_DFL` right after fork. Same lesson as closing inherited pipe ends: fork copies state you didn't mean to copy.
* Ctrl-C goes to the whole foreground process group, not one process. That's why the shell died alongside the child before this fix.
* `ls --color=auto` colorizes only when stdout is a terminal — ls calls `isatty(1)` itself and stays quiet when piped. `--color=always` would force escape codes into the pipe and corrupt whatever reads it. Programs adapting their output to whether stdout is a tty is why `ls | cat` prints one file per line but bare `ls` prints columns.
