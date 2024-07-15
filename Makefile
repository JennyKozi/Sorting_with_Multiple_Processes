all: mysort selectionsort bubblesort

mysort: mysort.o merge.o
	gcc -o mysort mysort.o merge.o

mysort.o: mysort.c
	gcc -g3 -c -o mysort.o mysort.c

merge.o: merge.c
	gcc -g3 -c -o merge.o merge.c

selectionsort: selectionsort.o
	gcc -o selectionsort selectionsort.o

selectionsort.o: selectionsort.c
	gcc -g3 -c -o selectionsort.o selectionsort.c

bubblesort: bubblesort.o
	gcc -o bubblesort bubblesort.o

bubblesort.o: bubblesort.c
	gcc -g3 -c -o bubblesort.o bubblesort.c

############################################################################

clean: mysort
	rm -rf mysort mysort.o merge.o selectionsort selectionsort.o bubblesort bubblesort.o

run: mysort
	./mysort -i data/voters500.bin -k 7 -e1 bubblesort -e2 selectionsort

time: mysort
	time ./mysort -i data/voters100000.bin -k 20 -e1 bubblesort -e2 selectionsort

valgrind: mysort
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --show-reachable=yes ./mysort -i data/voters50.bin -k 4 -e1 bubblesort -e2 selectionsort