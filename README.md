# CVX is a simple, lightweight shell written in C, designed to be fast, flexible, and pleasant to use.

### Features:
* Runs **normal Linux commands**, supports **pipes** and **redirections** (`>`, `>>`, `<`, `<<` heredoc)
* Built-in commands:
  - `cd` (with `~` support)
  - `pwd` (`-L` logical, `-P` physical, `--help`)
  - `help` (shows all built-ins)
  - `ls` (auto `--color=auto`)
  - `history` + `!!` (repeat last command)
  - `echo` (supports env vars)
  - `export [VAR=value]`
  - `exit`
* Command chaining with `&&`
* Pipelines with `|`
* Aliases & config:
  - Custom prompt, startup dir, history toggle via `/etc/cvx.conf`
* Arguments:
  - `cvx --version`, `cvx -v`, `cvx -version`
  - `cvx -c "<command>"` → run it and exit
  - `cvx -l` → loads `/etc/profile` + `~/.profile`
* Tiny, fast C implementation
* Line editing powered by [linenoise](https://github.com/antirez/linenoise)

###### See wiki on project page to read more.

### Installation:

Using Git + Make

1. Clone the repository:

`git clone https://github.com/JHXStudioriginal/CVX-Shell.git`

`cd CVX-Shell`

2. Build with make:

`sudo make install`

3. Just run it

`cvx`

---------------------------------------------------------------------------------------------------------------------------------------------

##### CVX Shell includes [linenoise](https://github.com/antirez/linenoise) by [antirez](https://github.com/antirez), which is licensed under the BSD 2-Clause License. See relevant files for details.
