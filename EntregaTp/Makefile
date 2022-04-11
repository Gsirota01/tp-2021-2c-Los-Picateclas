all:
	make -C ModuloKernel
	make -C ModuloMemoria
	make -C ModuloSwamp
	make install -C MateLib

clean:
	make clean -C ModuloKernel
	make clean -C ModuloMemoria
	make clean -C ModuloSwamp
	make clean -C MateLib

uninstall:
	make uninstall -C MateLib 