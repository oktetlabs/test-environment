#! /bin/sh

# Usage: DDRID moderator participant ...

# Obtain doclist
cd /home/helen/work/qms/
cvs -z9 update doclist

#cp /home/helen/work/qms/doclist .

# Obtain the last document number in doclist
awk '
/-/ { last = $1; next ; }
END { split(last,parts,"-") ; 
      n = parts[3] + 1 ; 
      printf "OKT-QMSDRR-%07d", n ; } ' doclist > /tmp/drr_id

DRR_ID=`cat /tmp/drr_id`
DRR_FILE=${DRR_ID}".txt"

echo $DRR_ID"      Review report for "$1 >> doclist

cvs commit -m " " doclist

cd /home/helen/work/test_environment/doc/drr

echo "ID "$DRR_ID > $DRR_FILE
echo >> $DRR_FILE
echo "COPYRIGHT Copyright(C) OKTET Ltd. 2003" >> $DRR_FILE
echo >> $DRR_FILE
awk '
BEGIN { id=0 ; }
/DOCID/ { if (id == 0) { id=1 ; print $0 ; } next ; }
{ if (id == 1) { print $0 ; id=2 ; } next ;}
' /home/helen/work/test_environment/doc/ddr/${1}.txt >> $DRR_FILE
echo >> $DRR_FILE
echo "DDRID "$1 >> $DRR_FILE
echo >> $DRR_FILE
echo "RESPMGR Elena Vengerova" >> $DRR_FILE
echo >> $DRR_FILE

echo -n "MODERATOR" >> $DRR_FILE
finger $2 | awk '/Login/ { split($0, array, ":") ; printf "%s", array[3] ; exit ; }' > /tmp/fullname 
cat /tmp/fullname >> $DRR_FILE
echo " <"$2"@oktetlabs.ru>" >> $DRR_FILE
echo >> $DRR_FILE

shift 2

for i in "$@" ; do

echo -n "PARTICIPANT" >> $DRR_FILE ;
finger $i | awk '/Login/ { split($0, array, ":") ; printf "%s", array[3] ; exit ; }' > /tmp/fullname ;
cat /tmp/fullname >> $DRR_FILE ;
echo " <"$i"@oktetlabs.ru>" >> $DRR_FILE ;
echo >> $DRR_FILE ;

done

echo "RESULT" >> $DRR_FILE
echo >> $DRR_FILE

cvs add $DRR_FILE
cvs commit -m " " $DRR_FILE
