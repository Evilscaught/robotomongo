# Pre-requisites
Make sure you've accepted x-code's terms & conditions and have the latest update.
```bash
xcode-select --install
```

Compiling will require the `cmake`, `openssl@3` commands, which you can install via `Homebrew` To install:
```bash
brew install cmake qt@5 mongo-cxx-driver
```
And the following libraries:
```bash
brew install qt@5 mongo-cxx-driver
```

# Compiling for Apple Silicon

1. Download and install the Shell source

From the main directory of your repository run:
```bash
git clone --depth 1 --branch roboshell-v4.2 https://github.com/paralect/robomongo-shell.git build/deps/robomongo-shell-v4.2
```
This will clone specifically the `build/deps/robomongo-shell-v4.2/...` of the [robomongo-shell](https://github.com/Studio3T/robomongo-shell) repo.

2. Once you've done that, run the command:
```bash
bin/patch-robomongo-shell-v42-macos-arm64 build/deps/robomongo-shell-v4.2
```

3. Set-up a Python virtual environment
```bash
python3 -m venv build/deps/venv # VE so system Python is not affected
```
No bash output shown. This will create a folder a `venv` folder in `build/deps`.

4. Install compilation requirements
```bash
build/deps/venv/bin/pip install -r build/deps/robomongo-shell-v4.2/etc/pip/compile-requirements.txt
```

5. Compile
```bash
cd build/deps/robomongo-shell-v4.2
../venv/bin/python buildscripts/scons.py mongo MONGO_VERSION=4.2.0 --release -j8 \
  --link-model=object --disable-warnings-as-errors --cxx-std=17 \
  CCFLAGS='-mmacosx-version-min=11.0 -D_LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION -Wno-enum-constexpr-conversion' \
  LINKFLAGS='-mmacosx-version-min=11.0'
cd -
```
This will take a few minutes to compile, depending on the computer. (Four-ish minutes on the M1 Pro). Once complete you will see something along the lines of
```
23 warnings generated.
Generating placeholder library build/opt/mongo/db/index/libindex_descriptor.a
Skipping ranlib for build/opt/mongo/db/index/libindex_descriptor.a
26 warnings generated.
Generating placeholder library build/opt/mongo/db/libservice_context.a
Skipping ranlib for build/opt/mongo/db/libservice_context.a
Linking build/opt/mongo/mongo
Install file: "build/opt/mongo/mongo" as "mongo"
scons: done building targets.
```

6. Verify the compilation succeeded, run
```bash
file build/deps/robomongo-shell-v4.2/mongo
```
Which should yield the following output:
```output
build/deps/robomongo-shell-v4.2/mongo: Mach-O 64-bit executable arm64
```
# Building for Apple Silicon


```bash
cmake -S . -B build/main-arm-v42 -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@5;$PWD/build/deps/robomongo-shell-v4.2;/opt/homebrew/opt/openssl@3" \
  -DCMAKE_INSTALL_PREFIX="$PWD/build/main-arm-v42/install"
```
Output:
```output
-- Generating done (2.0s)
-- Build files have been written to: /Users/<USERNAME>/Documents/robotomongo/build/main-arm-v42
```

Build the application
```bash
cmake --build build/main-arm-v42 --target robomongo -j8
```
Once complete you will see `[100%] Built target robomongo`

Package the application
```bash
cmake --install build/main-arm-v42
```

Now you can open the application by running the command
```
open build/main-arm-v42/install/dino-robomongo.app
```
Or by navigating to `/Users/<YOUR_USERNAME>/Documents/robotomongo/build/main-arm-v42/install/` in the Finder and double clicking on the generated `app` file.
