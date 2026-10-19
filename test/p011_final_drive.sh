aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt 0
sleep 3
for angle in 88 0 -88 0 88 0 -88 0 88 0 -88 0; do
  aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $angle
  sleep 0.7
done
sleep 2
