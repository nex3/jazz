#!/bin/bash

failures=(Success)
index=0

errors=(None)
eindex=0
rm -f test.errors

for file in $( find test/ -name '*.js' )
do
    errorf=`mktemp`
    res=`cat "$file" | ./jazz 2> $errorf`
    error=`cat $errorf`
    rm $errorf

    [ -z "$res" ] && res="-1"

    if [ "$error" != "" ]
    then
        echo -n 'E'
        failures[$index]="Error: $file (details in test.errors)"
        errors[$eindex]=`echo -e "Errors for $file:\n$error"`
        let "index += 1"
        let "eindex += 1"
    elif [ "$res" = true ]
    then
        echo -n '.'
    else
        echo -n 'F'
        failures[$index]="Failure: $file"
        let "index += 1"
    fi
done

echo -e "\n"

if [ "${failures[0]}" = Success ]
then echo "All tests passed!"
else
    for ((i = 0; i < $index; i++))
    do
        echo "${failures[$i]}"
    done
fi

if [ "${errors[0]}" != None ]
then
    for ((i = 0; i < $eindex; i++))
    do
        echo "${errors[$i]}" >> test.errors
        echo >> test.errors
    done
fi
