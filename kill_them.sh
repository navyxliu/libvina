for p in `ps aux | grep saxpy | awk -F ' ' '{print $2}' `
do
  kill -9 $p
done

