#!/bin/bash

failures=(Success)
index=0

errors=(None)
eindex=0
rm -f test.errors

for file in $( find test/*/ -name '*.js' -a ! -name '_*.js' )
do
    rm -f jazz.test
    errorf=`mktemp jazz.test`
    filename=`echo "$file" | sed 's/^test\///'`
    printf "%-30s " "$filename:"
    ./jazz < $file 2> $errorf
    error=`cat $errorf`
    rm $errorf

    if [ "$error" != "" ]
    then
        echo ERROR
        failures[$index]="Error: $file (details in test.errors)"
        errors[$eindex]=`echo -e "Errors for $file:\n$error"`
        let "index += 1"
        let "eindex += 1"
    fi
done

if [ "${errors[0]}" != None ]
then
    for ((i = 0; i < $eindex; i++))
    do
        echo "${errors[$i]}" >> test.errors
        echo >> test.errors
    done
fi
