

#STRIP=symbols

#%.wasm: %.rs Makefile $(wildcard src/*.rs)
#	rustc --edition 2021 --crate-type cdylib --target wasm32-unknown-unknown -C opt-level=z -C panic=abort -C overflow-checks=no -C strip=$(STRIP)  $<


%.wasm : Makefile $(wildcard $</src/*.rs)
	cargo rustc --target wasm32-unknown-unknown --release --verbose -p $(basename $@) -- -C link-args=-zstack-size=65536 && \
	cp target/wasm32-unknown-unknown/release/$@ built_wasms/$@

clean:
	cargo clean

