#########################################
###   Unit Testing with Google Test   ###
#########################################

### --- Important Note
if (SYSTEM_LINUX)        
    message("\n *Note: Currently unit testing is disabled for Linux " 
                "due to MongoDB linking problems")
    return()
endif()

### --- Enable and setup gtest 
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

### --- Setup test source files
# New test & source file pairs MUST have the following format: 
#  Test file:   /path/Foo_test.cpp
#  Source file: /path/Foo.cpp or /path/Foo.h
set(ROBO_SRC_DIR ${CMAKE_HOME_DIRECTORY}/src/robomongo)
set(SOURCES_TEST
    ${ROBO_SRC_DIR}/utils/RoboCrypt_test.cpp
    ${ROBO_SRC_DIR}/utils/StringOperations_test.cpp
    ${ROBO_SRC_DIR}/core/HexUtils_test.cpp
    ${ROBO_SRC_DIR}/core/domain/QueryResultFormatter_test.cpp
    ${ROBO_SRC_DIR}/core/domain/QueryResultExportPlan_test.cpp
    ${ROBO_SRC_DIR}/core/domain/ResultsStaleness_test.cpp
    ${ROBO_SRC_DIR}/core/domain/TabCollection_test.cpp
    ${ROBO_SRC_DIR}/core/domain/CollectionOrder_test.cpp
    ${ROBO_SRC_DIR}/core/domain/ResultPresentation_test.cpp
    ${ROBO_SRC_DIR}/gui/widgets/explorer/ExplorerTreeReveal_test.cpp
    ${ROBO_SRC_DIR}/core/settings/ResultsRefreshSettings_test.cpp
    ${ROBO_SRC_DIR}/core/settings/ExplorerSettings_test.cpp
    ${ROBO_SRC_DIR}/gui/widgets/workarea/RefreshSplitButton_test.cpp
    ${ROBO_SRC_DIR}/gui/widgets/workarea/EmptyResultsView_test.cpp
    ${ROBO_SRC_DIR}/core/domain/MongoNamespace_test.cpp
    ${ROBO_SRC_DIR}/core/domain/MongoQueryInfo_test.cpp
    ${ROBO_SRC_DIR}/core/domain/MongoDomainTypes_test.cpp
    ${ROBO_SRC_DIR}/core/domain/MongoUtils_test.cpp
    ${ROBO_SRC_DIR}/utils/SimpleCrypt_test.cpp
    ${ROBO_SRC_DIR}/utils/RoboCryptKey_test.cpp
    ${ROBO_SRC_DIR}/core/settings/CredentialSettings_test.cpp
    ${ROBO_SRC_DIR}/core/settings/SshSslSettings_test.cpp
    ${ROBO_SRC_DIR}/core/settings/ReplicaSetSettings_test.cpp
    ${ROBO_SRC_DIR}/core/settings/ConnectionSettings_test.cpp
    ${ROBO_SRC_DIR}/core/mongodb/modern/ModernConnectionUri_test.cpp
    ${ROBO_SRC_DIR}/core/mongodb/modern/ModernBsonConversion_test.cpp
    ${ROBO_SRC_DIR}/shell/bson/Json_test.cpp
    ${ROBO_SRC_DIR}/shell/db/PtimeUtil_test.cpp
    ${ROBO_SRC_DIR}/core/utils/BsonUtils_test.cpp
    ${ROBO_SRC_DIR}/core/HexUtilsUuid_test.cpp
    ${ROBO_SRC_DIR}/utils/Utilities_test.cpp
    ${ROBO_SRC_DIR}/core/events/MongoEventsInfo_test.cpp
    ${ROBO_SRC_DIR}/core/domain/ScriptInfo_test.cpp
    ${ROBO_SRC_DIR}/gui/widgets/workarea/BsonTreeItem_test.cpp
    ${ROBO_SRC_DIR}/gui/widgets/workarea/BsonTableModel_test.cpp
    ${ROBO_SRC_DIR}/core/domain/NotifierSelection_test.cpp
    ${ROBO_SRC_DIR}/core/EventBus_test.cpp
    ${ROBO_SRC_DIR}/core/KeyboardManager_test.cpp
)

### --- Setup robo_unit_tests exec. & link ROBO_OBJ_FILES
add_executable(robo_unit_tests ${SOURCES_TEST})
add_dependencies(robo_unit_tests robomongo)
target_include_directories(robo_unit_tests PRIVATE ${CMAKE_HOME_DIRECTORY}/src)

