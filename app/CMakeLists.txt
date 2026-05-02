# 1. 指定最低 CMake 版本需求
cmake_minimum_required(VERSION 3.15)

# 2. 定義專案名稱與使用的語言 (CXX 代表 C++)
project(AmadeusSNN LANGUAGES CXX)

# 放在 project() 指令之後
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 3. 設定 C++ 標準 (建議至少使用 C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # 禁用編譯器特定擴充，保持跨平台純潔性

# 4. 收集所有的原始碼檔案 (.cpp)
# 這裡使用 file(GLOB ...) 自動抓取 src 資料夾下所有的 cpp 檔
# (如果檔案非常多且經常增刪，未來也可以改為手動逐一列出檔案名稱)
file(GLOB SOURCES "src/*.cpp")

# 5. 建立執行檔目標
add_executable(Amadeus ${SOURCES})

# 6. 告訴編譯器去哪裡找標頭檔 (.h)
# 這樣在 cpp 檔裡就可以直接寫 #include "CorticalColumn.h"
target_include_directories(Amadeus PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ==============================================================================
# 7. ★ 極限效能優化區 (Amadeus 專屬) ★
# 因為我們大量依賴 Popcount 與位元運算，必須開啟最高優化與硬體指令集
# ==============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # 針對 GCC 或 Clang (Linux / Mac / MinGW)
    target_compile_options(Amadeus PRIVATE
        -O3             # 開啟最高等級的最佳化
        -march=native   # 允許編譯器使用你「當下這台電腦 CPU」所有支援的硬體加速指令 (如 AVX2, 硬體 Popcount)
        -Wall -Wextra   # 開啟常見的警告，幫助除錯
    )
elseif(MSVC)
    # 針對微軟 Visual Studio (Windows)
    target_compile_options(Amadeus PRIVATE
        /O2             # 最高速度最佳化
        /arch:AVX2      # 啟用進階向量擴充指令集
        /W4             # 警告等級 4
    )
endif()
