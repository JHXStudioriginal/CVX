CVX is a simple, lightweight shell written in C, perfect for experimenting.

Features:

Executes basic system commands

Line editing powered by linenoise

Minimalist and easy-to-extend code structure

CVX is opensource, source code is in "src" folder

INSTALLATION
Method 1: Using GCC

1.Go to the src folder:

cd /path/to/cvx/src

2.Compile the program:

gcc cvx.c linenoise.c -o cvx -Wall -Wextra -lm

Run it:

./cvx

Method 2: Using Git + Make

1.Clone the repository:

git clone https://github.com/your-repo/cvx.git

cd cvx

2.Build with make:

make

3.Run it

./cvx
