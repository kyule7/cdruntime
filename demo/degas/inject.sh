#!/bin/bash
if [ $1 == reexecute ]
then
  count=0
  for var in "$@"
  do
    touch reexecute.$var
  done
  sleep 2.5
  rm reexecute.*
elif [ $1 == escalate ]
then
  count=0
  for var in "$@"
  do
    touch escalate.$var
  done
  sleep 1.2
  rm escalate.*
fi

