set -e

LINUX_BUILD="build-linux"
WIN_BUILD="build-mingw"

echo "==> Building for NNIST Linux..."
cmake -S . -B $LINUX_BUILD -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build $LINUX_BUILD --config Release
./build-linux/nnist/tests/nnist_tests

echo "==> Building for NNIST Windows..."
cmake -S . -B $WIN_BUILD -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw64.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build $WIN_BUILD
wine build-mingw/nnist/tests/nnist_tests.exe | cat

echo "==> Checking formatting..."
scripts/format_check.sh

echo "==> Running static analysis (cert)..."
scripts/lint_cert.sh build

# cp build-mingw/OpenEFT/OpenEFT.exe bin/OpenEFT.exe
# wine bin/OpenEFT.exe