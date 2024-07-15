# Sorting with Multiple Processes

## Run
Use the Makefile to compile, run and clean using the following commands:

```bash
make 
make run
make clean
```

Other exec commands:

```bash
./mysort -i voters50.bin -k 3 -e1 quicksort -e2 bubblesort
```
```bash
./mysort -e1 bubblesort -e2 quicksort -i voters500.bin -k 4
```
```bash
./mysort -k 5 -e2 bubblesort -i voters5000.bin -e1 quicksort
```
```bash
./mysort -e2 bubblesort -k 6 -i voters50000.bin -e1 quicksort
```