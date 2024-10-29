OBJ = add_scalar.o \
	  fe.o \
	  ge.o \
	  key_exchange.o \
	  keypair.o \
	  sc.o \
	  seed.o \
	  sha512.o \
	  sign.o \
	  verify.o

libed25519.a: $(OBJ)
	ar rcs $@ $^
%.o: %.c
	$(LFICC) -Os -c $< -o $@ -DED25519_NO_SEED

clean:
	rm -f *.o
	rm -f libed25519.a

.PHONY: clean
