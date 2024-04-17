echo "#pragma once"
echo "namespace scs {"
echo "enum {"

cat lfi_contracts/libc/syscall.S | sed "s/syscall \([a-z_]*\)[ ]*\([0-9][0-9]*\)/\U\1 = \2,/" | grep -o "^[a-zA-Z_]* = [0-9]*,$"

echo "};"
echo "} /* namespace scs */"
