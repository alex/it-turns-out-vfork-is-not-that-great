all: test helper test2

time: test helper test2
	sh -c 'time ./test'
	sh -c 'time ./test2'

test:
	clang -O3 -Wmost -o test test.c

helper:
	clang -O3 -Wmost -o helper helper.c

test2:
	clang -O3 -Wmost -o test2 test2.c

clean:
	rm -f test test2 helper
