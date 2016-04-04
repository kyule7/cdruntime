#!/bin/bash
if [ $1 == reexecute ]
then
  count=0
  for var in "$@"
  do
    touch reexecute.$var
  done
  sleep 2
  rm reexecute.*
elif [ $1 == escalate ]
then
  count=0
  for var in "$@"
  do
    touch escalate.$var
  done
  sleep 1
  rm escalate.*
fi

