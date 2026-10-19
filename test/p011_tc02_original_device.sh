aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt 0
sleep 3
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt 0
start=$(date +%s%3N)
while :; do
  now=$(date +%s%3N)
  elapsed=$((now-start))
  if [ $elapsed -ge 2000 ]; then break; fi
  target=$((elapsed*20/2000))
  aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $target
done
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt 20
sleep 0.8
start=$(date +%s%3N)
while :; do
  now=$(date +%s%3N)
  elapsed=$((now-start))
  if [ $elapsed -ge 4000 ]; then break; fi
  target=$((20-elapsed*40/4000))
  aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $target
done
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt -20
sleep 0.8
start=$(date +%s%3N)
while :; do
  now=$(date +%s%3N)
  elapsed=$((now-start))
  if [ $elapsed -ge 2000 ]; then break; fi
  target=$((-20+elapsed*20/2000))
  aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $target
done
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt 0
start=$(date +%s%3N)
while :; do
  now=$(date +%s%3N)
  elapsed=$((now-start))
  if [ $elapsed -ge 2000 ]; then break; fi
  target=$((elapsed*20/2000))
  aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $target
done
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt 20
sleep 0.8
start=$(date +%s%3N)
while :; do
  now=$(date +%s%3N)
  elapsed=$((now-start))
  if [ $elapsed -ge 4000 ]; then break; fi
  target=$((20-elapsed*40/4000))
  aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $target
done
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt -20
sleep 0.8
start=$(date +%s%3N)
while :; do
  now=$(date +%s%3N)
  elapsed=$((now-start))
  if [ $elapsed -ge 2000 ]; then break; fi
  target=$((-20+elapsed*20/2000))
  aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $target
done
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt 0
start=$(date +%s%3N)
while :; do
  now=$(date +%s%3N)
  elapsed=$((now-start))
  if [ $elapsed -ge 2000 ]; then break; fi
  target=$((elapsed*20/2000))
  aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $target
done
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt 20
sleep 0.8
start=$(date +%s%3N)
while :; do
  now=$(date +%s%3N)
  elapsed=$((now-start))
  if [ $elapsed -ge 4000 ]; then break; fi
  target=$((20-elapsed*40/4000))
  aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $target
done
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt -20
sleep 0.8
start=$(date +%s%3N)
while :; do
  now=$(date +%s%3N)
  elapsed=$((now-start))
  if [ $elapsed -ge 2000 ]; then break; fi
  target=$((-20+elapsed*20/2000))
  aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $target
done
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt 0
start=$(date +%s%3N)
while :; do
  now=$(date +%s%3N)
  elapsed=$((now-start))
  if [ $elapsed -ge 2000 ]; then break; fi
  target=$((elapsed*20/2000))
  aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $target
done
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt 20
sleep 0.8
start=$(date +%s%3N)
while :; do
  now=$(date +%s%3N)
  elapsed=$((now-start))
  if [ $elapsed -ge 4000 ]; then break; fi
  target=$((20-elapsed*40/4000))
  aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $target
done
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt -20
sleep 0.8
start=$(date +%s%3N)
while :; do
  now=$(date +%s%3N)
  elapsed=$((now-start))
  if [ $elapsed -ge 2000 ]; then break; fi
  target=$((-20+elapsed*20/2000))
  aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $target
done
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt 0
start=$(date +%s%3N)
while :; do
  now=$(date +%s%3N)
  elapsed=$((now-start))
  if [ $elapsed -ge 2000 ]; then break; fi
  target=$((elapsed*20/2000))
  aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $target
done
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt 20
sleep 0.8
start=$(date +%s%3N)
while :; do
  now=$(date +%s%3N)
  elapsed=$((now-start))
  if [ $elapsed -ge 4000 ]; then break; fi
  target=$((20-elapsed*40/4000))
  aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $target
done
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt -20
sleep 0.8
start=$(date +%s%3N)
while :; do
  now=$(date +%s%3N)
  elapsed=$((now-start))
  if [ $elapsed -ge 2000 ]; then break; fi
  target=$((-20+elapsed*20/2000))
  aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $target
done
aa start -b com.example.helloworld -a EntryAbility --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt 0
sleep 2
