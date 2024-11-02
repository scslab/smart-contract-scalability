export LFICC = x86_64-lfi-linux-musl-gcc

CONTRACTS = hello.lfi \
			test_calldata.lfi \
			test_invoke.lfi \
			test_log.lfi \
			test_redirect_call.lfi \
			deployer.lfi \
			erc20.lfi \
			payment.lfi \
			test_gas_limit.lfi

INCLUDE = -Ilibc \
		  -Ilib/build/install/include \
		  -Ied25519/src

LIBS = -Llibc \
	   -Llibc/build/install/lib \
	   -Led25519/src

LINKLIBS = -led25519 \
		   -lsyscalls \
		   -lc \
		   -lsyscalls \
		   -lgcc

export CFLAGS = -Os -nostdlib -static -fno-pic
LDFLAGS = -Wl,--gc-sections -Tld.ld -z noexecstack

all: $(CONTRACTS)

ed25519:
	$(MAKE) -C ed25519/src

libc:
	$(MAKE) -C libc

%.lfi: %.o ed25519 libc ld.ld
	$(LFICC) $< -o $@ $(LIBS) $(LINKLIBS) $(CFLAGS) $(LDFLAGS)

%.o: %.c
	$(LFICC) $< -o $@ -c $(INCLUDE) $(CFLAGS)

clean:
	rm -f *.o *.lfi
	$(MAKE) -C libc clean
	$(MAKE) -C ed25519/src clean

.PHONY: all ed25519 libc clean
