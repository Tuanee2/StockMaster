cmake_minimum_required(VERSION 3.16)

project(StockMaster VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0004 NEW)

qt_add_executable(appStockMaster
    main.cpp
    src/stockmaster/application/customer_service.cpp
    src/stockmaster/application/order_service.cpp
    src/stockmaster/application/payment_service.cpp
    src/stockmaster/application/product_service.cpp
    src/stockmaster/application/report_service.cpp
    src/stockmaster/infra/db/database_service.cpp
    src/stockmaster/ui/viewmodels/app_view_model.cpp
    src/stockmaster/ui/viewmodels/customers_view_model.cpp
    src/stockmaster/ui/viewmodels/inventory_view_model.cpp
    src/stockmaster/ui/viewmodels/orders_view_model.cpp
    src/stockmaster/ui/viewmodels/payments_view_model.cpp
    src/stockmaster/ui/viewmodels/products_view_model.cpp
    src/stockmaster/ui/viewmodels/reports_view_model.cpp
)

target_include_directories(appStockMaster
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

qt_add_qml_module(appStockMaster
    URI StockMaster
    VERSION 1.0
    QML_FILES
        src/stockmaster/ui/qml/Main.qml
        src/stockmaster/ui/qml/AppShell.qml
        src/stockmaster/ui/qml/components/SectionHeader.qml
        src/stockmaster/ui/qml/components/MetricCard.qml
        src/stockmaster/ui/qml/screens/DashboardScreen.qml
        src/stockmaster/ui/qml/screens/CustomersScreen.qml
        src/stockmaster/ui/qml/screens/ProductsScreen.qml
        src/stockmaster/ui/qml/screens/OrdersScreen.qml
        src/stockmaster/ui/qml/screens/InventoryScreen.qml
        src/stockmaster/ui/qml/screens/PaymentsScreen.qml
        src/stockmaster/ui/qml/screens/ReportsScreen.qml
        src/stockmaster/ui/qml/screens/SettingsScreen.qml
    SOURCES
        src/stockmaster/core/money.h
        src/stockmaster/domain/entities.h
        src/stockmaster/application/customer_service.h
        src/stockmaster/application/order_service.h
        src/stockmaster/application/payment_service.h
        src/stockmaster/application/product_service.h
        src/stockmaster/application/report_service.h
        src/stockmaster/infra/db/database_service.h
        src/stockmaster/ui/viewmodels/app_view_model.h
        src/stockmaster/ui/viewmodels/customers_view_model.h
        src/stockmaster/ui/viewmodels/inventory_view_model.h
        src/stockmaster/ui/viewmodels/orders_view_model.h
        src/stockmaster/ui/viewmodels/payments_view_model.h
        src/stockmaster/ui/viewmodels/products_view_model.h
        src/stockmaster/ui/viewmodels/reports_view_model.h
)

set_target_properties(appStockMaster PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appStockMaster
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appStockMaster
    PRIVATE Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS appStockMaster
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_TESTING)
    add_executable(report_service_smoke
        tests/report_service_smoke.cpp
        src/stockmaster/application/customer_service.cpp
        src/stockmaster/application/order_service.cpp
        src/stockmaster/application/payment_service.cpp
        src/stockmaster/application/product_service.cpp
        src/stockmaster/application/report_service.cpp
        src/stockmaster/infra/db/database_service.cpp
    )

    target_include_directories(report_service_smoke
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(report_service_smoke
        PRIVATE Qt6::Quick
    )

    add_test(NAME report_service_smoke COMMAND report_service_smoke)

    add_executable(dashboard_view_model_smoke
        tests/dashboard_view_model_smoke.cpp
        src/stockmaster/application/customer_service.cpp
        src/stockmaster/application/order_service.cpp
        src/stockmaster/application/payment_service.cpp
        src/stockmaster/application/product_service.cpp
        src/stockmaster/infra/db/database_service.cpp
        src/stockmaster/ui/viewmodels/app_view_model.cpp
    )

    target_include_directories(dashboard_view_model_smoke
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(dashboard_view_model_smoke
        PRIVATE Qt6::Quick
    )

    add_test(NAME dashboard_view_model_smoke COMMAND dashboard_view_model_smoke)

    add_executable(inventory_service_smoke
        tests/inventory_service_smoke.cpp
        src/stockmaster/application/product_service.cpp
    )

    target_include_directories(inventory_service_smoke
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(inventory_service_smoke
        PRIVATE Qt6::Quick
    )

    add_test(NAME inventory_service_smoke COMMAND inventory_service_smoke)

    add_executable(payment_service_smoke
        tests/payment_service_smoke.cpp
        src/stockmaster/application/customer_service.cpp
        src/stockmaster/application/order_service.cpp
        src/stockmaster/application/payment_service.cpp
        src/stockmaster/application/product_service.cpp
        src/stockmaster/infra/db/database_service.cpp
    )

    target_include_directories(payment_service_smoke
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(payment_service_smoke
        PRIVATE Qt6::Quick
    )

    add_test(NAME payment_service_smoke COMMAND payment_service_smoke)
endif()
