cmake_minimum_required(VERSION 3.16)

project(IstowUpdater VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Core Sql QuickControls2)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appIstowUpdater
    main.cpp
    src/entity/Ship.h
    src/repository/repository.h
    src/repository/repository.cpp
    src/repository/localrepository.h
    src/repository/localrepository.cpp
    src/repository/backuprepository.h
    src/repository/backuprepository.cpp
    src/repository/installerrepository.h
    src/repository/installerrepository.cpp
    src/models/ShipModel.h
    src/models/ShipModel.cpp
    src/models/IstowImporter.h
    src/models/IstowImporter.cpp
    src/models/BackupSessionModel.h
    src/models/BackupSessionModel.cpp
    src/models/InstallerModel.h
    src/models/InstallerModel.cpp
    src/models/PatchDbModel.h
    src/models/PatchDbModel.cpp
)

qt_add_qml_module(appIstowUpdater
    URI IstowUpdater
    QML_FILES
        src/pages/AppButton.qml
        src/pages/MainPage.qml
        src/pages/BrowseDotIstow.qml
        src/pages/RollbackAssetsPage.qml
        src/pages/UpdateApplicationPage.qml
        src/pages/InstallerFormPopup.qml
        src/pages/RollbackApplicationPage.qml
        src/pages/PatchDbPage.qml
        src/pages/PatchDbTableView.qml
        src/pages/PatchDbTableAccordion.qml
    RESOURCES
        qtquickcontrols2.conf
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appIstowUpdater PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appIstowUpdater
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appIstowUpdater PRIVATE
    src/entity
    src/repository
    src/models
)

target_link_libraries(appIstowUpdater
    PRIVATE
        Qt6::Quick
        Qt6::Core
        Qt6::Sql
        Qt6::QuickControls2
)

include(GNUInstallDirs)
install(TARGETS appIstowUpdater
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
