# ~/.bashrc - RROS
case $- in *i*) ;; *) return ;; esac
HISTCONTROL=ignoreboth
HISTSIZE=5000
shopt -s histappend checkwinsize globstar
PS1='\[\e[38;2;124;92;255m\]rros\[\e[0m\] \[\e[38;2;34;211;238m\]\w\[\e[0m\] \[\e[38;2;124;92;255m\]➜\[\e[0m\] '
alias ls='ls --color=auto'
alias ll='ls -lah'
alias ..='cd ..'
alias r='rros'
alias ri='rros info'
alias rc='rros config'
alias rt='rros theme'
