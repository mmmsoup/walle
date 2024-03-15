#!/bin/sh

outfile=$1
infiles=${@:2}

echo -e "#ifndef SHADERS_H\n#define SHADERS_H\n" > $outfile

for f in $infiles; do
	name=$(echo $f | sed "s/.*\/\(\w*\)\..*/default_\1_shader_source/")
	xxd -i -n $name $f | sed "s/unsigned \(\w*\)/static const \1/" >> $outfile
done

echo "#endif" >> $outfile