# Same compatibility settings the app is built with (the MongoDB 4.2 headers use
# std::unary_function, removed in C++17)
target_compile_definitions(robo_unit_tests PRIVATE _LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION)
# Tests may declare QObject classes with signals/slots (Q_OBJECT)
set_target_properties(robo_unit_tests PROPERTIES AUTOMOC ON)
target_compile_options(robo_unit_tests PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wno-enum-constexpr-conversion>)

get_target_property(ROBO_SOURCES robomongo SOURCES)
list(FILTER ROBO_SOURCES INCLUDE REGEX "cpp")
list(FILTER ROBO_SOURCES EXCLUDE REGEX "main.cpp")

if(SYSTEM_WINDOWS)
    set(OBJ_DIR ${CMAKE_BINARY_DIR}/src/robomongo/robomongo.dir/${CMAKE_BUILD_TYPE}/)
    set(ROBO_OBJ_FILES "${ROBO_OBJ_FILES}${OBJ_DIR}mocs_compilation.obj;")
    set(ROBO_OBJ_FILES "${ROBO_OBJ_FILES}${OBJ_DIR}qrc_gui.obj;")
    set(ROBO_OBJ_FILES "${ROBO_OBJ_FILES}${OBJ_DIR}qrc_robo.obj;")
    foreach(SRC_FILE ${ROBO_SOURCES})
        get_filename_component(FILENAME ${SRC_FILE} NAME)
        string(REPLACE ".cpp" ".obj" FILENAME ${FILENAME})
        set(ROBO_OBJ_FILES "${ROBO_OBJ_FILES}${OBJ_DIR}${FILENAME};")        
    endforeach()
elseif(SYSTEM_MACOSX) 
    set(OBJ_DIR ${CMAKE_BINARY_DIR}/src/robomongo/CMakeFiles/robomongo.dir/)
    set(ROBO_OBJ_FILES "${ROBO_OBJ_FILES}${OBJ_DIR}/robomongo_autogen/mocs_compilation.cpp.o;")
    set(ROBO_OBJ_FILES "${ROBO_OBJ_FILES}${OBJ_DIR}/robomongo_autogen/YHP5W5E6RA/qrc_gui.cpp.o;")
    set(ROBO_OBJ_FILES "${ROBO_OBJ_FILES}${OBJ_DIR}/robomongo_autogen/3YJK5W5UP7/qrc_robo.cpp.o;")
    foreach(SRC_FILE ${ROBO_SOURCES})
        string(REPLACE ".cpp" ".cpp.o" OBJ_FILE ${SRC_FILE})
        set(ROBO_OBJ_FILES "${ROBO_OBJ_FILES}${OBJ_DIR}${OBJ_FILE};")        
    endforeach()

    find_library(SECURITY NAMES Security)
    find_library(CORE_FOUNDATION NAMES CoreFoundation)
    set(SSL_LIBRARIES ${SECURITY} ${CORE_FOUNDATION})
    target_link_libraries(robo_unit_tests ${SSL_LIBRARIES} -lresolv)
# elseif(SYSTEM_LINUX) 
#     set(OBJ_DIR ${CMAKE_BINARY_DIR}/src/robomongo/CMakeFiles/robomongo.dir/)
#     message("--- OBJ_DIR: " ${OBJ_DIR})
#     set(ROBO_OBJ_FILES "${ROBO_OBJ_FILES}${OBJ_DIR}/robomongo_automoc.cpp.o;")
#     set(ROBO_OBJ_FILES "${ROBO_OBJ_FILES}${OBJ_DIR}/robomongo_automoc.dir/gui/resources/qrc_gui.cpp.o;")
#     set(ROBO_OBJ_FILES "${ROBO_OBJ_FILES}${OBJ_DIR}/robomongo_automoc.dir/resources/qrc_robo.cpp.o;")    
#     foreach(SRC_FILE ${ROBO_SOURCES})
#         string(REPLACE ".cpp" ".cpp.o" OBJ_FILE ${SRC_FILE})
#         set(ROBO_OBJ_FILES "${ROBO_OBJ_FILES}${OBJ_DIR}${OBJ_FILE};")        
#     endforeach()
endif()

target_link_libraries(robo_unit_tests 
    gtest 
    gtest_main
    Qt5::Widgets
    Qt5::Network
    Qt5::Xml
    qjson
    qscintilla
    mongodb
    MongoCxx::Driver
    ssh
    Threads::Threads
    ${ROBO_OBJ_FILES}
)

