#!/bin/bash
while sleep 1; do
	DATE="$(date)";
	PS="$( ps -p `pidof AgentServer` -o 'pcpu,pmem,rss,vsz')";
	TOP="$( top -p `pidof AgentServer` -n1 -b -H -c )"
	OUTPUT=$DATE"\n\n"$TOP"\n\nps\n"$PS"\n";
	echo -e "$OUTPUT" | tee -a cpu-mem.log;
done;
