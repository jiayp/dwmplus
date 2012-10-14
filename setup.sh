#!/bin/bash 
function csset(){
echo "create cscope.files"
for arg in "$@"
do
    echo "find *.c *.h  *.cpp  *.S *.java  in $arg"
    find $arg -name "*.[chS]" -o -name "*.cpp" \
           -o -name "*.java"  >> cscope.files
 done

 echo "ctags -L cscope.files"
 ctags  -L cscope.files
 echo "creat cscope.out"
 cscope -bkq -i cscope.files

 echo "end success"

}

function csseta(){
echo "create cscope.files"
for arg in "$@"
do
    find $arg -type f  >> cscope.files
 done

 echo "ctags -L cscope.files"
 ctags  -L cscope.files
 echo "creat cscope.out"
 cscope -bkq -i cscope.files

 echo "end success"
}

function csclean(){
rm cscope.* tags
}
function kcs()
{
    echo "create cscope.files"

    find .  -name "*.[chS]"  -o -name "*.cpp" \
           -o -name "*.java" |sed -e '/^.\/arch/d' > cscope.files

    find arch/arm  -name "*.[chS]"  -o -name "*.cpp"  >> cscope.files

    echo "create tags"
        ctags -L cscope.files
    echo "create cscope.out"
        cscope -bkq -icscope.files


}
function ll()
{
    ls -l $*
}
