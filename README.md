
# Credit

Big thanks to https://github.com/gsingh93/display-manager/tree/master for the main outline for using pam and https://gsgx.me/posts/how-to-write-a-display-manager/. I rewrote a large portion of their code for use in this project. Another huge thanks to https://github.com/fairyglade/ly/tree/master for inspiration on the interface design and the mechanisms for starting a new user session

# Requirements

This requires the libpam-dev, libgtk-4-dev, libncurses-dev package to be installed

# Compilation

As usual, use `make microdm` to make the main executable.
`sudo make install` will make and install the package.

# About

I just wanted to learn a little more about how linux logins work, so made a (not very good) display manager. Feel free to use, but no guarantee that i will maintain the project.
