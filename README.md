# cryptonight-hash-lib

Copyright (c) 2017, Sumokoin.org

This a python-wrapper lib with cryptonight hashing functions

To complile on Linux:

	cd /path/to/cryptonight-hash-lib
	cmake .
	make
	
The library now has C functions. To compile with C:
	cd /path/to/cryptonight-hash-lib
	cmake . <-DMAKE_CLIB_STATIC=ON/-DMAKE_CLIB=ON>
	make
