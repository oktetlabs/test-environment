#! /bin/sh

# Usage: what moderator participant ...

# Obtain doclist
cd /home/helen/work/qms/
cvs -z9 update doclist

#cp /home/helen/work/qms/doclist .

# Obtain the last document number in doclist
awk '
/-/ { last = $1; next ; }
END { split(last,parts,"-") ; 
      n = parts[3] + 1 ; 
      printf "OKT-QMSCRR-%07d", n ; } ' doclist > /tmp/crr_id

CRR_ID=`cat /tmp/crr_id`
CRR_FILE=${CRR_ID}".txt"

echo $CRR_ID"      Code Review Report for "$1 >> doclist

cvs commit -m " " doclist

cd /home/helen/work/test_environment/doc/crr

echo "ID "$CRR_ID > $CRR_FILE
echo >> $CRR_FILE
echo "COPYRIGHT Copyright(C) OKTET Ltd. 2003" >> $CRR_FILE
echo >> $CRR_FILE
echo "RESPMGR Elena Vengerova" >> $CRR_FILE
echo >> $CRR_FILE
echo "SOURCES" >> $CRR_FILE
echo >> $CRR_FILE
echo "Repository: galba Module: test_environment" >> $CRR_FILE
echo >> $CRR_FILE

echo -n "MODERATOR" >> $CRR_FILE
finger $2 | awk '/Login/ { split($0, array, ":") ; printf "%s", array[3] ; exit ; }' > /tmp/fullname 
cat /tmp/fullname >> $CRR_FILE
echo " <"$2"@oktetlabs.ru>" >> $CRR_FILE
echo >> $CRR_FILE

shift 2

for i in "$@" ; do

echo -n "PARTICIPANT" >> $CRR_FILE ;
finger $i | awk '/Login/ { split($0, array, ":") ; printf "%s", array[3] ; exit ; }' > /tmp/fullname ;
cat /tmp/fullname >> $CRR_FILE ;
echo " <"$i"@oktetlabs.ru>" >> $CRR_FILE ;
echo >> $CRR_FILE ;

done

echo "RESULT" >> $CRR_FILE
echo >> $CRR_FILE

cvs add $CRR_FILE
cvs commit -m " " $CRR_FILE
