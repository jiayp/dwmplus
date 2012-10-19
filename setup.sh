#!/bin/bash
function csset()
{
	csclean 
	echo "create cscope.files in $@"
	for arg in $@
	do
		find ${arg} -type f -name "*.[chmSs]" -o -name "*.java" -o -name "*.cpp" \
		      	-o -name "*.hp" -o -name "*.cc"  >> cscope.files
	done

	echo "create cscope"
#cscope -bkqi cscope.files
	 cscope -bki cscope.files

	echo "create tags"
	ctags --c++-kinds=+px --fields=+iaS --extra=+q  -L cscope.files
#	echo "create TAGS"
#	cat cscope.files|etags -
	echo "create  success"
}

function csseta()
{
	rm  -rf cscope.* tags TAGS  
	echo "create cscope.files in $@"
	for arg in $@
	do
		find ${arg}  -type f >> cscope.files
	done

	echo "create cscope"
	#cscope -bkqi cscope.files
	cscope -bki cscope.files

	echo "create tags"
	ctags --c++-kinds=+px --fields=+iaS --extra=+q  -L cscope.files
#	echo "create TAGS"
#	etags -L cscope.files
#	cat cscope.files|etags -
	echo "create  success"
}


function csclean()
{
	if [ $# -eq 0 ];then
		rm  -rf cscope* tags TAGS 
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