### --- Isolated tests: code that uses AppRegistry/SettingsManager
# These run in their own executable with HOME pointed at a scratch directory, because the
# config file and key paths are fixed from $HOME at startup. IsolatedMain.cpp refuses to run
# otherwise. Run them with:  cmake --build <build dir> --target run_isolated_tests
set(SOURCES_ISOLATED_TEST
    ${ROBO_SRC_DIR}/core/settings/SettingsManager_isolated_test.cpp
    ${ROBO_SRC_DIR}/gui/widgets/workarea/BsonTreeModel_isolated_test.cpp
    ${ROBO_SRC_DIR}/gui/widgets/workarea/PagingWidget_isolated_test.cpp
    ${ROBO_SRC_DIR}/gui/dialogs/DocumentTextEditor_isolated_test.cpp
)

add_executable(robo_isolated_tests ${CMAKE_CURRENT_SOURCE_DIR}/IsolatedMain.cpp ${SOURCES_ISOLATED_TEST})
add_dependencies(robo_isolated_tests robomongo)
target_include_directories(robo_isolated_tests PRIVATE ${CMAKE_HOME_DIRECTORY}/src)
target_compile_definitions(robo_isolated_tests PRIVATE
    _LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION
    # Same values as the app: the config file path is built from PROJECT_VERSION
    PROJECT_NAME="${PROJECT_NAME}"
    PROJECT_VERSION="${PROJECT_VERSION}")
target_compile_options(robo_isolated_tests PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wno-enum-constexpr-conversion>)
set_target_properties(robo_isolated_tests PROPERTIES AUTOMOC ON)
if(SYSTEM_MACOSX)
    target_link_libraries(robo_isolated_tests ${SSL_LIBRARIES} -lresolv)
endif()
target_link_libraries(robo_isolated_tests
    gtest
    Qt5::Widgets
    Qt5::Network
    Qt5::Xml
    qjson
    qscintilla
    mongodb
    MongoCxx::Driver
    ssh
    Threads::Threads
    ${ROBO_OBJ_FILES}
)

set(ROBO_TEST_HOME ${CMAKE_BINARY_DIR}/robo-test-home)
add_custom_target(run_isolated_tests
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${ROBO_TEST_HOME}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ROBO_TEST_HOME}
    COMMAND ${CMAKE_COMMAND} -E env HOME=${ROBO_TEST_HOME} ROBO_TEST_ISOLATED_HOME=${ROBO_TEST_HOME}
            QT_QPA_PLATFORM=offscreen $<TARGET_FILE:robo_isolated_tests>
    DEPENDS robo_isolated_tests
    USES_TERMINAL)

### --- Install DLLs for Windows
if(CMAKE_BUILD_TYPE STREQUAL "Debug") 
    set(D "d") 
endif()

if(SYSTEM_WINDOWS)
    get_target_property(target_path Qt5::Core LOCATION)
    get_filename_component(qt_bin_dir ${target_path} DIRECTORY)
    set(QT_BIN_FILES "${QT_BIN_FILES}" ${qt_bin_dir}/Qt5Core${D}${CMAKE_SHARED_LIBRARY_SUFFIX};)
    set(QT_BIN_FILES "${QT_BIN_FILES}" ${qt_bin_dir}/Qt5Gui${D}${CMAKE_SHARED_LIBRARY_SUFFIX};)
    set(QT_BIN_FILES "${QT_BIN_FILES}" ${qt_bin_dir}/Qt5Network${D}${CMAKE_SHARED_LIBRARY_SUFFIX};)
    set(QT_BIN_FILES "${QT_BIN_FILES}" ${qt_bin_dir}/Qt5PrintSupport${D}${CMAKE_SHARED_LIBRARY_SUFFIX};)
    set(QT_BIN_FILES "${QT_BIN_FILES}" ${qt_bin_dir}/Qt5Widgets${D}${CMAKE_SHARED_LIBRARY_SUFFIX};)
    set(QT_BIN_FILES "${QT_BIN_FILES}" ${qt_bin_dir}/Qt5Xml${D}${CMAKE_SHARED_LIBRARY_SUFFIX};)

    get_target_property(TESTS_BIN_DIR robo_unit_tests BINARY_DIR)
    file(COPY ${QT_BIN_FILES} DESTINATION ${TESTS_BIN_DIR}/${CMAKE_BUILD_TYPE})
    file(GLOB OPENSSL_RUNTIME_FILES
        "${OpenSSL_DIR}/libssl*.dll"
        "${OpenSSL_DIR}/libcrypto*.dll"
        "${OpenSSL_DIR}/bin/libssl*.dll"
        "${OpenSSL_DIR}/bin/libcrypto*.dll")
    file(COPY ${OPENSSL_RUNTIME_FILES} DESTINATION ${TESTS_BIN_DIR}/${CMAKE_BUILD_TYPE})
endif()
