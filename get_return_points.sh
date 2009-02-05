#!/bin/bash
grep 'exec_NODE(' exec_run.c |
sed -e 's/^.*, \([0-9]\+\),.*$/		switchline(\1) \\/;t;d'
