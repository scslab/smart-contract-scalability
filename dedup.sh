
IFS= read -r line

strarray=($line)

for i in ${strarray[@]}
do
  echo $i
done