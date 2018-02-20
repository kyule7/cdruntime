#!/bin/bash
ls -1 *.json | awk -F'/' '{print $NF}' | xargs echo -n > file_list.txt
