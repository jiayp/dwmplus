#!/bin/bash
function vim_replace()
{
    if [[ $# != 3 ]];then
        return
    fi

    pattern=$2
    rep_pattern=$3
    filelist=`cat $1`
    repeatfile=""
    for arg in $filelist
    do
        if [[ $arg != $repeatfile ]];then
            sed "s/$pattern/$rep_pattern/" $arg  > ~/.gvim_repfile
            rm $arg 
            cp -rf ~/.gvim_repfile $arg
            rm  ~/.gvim_repfile
        fi
        repeatfile=$arg
    done
}

vim_replace $@
