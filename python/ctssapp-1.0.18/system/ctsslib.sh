


##########################################
# FLOAT Usage, use AWK to evaluate Floats.
#
#x=1.2
#y=1.02
#float_test "$x > $y" && echo "$x > $y"
#float_test "$x < $y" && echo "$x < $y"
#z=`float_val "$x + $y"`
#echo "$x + $y = $z"
#z=`float_val "$x - $y"`
#echo "$x - $y = $z"
#
# ---------------------
# Float comparison eg float '1.2 > 1.3'
# Return the value of an operation
float_val() {
     echo | awk 'END { print '"$1"'; }'
}
# ---------------------
# Return status code of a comparison
float_test() {
     echo | awk 'END { exit ( !( '"$1"')); }'
}
##########################################
#

