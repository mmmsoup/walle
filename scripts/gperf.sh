#!/bin/sh

tmp_dir="/tmp/gperf-$(uuidgen)"
mkdir -p "$tmp_dir"

gperf_input="$tmp_dir/gperf_input"
gperf_output="$tmp_dir/gperf_output"

# popuplated based on cmdline args
raw_input=""
source_output=""
header_output=""

includes_set=0
strcmp="strcmp"
lookup_function_name=""
hash_function_name=""

args=""

show_help() {
	echo -e "\e[1mgperf.sh\e[0m is a wrapper for gperf.\n"
	echo -e "It should make it easier to use gperf with C by creating both a source file and a header file for the generated code, so there should (hopefully!) be no problems with multiple definition errors."
	echo -e "It should be almost completely backwards-compatible, with the notable exception of not supporting outputting the generated code to stdout (as it generates two files instead of one).\n"
	echo -e "Usage: gperf.sh [OPTIONS] [INPUT_FILE]\n"
	echo -e "As well as the regular gperf arguments, the following are also supported:"
	echo -e "\t\e[1m--output-source=PATH\e[0m\t\t\tpath for outputted c source file"
	echo -e "\t\e[1m--output-header=PATH\e[0m\t\t\tpath for outputted header file"
	echo -e "\t\e[1m--include-path=[PATH | \"PATH\" | <PATH>]\e[0m\tthe string used to include the header file (needed as it is included in the generated source file). Defaults to the base name of the output header file (with double quotes)"
	echo -e "\t\e[1m--strncmp-no-null\e[0m\t\t\tuse strncmp for string comparisons and don't check whether the last character is a null byte (useful if strings are not null-terminated)"
	echo -e "\t\e[1m--sh-help\e[0m\t\t\t\tonly show added help for gperf.sh"
	echo -e ""
	echo -e "The behaviour of the following argument(s) has also been modified:"
	echo -e "\t\e[1m--output-file=PATH\e[0m\t\t\tpreferable to use \e[1m--output-source\e[0m and \e[1m--output-header\e[0m instead; this is included for backwards-compatibility. If the argument has a '.c' file extension, this is preserved for the source file, otherwise it is added. The generated header file will have the same name, differing only in that its extension will be '.h' instead of '.c'."
}

short_opts="e:tL:K:F:H:N:Z:7lcCEIGPQ:W:S:Tk:Dm:i:j:nrs:hvd"
long_opts="output-file:,delimiters:,struct-type,ignore-case,language:,slot-name:,initializer-suffix:,hash-function-name:,lookup-function-name:,class-name:,seven-bit,compare-lengths,compare-strncmp,readonly-tables,enum,includes,global-table,pic,string-pool-name:,null-strings,constants-prefix:,word-array-name:,length-table-name:,switch:,omit-struct-type,key-positions:,duplicates,multiple-iterations:,initial-asso:,jump:,no-strlen,random,size-multiple:,help,version,debug"

# additional non-gperf options
long_opts="$long_opts,output-source:,output-header:,include-path:,strncmp-no-null,sh-help"

parsed_opts=$(getopt --options $short_opts --longoptions $long_opts -- "$@")
[[ $? -ne 0 ]] && (rm -r "$tmp_dir"; exit 1)

eval set -- "$parsed_opts"

while :; do
	case "$1" in
		"--output-file") # for compatibility
			source_output="$(echo $2 | sed 's/\.c$//').c"
			header_output="$(echo $source_output | sed 's/c$/h/')"
			shift 2
			;;
		"--output-source") # additional non-gperf option
			source_output="$2"
			shift 2
			;;
		"--output-header") # additional non-gperf option
			header_output="$2"
			shift 2
			;;
		"--include-path") # additional non-gperf option
			firstlast=$(echo "$2" | sed "s/^\(.\).*\(.\)$/\1\2/")
			[[ "$firstlast" == "<>" || "$firstlast" == '""' ]] && header_include_string="$2" || header_include_string='"'$2'"'
			shift 2
			;;
		"--compare-strncmp" | "-c")
			if [[ "$strcmp" == "strcmp" ]]; then
				strcmp="strncmp"
				args="$args --compare-strncmp"
			fi
			shift
			;;
		"--strncmp-no-null") # additional non-gperf option
			if [[ "$strcmp" == "strcmp" ]]; then
				args="$args --compare-strncmp"
			fi
			strcmp="no_null"
			shift
			;;
		"--lookup-function-name" | "-N")
			lookup_function_name="$2"
			args="$args --lookup-function-name '$2'"
			shift 2
			;;
		"--hash-function-name" | "-H")
			hash_function_name="$2"
			args="$args --hash-function-name '$2'"
			shift 2
			;;
		"--includes" | "-I") # abusing '#include <string.h>' and substituting it for '#include "[NAME].h"
			includes_set=1
			shift
			;;
		"--sh-help")
			show_help
			rm -r "$tmp_dir"
			exit 0
			;;
		"--help" | "-h")
			show_help
			echo -e "\nThe help for gperf is printed below:\n"
			gperf --help
			rm -r "$tmp_dir"
			exit 0
			;;
		"--")
			shift
			break
			;;
		*)
			args="$args '$1'"
			shift
			;;
	esac
