
SIXENSE_LIB_DIR = ../lib/sixense/lib/linux_x64/release/

hydrajoy64: hydrajoy.c
	gcc -L"${SIXENSE_LIB_DIR}" hydrajoy.c -o hydrajoy64 -lsixense_x64

clean:
	rm hydrajoy