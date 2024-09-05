#!/bin/sh

. ./build.sh;

project=${PWD##*/}
session="debug-rpi-pico-${project}"

if [[ $(tmux has-session "${session}") ]]; then
	tmux kill-session -t "${session}";
fi

tmux new -d -s "${session}" './listen.sh';
tmux splitw -t "${session}" -dh './upload.sh';

tmux attach -t "${session}";
