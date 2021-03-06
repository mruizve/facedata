#!/usr/bin/env bash

# validate input command line arguments
if [ "$#" -ne 1 ]; then
    echo "usage: $0 <path to the dataset>"
    exit 1
fi

export RED="\033[1;31m"
export YLL="\033[1;33m"
export WHT="\033[0;37m"
export DATASET="${1%/}"

# validate dataset info
source ${0%/*}/evaluation_check.sh

# get number of images
export NIMG=$(cat $LABELS |wc -l)

# get number of labels
export NLBL=$(cat $LABELS |cut -d\  -f2|sort|uniq|wc -l)

# generate a record counting the number of images for each label
cat "$LABELS" |cut -d\  -f2|sort -n|uniq -c|awk '{printf "%d %d\n",$2,$1}' > "$COUNT"

# get the maximum and minimum number of images for label
HMIN=$(cat $COUNT|awk '{printf "%03d\n",$2}'|sort|uniq|head -n1)
HMAX=$(cat $COUNT|awk '{printf "%03d\n",$2}'|sort|uniq|tail -n1)
export HMIN=${HMIN##*0}
export HMAX=${HMAX##*0}

# retrieve values and generate histogram plot
source ${0%/*}/evaluation_histogram.sh

# show stats
echo "[+] dataset statistics";
printf " |-number of images: % 6d\n" "$NIMG";
printf " |-number of labels: % 6d\n" "$NLBL";
echo " |-histogram of labels with respect the number of images:"
# histogram indexes (bins)
printf " | images";
for i in ${!H[@]}; do
	j=$(expr "$i" "+" "1");
	printf "% 5d" "$j";
done
# histogram values
printf "\n | labels";
for i in ${H[@]}; do
	printf "% 5d" "$i";
done
echo;

# get threshold (desired number of images for each label)
REPLY="0"
while [ -z "${REPLY##*[!0-9]*}" ] || [ "$REPLY" -lt "$HMIN" ] || [ "$REPLY" -gt "$HMAX" ]; do
	printf " |< desired image threshold? ";
	read;
done;

# compute thresholded statistics
n="$REPLY"
nIMG=0;
nLBL=0;
for i in ${!H[@]}; do
	j=$(expr "$i" "+" "1");
	if [ "$j" -eq "$n" ]; then
		l=$(expr "${H[$i]}" '*' "$j");
		nIMG=$(expr "$nIMG" "+" "$l");
		nLBL=$(expr "${H[$i]}" "+" "$nLBL");
	fi;
done
# compute labels and images percents
pIMG=$(awk '{printf "%.2f%%\n", 100*$1/$2}' <<< "$nIMG $NIMG");
pLBL=$(awk '{printf "%.2f%%\n", 100*$1/$2}' <<< "$nLBL $NLBL");

# show thresholded statistics
echo " '-number of labels with $n images: $nLBL ($pLBL of all labels) with $nIMG images ($pIMG of all images).";
echo;

# generate output data (file paths)
export OUTPUT_LABELS=$(printf $OUTPUT_LABELS $n);
export OUTPUT_IMAGES=$(printf $OUTPUT_IMAGES $n);
export OUTPUT_ASSOCIATION=$(printf $OUTPUT_ASSOCIATION $n);

# generate output hierarchy
source ${0%/*}/evaluation_data.sh

# delete extra data
rm "$COUNT" "$OUTPUT_LABELS" "$OUTPUT_IMAGES"

exit 0
