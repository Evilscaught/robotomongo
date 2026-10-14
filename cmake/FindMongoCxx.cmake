find_package(PkgConfig REQUIRED)

# mongo-cxx-driver 4.x renamed its pkg-config modules to libmongocxx1/libbsoncxx1
pkg_search_module(MONGOCXX REQUIRED IMPORTED_TARGET libmongocxx libmongocxx1)
pkg_search_module(BSONCXX REQUIRED IMPORTED_TARGET libbsoncxx libbsoncxx1)
pkg_check_modules(MONGOC_LEGACY IMPORTED_TARGET libmongoc-1.0)
pkg_check_modules(BSON_LEGACY IMPORTED_TARGET libbson-1.0)

add_library(MongoCxx::Driver INTERFACE IMPORTED)
target_link_libraries(MongoCxx::Driver
    INTERFACE
        PkgConfig::MONGOCXX
        PkgConfig::BSONCXX)

set(MongoCxx_FOUND TRUE)
