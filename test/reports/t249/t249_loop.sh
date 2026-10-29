#!/bin/sh
i=0
while [ $i -lt 60 ]; do
  power-shell wakeup >/dev/null 2>&1
  uitest uiInput longClick 640 1600 >/dev/null 2>&1
  i=$((i+1))
done
