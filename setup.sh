#!/bin/bash

function creat_cscope()
{
    if [ -e "cscope.files" ]; then
        rm -rf cscope.files
    fi

    echo "create cscope.files in $@"
    for arg in $@
    do
        find  ${arg}  -type f |sed  -e '/ /d' -n -e '/\.\(\([chmSs]\)\|\(java\)\|\(cpp\)\|\(hp\)\|\(cc\)\|\(def\)\)$/p' >> cscope.files
    done

    echo "create cscope"
    cscope -bki cscope.files
}

function csset()
{
    if [ $# -eq 0 ];then
        echo "need dirname"
        return
    fi

    csclean 
    creat_cscope $@

    echo "create tags"
    ctags --c++-kinds=+px --fields=+iaS --extra=+q  -L cscope.files

    echo "create  success"
}

function csseta()
{
    if [ $# -eq 0 ];then
        echo "need dirname"
        return 
    fi

    csclean
    creat_cscope $@

    rm  -rf cscope.files tags TAGS

    for arg in $@
    do
        find ${arg}  -type f | sed '/\/\.\| \|\.o\|\.taghl\|tags\|cscope\|\.a/d'  >> cscope.files
    done

    echo "create tags"
    ctags --c++-kinds=+px --fields=+iaS --extra=+q  -L cscope.files

    echo "create  success"
}


function csclean()
{
    if [ $# -eq 0 ];then
        rm  -rf cscope* tags TAGS ncscope* 
    else
        for arg in $@
        do
            find $arg  -type f -name "cscope.*" -o -name tags |xargs rm -rf
        done
    fi
}
function cscleana()
{
    if [ $# -eq 0 ];then
        rm  -rf cscope* tags TAGS *.taghl 
    else
        for arg in $@
        do
            find $arg  -type f -name "cscope.*" -o -name tags -o  -name "*.taghl" -o -name TAGS |xargs rm -rf
        done
    fi
}

function ll()
{
    ls -l $*
}
