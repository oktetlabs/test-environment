#! /bin/bash

#rm -rf /tmp/te-replace
#mkdir /tmp/te-replace
#find . -name *.h > /tmp/te-replace/files
#find . -name *.h >> /tmp/te-replace/files
#tar czf /tmp/te-replace/files.tgz `cat /tmp/te-replace/files`
#TE_DIR=`pwd`
#cd /tmp/te-replace
#
#for i in `cat files`

for i in *.h *.c ; do
    cat $i | sed -e "s/LOG_TRACE *\([^(]*(\)[^,]*, */VERB\1/" |
             sed -e "s/LOG_DEBUG *\([^(]*(\)[^,]*, */ERROR\1/" |
             sed -e "s/LOGS_TRACE *\([^(]*(\)[^,]*, */VERB\1/" |
             sed -e "s/LOGS_DEBUG *\([^(]*(\)[^,]*, */ERROR\1/" |
             sed -e "s/LOGF_TRACE *\([^(]*(\)[^,]*, */F_VERB\1/" |
             sed -e "s/LOGF_DEBUG *\([^(]*(\)[^,]*, */F_ERROR\1/" > x
    mv x $i ; 
done

#cd $TE_DIR
#tar xzf /tmp/te-replace/files.tgz
#rm -rf /tmp/te-replace