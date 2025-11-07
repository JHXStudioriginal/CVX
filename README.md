# CVX is a simple, lightweight shell written in C, designed to be fast, flexible, and pleasant to use.

### Features:
* Full support for external commands, pipes, and redirections
* Built-in commands like `cd`, `pwd` (with options), `help`, `!!`, and more
* Command history
* Configurable through /etc/cvx.conf:
  - Custom prompt
  - Aliases
  - Startup directory
  - History toggle
* Supports aliases, configuration files, and customizable startup behavior
* Simple and fast C implementation
* Line editing powered by [linenoise](https://github.com/antirez/linenoise)

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