done

mkdir -p "$(dirname $source_output)"
mkdir -p "$(dirname $header_output)"

[[ -z "$source_output" ]] && echo "output file not specified (use '--output-file')" >&2 && exit 1

raw_input="$1"
[[ -z "$raw_input" ]] && echo "input file not specified" >&2 && exit 1

[[ -z "$header_include_string" ]] && header_include_string='"%s"'
header_include_string=$(printf "$header_include_string" "$(basename $header_output)")

# handle '%define ...'s in input file (if equivalent cmdline options are also passed, cmdline takes precedence)
input_file_defines=$(cat "$raw_input" | grep -Po "(?<=^%define ).*$")
[[ -z "$lookup_function_name" ]] && lookup_function_name=$(echo "$input_file_defines" | grep -Po "(?<=lookup-function-name ).*$" || echo "in_word_set")
[[ -z "$hash_function_name" ]] && hash_function_name=$(echo "$input_file_defines" | grep -Po "(?<=hash-function-name ).*$" || echo "hash")

echo -n "" > "$gperf_input"
echo -n "" > "$gperf_output"
echo -n "" > "$source_output"
echo -e "/* file generated by gperf.sh (in turn using gperf) */\n" > "$header_output"

header_define="GPERF_$(basename "$header_output" | tr "[:lower:]" "[:upper:]" | sed "s/[ \.\-]/_/g")"
echo -e "#ifndef $header_define" >> "$header_output"
echo -e "#define $header_define\n" >> "$header_output"
[[ "$includes_set" -eq 1 ]] && echo -e "#include <string.h>\n" >> "$header_output"

# put any text in optional '%{ ... %}' section in the header file
if csplit --prefix="$tmp_dir/csplit" "$raw_input" "/^%{/+1" "/^%}/" &> /dev/null; then
	cat "$tmp_dir/csplit00" >> "$gperf_input"
	cat "$tmp_dir/csplit02" >> "$gperf_input"

	echo -e "/* included code from gperf input file declaration section */" >> "$header_output"
	cat "$tmp_dir/csplit01" >> "$header_output"
	echo -e "/* end of included code */\n" >> "$header_output"
else
	cp "$raw_input" "$gperf_input"
fi

# add function declarations to the header file
#echo "/* gperf hash function */" >> "$header_output"
#echo -e "#ifdef __GNUC__\n__inline\n#else\n#ifdef __cplusplus\ninline\n#endif\n#endif\nstatic unsigned int" >> "$header_output"
#echo -e "$hash_function_name(register const char *, register size_t);\n" >> "$header_output"

eval gperf --includes --output-file="$gperf_output" $args "$gperf_input"
[[ $? -ne 0 ]] && (rm -r "$tmp_dir"; exit 1)

cat "$gperf_output" | sed "s|#include <string.h>|#include $header_include_string|" | sed 's|^#line \([0-9]*\) "'$gperf_input'"$|#line \1 "'$raw_input'"|' > "$source_output"

if [[ "$strcmp" == "no_null" ]]; then
	sed -i "s/ && s\\[len\\] == '\\\\0'//" "$source_output"
fi

echo "/* gperf lookup function */" >> "$header_output"
cat "$source_output" | grep -B1 "^$lookup_function_name (str, len)$" | sed "s/ (str, len)/(register const char *, register size_t);\n/" >> "$header_output"

echo -e "#endif" >> "$header_output"

rm -r "$tmp_dir"
