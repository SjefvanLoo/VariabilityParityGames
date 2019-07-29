for i in PGtestsuite/modelchecking/selected/*.gmN;do echo $i;./assist_PG.sh "$i" > "$i.csv";done
for i in PGtestsuite/pgsolvergames/random/*.gmN;do echo $i;./assist_PG.sh "$i" > "$i.csv";done

