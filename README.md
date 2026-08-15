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

### In progress
* [ ] Signal handling: Ctrl-C interrupts child, not the shell

### Todo
* [ ] Colorize `ls` output (needs isatty — child's stdout must look like a terminal)
* [ ] Split code into multiple files + Makefile
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
