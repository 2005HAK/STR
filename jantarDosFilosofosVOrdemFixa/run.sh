echo "Compilando código..."
rm -rf build/*
mkdir -p build
cd build
cmake ..
make -j$(nproc)
./run

# Para rodar direto no terminal
# g++ -pthread src/problemaDosFilosofos.cpp -o run