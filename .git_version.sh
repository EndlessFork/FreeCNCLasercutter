#!/bin/sh
t=`git describe --always --dirty --tags`
n=`git log -n 1 --pretty=format:"(%s)" --date=short`

#h=`git log --pretty=format:\"%h %ad | %s%d [%an]\" --graph --date=short`

echo "#define VERSION \"$t $n\""
