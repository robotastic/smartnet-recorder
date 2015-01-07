#!/bin/bash
echo "Encoding: $1"
filename="$1"
basename="${filename%.*}"
#mp3encoded="$basename.mp3"
mp4encoded="$basename.m4a"
json="$basename.json"
#eval "lame --preset voice" ${filename}
eval "ffmpeg -i $filename  -c:a libfdk_aac -vbr 3 $mp4encoded > /dev/null 2>&1"
#eval "avconv -i $filename -c:a alac $mp4encoded >> eval_avconv.log 2>&1"

#echo "Upload: $encoded"
eval "scp $json $mp4encoded developer@boston.scannergrabber.com:~/Desktop/smartnet-player/smartnet-upload "

