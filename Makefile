all:
	gcc -g -o crab *.c
test: all
	./crab words.txt "热爱中国共产党, 响应中央号召"
clean:
	rm crab
