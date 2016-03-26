#!/bin/bash
# -------------------------------------------------------------------------
# This takes the .c file output from BitfontCreator and adds some stuff
# needed when it gets converted to a loadable module.
# -------------------------------------------------------------------------
  
if [ $# -ne 1 ];then 
        echo no file specified
        exit 1
fi

SRC=$1
FNTNAME=${1%.c}
TMPFILE="/tmp/mkFntSrc.tmp"

echo SRC = $SRC         >&2
echo FNTNAME = $FNTNAME >&2

#
# Convert from MSDOS to unix line endings
#
tr -d '' < $SRC > fntTmp
mv fntTmp $SRC


echo 'const char *fontName = "'$FNTNAME'";' > $TMPFILE

exec <$1

addStuff()
{
    case "$1" in
        "const unsigned char data_table"*)
            printf "const unsigned char %s_data_table[] = {\n" $FNTNAME
            ;;

        "const unsigned int offset_table"*)
            printf "const unsigned int %s_offset_table[] = {\n" $FNTNAME
            printf "offset_table_found\n" >> $TMPFILE
            ;;

        "const unsigned char index_table"*)
            printf "const unsigned char %s_index_table[] = {\n" $FNTNAME
            ;;

        "const unsigned char width_table"*)
            printf "const unsigned char %s_width_table[] = {\n" $FNTNAME
            printf "width_table_found\n" >> $TMPFILE
            ;;

        *"Font width"*)
            set -- $1
            if [ "$3" = "varialbe" ];then # yes, it's really spelled like that
                printf "const unsigned int %s_FontWidth = 0;\n" $FNTNAME   >> $TMPFILE
            else
                printf "const unsigned int %s_FontWidth = %d;\n" $FNTNAME $3   >> $TMPFILE
            fi
            ;;

         *"Font height"*)
            set -- $1
            printf "const unsigned int %s_FontHeight = %d;\n" $FNTNAME  $3     >> $TMPFILE
            ;;

        *"Data length"*)
            set -- $1
            if [ "$3" = "varialbe" ];then
                printf "const unsigned int %s_DataLength = -1;\n" $FNTNAME     >> $TMPFILE
            else
                printf "const unsigned int %s_DataLength = %d;\n" $FNTNAME  $3 >> $TMPFILE
            fi
            ;;

        *)
            echo "$@"
            ;;
    esac
}


#
# Add variables for width, height, etc
#
while read line;do 
    addStuff "$line"
done

#
# A couple tables are not there for fixed width fonts. If they are there
# the tmp file will have lines saying so. If not, add dummy tables
#
tmp=`cat $TMPFILE`
if [[ $tmp == *offset_table_found* ]]; then
    :
else
    printf "const unsigned int %s_offset_table[] = {0};\n" $FNTNAME
fi

if [[ $tmp == *width_table_found* ]]; then
    :
else
    printf "const unsigned char %s_width_table[] = {0};\n" $FNTNAME
fi

#
# Now we can append the stuff we added. Don't add the flags saying that
# the optional tables are there.
#
while read line; do
    case "$line" in
        *width_table_found*)
            ;;
        *offset_table_found*)
            ;;
        *)
            echo "$line"
            ;;
    esac
done < $TMPFILE

cat <<EOF
const unsigned int ${FNTNAME}_dataTableLength   = sizeof ${FNTNAME}_data_table;
const unsigned int ${FNTNAME}_offsetTableLength = sizeof ${FNTNAME}_offset_table;
const unsigned int ${FNTNAME}_indexTableLength  = sizeof ${FNTNAME}_index_table;
const unsigned int ${FNTNAME}_widthTableLength  = sizeof ${FNTNAME}_width_table;
EOF
