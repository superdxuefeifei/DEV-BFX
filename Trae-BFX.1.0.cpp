#include <iostream>
#include <string>
#include <stack>
#include <vector>
#include <fstream>
#include <cstring>
#include <map>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cmath>

/*
Brainfuck IDE - 主要功能和全局变量说明

语言相关：
- Language枚举：支持的语言类型（英文、中文、西班牙文、法文、德文、日文、俄文、葡萄牙文）
- translations：存储各语言的翻译文本映射
- currentLanguage：全局语言设置
- initializeTranslations()：初始化所有语言的翻译文本
- initializeLanguageNames()：初始化语言名称
- tr()：获取指定键的翻译文本
- trf()：格式化翻译文本（支持占位符）

颜色相关：
- Color枚举：支持的颜色类型
- colorNames：颜色名称映射
- colorToWinConsole/colorToLinuxConsole：颜色到终端颜色代码的映射
- backgroundColor/codeColor/commentColor：全局颜色设置
- initializeColorNames()：初始化颜色名称
- initializeColorMaps()：初始化颜色映射
- applyColors()：应用代码颜色到终端
- applyCommentColor()：应用注释颜色
- resetColors()：重置终端颜色

设置相关：
- languageSettings()：语言设置菜单
- colorSettings()：颜色设置菜单
- editorSettings()：编辑器设置菜单
- saveSettings()：保存设置到文件
- loadSettings()：从文件加载设置

文件操作：
- DirectoryReader类：用于递归搜索.bf文件
- getExeDir()：获取程序所在目录
- createDirectory()：创建目录
- find_all_bf()：递归查找所有.bf文件
- saveProgram()：保存程序到文件
- loadProgram()：从文件加载程序
- create()：创建新程序
- open()：打开现有程序

编辑器功能：
- createEditor()：创建编辑器界面
- displayProgramWithColors()：带颜色显示程序代码
- input_Bf()：输入Brainfuck程序
- run()：执行Brainfuck程序
- running()：处理运行结果

工具函数：
- clearScreen()：清屏
- pauseScreen()：暂停屏幕显示

常量和结构体：
- MEMORY_SIZE：Brainfuck内存大小（30000字节）
- PROGRAM_SIZE：程序最大长度
- ProgramData结构体：存储原始代码和过滤后的代码

主函数：
- main()：程序入口点，初始化设置和主菜单
*/

// 平台相关的头文件
#ifdef _WIN32
    #include <windows.h>
    #include <tchar.h>
    #include <conio.h>
    #include <commdlg.h>  // 用于Windows文件对话框
#else
    #include <dirent.h>
    #include <unistd.h>
    #include <limits.h>
    #include <sys/stat.h>
#endif

/*
 * BrainfuckCompiler类 - Brainfuck语言编译器和解释器
 * 提供优化的Brainfuck代码解释执行、编译和调试功能
 * 特性：
 * - 标准的30000字节Brainfuck内存空间
 * - 预计算循环跳转表，优化执行性能
 * - 支持从字符串或文件加载代码
 * - 提供完整的解释执行功能
 * - 提供单步执行功能，便于调试
 * - 支持编译为C或C++代码
 * - 提供内存状态和执行状态的调试输出
 */
class BrainfuckCompiler {
private:
    static const int MEMORY_SIZE = 30000; // Brainfuck标准内存大小（30000字节）
    char memory[MEMORY_SIZE];             // 内存数组，模拟Brainfuck的内存空间
    int memoryPointer;                    // 内存指针，指向当前操作的内存位置
    int instructionPointer;               // 指令指针，指向当前执行的指令位置
    std::string code;                     // 存储编译后的Brainfuck代码（仅包含有效指令）
    std::map<int, int> jumpTable;         // 用于优化循环跳转的映射表，存储'['和']'的对应关系

    /*
     * 预计算循环跳转位置，优化执行性能
     * 构建'['和']'指令之间的映射关系，避免运行时重复查找
     */
    void precomputeJumps() {
        std::stack<int> loopStartStack;
        for (int i = 0; i < code.length(); i++) {
            if (code[i] == '[') {
                loopStartStack.push(i);
            } else if (code[i] == ']') {
                if (!loopStartStack.empty()) {
                    int startPos = loopStartStack.top();
                    loopStartStack.pop();
                    jumpTable[startPos] = i;
                    jumpTable[i] = startPos;
                }
            }
        }
    }

public:
    /*
     * 构造函数 - 初始化内存和指针
     * 设置内存指针和指令指针为0，并将所有内存单元初始化为0
     */
    BrainfuckCompiler() {
        memoryPointer = 0;
        instructionPointer = 0;
        // 初始化内存为0
        for (int i = 0; i < MEMORY_SIZE; i++) {
            memory[i] = 0;
        }
    }

    /*
     * 加载Brainfuck代码
     * 参数：
     * - program: 要加载的Brainfuck代码字符串
     * 副作用：
     * - 重置内存和所有指针
     * - 预计算跳转表
     */
    void loadCode(const std::string& program) {
        code = program;
        instructionPointer = 0;
        memoryPointer = 0;
        // 重置内存
        for (int i = 0; i < MEMORY_SIZE; i++) {
            memory[i] = 0;
        }
        // 预计算跳转表
        precomputeJumps();
    }

    /*
     * 从文件加载Brainfuck代码，自动过滤非Brainfuck指令
     * 参数：
     * - filename: 要加载的文件路径
     * 返回值：
     * - 成功加载返回true，文件打开失败返回false
     */
    bool loadCodeFromFile(const std::string& filename) {
        std::ifstream file(filename.c_str());
        if (!file.is_open()) {
            return false;
        }

        std::string program;
        char ch;
        while (file.get(ch)) {
            // 只保留有效的Brainfuck指令
            if (ch == '+' || ch == '-' || ch == '<' || ch == '>' || 
                ch == '.' || ch == ',' || ch == '[' || ch == ']') {
                program += ch;
            }
        }
        file.close();
        loadCode(program);
        return true;
    }

    /*
     * 解释执行完整的Brainfuck代码
     * 逐条执行代码中的指令，直到执行完毕
     */
    void interpret() {
        while (instructionPointer < code.length()) {
            switch (code[instructionPointer]) {
                case '+': // 增加内存值
                    memory[memoryPointer]++;
                    break;
                case '-': // 减少内存值
                    memory[memoryPointer]--;
                    break;
                case '>': // 内存指针右移
                    memoryPointer = (memoryPointer + 1) % MEMORY_SIZE;
                    break;
                case '<': // 内存指针左移
                    memoryPointer = (memoryPointer - 1 + MEMORY_SIZE) % MEMORY_SIZE;
                    break;
                case '.': // 输出内存值
                    std::cout << memory[memoryPointer];
                    break;
                case ',': // 输入值到内存
                    memory[memoryPointer] = getchar();
                    break;
                case '[': // 循环开始
                    if (memory[memoryPointer] == 0) {
                        // 如果当前内存值为0，跳转到对应的']'
                        instructionPointer = jumpTable[instructionPointer];
                    }
                    break;
                case ']': // 循环结束
                    if (memory[memoryPointer] != 0) {
                        // 如果当前内存值不为0，跳转到对应的'['
                        instructionPointer = jumpTable[instructionPointer];
                    }
                    break;
            }
            instructionPointer++;
        }
    }

    /*
     * 单步执行Brainfuck代码，用于调试
     * 执行当前指令指针指向的一条指令，然后将指令指针向前移动
     * 返回值：
     * - 如果执行了指令返回true
     * - 如果程序已执行完毕返回false
     */
    bool step() {
        if (instructionPointer >= code.length()) {
            return false; // 执行完毕
        }

        switch (code[instructionPointer]) {
            case '+':
                memory[memoryPointer]++;
                break;
            case '-':
                memory[memoryPointer]--;
                break;
            case '>':
                memoryPointer = (memoryPointer + 1) % MEMORY_SIZE;
                break;
            case '<':
                memoryPointer = (memoryPointer - 1 + MEMORY_SIZE) % MEMORY_SIZE;
                break;
            case '.':
                std::cout << memory[memoryPointer];
                break;
            case ',':
                memory[memoryPointer] = getchar();
                break;
            case '[':
                if (memory[memoryPointer] == 0) {
                    instructionPointer = jumpTable[instructionPointer];
                }
                break;
            case ']':
                if (memory[memoryPointer] != 0) {
                    instructionPointer = jumpTable[instructionPointer];
                }
                break;
        }
        instructionPointer++;
        return true;
    }

    /*
     * 打印当前内存状态，用于调试
     * 显示内存指针周围的内存值，便于观察程序执行情况
     * 参数：
     * - range: 要显示的内存范围（指针前后各显示range个值），默认为10
     */
    void printMemoryState(int range = 10) {
        std::cout << "Memory state around pointer (" << memoryPointer << "):\n";
        int start = std::max(0, memoryPointer - range);
        int end = std::min(MEMORY_SIZE - 1, memoryPointer + range);
        
        for (int i = start; i <= end; i++) {
            if (i == memoryPointer) {
                std::cout << "[" << static_cast<int>(memory[i]) << "] ";
            } else {
                std::cout << static_cast<int>(memory[i]) << " ";
            }
        }
        std::cout << std::endl;
    }

    /*
     * 打印当前执行状态，包括内存指针位置和下一条指令
     * 提供程序执行的上下文信息，便于调试
     */
    void printCurrentState() {
        std::cout << "Current state:\n";
        std::cout << "  Memory pointer: " << memoryPointer << " (value: " << static_cast<int>(memory[memoryPointer]) << ")\n";
        std::cout << "  Instruction pointer: " << instructionPointer;
        if (instructionPointer < code.length()) {
            std::cout << " (next instruction: '" << code[instructionPointer] << "')";
        }
        std::cout << std::endl;
    }

    /*
     * 将Brainfuck代码编译为等价的C代码
     * 返回值：
     * - 生成的C代码字符串
     */
    std::string compileToC() {
        std::string cCode = "#include <stdio.h>\n\n";
        cCode += "int main() {\n";
        cCode += "    unsigned char memory[30000] = {0};\n";
        cCode += "    unsigned char* ptr = memory;\n\n";

        for (int i = 0; i < code.length(); i++) {
            switch (code[i]) {
                case '+':
                    cCode += "    ++(*ptr);\n";
                    break;
                case '-':
                    cCode += "    --(*ptr);\n";
                    break;
                case '>':
                    cCode += "    ++ptr;\n";
                    break;
                case '<':
                    cCode += "    --ptr;\n";
                    break;
                case '.':
                    cCode += "    putchar(*ptr);\n";
                    break;
                case ',':
                    cCode += "    *ptr = getchar();\n";
                    break;
                case '[':
                    cCode += "    while (*ptr) {\n";
                    break;
                case ']':
                    cCode += "    }\n";
                    break;
            }
        }

        cCode += "\n    return 0;\n";
        cCode += "}";
        return cCode;
    }

    /*
     * 将Brainfuck代码编译为等价的C++代码
     * 返回值：
     * - 生成的C++代码字符串
     */
    std::string compileToCpp() {
        std::string cppCode = "#include <iostream>\n\n";
        cppCode += "int main() {\n";
        cppCode += "    unsigned char memory[30000] = {0};\n";
        cppCode += "    unsigned char* ptr = memory;\n\n";

        for (int i = 0; i < code.length(); i++) {
            switch (code[i]) {
                case '+':
                    cppCode += "    ++(*ptr);\n";
                    break;
                case '-':
                    cppCode += "    --(*ptr);\n";
                    break;
                case '>':
                    cppCode += "    ++ptr;\n";
                    break;
                case '<':
                    cppCode += "    --ptr;\n";
                    break;
                case '.':
                    cppCode += "    std::cout << *ptr;\n";
                    break;
                case ',':
                    cppCode += "    *ptr = std::cin.get();\n";
                    break;
                case '[':
                    cppCode += "    while (*ptr) {\n";
                    break;
                case ']':
                    cppCode += "    }\n";
                    break;
            }
        }

        cppCode += "\n    return 0;\n";
        cppCode += "}";
        return cppCode;
    }

    /*
     * 获取当前代码长度
     * 返回值：
     * - 当前加载的Brainfuck代码的长度
     */
    int getCodeLength() {
        return code.length();
    }

    /*
     * 获取编译后的代码
     * 返回值：
     * - 当前加载的Brainfuck代码的常量引用
     */
    const std::string& getCode() {
        return code;
    }
};

/*
 * Language枚举 - 定义程序支持的界面语言选项
 * 程序支持多种语言的界面本地化，便于不同国家和地区的用户使用
 */
enum Language {
    ENGLISH,      // 英语 - 默认语言
    CHINESE,      // 中文 - 简体中文界面
    SPANISH,      // 西班牙语 - español界面
    FRENCH,       // 法语 - français界面
    GERMAN,       // 德语 - Deutsch界面
    JAPANESE,     // 日语 - 日本语界面
    RUSSIAN,      // 俄语 - русский язык界面
    PORTUGUESE,   // 葡萄牙语 - português界面
    KOREAN,       // 韩语 - 한국어界面
    ITALIAN,      // 意大利语 - italiano界面
    DUTCH,        // 荷兰语 - Nederlands界面
    TURKISH,      // 土耳其语 - Türkçe界面
    POLISH,       // 波兰语 - język polski界面
    SWEDISH,      // 瑞典语 - svenska界面
    UKRAINIAN     // 乌克兰语 - українська мова界面
};

/*
 * Color枚举 - 定义程序支持的颜色选项
 * 用于控制台界面的文本和背景颜色设置，提供良好的视觉体验
 */
enum Color {
    BLACK,   // 黑色 - 常用于背景或文本
    RED,     // 红色 - 常用于强调或错误提示
    GREEN,   // 绿色 - 常用于成功状态或正数
    YELLOW,  // 黄色 - 常用于警告或提示
    BLUE,    // 蓝色 - 常用于信息或链接
    MAGENTA, // 洋红色 - 常用于特殊标记
    CYAN,    // 青色 - 常用于次要信息
    WHITE,   // 白色 - 常用于文本
    ORANGE,  // 橙色 - 常用于额外强调
    BROWN    // 棕色 - 常用于次要文本或背景
};

// 预定义颜色的RGB值数组（更兼容旧版C++编译器）
const int COLOR_RGB[10][3] = {
    {0, 0, 0},     // BLACK
    {255, 0, 0},   // RED
    {0, 255, 0},   // GREEN
    {255, 255, 0}, // YELLOW
    {0, 0, 255},   // BLUE
    {255, 0, 255}, // MAGENTA
    {0, 255, 255}, // CYAN
    {255, 255, 255}, // WHITE
    {255, 165, 0}, // ORANGE
    {165, 42, 42}  // BROWN
};

// 翻译文本映射
std::map<Language, std::map<std::string, std::string> > translations;

// 初始化翻译映射
void initializeTranslations() {
    // English
    std::map<std::string, std::string> english;
    english["bf_editor_title"] = "BF Editor";
    english["main_menu_title"] = "Brainfuck IDE";
    english["main_menu"] = "Brainfuck IDE\nMain Menu\nCreate Program\nOpen Program\nEditor Settings\nExit\nChoice: ";
    english["create_program"] = "Create Program";
    english["open_program"] = "Open Program";
    english["exit_program"] = "Exit";
    english["folder"] = "(folder)";
    english["editor_title"] = "Brainfuck Program Editor";
    english["current_program"] = "Current Program (Original):";
    english["filtered_code"] = "Filtered executable code:";
    english["editor_commands"] = "Program Interface";
    english["program_interface"] = "Program Interface";
    english["program_name"] = "Program Name";
    english["file_path"] = "Path";
    english["file_content"] = "File Content";
    english["empty_content"] = "empty";
    english["menu_label"] = "Menu";
    english["edit_program"] = "Input/Edit program (supports comments)";
    english["run_program"] = "Run program";
    english["save_program"] = "Save program";
    english["clear_program"] = "Clear program";
    english["show_filtered"] = "Show filtered code";
    english["language_settings_editor"] = "Language Settings";
    english["back_menu"] = "Return to main menu";
    english["select_option"] = "Select: ";
    english["program_empty"] = "Program is empty!";
    english["program_cleared"] = "Program cleared!";
    english["invalid_choice"] = "Invalid choice!";
    english["enter_filename"] = "Enter filename (without extension): ";
    english["save_success"] = "Program saved successfully!";
    english["save_failed"] = "Save failed!";
    english["run_results"] = "Run Results:";
    english["run_success"] = "Program executed successfully.";
    english["pointer_error"] = "Pointer out of bounds!";
    english["compile_error"] = "Compile error!";
    english["input_program"] = "Enter Brainfuck program (characters other than the 8 valid commands and //, /* */ are treated as comments, enter '0' alone to end):";
    english["comments_supported"] = "Supports single-line (//) and multi-line (/* */) comments, other characters are also treated as comments";
    english["no_bf_files"] = "No .bf files found!";
    english["found_files"] = "Found %d .bf files (including subdirectories):";
    english["open_file_prompt"] = "Enter file number to open (0 to return): ";
    english["invalid_selection"] = "Invalid selection!";
    english["opening_file"] = "Opening file: %s";
    english["file_content"] = "Program content (with comments):";
    english["enter_to_editor"] = "Press any key to enter editor...";
    english["file_empty"] = "File is empty or read failed!";
    english["language_settings"] = "Language Settings";
    english["current_language"] = "Current language: ";
    english["select_language"] = "Select language:";
    english["back_to_main"] = "Back to main menu";
    english["language_changed"] = "Language changed successfully!";
    english["editor_settings"] = "Editor Settings";
    english["color_settings"] = "Color Settings";
    english["current_background"] = "Current background color: ";
    english["current_code_color"] = "Current code color: ";
    english["press_any_key"] = "Press any key to continue . . .";
    english["current_comment_color"] = "Current comment color: ";
    english["select_background_color"] = "Select background color:";
    english["select_code_color"] = "Select code color:";
    english["select_comment_color"] = "Select comment color:";
    english["color_options"] = "Predefined colors\nCustom RGB color\nCustom HSL color\nBack to color settings";
    english["predefined_colors"] = "Predefined colors:";
    english["enter_rgb"] = "Enter RGB values (0-255) separated by spaces (R G B): ";
    english["enter_hsl"] = "Enter HSL values (H:0-360, S:0-100, L:0-100) separated by spaces (H S L): ";
    english["color_changed"] = "Color changed successfully!";
    english["invalid_rgb"] = "Invalid RGB values! Using default color.";
    english["invalid_hsl"] = "Invalid HSL values! Using default color.";
    english["reset_colors"] = "Reset to default colors";
    english["colors_reset"] = "Colors reset to default!";
    english["unnamed_file"] = "未命名文件";
    english["current_file"] = "Current file";
    english["navigation_hint"] = "Use up/down arrow keys to select, press Enter to confirm.";
    english["enter_new_folder_name"] = "Enter new folder name: ";
    english["folder_created"] = "Folder created successfully!";
    english["create_folder_failed"] = "Failed to create folder!";
    english["enter_new_name"] = "Enter new name: ";
    english["rename_success"] = "Renamed successfully!";
    english["rename_failed"] = "Failed to rename!";
    english["create_new_folder"] = "Create New Folder";
    english["save_here"] = "Save Here";
    english["browse_directories"] = "Browse Directories";
    english["go_up"] = "Go Up";
    english["rename_item"] = "Rename item";
    english["item_renamed"] = "Item renamed";
    english["current_directory"] = "Current Directory";
    translations[ENGLISH] = english;
    
    // Chinese
    std::map<std::string, std::string> chinese;
    chinese["bf_editor_title"] = "BF 编辑器";
    chinese["main_menu_title"] = "Brainfuck IDE";
    chinese["main_menu"] = "Brainfuck IDE\n主菜单\n创建程序\n打开程序\n编辑器设置\n退出\n选择: ";
    chinese["create_program"] = "创建程序";
    chinese["open_program"] = "打开程序";
    chinese["exit_program"] = "退出";
    chinese["folder"] = "(文件夹)";
    chinese["editor_title"] = "Brainfuck 程序编辑器";
    chinese["current_program"] = "当前程序 (原始):";
    chinese["filtered_code"] = "过滤后可执行代码:";
    chinese["editor_commands"] = "程序界面";
    chinese["program_name"] = "程序名";
    chinese["file_path"] = "路径";
    chinese["file_content"] = "文件内容";
    chinese["empty_content"] = "空";
    chinese["menu_label"] = "菜单";
    chinese["edit_program"] = "输入/编辑程序 (支持注释)";
    chinese["run_program"] = "运行程序";
    chinese["save_program"] = "保存程序";
    chinese["clear_program"] = "清空程序";
    chinese["show_filtered"] = "显示过滤后的代码";
    chinese["language_settings_editor"] = "语言设置";
    chinese["back_menu"] = "返回主菜单";
    chinese["select_option"] = "选择: ";
    chinese["program_empty"] = "程序为空!";
    chinese["program_cleared"] = "程序已清空!";
    chinese["invalid_choice"] = "无效选择!";
    chinese["enter_filename"] = "请输入文件名(不含扩展名): ";
    chinese["save_success"] = "程序保存成功!";
    chinese["save_failed"] = "保存失败!";
    chinese["run_results"] = "运行结果:";
    chinese["run_success"] = "程序执行成功。";
    chinese["pointer_error"] = "指针越界!";
    chinese["compile_error"] = "编译错误!";
    chinese["input_program"] = "请输入Brainfuck程序 (除8个有效命令和//, /* */外的字符都视为注释，输入0单独一行结束):";
    chinese["comments_supported"] = "支持单行注释(//)和多行注释(/* */)，其他字符也视为注释";
    chinese["no_bf_files"] = "未找到任何.bf文件!";
    chinese["found_files"] = "找到 %d 个 .bf 文件 (包含子目录):";
    chinese["open_file_prompt"] = "请输入要打开的文件序号 (0返回): ";
    chinese["invalid_selection"] = "无效的选择!";
    chinese["opening_file"] = "打开文件: %s";
    chinese["file_content"] = "程序内容 (含注释):";
    chinese["enter_to_editor"] = "按任意键进入编辑器...";
    chinese["file_empty"] = "文件为空或读取失败!";
    chinese["language_settings"] = "语言设置";
    chinese["current_language"] = "当前语言: ";
    chinese["select_language"] = "选择语言:";
    chinese["back_to_main"] = "返回主菜单";
    chinese["language_changed"] = "语言更改成功!";
    chinese["editor_settings"] = "编辑器设置";
    chinese["color_settings"] = "颜色设置";
    chinese["current_background"] = "当前背景颜色: ";
    chinese["current_code_color"] = "当前代码颜色: ";
    chinese["current_comment_color"] = "当前注释颜色: ";
    chinese["select_background_color"] = "选择背景颜色:";
    chinese["select_code_color"] = "选择代码颜色:";
    chinese["select_comment_color"] = "选择注释颜色:";
    chinese["color_options"] = "预定义颜色\n自定义RGB颜色\n自定义HSL颜色\n返回颜色设置";
    chinese["predefined_colors"] = "预定义颜色:";
    chinese["enter_rgb"] = "请输入RGB值 (0-255)用空格分隔 (R G B): ";
    chinese["enter_hsl"] = "请输入HSL值 (H:0-360, S:0-100, L:0-100)用空格分隔 (H S L): ";
    chinese["color_changed"] = "颜色更改成功!";
    chinese["invalid_rgb"] = "无效的RGB值! 使用默认颜色。";
    chinese["invalid_hsl"] = "无效的HSL值! 使用默认颜色。";
    chinese["reset_colors"] = "重置为默认颜色";
    chinese["colors_reset"] = "颜色已重置为默认值!";
    chinese["unnamed_file"] = "未命名文件";
    chinese["current_file"] = "当前文件";
    chinese["navigation_hint"] = "使用上下方向键选择，按Enter键确认。";
    chinese["enter_new_folder_name"] = "输入新文件夹名称：";
    chinese["folder_created"] = "文件夹创建成功!";
    chinese["create_folder_failed"] = "创建文件夹失败!";
    chinese["enter_new_name"] = "输入新名称：";
    chinese["rename_success"] = "重命名成功!";
    chinese["rename_failed"] = "重命名失败!";
    chinese["press_any_key"] = "按任意键继续 . . .";
    chinese["create_new_folder"] = "创建新文件夹";
    chinese["save_here"] = "保存在此";
    chinese["browse_directories"] = "浏览目录";
    chinese["go_up"] = "返回上级";
    chinese["rename_item"] = "重命名项目";
    chinese["item_renamed"] = "项目已重命名";
    chinese["current_directory"] = "当前目录";
    translations[CHINESE] = chinese;
    
    // Spanish
    std::map<std::string, std::string> spanish;
    spanish["bf_editor_title"] = "Editor BF";
    spanish["main_menu_title"] = "Brainfuck IDE";
    spanish["main_menu"] = "Brainfuck IDE\nMenú Principal\nCrear Programa\nAbrir Programa\nConfiguración del Editor\nSalir\nSelección: ";
    spanish["create_program"] = "Crear Programa";
    spanish["open_program"] = "Abrir Programa";
    spanish["exit_program"] = "Salir";
    spanish["folder"] = "(carpeta)";
    spanish["editor_title"] = "Editor de Programa Brainfuck";
    spanish["current_program"] = "Programa Actual (Original):";
    spanish["filtered_code"] = "Código ejecutable filtrado:";
    spanish["editor_commands"] = "Interfaz de Programa";
    spanish["program_name"] = "Nombre del Programa";
    spanish["file_path"] = "Ruta";
    spanish["file_content"] = "Contenido del Archivo";
    spanish["empty_content"] = "vacío";
    spanish["menu_label"] = "Menú";
    spanish["edit_program"] = "Introducir/Editar programa (soporta comentarios)";
    spanish["run_program"] = "Ejecutar programa";
    spanish["save_program"] = "Guardar programa";
    spanish["clear_program"] = "Limpiar programa";
    spanish["show_filtered"] = "Mostrar código filtrado";
    spanish["language_settings_editor"] = "Configuración de Idioma";
    spanish["back_menu"] = "Volver al menú principal";
    spanish["select_option"] = "Selección: ";
    spanish["program_empty"] = "¡Programa vacío!";
    spanish["program_cleared"] = "¡Programa limpiado!";
    spanish["invalid_choice"] = "¡Selección inválida!";
    spanish["enter_filename"] = "Introduce el nombre del archivo (sin extensión): ";
    spanish["save_success"] = "¡Programa guardado con éxito!";
    spanish["save_failed"] = "¡Error al guardar!";
    spanish["run_results"] = "Resultados de ejecución:";
    spanish["run_success"] = "Programa ejecutado exitosamente.";
    spanish["pointer_error"] = "¡Puntero fuera de límites!";
    spanish["compile_error"] = "¡Error de compilación!";
    spanish["input_program"] = "Ingrese programa Brainfuck (caracteres distintos a los 8 comandos válidos y //, /* */ son tratados como comentarios, ingrese '0' solo para terminar):";
    spanish["comments_supported"] = "Soporta comentarios de una línea (//) y multi-línea (/* */), otros caracteres también son tratados como comentarios";
    spanish["no_bf_files"] = "¡No se encontraron archivos .bf!";
    spanish["found_files"] = "Encontrados %d archivos .bf (incluyendo subdirectorios):";
    spanish["open_file_prompt"] = "Ingrese número de archivo a abrir (0 para volver): ";
    spanish["invalid_selection"] = "¡Selección inválida!";
    spanish["opening_file"] = "Abriendo archivo: %s";
    spanish["file_content"] = "Contenido del programa (con comentarios):";
    spanish["enter_to_editor"] = "Presione cualquier tecla para entrar al editor...";
    spanish["file_empty"] = "¡El archivo está vacío o la lectura falló!";
    spanish["language_settings"] = "Configuración del Editor - Configuración de Idioma";
    spanish["current_language"] = "Idioma actual: ";
    spanish["select_language"] = "Seleccionar idioma:";
    spanish["back_to_main"] = "Volver al menú principal";
    spanish["language_changed"] = "¡Idioma cambiado exitosamente!";
    spanish["editor_settings"] = "Configuración del Editor";
    spanish["color_settings"] = "Configuración del Editor - Configuración de Colores";
    spanish["current_background"] = "Color de fondo actual: ";
    spanish["current_code_color"] = "Color de código actual: ";
    spanish["current_comment_color"] = "Color de comentario actual: ";
    spanish["select_background_color"] = "Seleccionar color de fondo:";
    spanish["select_code_color"] = "Seleccionar color de código:";
    spanish["select_comment_color"] = "Seleccionar color de comentario:";
    spanish["color_options"] = "Colores predefinidos\nColor RGB personalizado\nColor HSL personalizado\nVolver a configuración de colores";
    spanish["predefined_colors"] = "Colores predefinidos:";
    spanish["enter_rgb"] = "Ingrese valores RGB (0-255) separados por espacios (R G B): ";
    spanish["enter_hsl"] = "Ingrese valores HSL (H:0-360, S:0-100, L:0-100) separados por espacios (H S L): ";
    spanish["color_changed"] = "¡Color cambiado exitosamente!";
    spanish["invalid_rgb"] = "¡Valores RGB inválidos! Usando color por defecto.";
    spanish["invalid_hsl"] = "¡Valores HSL inválidos! Usando color por defecto.";
    spanish["reset_colors"] = "Restablecer colores por defecto";
    spanish["colors_reset"] = "¡Colores restablecidos a los valores por defecto!";
    spanish["unnamed_file"] = "archivo sin nombre";
    spanish["current_file"] = "Archivo actual";
    spanish["navigation_hint"] = "Use las teclas de flecha arriba/abajo para seleccionar, presione Enter para confirmar.";
    spanish["enter_new_folder_name"] = "Introduce el nombre de la nueva carpeta: ";
    spanish["folder_created"] = "¡Carpeta creada con éxito!";
    spanish["create_folder_failed"] = "¡Error al crear la carpeta!";
    spanish["enter_new_name"] = "Introduce el nuevo nombre: ";
    spanish["rename_success"] = "¡Cambio de nombre exitoso!";
    spanish["rename_failed"] = "¡Error al cambiar el nombre!";
    spanish["press_any_key"] = "Presione cualquier tecla para continuar . . .";
    spanish["create_new_folder"] = "Crear nueva carpeta";
    spanish["save_here"] = "Guardar aquí";
    spanish["browse_directories"] = "Explorar directorios";
    spanish["go_up"] = "Subir nivel";
    spanish["rename_item"] = "Renombrar elemento";
    spanish["item_renamed"] = "Elemento renombrado";
    spanish["current_directory"] = "Directorio actual";
    translations[SPANISH] = spanish;
    
    // French
    std::map<std::string, std::string> french;
    french["bf_editor_title"] = "Éditeur BF";
    french["main_menu_title"] = "Brainfuck IDE";
    french["main_menu"] = "Brainfuck IDE\nMenu Principal\nCréer un Programme\nOuvrir un Programme\nParamètres de l'éditeur\nQuitter\nChoix: ";
    french["create_program"] = "Créer un Programme";
    french["open_program"] = "Ouvrir un Programme";
    french["exit_program"] = "Quitter";
    french["folder"] = "(dossier)";
    french["editor_title"] = "Éditeur de Programme Brainfuck";
    french["current_program"] = "Programme Actuel (Original):";
    french["filtered_code"] = "Code exécutable filtré:";
    french["editor_commands"] = "Interface de Programme";
    french["program_name"] = "Nom du Programme";
    french["file_path"] = "Chemin";
    french["file_content"] = "Contenu du Fichier";
    french["empty_content"] = "vide";
    french["menu_label"] = "Menu";
    french["edit_program"] = "Saisir/éditer le programme (supporte les commentaires)";
    french["run_program"] = "Exécuter le programme";
    french["save_program"] = "Sauvegarder le programme";
    french["clear_program"] = "Effacer le programme";
    french["show_filtered"] = "Afficher le code filtré";
    french["language_settings_editor"] = "Paramètres de Langue";
    french["back_menu"] = "Retour au menu principal";
    french["select_option"] = "Sélectionner: ";
    french["program_empty"] = "Programme vide !";
    french["program_cleared"] = "Programme effacé !";
    french["invalid_choice"] = "Choix invalide !";
    french["enter_filename"] = "Entrez le nom du fichier (sans extension): ";
    french["save_success"] = "Programme sauvegardé avec succès !";
    french["save_failed"] = "Erreur lors de la sauvegarde !";
    french["run_results"] = "Résultats de l'Exécution:";
    french["run_success"] = "Programme exécuté avec succès.";
    french["pointer_error"] = "Pointeur hors limites !";
    french["compile_error"] = "Erreur de compilation !";
    french["input_program"] = "Entrez le programme Brainfuck (les caractères autres que les 8 commandes valides et //, /* */ sont traités comme des commentaires, entrez '0' seul pour terminer):";
    french["comments_supported"] = "Supporte les commentaires d'une ligne (//) et multi-lignes (/* */), les autres caractères sont également traités comme des commentaires";
    french["no_bf_files"] = "Aucun fichier .bf trouvé !";
    french["found_files"] = "Trouvé %d fichiers .bf (incluant les sous-répertoires):";
    french["open_file_prompt"] = "Entrez le numéro du fichier à ouvrir (0 pour revenir): ";
    french["invalid_selection"] = "Sélection invalide !";
    french["opening_file"] = "Ouverture du fichier: %s";
    french["file_content"] = "Contenu du programme (avec commentaires):";
    french["enter_to_editor"] = "Appuyez sur une touche pour entrer dans l'éditeur...";
    french["file_empty"] = "Le fichier est vide ou la lecture a échoué !";
    french["language_settings"] = "Paramètres de l'éditeur - Paramètres de Langue";
    french["current_language"] = "Langue actuelle: ";
    french["select_language"] = "Sélectionner la langue:";
    french["back_to_main"] = "Retour au menu principal";
    french["language_changed"] = "Langue changée avec succès !";
    french["editor_settings"] = "Paramètres de l'éditeur";
    french["color_settings"] = "Paramètres de l'éditeur - Paramètres de Couleur";
    french["current_background"] = "Couleur de fond actuelle: ";
    french["current_code_color"] = "Couleur du code actuelle: ";
    french["current_comment_color"] = "Couleur des commentaires actuelle: ";
    french["select_background_color"] = "Sélectionner la couleur de fond:";
    french["select_code_color"] = "Sélectionner la couleur du code:";
    french["select_comment_color"] = "Sélectionner la couleur des commentaires:";
    french["color_options"] = "Couleurs prédéfinies\nCouleur RVB personnalisée\nCouleur HSL personnalisée\nRetour aux paramètres de couleur";
    french["predefined_colors"] = "Couleurs prédéfinies:";
    french["enter_rgb"] = "Entrez les valeurs RVB (0-255) séparées par des espaces (R V B): ";
    french["enter_hsl"] = "Entrez les valeurs HSL (H:0-360, S:0-100, L:0-100) séparées par des espaces (H S L): ";
    french["color_changed"] = "Couleur changée avec succès !";
    french["invalid_rgb"] = "Valeurs RVB invalides ! Utilisation de la couleur par défaut.";
    french["invalid_hsl"] = "Valeurs HSL invalides ! Utilisation de la couleur par défaut.";
    french["reset_colors"] = "Réinitialiser les couleurs par défaut";
    french["colors_reset"] = "Couleurs réinitialisées aux valeurs par défaut !";
    french["unnamed_file"] = "fichier sans nom";
    french["current_file"] = "Fichier actuel";
    french["navigation_hint"] = "Utilisez les touches fléchées haut/bas pour sélectionner, appuyez sur Entrée pour confirmer.";
    french["enter_new_folder_name"] = "Entrez le nom du nouveau dossier: ";
    french["folder_created"] = "Dossier créé avec succès !";
    french["create_folder_failed"] = "Erreur lors de la création du dossier !";
    french["enter_new_name"] = "Entrez le nouveau nom: ";
    french["rename_success"] = "Renommage réussi !";
    french["rename_failed"] = "Échec du renommage !";
    french["press_any_key"] = "Appuyez sur une touche pour continuer . . .";
    french["create_new_folder"] = "Créer un nouveau dossier";
    french["save_here"] = "Enregistrer ici";
    french["browse_directories"] = "Parcourir les répertoires";
    french["go_up"] = "Remonter";
    french["rename_item"] = "Renommer l'élément";
    french["item_renamed"] = "Élément renommé";
    french["current_directory"] = "Répertoire actuel";
    translations[FRENCH] = french;
    
    // German
    std::map<std::string, std::string> german;
    german["bf_editor_title"] = "BF-Editor";
    german["main_menu_title"] = "Brainfuck IDE";
    german["main_menu"] = "Brainfuck IDE\nHauptmenü\nProgramm erstellen\nProgramm öffnen\nEditor-Einstellungen\nBeenden\nAuswahl: ";
    german["create_program"] = "Programm erstellen";
    german["open_program"] = "Programm öffnen";
    german["exit_program"] = "Beenden";
    german["folder"] = "(ordner)";
    german["editor_title"] = "Brainfuck Programm-Editor";
    german["current_program"] = "Aktuelles Programm (Original):";
    german["filtered_code"] = "Gefilterter ausführbarer Code:";
    german["editor_commands"] = "Programmoberfläche";
    german["program_interface"] = "Programmoberfläche";
    german["program_name"] = "Programmname";
    german["file_path"] = "Pfad";
    german["file_content"] = "Dateiinhalt";
    german["empty_content"] = "leer";
    german["menu_label"] = "Menü";
    german["edit_program"] = "Programm eingeben/bearbeiten (unterstützt Kommentare)";
    german["run_program"] = "Programm ausführen";
    german["save_program"] = "Programm speichern";
    german["clear_program"] = "Programm löschen";
    german["show_filtered"] = "Gefilterten Code anzeigen";
    german["language_settings_editor"] = "Spracheinstellungen";
    german["back_menu"] = "Zurück zum Hauptmenü";
    german["select_option"] = "Auswahl: ";
    german["program_empty"] = "Programm ist leer!";
    german["program_cleared"] = "Programm gelöscht!";
    german["invalid_choice"] = "Ungültige Auswahl!";
    german["enter_filename"] = "Dateinamen eingeben (ohne Erweiterung): ";
    german["save_success"] = "Programm erfolgreich gespeichert!";
    german["save_failed"] = "Fehler beim Speichern!";
    german["run_results"] = "Ausführungsergebnisse:";
    german["run_success"] = "Programm erfolgreich ausgeführt.";
    german["pointer_error"] = "Zeiger außerhalb der Grenzen!";
    german["compile_error"] = "Kompilierungsfehler!";
    german["input_program"] = "Brainfuck-Programm eingeben (Zeichen außer den 8 gültigen Befehlen und //, /* */ werden als Kommentare behandelt, '0' alleine eingeben zum Beenden):";
    german["comments_supported"] = "Unterstützt einzeilige (//) und mehrzeilige (/* */) Kommentare, andere Zeichen werden ebenfalls als Kommentare behandelt";
    german["no_bf_files"] = "Keine .bf-Dateien gefunden!";
    german["found_files"] = "%d .bf-Dateien gefunden (einschließlich Unterverzeichnisse):";
    german["open_file_prompt"] = "Dateinummer zum Öffnen eingeben (0 zum Zurückkehren): ";
    german["invalid_selection"] = "Ungültige Auswahl!";
    german["opening_file"] = "Öffne Datei: %s";
    german["file_content"] = "Programminhalt (mit Kommentaren):";
    german["enter_to_editor"] = "Drücken Sie eine beliebige Taste, um den Editor zu öffnen...";
    german["file_empty"] = "Datei ist leer oder Lesen fehlgeschlagen!";
    german["language_settings"] = "Editor-Einstellungen - Spracheinstellungen";
    german["current_language"] = "Aktuelle Sprache: ";
    german["select_language"] = "Sprache auswählen:";
    german["back_to_main"] = "Zurück zum Hauptmenü";
    german["language_changed"] = "Sprache erfolgreich geändert!";
    german["editor_settings"] = "Editor-Einstellungen";
    german["color_settings"] = "Editor-Einstellungen - Farbeinstellungen";
    german["current_background"] = "Aktuelle Hintergrundfarbe: ";
    german["current_code_color"] = "Aktuelle Codefarbe: ";
    german["current_comment_color"] = "Aktuelle Kommentarfarbe: ";
    german["select_background_color"] = "Hintergrundfarbe auswählen:";
    german["select_code_color"] = "Codefarbe auswählen:";
    german["select_comment_color"] = "Kommentarfarbe auswählen:";
    german["color_options"] = "Vordefinierte Farben\nBenutzerdefinierte RGB-Farbe\nBenutzerdefinierte HSL-Farbe\nZurück zu Farbeinstellungen";
    german["predefined_colors"] = "Vordefinierte Farben:";
    german["enter_rgb"] = "RGB-Werte eingeben (0-255) durch Leerzeichen getrennt (R G B): ";
    german["enter_hsl"] = "HSL-Werte eingeben (H:0-360, S:0-100, L:0-100) durch Leerzeichen getrennt (H S L): ";
    german["color_changed"] = "Farbe erfolgreich geändert!";
    german["invalid_rgb"] = "Ungültige RGB-Werte! Verwende Standardfarbe.";
    german["invalid_hsl"] = "Ungültige HSL-Werte! Verwende Standardfarbe.";
    german["reset_colors"] = "Auf Standardfarben zurücksetzen";
    german["colors_reset"] = "Farben auf Standardwerte zurückgesetzt!";
    german["unnamed_file"] = "unbenannte Datei";
    german["current_file"] = "Aktuelle Datei";
    german["navigation_hint"] = "Verwenden Sie die Pfeiltasten nach oben/unten zur Auswahl, drücken Sie Enter zur Bestätigung.";
    german["enter_new_folder_name"] = "Geben Sie den Namen des neuen Ordners ein: ";
    german["folder_created"] = "Ordner erfolgreich erstellt!";
    german["create_folder_failed"] = "Erstellen des Ordners fehlgeschlagen!";
    german["enter_new_name"] = "Geben Sie den neuen Namen ein: ";
    german["rename_success"] = "Umbenennen erfolgreich!";
    german["rename_failed"] = "Umbenennen fehlgeschlagen!";
    german["press_any_key"] = "Drücken Sie eine beliebige Taste, um fortzufahren . . .";
    german["create_new_folder"] = "Neuen Ordner erstellen";
    german["save_here"] = "Hier speichern";
    german["browse_directories"] = "Verzeichnisse durchsuchen";
    german["go_up"] = "Hoch navigieren";
    german["rename_item"] = "Element umbenennen";
    german["item_renamed"] = "Element umbenannt";
    german["current_directory"] = "Aktuelles Verzeichnis";
    translations[GERMAN] = german;
    
    // Japanese
    std::map<std::string, std::string> japanese;
    japanese["bf_editor_title"] = "BF エディタ";
    japanese["main_menu_title"] = "メインメニュー";
    japanese["main_menu"] = "Brainfuck IDE\nメインメニュー\nプログラムを作成\nプログラムを開く\nエディタ設定\n終了\n選択: ";
    japanese["create_program"] = "プログラムを作成";
    japanese["open_program"] = "プログラムを開く";
    japanese["exit_program"] = "終了";
    japanese["folder"] = "(フォルダ)";
    japanese["editor_title"] = "Brainfuck プログラムエディタ";
    japanese["current_program"] = "現在のプログラム (元の):";
    japanese["filtered_code"] = "フィルタリングされた実行可能コード:";
    japanese["editor_commands"] = "プログラムインターフェース:";
    japanese["program_interface"] = "プログラムインターフェース";
    japanese["program_name"] = "プログラム名";
    japanese["file_path"] = "パス";
    japanese["file_content"] = "ファイル内容";
    japanese["empty_content"] = "空";
    japanese["menu_label"] = "メニュー";
    japanese["edit_program"] = "プログラムの入力/編集 (コメントをサポート)";
    japanese["run_program"] = "プログラムを実行";
    japanese["save_program"] = "プログラムを保存 (コメント付き)";
    japanese["clear_program"] = "プログラムをクリア";
    japanese["show_filtered"] = "フィルタリングされたコードを表示";
    japanese["language_settings_editor"] = "言語設定";
    japanese["back_menu"] = "メインメニューに戻る";
    japanese["select_option"] = "選択: ";
    japanese["program_empty"] = "プログラムが空です!";
    japanese["program_cleared"] = "プログラムがクリアされました!";
    japanese["invalid_choice"] = "無効な選択です!";
    japanese["enter_filename"] = "ファイル名を入力してください (拡張子なし): ";
    japanese["save_success"] = "プログラムが正常に保存されました!";
    japanese["save_failed"] = "保存に失敗しました!";
    japanese["run_results"] = "実行結果:";
    japanese["run_success"] = "プログラムが正常に実行されました。";
    japanese["pointer_error"] = "ポインタが範囲外です!";
    japanese["compile_error"] = "コンパイルエラー!";
    japanese["input_program"] = "Brainfuckプログラムを入力してください (8つの有効なコマンドと//, /* */以外の文字はコメントとして扱われ、'0'のみで入力を終了):";
    japanese["comments_supported"] = "単一行コメント(//)と複数行コメント(/* */)をサポートします。その他の文字もコメントとして扱われます";
    japanese["no_bf_files"] = ".bfファイルが見つかりません!";
    japanese["found_files"] = "%d 個の .bf ファイルが見つかりました (サブディレクトリを含む):";
    japanese["open_file_prompt"] = "開くファイル番号を入力してください (0で戻る): ";
    japanese["invalid_selection"] = "無効な選択です!";
    japanese["opening_file"] = "ファイルを開いています: %s";
    japanese["file_content"] = "プログラムの内容 (コメント付き):";
    japanese["enter_to_editor"] = "エディタに入るには任意のキーを押してください...";
    japanese["file_empty"] = "ファイルが空であるか読み取りに失敗しました!";
    japanese["language_settings"] = "エディタ設定-言語設定";
    japanese["current_language"] = "現在の言語: ";
    japanese["select_language"] = "言語を選択:";
    japanese["back_to_main"] = "メインメニューに戻る";
    japanese["language_changed"] = "言語が正常に変更されました!";
    japanese["editor_settings"] = "エディタ設定";
    japanese["color_settings"] = "エディタ設定-色設定";
    japanese["current_background"] = "現在の背景色: ";
    japanese["current_code_color"] = "現在のコード色: ";
    japanese["current_comment_color"] = "現在のコメント色: ";
    japanese["select_background_color"] = "背景色を選択:";
    japanese["select_code_color"] = "コード色を選択:";
    japanese["select_comment_color"] = "コメント色を選択:";
    japanese["color_options"] = "プリセットカラー\nカスタムRGBカラー\nカスタムHSLカラー\n色設定に戻る";
    japanese["predefined_colors"] = "プリセットカラー:";
    japanese["enter_rgb"] = "RGB値を入力してください (0-255) スペースで区切ってください (R G B): ";
    japanese["enter_hsl"] = "HSL値を入力してください (H:0-360, S:0-100, L:0-100) スペースで区切ってください (H S L): ";
    japanese["color_changed"] = "色が正常に変更されました!";
    japanese["invalid_rgb"] = "無効なRGB値です! デフォルトの色を使用します。";
    japanese["invalid_hsl"] = "無効なHSL値です! デフォルトの色を使用します。";
    japanese["reset_colors"] = "デフォルトの色にリセット";
    japanese["colors_reset"] = "色がデフォルトの値にリセットされました!";
    japanese["unnamed_file"] = "未命名ファイル";
    japanese["current_file"] = "現在のファイル";
    japanese["navigation_hint"] = "上下キーで選択し、Enterキーで確定してください。";
    japanese["enter_new_folder_name"] = "新しいフォルダー名を入力してください：";
    japanese["folder_created"] = "フォルダーが正常に作成されました！";
    japanese["create_folder_failed"] = "フォルダーの作成に失敗しました！";
    japanese["enter_new_name"] = "新しい名前を入力してください：";
    japanese["rename_success"] = "名前の変更に成功しました！";
    japanese["rename_failed"] = "名前の変更に失敗しました！";
    japanese["press_any_key"] = "続けるには任意のキーを押してください . . .";
    japanese["create_new_folder"] = "新しいフォルダーを作成";
    japanese["save_here"] = "ここに保存";
    japanese["browse_directories"] = "ディレクトリを参照";
    japanese["go_up"] = "上に移動";
    japanese["current_directory"] = "現在のディレクトリ";
    translations[JAPANESE] = japanese;
    
    // Russian
    std::map<std::string, std::string> russian;
    russian["bf_editor_title"] = "BF Редактор";
    russian["main_menu_title"] = "Главное меню";
    russian["main_menu"] = "Brainfuck IDE\nГлавное меню\nСоздать программу\nОткрыть программу\nНастройки редактора\nВыход\nВыбор: ";
    russian["create_program"] = "Создать программу";
    russian["open_program"] = "Открыть программу";
    russian["exit_program"] = "Выход";
    russian["folder"] = "(папка)";
    russian["editor_title"] = "Редактор программы Brainfuck";
    russian["current_program"] = "Текущая программа (Оригинал):";
    russian["filtered_code"] = "Отфильтрованный исполняемый код:";
    russian["editor_commands"] = "Программный Интерфейс:";
    russian["program_interface"] = "Программный Интерфейс";
    russian["program_name"] = "Название программы";
    russian["file_path"] = "Путь";
    russian["file_content"] = "Содержимое файла";
    russian["empty_content"] = "пусто";
    russian["menu_label"] = "Меню";
    russian["edit_program"] = "Ввести/Редактировать программу (поддерживает комментарии)";
    russian["run_program"] = "Запустить программу";
    russian["save_program"] = "Сохранить программу (с комментариями)";
    russian["clear_program"] = "Очистить программу";
    russian["show_filtered"] = "Показать отфильтрованный код";
    russian["language_settings_editor"] = "Настройки языка";
    russian["back_menu"] = "Вернуться в главное меню";
    russian["select_option"] = "Выбор: ";
    russian["program_empty"] = "Программа пуста!";
    russian["program_cleared"] = "Программа очищена!";
    russian["invalid_choice"] = "Неверный выбор!";
    russian["enter_filename"] = "Введите имя файла (без расширения): ";
    russian["save_success"] = "Программа успешно сохранена!";
    russian["save_failed"] = "Сохранение не удалось!";
    russian["run_results"] = "Результаты выполнения:";
    russian["run_success"] = "Программа успешно выполнена.";
    russian["pointer_error"] = "Указатель вышел за пределы!";
    russian["compile_error"] = "Ошибка компиляции!";
    russian["input_program"] = "Введите программу Brainfuck (символы, отличные от 8 действительных команд и //, /* */, обрабатываются как комментарии, введите '0'单独一行 для завершения):";
    russian["comments_supported"] = "Поддерживает однострочные (//) и многострочные (/* */) комментарии, другие символы также обрабатываются как комментарии";
    russian["no_bf_files"] = "Файлы .bf не найдены!";
    russian["found_files"] = "Найдено %d файлов .bf (включая поддиректории):";
    russian["open_file_prompt"] = "Введите номер файла для открытия (0 для возврата): ";
    russian["invalid_selection"] = "Неверный выбор!";
    russian["opening_file"] = "Открывается файл: %s";
    russian["file_content"] = "Содержимое программы (с комментариями):";
    russian["enter_to_editor"] = "Нажмите любую клавишу, чтобы войти в редактор...";
    russian["file_empty"] = "Файл пуст или чтение не удалось!";
    russian["language_settings"] = "Настройки редактора-Настройки языка";
    russian["current_language"] = "Текущий язык: ";
    russian["select_language"] = "Выберите язык:";
    russian["back_to_main"] = "Вернуться в главное меню";
    russian["language_changed"] = "Язык успешно изменен!";
    russian["editor_settings"] = "Настройки редактора";
    russian["color_settings"] = "Настройки редактора-Настройки цвета";
    russian["current_background"] = "Текущий цвет фона: ";
    russian["current_code_color"] = "Текущий цвет кода: ";
    russian["current_comment_color"] = "Текущий цвет комментариев: ";
    russian["select_background_color"] = "Выберите цвет фона:";
    russian["select_code_color"] = "Выберите цвет кода:";
    russian["select_comment_color"] = "Выберите цвет комментариев:";
    russian["color_options"] = "Предопределенные цвета\nПользовательский цвет RGB\nВернуться к настройкам цвета";
    russian["predefined_colors"] = "Предопределенные цвета:";
    russian["enter_rgb"] = "Введите значения RGB (0-255), разделенные пробелами (R G B): ";
    russian["color_changed"] = "Цвет успешно изменен!";
    russian["invalid_rgb"] = "Неверные значения RGB! Используется цвет по умолчанию.";
    russian["reset_colors"] = "Сбросить к цветам по умолчанию";
    russian["colors_reset"] = "Цвета сброшены к значениям по умолчанию!";
    russian["unnamed_file"] = "未命名文件";
    russian["current_file"] = "Текущий файл";
    russian["navigation_hint"] = "Используйте клавиши со стрелками вверх/вниз для выбора, нажмите Enter для подтверждения.";
    russian["enter_new_folder_name"] = "Введите имя новой папки: ";
    russian["folder_created"] = "Папка успешно создана!";
    russian["create_folder_failed"] = "Не удалось создать папку!";
    russian["enter_new_name"] = "Введите новое имя: ";
    russian["rename_success"] = "Переименование прошло успешно!";
    russian["rename_failed"] = "Переименование не удалось!";
    russian["press_any_key"] = "Нажмите любую клавишу, чтобы продолжить . . .";
    russian["create_new_folder"] = "Создать новую папку";
    russian["save_here"] = "Сохранить здесь";
    russian["browse_directories"] = "Обзор директорий";
    russian["go_up"] = "Перейти наверх";
    russian["current_directory"] = "Текущая директория";
    translations[RUSSIAN] = russian;
    
    // Portuguese
    std::map<std::string, std::string> portuguese;
    portuguese["bf_editor_title"] = "Editor BF";
    portuguese["main_menu_title"] = "Menu Principal";
    portuguese["main_menu"] = "IDE Brainfuck\nMenu Principal\nCriar Programa\nAbrir Programa\nConfigurações do Editor\nSair\nEscolha: ";
    portuguese["create_program"] = "Criar Programa";
    portuguese["open_program"] = "Abrir Programa";
    portuguese["exit_program"] = "Sair";
    portuguese["folder"] = "(pasta)";
    portuguese["editor_title"] = "Editor de Programa Brainfuck";
    portuguese["current_program"] = "Programa Atual (Original):";
    portuguese["filtered_code"] = "Código executável filtrado:";
    portuguese["editor_commands"] = "Interface de Programa:";
    portuguese["edit_program"] = "Inserir/Editar programa (suporta comentários)";
    portuguese["run_program"] = "Executar programa";
    portuguese["save_program"] = "Salvar programa (com comentários)";
    portuguese["clear_program"] = "Limpar programa";
    portuguese["show_filtered"] = "Mostrar código filtrado";
    portuguese["language_settings_editor"] = "Configurações de Idioma";
    portuguese["back_menu"] = "Voltar ao menu principal";
    portuguese["select_option"] = "Selecionar: ";
    portuguese["program_empty"] = "Programa está vazio!";
    portuguese["program_cleared"] = "Programa limpo!";
    portuguese["invalid_choice"] = "Escolha inválida!";
    portuguese["enter_filename"] = "Digite o nome do arquivo (sem extensão): ";
    portuguese["save_success"] = "Programa salvo com sucesso!";
    portuguese["save_failed"] = "Falha ao salvar!";
    portuguese["run_results"] = "Resultados da Execução:";
    portuguese["run_success"] = "Programa executado com sucesso.";
    portuguese["pointer_error"] = "Ponteiro fora dos limites!";
    portuguese["compile_error"] = "Erro de compilação!";
    portuguese["input_program"] = "Digite o programa Brainfuck (caracteres diferentes dos 8 comandos válidos e //, /* */ são tratados como comentários, digite '0' sozinho para terminar):";
    portuguese["comments_supported"] = "Suporta comentários de linha única (//) e multi-linha (/* */), outros caracteres também são tratados como comentários";
    portuguese["no_bf_files"] = "Nenhum arquivo .bf encontrado!";
    portuguese["found_files"] = "Encontrados %d arquivos .bf (incluindo subdiretórios):";
    portuguese["open_file_prompt"] = "Digite o número do arquivo para abrir (0 para voltar): ";
    portuguese["invalid_selection"] = "Seleção inválida!";
    portuguese["opening_file"] = "Abrindo arquivo: %s";
    portuguese["file_content"] = "Conteúdo do programa (com comentários):";
    portuguese["enter_to_editor"] = "Pressione qualquer tecla para entrar no editor...";
    portuguese["file_empty"] = "Arquivo está vazio ou a leitura falhou!";
    portuguese["language_settings"] = "Configurações do Editor-Configurações de Idioma";
    portuguese["current_language"] = "Idioma atual: ";
    portuguese["select_language"] = "Selecionar idioma:";
    portuguese["back_to_main"] = "Voltar ao menu principal";
    portuguese["language_changed"] = "Idioma alterado com sucesso!";
    portuguese["editor_settings"] = "Configurações do Editor";
    portuguese["color_settings"] = "Configurações do Editor-Configurações de Cor";
    portuguese["current_background"] = "Cor de fundo atual: ";
    portuguese["current_code_color"] = "Cor do código atual: ";
    portuguese["current_comment_color"] = "Cor do comentário atual: ";
    portuguese["select_background_color"] = "Selecionar cor de fundo:";
    portuguese["select_code_color"] = "Selecionar cor do código:";
    portuguese["select_comment_color"] = "Selecionar cor do comentário:";
    portuguese["color_options"] = "Cores predefinidas\nCor RGB personalizada\nVoltar às configurações de cor";
    portuguese["predefined_colors"] = "Cores predefinidas:";
    portuguese["enter_rgb"] = "Digite os valores RGB (0-255) separados por espaços (R G B): ";
    portuguese["color_changed"] = "Cor alterada com sucesso!";
    portuguese["invalid_rgb"] = "Valores RGB inválidos! Usando cor padrão.";
    portuguese["reset_colors"] = "Redefinir cores padrão";
    portuguese["colors_reset"] = "Cores redefinidas para os valores padrão!";
    portuguese["unnamed_file"] = "未命名文件";
    portuguese["current_file"] = "Arquivo atual";
    portuguese["navigation_hint"] = "Use as teclas de seta para cima/baixo para selecionar, pressione Enter para confirmar.";
    portuguese["enter_new_folder_name"] = "Digite o nome da nova pasta: ";
    portuguese["folder_created"] = "Pasta criada com sucesso!";
    portuguese["create_folder_failed"] = "Falha ao criar pasta!";
    portuguese["enter_new_name"] = "Digite o novo nome: ";
    portuguese["rename_success"] = "Renomeação bem-sucedida!";
    portuguese["rename_failed"] = "Falha ao renomear!";
    portuguese["press_any_key"] = "Pressione qualquer tecla para continuar . . .";
    portuguese["create_new_folder"] = "Criar nova pasta";
    portuguese["save_here"] = "Salvar aqui";
    portuguese["browse_directories"] = "Navegar diretórios";
    portuguese["go_up"] = "Subir";
    portuguese["current_directory"] = "Diretório atual";
    translations[PORTUGUESE] = portuguese;
    
    // Korean
    std::map<std::string, std::string> korean;
    korean["bf_editor_title"] = "BF 편집기";
    korean["main_menu_title"] = "메인 메뉴";
    korean["main_menu"] = "Brainfuck 통합개발환경\n메인 메뉴\n프로그램 생성\n프로그램 열기\n편집기 설정\n종료\n선택: ";
    korean["create_program"] = "프로그램 생성";
    korean["open_program"] = "프로그램 열기";
    korean["exit_program"] = "종료";
    korean["folder"] = "(폴더)";
    korean["editor_title"] = "Brainfuck 프로그램 편집기";
    korean["current_program"] = "현재 프로그램 (원본):";
    korean["filtered_code"] = "필터링된 실행 코드:";
    korean["editor_commands"] = "프로그램 인터페이스:";
    korean["edit_program"] = "프로그램 입력/편집 (주석 지원)";
    korean["run_program"] = "프로그램 실행";
    korean["save_program"] = "프로그램 저장 (주석 포함)";
    korean["clear_program"] = "프로그램 지우기";
    korean["show_filtered"] = "필터링된 코드 보기";
    korean["language_settings_editor"] = "언어 설정";
    korean["back_menu"] = "메인 메뉴로 돌아가기";
    korean["select_option"] = "선택: ";
    korean["program_empty"] = "프로그램이 비어 있습니다!";
    korean["program_cleared"] = "프로그램이 지워졌습니다!";
    korean["invalid_choice"] = "잘못된 선택입니다!";
    korean["enter_filename"] = "파일 이름을 입력하세요 (확장자 제외): ";
    korean["save_success"] = "프로그램이 성공적으로 저장되었습니다!";
    korean["save_failed"] = "저장에 실패했습니다!";
    korean["run_results"] = "실행 결과:";
    korean["run_success"] = "프로그램이 성공적으로 실행되었습니다.";
    korean["pointer_error"] = "포인터 범위를 벗어났습니다!";
    korean["compile_error"] = "컴파일 오류!";
    korean["input_program"] = "Brainfuck 프로그램을 입력하세요 (8개의 유효 명령어와 //, /* */ 외의 문자는 주석으로 처리됩니다. '0'을 단독으로 입력하여 종료하세요):";
    korean["comments_supported"] = "한 줄 주석(//)과 여러 줄 주석(/* */)을 지원합니다. 다른 문자도 주석으로 처리됩니다";
    korean["no_bf_files"] = ".bf 파일이 없습니다!";
    korean["found_files"] = "총 %d 개의 .bf 파일을 찾았습니다 (하위 디렉토리 포함):";
    korean["open_file_prompt"] = "열 파일 번호를 입력하세요 (0으로 돌아가기): ";
    korean["invalid_selection"] = "잘못된 선택입니다!";
    korean["opening_file"] = "파일 열기: %s";
    korean["file_content"] = "프로그램 내용 (주석 포함):";
    korean["enter_to_editor"] = "편집기로 들어가려면 아무 키나 누르세요...";
    korean["file_empty"] = "파일이 비어 있거나 읽기 실패!";
    korean["language_settings"] = "편집기 설정-언어 설정";
    korean["current_language"] = "현재 언어: ";
    korean["select_language"] = "언어 선택:";
    korean["back_to_main"] = "메인 메뉴로 돌아가기";
    korean["language_changed"] = "언어가 성공적으로 변경되었습니다!";
    korean["editor_settings"] = "편집기 설정";
    korean["color_settings"] = "편집기 설정-색상 설정";
    korean["current_background"] = "현재 배경색: ";
    korean["current_code_color"] = "현재 코드색: ";
    korean["current_comment_color"] = "현재 주석색: ";
    korean["select_background_color"] = "배경색 선택:";
    korean["select_code_color"] = "코드색 선택:";
    korean["select_comment_color"] = "주석색 선택:";
    korean["color_options"] = "미리 정의된 색상\n사용자 정의 RGB 색상\n색상 설정으로 돌아가기";
    korean["predefined_colors"] = "미리 정의된 색상:";
    korean["enter_rgb"] = "RGB 값을 입력하세요 (0-255) 공백으로 구분 (R G B): ";
    korean["color_changed"] = "색상이 성공적으로 변경되었습니다!";
    korean["invalid_rgb"] = "유효하지 않은 RGB 값입니다! 기본 색상을 사용합니다.";
    korean["reset_colors"] = "기본 색상으로 초기화";
    korean["colors_reset"] = "색상이 기본값으로 초기화되었습니다!";
    korean["unnamed_file"] = "未命名文件";
    korean["current_file"] = "현재 파일";
    korean["navigation_hint"] = "위/아래 방향키로 선택하고, Enter 키를 눌러 확인하세요.";
    korean["enter_new_folder_name"] = "새 폴더 이름을 입력하세요：";
    korean["folder_created"] = "폴더가 성공적으로 생성되었습니다!";
    korean["create_folder_failed"] = "폴더 생성에 실패했습니다!";
    korean["enter_new_name"] = "새 이름을 입력하세요：";
    korean["rename_success"] = "이름 변경에 성공했습니다!";
    korean["rename_failed"] = "이름 변경에 실패했습니다!";
    korean["press_any_key"] = "계속하려면 아무 키나 누르세요 . . .";
    korean["create_new_folder"] = "새 폴더 생성";
    korean["save_here"] = "여기에 저장";
    korean["browse_directories"] = "디렉토리 브라우징";
    korean["go_up"] = "위로 이동";
    korean["current_directory"] = "현재 디렉토리";
    translations[KOREAN] = korean;
    
    // Italian
    std::map<std::string, std::string> italian;
    italian["bf_editor_title"] = "Editor BF";
    italian["main_menu_title"] = "Menù Principale";
    italian["main_menu"] = "Brainfuck IDE\n主菜单\nCrea Programma\nApri Programma\nImpostazioni Editor\nEsci\nScelta: ";
    italian["create_program"] = "Crea Programma";
    italian["open_program"] = "Apri Programma";
    italian["exit_program"] = "Esci";
    italian["folder"] = "(cartella)";
    italian["editor_title"] = "Editor di Programmi Brainfuck";
    italian["current_program"] = "Programma Attuale (Originale):";
    italian["filtered_code"] = "Codice eseguibile filtrato:";
    italian["editor_commands"] = "Interfaccia di Programma:";
    italian["edit_program"] = "Inserisci/Modifica programma (supporta commenti)";
    italian["run_program"] = "Esegui programma";
    italian["save_program"] = "Salva programma (con commenti)";
    italian["clear_program"] = "Pulisci programma";
    italian["show_filtered"] = "Mostra codice filtrato";
    italian["language_settings_editor"] = "Impostazioni Lingua";
    italian["back_menu"] = "Torna al menu principale";
    italian["select_option"] = "Seleziona: ";
    italian["program_empty"] = "Il programma è vuoto!";
    italian["program_cleared"] = "Programma pulito!";
    italian["invalid_choice"] = "Scelta non valida!";
    italian["enter_filename"] = "Inserisci nome file (senza estensione): ";
    italian["save_success"] = "Programma salvato con successo!";
    italian["save_failed"] = "Salvataggio fallito!";
    italian["run_results"] = "Risultati dell'Esecuzione:";
    italian["run_success"] = "Programma eseguito con successo.";
    italian["pointer_error"] = "Puntatore fuori dai limiti!";
    italian["compile_error"] = "Errore di compilazione!";
    italian["input_program"] = "Inserisci programma Brainfuck (caratteri diversi dai 8 comandi validi e //, /* */ sono trattati come commenti, inserisci '0' da solo per terminare):";
    italian["comments_supported"] = "Supporta commenti su singola riga (//) e multilinea (/* */), anche gli altri caratteri sono trattati come commenti";
    italian["no_bf_files"] = "Nessun file .bf trovato!";
    italian["found_files"] = "Trovati %d file .bf (inclusi sottodirectory):";
    italian["open_file_prompt"] = "Inserisci numero file da aprire (0 per tornare): ";
    italian["invalid_selection"] = "Selezione non valida!";
    italian["opening_file"] = "Apertura file: %s";
    italian["file_content"] = "Contenuto del programma (con commenti):";
    italian["enter_to_editor"] = "Premi un tasto per entrare nell'editor...";
    italian["file_empty"] = "Il file è vuoto o la lettura è fallita!";
    italian["language_settings"] = "Impostazioni Editor- Impostazioni Lingua";
    italian["current_language"] = "Lingua attuale: ";
    italian["select_language"] = "Seleziona lingua:";
    italian["back_to_main"] = "Torna al menu principale";
    italian["language_changed"] = "Lingua cambiata con successo!";
    italian["editor_settings"] = "Impostazioni Editor";
    italian["color_settings"] = "Impostazioni Editor- Impostazioni Colore";
    italian["current_background"] = "Colore di sfondo attuale: ";
    italian["current_code_color"] = "Colore codice attuale: ";
    italian["current_comment_color"] = "Colore commento attuale: ";
    italian["select_background_color"] = "Seleziona colore di sfondo:";
    italian["select_code_color"] = "Seleziona colore codice:";
    italian["select_comment_color"] = "Seleziona colore commento:";
    italian["color_options"] = "Colori predefiniti\nColore RGB personalizzato\nTorna alle impostazioni colore";
    italian["predefined_colors"] = "Colori predefiniti:";
    italian["enter_rgb"] = "Inserisci valori RGB (0-255) separati da spazi (R G B): ";
    italian["color_changed"] = "Colore cambiato con successo!";
    italian["invalid_rgb"] = "Valori RGB non validi! Utilizzo colore predefinito.";
    italian["reset_colors"] = "Ripristina colori predefiniti";
    italian["colors_reset"] = "Colori ripristinati ai valori predefiniti!";
    italian["unnamed_file"] = "未命名文件";
    italian["current_file"] = "File attuale";
    italian["navigation_hint"] = "Usa i tasti freccia su/giu per selezionare, premi Invio per confermare.";
    italian["enter_new_folder_name"] = "Inserisci il nome della nuova cartella: ";
    italian["folder_created"] = "Cartella creata con successo!";
    italian["create_folder_failed"] = "Errore nella creazione della cartella!";
    italian["enter_new_name"] = "Inserisci il nuovo nome: ";
    italian["rename_success"] = "Rinominazione avvenuta con successo!";
    italian["rename_failed"] = "Errore nella rinominazione!";
    italian["press_any_key"] = "Premi un tasto qualsiasi per continuare . . .";
    italian["create_new_folder"] = "Crea nuova cartella";
    italian["save_here"] = "Salva qui";
    italian["browse_directories"] = "Sfoglia directory";
    italian["go_up"] = "Vai su";
    italian["current_directory"] = "Directory corrente";
    translations[ITALIAN] = italian;
    
    // Dutch
    std::map<std::string, std::string> dutch;
    dutch["bf_editor_title"] = "BF-Editor";
    dutch["main_menu_title"] = "Hoofdmenu";
    dutch["main_menu"] = "Brainfuck IDE\n主菜单\nMaak Programma\nOpen Programma\nEditor Instellingen\nAfsluiten\nKies: ";
    dutch["create_program"] = "Maak Programma";
    dutch["open_program"] = "Open Programma";
    dutch["exit_program"] = "Afsluiten";
    dutch["folder"] = "(map)";
    dutch["editor_title"] = "Brainfuck Programma Editor";
    dutch["current_program"] = "Huidig Programma (Origineel):";
    dutch["filtered_code"] = "Gefilterde uitvoerbare code:";
    dutch["editor_commands"] = "Programma Interface:";
    dutch["edit_program"] = "Voer/Bewerk programma in (ondersteunt commentaren)";
    dutch["run_program"] = "Voer programma uit";
    dutch["save_program"] = "Sla programma op (met commentaren)";
    dutch["clear_program"] = "Wis programma";
    dutch["show_filtered"] = "Toon gefilterde code";
    dutch["language_settings_editor"] = "Taal Instellingen";
    dutch["back_menu"] = "Terug naar hoofdmenu";
    dutch["select_option"] = "Selecteer: ";
    dutch["program_empty"] = "Programma is leeg!";
    dutch["program_cleared"] = "Programma gewist!";
    dutch["invalid_choice"] = "Ongeldige keuze!";
    dutch["enter_filename"] = "Voer bestandsnaam in (zonder extensie): ";
    dutch["save_success"] = "Programma succesvol opgeslagen!";
    dutch["save_failed"] = "Opslaan mislukt!";
    dutch["run_results"] = "Uitvoerresultaten:";
    dutch["run_success"] = "Programma succesvol uitgevoerd.";
    dutch["pointer_error"] = "Pointer buiten grenzen!";
    dutch["compile_error"] = "Compilatiefout!";
    dutch["input_program"] = "Voer Brainfuck-programma in (karakters anders dan de 8 geldige commando's en //, /* */ worden behandeld als commentaren, voer '0' alleen in om te eindigen):";
    dutch["comments_supported"] = "Ondersteunt enkele regel (//) en meerdere regels (/* */) commentaren, andere karakters worden ook behandeld als commentaren";
    dutch["no_bf_files"] = "Geen .bf-bestanden gevonden!";
    dutch["found_files"] = "Gevonden %d .bf-bestanden (inclusief subdirectories):";
    dutch["open_file_prompt"] = "Voer bestandsnummer in om te openen (0 om terug te gaan): ";
    dutch["invalid_selection"] = "Ongeldige selectie!";
    dutch["opening_file"] = "Bestand openen: %s";
    dutch["file_content"] = "Programmainhoud (met commentaren):";
    dutch["enter_to_editor"] = "Druk op een toets om de editor te openen...";
    dutch["file_empty"] = "Bestand is leeg of lezen is mislukt!";
    dutch["language_settings"] = "Editor Instellingen- Taal Instellingen";
    dutch["current_language"] = "Huidige taal: ";
    dutch["select_language"] = "Selecteer taal:";
    dutch["back_to_main"] = "Terug naar hoofdmenu";
    dutch["language_changed"] = "Taal succesvol gewijzigd!";
    dutch["editor_settings"] = "Editor Instellingen";
    dutch["color_settings"] = "Editor Instellingen- Kleur Instellingen";
    dutch["current_background"] = "Huidige achtergrondkleur: ";
    dutch["current_code_color"] = "Huidige code kleur: ";
    dutch["current_comment_color"] = "Huidige commentaar kleur: ";
    dutch["select_background_color"] = "Selecteer achtergrondkleur:";
    dutch["select_code_color"] = "Selecteer code kleur:";
    dutch["select_comment_color"] = "Selecteer commentaar kleur:";
    dutch["color_options"] = "Vooraf gedefinieerde kleuren\nAangepaste RGB kleur\nTerug naar kleur instellingen";
    dutch["predefined_colors"] = "Vooraf gedefinieerde kleuren:";
    dutch["enter_rgb"] = "Voer RGB-waarden in (0-255) gescheiden door spaties (R G B): ";
    dutch["color_changed"] = "Kleur succesvol gewijzigd!";
    dutch["invalid_rgb"] = "Ongeldige RGB-waarden! Standaard kleur gebruiken.";
    dutch["reset_colors"] = "Terugzetten naar standaard kleuren";
    dutch["colors_reset"] = "Kleuren teruggezet naar standaardwaarden!";
    dutch["unnamed_file"] = "未命名文件";
    dutch["current_file"] = "Huidig bestand";
    dutch["navigation_hint"] = "Gebruik de pijltjestoetsen omhoog/omlaag om te selecteren, druk op Enter om te bevestigen.";
    dutch["enter_new_folder_name"] = "Voer de naam van de nieuwe map in: ";
    dutch["folder_created"] = "Map succesvol aangemaakt!";
    dutch["create_folder_failed"] = "Maken van map mislukt!";
    dutch["enter_new_name"] = "Voer de nieuwe naam in: ";
    dutch["rename_success"] = "Hernoemen succesvol!";
    dutch["rename_failed"] = "Hernoemen mislukt!";
    dutch["press_any_key"] = "Druk op een toets om verder te gaan . . .";
    dutch["create_new_folder"] = "Maak nieuwe map";
    dutch["save_here"] = "Sla hier op";
    dutch["browse_directories"] = "Blader door mappen";
    dutch["go_up"] = "Ga omhoog";
    dutch["current_directory"] = "Huidige map";
    translations[DUTCH] = dutch;

    // Turkish
    std::map<std::string, std::string> turkish;
    turkish["bf_editor_title"] = "BF Editörü";
    turkish["main_menu_title"] = "Ana Menü";
    turkish["main_menu"] = "Brainfuck IDE\n主菜单\nProgram Oluştur\nProgram Aç\nEditör Ayarları\nÇıkış\nSeçim: ";
    turkish["create_program"] = "Program Oluştur";
    turkish["open_program"] = "Program Aç";
    turkish["exit_program"] = "Çıkış";
    turkish["folder"] = "(klasör)";
    turkish["editor_title"] = "Brainfuck Program Editörü";
    turkish["current_program"] = "Mevcut Program (Orijinal):";
    turkish["filtered_code"] = "Filtrelenmiş çalıştırılabilir kod:";
    turkish["editor_commands"] = "Program Arayüzü:";
    turkish["edit_program"] = "Program gir/ Düzenle (yorumları destekler)";
    turkish["run_program"] = "Programı çalıştır";
    turkish["save_program"] = "Programı kaydet (yorumlarla birlikte)";
    turkish["clear_program"] = "Programı temizle";
    turkish["show_filtered"] = "Filtrelenmiş kodu göster";
    turkish["language_settings_editor"] = "Dil Ayarları";
    turkish["back_menu"] = "Ana menüye dön";
    turkish["select_option"] = "Seçim: ";
    turkish["program_empty"] = "Program boş!";
    turkish["program_cleared"] = "Program temizlendi!";
    turkish["invalid_choice"] = "Geçersiz seçim!";
    turkish["enter_filename"] = "Dosya adını girin (uzantı olmadan): ";
    turkish["save_success"] = "Program başarıyla kaydedildi!";
    turkish["save_failed"] = "Kaydetme başarısız!";
    turkish["run_results"] = "Çalıştırma Sonuçları:";
    turkish["run_success"] = "Program başarıyla çalıştırıldı.";
    turkish["pointer_error"] = "İmleç sınırların dışında!";
    turkish["compile_error"] = "Derleme hatası!";
    turkish["input_program"] = "Brainfuck programını girin (8 geçerli komut ve //, /* */ dışındaki karakterler yorum olarak kabul edilir, sonlandırmak için '0' tek başına girin):";
    turkish["comments_supported"] = "Tek satır (//) ve çok satırlı (/* */) yorumları destekler, diğer karakterler de yorum olarak kabul edilir";
    turkish["no_bf_files"] = ".bf dosyaları bulunamadı!";
    turkish["found_files"] = "%d adet .bf dosyası bulundu (alt dizinler dahil):";
    turkish["open_file_prompt"] = "Açmak için dosya numarasını girin (geri dönmek için 0): ";
    turkish["invalid_selection"] = "Geçersiz seçim!";
    turkish["opening_file"] = "Dosya açılıyor: %s";
    turkish["file_content"] = "Program içeriği (yorumlarla birlikte):";
    turkish["enter_to_editor"] = "Editöre girmek için herhangi bir tuşa basın...";
    turkish["file_empty"] = "Dosya boş veya okuma başarısız!";
    turkish["language_settings"] = "Editör Ayarları- Dil Ayarları";
    turkish["current_language"] = "Mevcut dil: ";
    turkish["select_language"] = "Dil seçin:";
    turkish["back_to_main"] = "Ana menüye dön";
    turkish["language_changed"] = "Dil başarıyla değiştirildi!";
    turkish["editor_settings"] = "Editör Ayarları";
    turkish["color_settings"] = "Editör Ayarları- Renk Ayarları";
    turkish["current_background"] = "Mevcut arka plan rengi: ";
    turkish["current_code_color"] = "Mevcut kod rengi: ";
    turkish["current_comment_color"] = "Mevcut yorum rengi: ";
    turkish["select_background_color"] = "Arka plan rengini seçin:";
    turkish["select_code_color"] = "Kod rengini seçin:";
    turkish["select_comment_color"] = "Yorum rengini seçin:";
    turkish["color_options"] = "Önceden tanımlanmış renkler\nÖzel RGB rengi\nRenk ayarlarına dön";
    turkish["predefined_colors"] = "Önceden tanımlanmış renkler:";
    turkish["enter_rgb"] = "RGB değerlerini girin (0-255) boşlukla ayırın (R G B): ";
    turkish["color_changed"] = "Renk başarıyla değiştirildi!";
    turkish["invalid_rgb"] = "Geçersiz RGB değerleri! Varsayılan rengi kullanılıyor.";
    turkish["reset_colors"] = "Varsayılan renklere sıfırla";
    turkish["colors_reset"] = "Renkler varsayılan değerlere sıfırlandı!";
    turkish["unnamed_file"] = "未命名文件";
    turkish["current_file"] = "Mevcut dosya";
    turkish["navigation_hint"] = "Seçmek için yukarı/aşağı ok tuşlarını kullanın, onaylamak için Enter tuşuna basın.";
    turkish["enter_new_folder_name"] = "Yeni klasör adını girin: ";
    turkish["folder_created"] = "Klasör başarıyla oluşturuldu!";
    turkish["create_folder_failed"] = "Klasör oluşturma başarısız!";
    turkish["enter_new_name"] = "Yeni adı girin: ";
    turkish["rename_success"] = "Yeniden adlandırma başarılı!";
    turkish["rename_failed"] = "Yeniden adlandırma başarısız!";
    turkish["press_any_key"] = "Devam etmek için herhangi bir tuşa basın . . .";
    turkish["create_new_folder"] = "Yeni klasör oluştur";
    turkish["save_here"] = "Buraya kaydet";
    turkish["browse_directories"] = "Klasörleri gezin";
    turkish["go_up"] = "Yukarı git";
    turkish["current_directory"] = "Mevcut klasör";
    translations[TURKISH] = turkish;
    
    // Polish
    std::map<std::string, std::string> polish;
    polish["bf_editor_title"] = "Edytor BF";
    polish["main_menu_title"] = "Menu Główne";
    polish["main_menu"] = "Brainfuck IDE\n主菜单\nUtwórz Program\nOtwórz Program\nUstawienia Edytora\nWyjdź\nWybór: ";
    polish["create_program"] = "Utwórz Program";
    polish["open_program"] = "Otwórz Program";
    polish["exit_program"] = "Wyjdź";
    polish["folder"] = "(folder)";
    polish["editor_title"] = "Edytor Programów Brainfuck";
    polish["current_program"] = "Aktualny Program (Oryginał):";
    polish["filtered_code"] = "Filtrowany kod wykonywalny:";
    polish["editor_commands"] = "Interfejs Programu:";
    polish["edit_program"] = "Wprowadź/Edytuj program (obsługuje komentarze)";
    polish["run_program"] = "Uruchom program";
    polish["save_program"] = "Zapisz program (z komentarzami)";
    polish["clear_program"] = "Wyczyść program";
    polish["show_filtered"] = "Pokaż filtrowany kod";
    polish["language_settings_editor"] = "Ustawienia Języka";
    polish["back_menu"] = "Wróć do menu głównego";
    polish["select_option"] = "Wybierz: ";
    polish["program_empty"] = "Program jest pusty!";
    polish["program_cleared"] = "Program wyczyszczony!";
    polish["invalid_choice"] = "Nieprawidłowy wybór!";
    polish["enter_filename"] = "Podaj nazwę pliku (bez rozszerzenia): ";
    polish["save_success"] = "Program został pomyślnie zapisany!";
    polish["save_failed"] = "Zapisanie nie powiodło się!";
    polish["run_results"] = "Wyniki Wykonania:";
    polish["run_success"] = "Program został pomyślnie wykonany.";
    polish["pointer_error"] = "Wskaźnik poza zakresem!";
    polish["compile_error"] = "Błąd kompilacji!";
    polish["input_program"] = "Wprowadź program Brainfuck (znaki inne niż 8 ważnych poleceń i //, /* */ są traktowane jako komentarze, wprowadź '0' pojedynczo aby zakończyć):";
    polish["comments_supported"] = "Obsługuje komentarze jednolinijkowe (//) i wielolinijkowe (/* */), inne znaki są również traktowane jako komentarze";
    polish["no_bf_files"] = "Nie znaleziono żadnych plików .bf!";
    polish["found_files"] = "Znaleziono %d plików .bf (włącznie z podkatalogami):";
    polish["open_file_prompt"] = "Wprowadź numer pliku do otwarcia (0 aby wrócić): ";
    polish["invalid_selection"] = "Nieprawidłowy wybór!";
    polish["opening_file"] = "Otwieranie pliku: %s";
    polish["file_content"] = "Zawartość programu (z komentarzami):";
    polish["enter_to_editor"] = "Naciśnij dowolny klawisz, aby wejść do edytora...";
    polish["file_empty"] = "Plik jest pusty lub odczytanie się nie powiodło!";
    polish["language_settings"] = "Ustawienia Edytora- Ustawienia Języka";
    polish["current_language"] = "Aktualny język: ";
    polish["select_language"] = "Wybierz język:";
    polish["back_to_main"] = "Wróć do menu głównego";
    polish["language_changed"] = "Język został pomyślnie zmieniony!";
    polish["editor_settings"] = "Ustawienia Edytora";
    polish["color_settings"] = "Ustawienia Edytora- Ustawienia Kolorów";
    polish["current_background"] = "Aktualny kolor tła: ";
    polish["current_code_color"] = "Aktualny kolor kodu: ";
    polish["current_comment_color"] = "Aktualny kolor komentarza: ";
    polish["select_background_color"] = "Wybierz kolor tła:";
    polish["select_code_color"] = "Wybierz kolor kodu:";
    polish["select_comment_color"] = "Wybierz kolor komentarza:";
    polish["color_options"] = "Predefiniowane kolory\nWłasny kolor RGB\nWróć do ustawień kolorów";
    polish["predefined_colors"] = "Predefiniowane kolory:";
    polish["enter_rgb"] = "Wprowadź wartości RGB (0-255) oddzielone spacją (R G B): ";
    polish["color_changed"] = "Kolor został pomyślnie zmieniony!";
    polish["invalid_rgb"] = "Nieprawidłowe wartości RGB! Używam domyślnego koloru.";
    polish["reset_colors"] = "Resetuj do domyślnych kolorów";
    polish["colors_reset"] = "Kolory zostały zresetowane do wartości domyślnych!";
    polish["unnamed_file"] = "未命名文件";
    polish["current_file"] = "Aktualny plik";
    polish["navigation_hint"] = "Użyj klawiszy strzałek góra/dół do wyboru, naciśnij Enter aby potwierdzić.";
    polish["enter_new_folder_name"] = "Wprowadź nazwę nowego folderu: ";
    polish["folder_created"] = "Folder został pomyślnie utworzony!";
    polish["create_folder_failed"] = "Utworzenie folderu nie powiodło się!";
    polish["enter_new_name"] = "Wprowadź nową nazwę: ";
    polish["rename_success"] = "Zmiana nazwy powiodła się!";
    polish["rename_failed"] = "Zmiana nazwy nie powiodła się!";
    polish["press_any_key"] = "Naciśnij dowolny klawisz, aby kontynuować . . .";
    polish["create_new_folder"] = "Utwórz nowy folder";
    polish["save_here"] = "Zapisz tutaj";
    polish["browse_directories"] = "Przeglądaj katalogi";
    polish["go_up"] = "Idź w górę";
    polish["current_directory"] = "Aktualny katalog";
    translations[POLISH] = polish;
    
    // Swedish
    std::map<std::string, std::string> swedish;
    swedish["bf_editor_title"] = "BF-redaktör";
    swedish["main_menu_title"] = "Huvudmeny";
    swedish["main_menu"] = "Brainfuck IDE\n主菜单\nSkapa Program\nÖppna Program\nRedaktörsinställningar\nAvsluta\nVal: ";
    swedish["create_program"] = "Skapa Program";
    swedish["open_program"] = "Öppna Program";
    swedish["exit_program"] = "Avsluta";
    swedish["folder"] = "(mapp)";
    swedish["editor_title"] = "Brainfuck Programredaktör";
    swedish["current_program"] = "Aktuellt Program (Original):";
    swedish["filtered_code"] = "Filtrerad exekverbar kod:";
    swedish["editor_commands"] = "Programgränssnitt:";
    swedish["edit_program"] = "Mata in/Redigera program (stödjer kommentarer)";
    swedish["run_program"] = "Kör program";
    swedish["save_program"] = "Spara program (med kommentarer)";
    swedish["clear_program"] = "Rensa program";
    swedish["show_filtered"] = "Visa filtrerad kod";
    swedish["language_settings_editor"] = "Språkinställningar";
    swedish["back_menu"] = "Återgå till huvudmenyn";
    swedish["select_option"] = "Välj: ";
    swedish["program_empty"] = "Programmet är tomt!";
    swedish["program_cleared"] = "Programmet rensat!";
    swedish["invalid_choice"] = "Ogiltigt val!";
    swedish["enter_filename"] = "Ange filnamn (utan tillägg): ";
    swedish["save_success"] = "Programmet sparades framgångsrikt!";
    swedish["save_failed"] = "Spara misslyckades!";
    swedish["run_results"] = "Körningsresultat:";
    swedish["run_success"] = "Programmet exekverades framgångsrikt.";
    swedish["pointer_error"] = "Pekare utanför gränser!";
    swedish["compile_error"] = "Kompileringsfel!";
    swedish["input_program"] = "Ange Brainfuck-program (tecken andra än de 8 giltiga kommandona och //, /* */ behandlas som kommentarer, ange '0' ensamt för att avsluta):";
    swedish["comments_supported"] = "Stödjer enkelradiga (//) och flerradiga (/* */) kommentarer, andra tecken behandlas också som kommentarer";
    swedish["no_bf_files"] = "Inga .bf-filer hittades!";
    swedish["found_files"] = "Hittade %d .bf-filer (inkluderande undermappar):";
    swedish["open_file_prompt"] = "Ange filnummer att öppna (0 för att återgå): ";
    swedish["invalid_selection"] = "Ogiltigt val!";
    swedish["opening_file"] = "Öppnar fil: %s";
    swedish["file_content"] = "Programinnehåll (med kommentarer):";
    swedish["enter_to_editor"] = "Tryck på valfri tangent för att komma in i redaktören...";
    swedish["file_empty"] = "Filen är tom eller läsning misslyckades!";
    swedish["language_settings"] = "Redaktörsinställningar- Språkinställningar";
    swedish["current_language"] = "Aktuellt språk: ";
    swedish["select_language"] = "Välj språk:";
    swedish["back_to_main"] = "Återgå till huvudmenyn";
    swedish["language_changed"] = "Språk ändrades framgångsrikt!";
    swedish["editor_settings"] = "Redaktörsinställningar";
    swedish["color_settings"] = "Redaktörsinställningar- Färginställningar";
    swedish["current_background"] = "Aktuell bakgrundsfärg: ";
    swedish["current_code_color"] = "Aktuell kodfärg: ";
    swedish["current_comment_color"] = "Aktuell kommentarfärg: ";
    swedish["select_background_color"] = "Välj bakgrundsfärg:";
    swedish["select_code_color"] = "Välj kodfärg:";
    swedish["select_comment_color"] = "Välj kommentarfärg:";
    swedish["color_options"] = "Fördefinierade färger\nAnpassad RGB-färg\nÅtergå till färginställningar";
    swedish["predefined_colors"] = "Fördefinierade färger:";
    swedish["enter_rgb"] = "Ange RGB-värden (0-255) separerade med blanksteg (R G B): ";
    swedish["color_changed"] = "Färg ändrades framgångsrikt!";
    swedish["invalid_rgb"] = "Ogiltiga RGB-värden! Använder standardfärg.";
    swedish["reset_colors"] = "Återställ till standardfärger";
    swedish["colors_reset"] = "Färger återställda till standardvärden!";
    swedish["unnamed_file"] = "未命名文件";
    swedish["current_file"] = "Aktuell fil";
    swedish["navigation_hint"] = "Använd piltangenterna upp/ned för att välja, tryck Enter för att bekräfta.";
    swedish["enter_new_folder_name"] = "Ange namn på ny mapp: ";
    swedish["folder_created"] = "Mapp skapades framgångsrikt!";
    swedish["create_folder_failed"] = "Misslyckades att skapa mapp!";
    swedish["enter_new_name"] = "Ange nytt namn: ";
    swedish["rename_success"] = "Byt namn lyckades!";
    swedish["rename_failed"] = "Misslyckades att byta namn!";
    swedish["press_any_key"] = "Tryck på valfri tangent för att fortsätta . . .";
    swedish["create_new_folder"] = "Skapa ny mapp";
    swedish["save_here"] = "Spara här";
    swedish["browse_directories"] = "Bläddra bland kataloger";
    swedish["go_up"] = "Gå upp";
    swedish["current_directory"] = "Aktuell katalog";
    translations[SWEDISH] = swedish;
    
    // Ukrainian
    std::map<std::string, std::string> ukrainian;
    ukrainian["bf_editor_title"] = "BF Редактор";
    ukrainian["main_menu_title"] = "Головне меню";
    ukrainian["main_menu"] = "Brainfuck IDE\n主菜单\nСтворити програму\nВідкрити програму\nНалаштування редактора\nВихід\nВибір: ";
    ukrainian["create_program"] = "Створити програму";
    ukrainian["open_program"] = "Відкрити програму";
    ukrainian["exit_program"] = "Вихід";
    ukrainian["folder"] = "(папка)";
    ukrainian["editor_title"] = "Редактор програм Brainfuck";
    ukrainian["current_program"] = "Поточна програма (Оригінал):";
    ukrainian["filtered_code"] = "Відфільтрований виконуваний код:";
    ukrainian["editor_commands"] = "Інтерфейс програми:";
    ukrainian["edit_program"] = "Ввести/Редагувати програму (підтримує коментарі)";
    ukrainian["run_program"] = "Запустити програму";
    ukrainian["save_program"] = "Зберегти програму (з коментарями)";
    ukrainian["clear_program"] = "Очистити програму";
    ukrainian["show_filtered"] = "Показати відфільтрований код";
    ukrainian["language_settings_editor"] = "Налаштування мови";
    ukrainian["back_menu"] = "Повернутися до головного меню";
    ukrainian["select_option"] = "Вибір: ";
    ukrainian["program_empty"] = "Програма порожня!";
    ukrainian["program_cleared"] = "Програма очищена!";
    ukrainian["invalid_choice"] = "Невірний вибір!";
    ukrainian["enter_filename"] = "Введіть назву файлу (без розширення): ";
    ukrainian["save_success"] = "Програма успішно збережена!";
    ukrainian["save_failed"] = "Збереження не вдалося!";
    ukrainian["run_results"] = "Результати виконання:";
    ukrainian["run_success"] = "Програма успішно виконана.";
    ukrainian["pointer_error"] = "Вказівник за межами!";
    ukrainian["compile_error"] = "Помилка компіляції!";
    ukrainian["input_program"] = "Введіть програму Brainfuck (символи, відмінні від 8 дійсних команд та //, /* */, обробляються як коментарі, введіть '0' окремо для завершення):";
    ukrainian["comments_supported"] = "Підтримує одинарні рядки (//) та багаторядкові (/* */) коментарі, інші символи також обробляються як коментарі";
    ukrainian["no_bf_files"] = "Файли .bf не знайдено!";
    ukrainian["found_files"] = "Знайдено %d файлів .bf (включаючи піддиректорії):";
    ukrainian["open_file_prompt"] = "Введіть номер файлу для відкриття (0 для повернення): ";
    ukrainian["invalid_selection"] = "Невірний вибір!";
    ukrainian["opening_file"] = "Відкривається файл: %s";
    ukrainian["file_content"] = "Вміст програми (з коментарями):";
    ukrainian["enter_to_editor"] = "Натисніть будь-яку клавішу, щоб увійти в редактор...";
    ukrainian["file_empty"] = "Файл порожній або читання не вдалося!";
    ukrainian["language_settings"] = "Налаштування редактора- Налаштування мови";
    ukrainian["current_language"] = "Поточна мова: ";
    ukrainian["select_language"] = "Виберіть мову:";
    ukrainian["back_to_main"] = "Повернутися до головного меню";
    ukrainian["language_changed"] = "Мова успішно змінена!";
    ukrainian["editor_settings"] = "Налаштування редактора";
    ukrainian["color_settings"] = "Налаштування редактора- Налаштування кольору";
    ukrainian["current_background"] = "Поточний колір фону: ";
    ukrainian["current_code_color"] = "Поточний колір коду: ";
    ukrainian["current_comment_color"] = "Поточний колір коментарів: ";
    ukrainian["select_background_color"] = "Виберіть колір фону:";
    ukrainian["select_code_color"] = "Виберіть колір коду:";
    ukrainian["select_comment_color"] = "Виберіть колір коментарів:";
    ukrainian["color_options"] = "Предопределені кольори\nВласний колір RGB\nПовернутися до налаштувань кольору";
    ukrainian["predefined_colors"] = "Предопределені кольори:";
    ukrainian["enter_rgb"] = "Введіть значення RGB (0-255), розділені пробілами (R G B): ";
    ukrainian["color_changed"] = "Колір успішно змінено!";
    ukrainian["invalid_rgb"] = "Невірні значення RGB! Використовується колір за замовчуванням.";
    ukrainian["reset_colors"] = "Скинути до кольорів за замовчуванням";
    ukrainian["colors_reset"] = "Кольори скинуті до значень за замовчуванням!";
    ukrainian["unnamed_file"] = "未命名文件";
    ukrainian["current_file"] = "Поточний файл";
    ukrainian["navigation_hint"] = "Використовуйте клавіші зі стрілками вгору/вниз для вибору, натисніть Enter для підтвердження.";
    ukrainian["enter_new_folder_name"] = "Введіть назву нової папки: ";
    ukrainian["folder_created"] = "Папка успішно створена!";
    ukrainian["create_folder_failed"] = "Не вдалося створити папку!";
    ukrainian["enter_new_name"] = "Введіть нову назву: ";
    ukrainian["rename_success"] = "Перейменування пройшло успішно!";
    ukrainian["rename_failed"] = "Перейменування не вдалося!";
    ukrainian["press_any_key"] = "Натисніть будь-яку клавішу, щоб продовжити . . .";
    ukrainian["create_new_folder"] = "Створити нову папку";
    ukrainian["save_here"] = "Зберегти тут";
    ukrainian["browse_directories"] = "Переглядати директорії";
    ukrainian["go_up"] = "Піднятися на рівень вище";
    ukrainian["current_directory"] = "Поточна директорія";
    translations[UKRAINIAN] = ukrainian;
}

// 语言名称映射
std::map<Language, std::string> languageNames;

// 初始化语言名称
void initializeLanguageNames() {
    languageNames[ENGLISH] = "English";
    languageNames[CHINESE] = "中文 (Chinese)";
    languageNames[SPANISH] = "Español (Spanish)";
    languageNames[FRENCH] = "Français (French)";
    languageNames[GERMAN] = "Deutsch (German)";
    languageNames[JAPANESE] = "日本語 (Japanese)";
    languageNames[RUSSIAN] = "Русский (Russian)";
    languageNames[PORTUGUESE] = "Português (Portuguese)";
    languageNames[KOREAN] = "한국어 (Korean)";
    languageNames[ITALIAN] = "Italiano (Italian)";
    languageNames[DUTCH] = "Nederlands (Dutch)";
    languageNames[TURKISH] = "Türkçe (Turkish)";
    languageNames[POLISH] = "Polski (Polish)";
    languageNames[SWEDISH] = "Svenska (Swedish)";
    languageNames[UKRAINIAN] = "Українська (Ukrainian)";
}

// 颜色名称映射
std::map<Color, std::string> colorNames;

// 初始化颜色名称
void initializeColorNames() {
    colorNames[BLACK] = "Black";
    colorNames[RED] = "Red";
    colorNames[GREEN] = "Green";
    colorNames[YELLOW] = "Yellow";
    colorNames[BLUE] = "Blue";
    colorNames[MAGENTA] = "Magenta";
    colorNames[CYAN] = "Cyan";
    colorNames[WHITE] = "White";
    colorNames[ORANGE] = "Orange";
    colorNames[BROWN] = "Brown";
}

// 颜色到终端颜色的映射
#ifdef _WIN32
std::map<Color, int> colorToWinConsole;
#else
std::map<Color, std::string> colorToLinuxConsole;
#endif

// 初始化颜色映射
void initializeColorMaps() {
#ifdef _WIN32
    colorToWinConsole[BLACK] = 0;
    colorToWinConsole[RED] = 4;
    colorToWinConsole[GREEN] = 2;
    colorToWinConsole[YELLOW] = 6;
    colorToWinConsole[BLUE] = 1;
    colorToWinConsole[MAGENTA] = 5;
    colorToWinConsole[CYAN] = 3;
    colorToWinConsole[WHITE] = 7;
    colorToWinConsole[ORANGE] = 6;
    colorToWinConsole[BROWN] = 6;
#else
    colorToLinuxConsole[BLACK] = "30";
    colorToLinuxConsole[RED] = "31";
    colorToLinuxConsole[GREEN] = "32";
    colorToLinuxConsole[YELLOW] = "33";
    colorToLinuxConsole[BLUE] = "34";
    colorToLinuxConsole[MAGENTA] = "35";
    colorToLinuxConsole[CYAN] = "36";
    colorToLinuxConsole[WHITE] = "37";
    colorToLinuxConsole[ORANGE] = "33";
    colorToLinuxConsole[BROWN] = "33";
#endif
}

// 等待用户按键
void waitForKey() {
#ifdef _WIN32
    _getch();
#else
    getchar();
#endif
}

// 移动光标到指定位置
void moveCursor(int x, int y) {
#ifdef _WIN32
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
#else
    printf("\033[%d;%dH", y + 1, x + 1);
#endif
}

// 函数声明
std::string tr(const std::string& key);
void languageSettings();
void colorSettings();
void saveSettings();
void loadSettings();
void applyColors();
void applyCommentColor();
void resetColors();
std::string getExeDir();
bool createDirectory(const std::string& path);
void clearScreen();
void pauseScreen();

// 交互式控制台菜单类
/*
 * WinConsoleMenu类 - 控制台菜单系统
 * 提供了控制台环境下的交互式菜单界面，支持多语言显示、自定义颜色和位置
 * 特性：
 * - 支持程序名、文件路径和文件内容的顶部显示
 * - 支持菜单项的高亮显示和上下导航
 * - 支持自定义菜单标题和位置
 * - 适配Windows和Linux平台的控制台颜色和输入处理
 */
class WinConsoleMenu {
private:
    std::vector<std::string> options;     // 菜单项列表
    int currentSelection;                 // 当前选中的菜单项索引
    std::string title;                    // 菜单标题
    int startX;                           // 菜单起始X坐标
    int startY;                           // 菜单起始Y坐标
    std::string currentFileName;          // 当前文件名（显示在顶部）
    std::string currentFilePath;          // 当前文件路径（显示在顶部）
    std::string programContent;           // 程序内容（显示在顶部）

public:
    /*
     * 构造函数 - 初始化菜单
     * 参数：
     * - menuTitle: 菜单标题
     * - menuOptions: 菜单项列表
     * - fileName: 当前文件名（可选）
     * - filePath: 当前文件路径（可选）
     * - content: 程序内容（可选）
     * - x: 菜单起始X坐标（可选，默认0）
     * - y: 菜单起始Y坐标（可选，默认0）
     */
    WinConsoleMenu(const std::string& menuTitle, const std::vector<std::string>& menuOptions, const std::string& fileName = "", const std::string& filePath = "", const std::string& content = "", int x = 0, int y = 0)
        : title(menuTitle), options(menuOptions), currentFileName(fileName), currentFilePath(filePath), programContent(content), currentSelection(0), startX(x), startY(y) {
    }
    
    /*
     * 设置当前文件名
     * 参数：
     * - fileName: 要设置的文件名
     */
    void setCurrentFileName(const std::string& fileName) {
        currentFileName = fileName;
    }
    
    /*
     * 设置当前文件路径
     * 参数：
     * - filePath: 要设置的文件路径
     */
    void setCurrentFilePath(const std::string& filePath) {
        currentFilePath = filePath;
    }
    
    /*
     * 设置程序内容
     * 参数：
     * - content: 要设置的程序内容
     */
    void setProgramContent(const std::string& content) {
        programContent = content;
    }

    /*
     * 设置菜单位置
     * 参数：
     * - x: 菜单起始X坐标
     * - y: 菜单起始Y坐标
     */
    void setPosition(int x, int y) {
        startX = x;
        startY = y;
    }

    /*
     * 显示菜单 - 在控制台中渲染完整的菜单界面
     * 布局结构：
     * 1. 顶部显示编辑器标题和程序界面标题
     * 2. 分隔线
     * 3. 程序名、文件路径和文件内容
     * 4. 分隔线
     * 5. 菜单选项（当前选中项高亮显示）
     * 6. 分隔线
     * 7. 导航提示信息
     * 
     * 参数：
     * - bgColor: 背景颜色
     * - fgColor: 前景颜色
     */
    void display(Color bgColor, Color fgColor) {
        clearScreen();
        
        // 获取控制台宽度（假设为80列）
        const int consoleWidth = 80;
        int separatorLength = 76; // 分隔线长度
        
        // 第一行：居中显示"BF 编辑器"（使用翻译）
        std::string bfEditorTitle = tr("bf_editor_title");
        int titleLength = bfEditorTitle.length();
        int titleX = (consoleWidth - titleLength) / 2;
        moveCursor(titleX, startY);
        printf("%s\n\n", bfEditorTitle.c_str());
        
        // 第二行：居中显示"程序界面"（使用翻译）
        std::string programInterfaceTitle = tr("program_interface");
        int interfaceTitleLength = programInterfaceTitle.length();
        int interfaceTitleX = (consoleWidth - interfaceTitleLength) / 2;
        moveCursor(interfaceTitleX, startY + 2);
        printf("%s\n\n", programInterfaceTitle.c_str());
        
        // 显示顶部分隔线
        moveCursor(2, startY + 4);
        for (int i = 0; i < separatorLength; i++) {
            printf("-");
        }
        printf("\n");
        
        // 显示程序名
        moveCursor(4, startY + 5);
        std::string programName = currentFileName.empty() ? tr("unnamed_file") : currentFileName;
        printf("%s: %s\n", tr("program_name").c_str(), programName.c_str());
        
        // 显示路径
        moveCursor(4, startY + 6);
        printf("%s: %s\n", tr("file_path").c_str(), currentFilePath.empty() ? "" : currentFilePath.c_str());
        
        // 显示文件内容
        moveCursor(4, startY + 7);
        printf("%s:\n", tr("file_content").c_str());
        
        if (!programContent.empty()) {
            // 限制显示的内容行数
            int maxLines = 10; // 最多显示10行内容
            int lineCount = 0;
            std::istringstream contentStream(programContent);
            std::string line;
            
            while (std::getline(contentStream, line) && lineCount < maxLines) {
                moveCursor(6, startY + 8 + lineCount);
                printf("%s\n", line.c_str());
                lineCount++;
            }
            
            // 如果内容行数过多，显示省略号
            if (lineCount >= maxLines && contentStream.good()) {
                moveCursor(6, startY + 8 + lineCount);
                printf("...\n");
            }
        } else {
            moveCursor(6, startY + 8);
            printf("//%s\n", tr("empty_content").c_str());
        }
        
        // 显示中间分隔线
        int contentEndLine = (programContent.empty() ? 9 : std::min(8 + (int)std::count(programContent.begin(), programContent.end(), '\n') + 1, startY + 18));
        moveCursor(2, contentEndLine);
        for (int i = 0; i < separatorLength; i++) {
            printf("-");
        }
        printf("\n");
        
        // 显示菜单标签
        moveCursor(4, contentEndLine + 1);
        printf("%s:\n", tr("menu_label").c_str());
        
        // 显示所有选项，居左显示
        for (size_t i = 0; i < options.size(); i++) {
            moveCursor(startX + 4, contentEndLine + 2 + static_cast<int>(i)); // 居左显示
            if (i == currentSelection) {
                // 高亮显示当前选项 - 使用用户设置的背景色和前景色的反转
                #ifdef _WIN32
                    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
                    // 临时保存当前的控制台文本属性
                    CONSOLE_SCREEN_BUFFER_INFO csbi;
                    GetConsoleScreenBufferInfo(hConsole, &csbi);
                    WORD originalAttr = csbi.wAttributes;
                    
                    // 使用用户设置的颜色进行反转
                    int winBgColor = (bgColor == BLACK) ? 0 : (colorToWinConsole[bgColor] + 8);
                    int winFgColor = colorToWinConsole[fgColor];
                    
                    // 反转颜色：前景色变背景色，背景色变前景色
                    SetConsoleTextAttribute(hConsole, (winFgColor << 4) | (winBgColor & 0x0F));
                    // 直接显示菜单项（不再需要移除序号，因为现在创建菜单时就不包含序号）
                    printf("%s\n", options[i].c_str());
                    
                    // 恢复原始颜色
                    SetConsoleTextAttribute(hConsole, originalAttr);
                #else
                    printf("\033[7m"); // 反转颜色
                    // 直接显示菜单项
                    printf("%s\n", options[i].c_str());
                    printf("\033[0m"); // 恢复默认
                #endif
            } else {
                // 直接显示菜单项
                printf("%s\n", options[i].c_str());
            }
        }
        
        // 显示底部分隔线
        int menuEndLine = contentEndLine + 2 + static_cast<int>(options.size());
        moveCursor(2, menuEndLine);
        for (int i = 0; i < separatorLength; i++) {
            printf("-");
        }
        printf("\n");
        
        // 显示提示信息（使用翻译）
        moveCursor(startX, menuEndLine + 1);
        printf("%s", tr("navigation_hint").c_str());
    }

    /*
     * 运行菜单并返回用户选择的索引
     * 处理用户输入（上下箭头键导航，Enter键确认，ESC键取消）
     * 
     * 参数：
     * - bgColor: 背景颜色
     * - fgColor: 前景颜色
     * 
     * 返回值：
     * - 用户选择的菜单项索引（从0开始）
     * - 如果用户取消（按ESC键），返回-1
     */
    int run(Color bgColor, Color fgColor) {
        char key = 0;
        
        // 设置控制台为不回显模式
        #ifdef _WIN32
            HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
            DWORD mode;
            GetConsoleMode(hConsole, &mode);
            SetConsoleMode(hConsole, mode & ~ENABLE_ECHO_INPUT & ~ENABLE_LINE_INPUT);
        #endif
        
        // 显示初始菜单
        display(bgColor, fgColor);
        
        while (true) {
            // 获取用户输入
            #ifdef _WIN32
                key = _getch();
                // 处理扩展键
                if (key == 0 || key == -32) {
                    key = _getch(); // 获取实际的扩展键码
                }
            #else
                key = getchar();
                if (key == 27) { // ESC键的开始
                    getchar(); // 跳过'['
                    key = getchar(); // 获取实际的方向键码
                }
            #endif
            
            // 根据按键更新选择
            switch (key) {
                case '\r': // Enter键
                case '\n':
                    // 恢复控制台模式
                    #ifdef _WIN32
                        SetConsoleMode(hConsole, mode);
                    #endif
                    return currentSelection;
                case 'H': // 上箭头键（Windows）
                case 'A': // 上箭头键（Linux）
                    if (currentSelection > 0) {
                        currentSelection--;
                    } else {
                        // 循环到最后一个选项
                        currentSelection = static_cast<int>(options.size()) - 1;
                    }
                    display(bgColor, fgColor);
                    break;
                case 'P': // 下箭头键（Windows）
                case 'B': // 下箭头键（Linux）
                    if (currentSelection < static_cast<int>(options.size()) - 1) {
                        currentSelection++;
                    } else {
                        // 循环到第一个选项
                        currentSelection = 0;
                    }
                    display(bgColor, fgColor);
                    break;
                case 27: // ESC键
                    // 恢复控制台模式
                    #ifdef _WIN32
                        SetConsoleMode(hConsole, mode);
                    #endif
                    return -1; // 表示用户取消选择
            }
        }
        
        // 恢复控制台模式（以防上面的代码没有执行到）
        #ifdef _WIN32
            SetConsoleMode(hConsole, mode);
        #endif
        
        return -1;
    }
};

// 全局语言设置 - 默认英语
// 当前语言设置 - 默认英语
Language currentLanguage = ENGLISH;

// 全局颜色设置
// 背景颜色 - 默认黑色
Color backgroundColor = BLACK;
// 代码颜色 - 默认白色
Color codeColor = WHITE;
// 注释颜色 - 默认绿色
Color commentColor = GREEN;

/*
 * 获取翻译文本
 * 根据当前语言设置返回对应键的翻译文本
 * 如果当前语言没有该键的翻译，则尝试使用英语翻译
 * 如果英语也没有，则返回键名本身
 * 
 * 参数：
 * - key: 翻译键名
 * 
 * 返回值：
 * - 翻译后的文本字符串
 */
std::string tr(const std::string& key) {
    std::map<Language, std::map<std::string, std::string> >::iterator langIt = translations.find(currentLanguage);
    if (langIt != translations.end()) {
        std::map<std::string, std::string>::iterator it = langIt->second.find(key);
        if (it != langIt->second.end()) {
            return it->second;
        }
    }
    // 如果当前语言没有翻译，则回退到英语
    if (currentLanguage != ENGLISH) {
        std::map<Language, std::map<std::string, std::string> >::iterator engIt = translations.find(ENGLISH);
        if (engIt != translations.end()) {
            std::map<std::string, std::string>::iterator engKeyIt = engIt->second.find(key);
            if (engKeyIt != engIt->second.end()) {
                return engKeyIt->second;
            }
        }
    }
    return key; // 如果英语也没有，则返回键名
}

/*
 * 格式化翻译文本（支持整数参数）
 * 用于带参数的翻译文本，如包含%d的字符串
 * 
 * 参数：
 * - key: 翻译键名
 * - value: 要插入的整数值
 * 
 * 返回值：
 * - 格式化后的翻译文本字符串
 */
std::string trf(const std::string& key, int value) {
    std::string text = tr(key);
    char buffer[256];
    snprintf(buffer, sizeof(buffer), text.c_str(), value);
    return std::string(buffer);
}

/*
 * 格式化翻译文本（支持字符串参数）
 * 用于带参数的翻译文本，如包含%s的字符串
 * 
 * 参数：
 * - key: 翻译键名
 * - value: 要插入的字符串值
 * 
 * 返回值：
 * - 格式化后的翻译文本字符串
 */
std::string trf(const std::string& key, const char* value) {
    std::string text = tr(key);
    char buffer[256];
    snprintf(buffer, sizeof(buffer), text.c_str(), value);
    return std::string(buffer);
}

/*
 * 应用代码颜色到终端
 * 根据用户设置的背景色和代码颜色设置终端文本颜色
 * 兼容Windows和Linux平台
 */
void applyColors() {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    // Windows控制台背景色需要特殊处理，将前景色代码转换为背景色代码
    int bgColor = (backgroundColor == BLACK) ? 0 : (colorToWinConsole[backgroundColor] + 8);
    int fgColor = colorToWinConsole[codeColor];
    SetConsoleTextAttribute(hConsole, (bgColor << 4) | fgColor);
#else
    // Linux终端颜色设置 - 背景色需要在数字前加4
    std::string bgCode = "4" + colorToLinuxConsole[backgroundColor].substr(1);
    std::string fgCode = colorToLinuxConsole[codeColor];
    std::cout << "\033[" << bgCode << "m\033[" << fgCode << "m";
#endif
}

/*
 * 应用注释颜色
 * 根据用户设置的背景色和注释颜色设置终端文本颜色
 * 兼容Windows和Linux平台
 */
void applyCommentColor() {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    // Windows控制台背景色需要特殊处理，将前景色代码转换为背景色代码
    int bgColor = (backgroundColor == BLACK) ? 0 : (colorToWinConsole[backgroundColor] + 8);
    int fgColor = colorToWinConsole[commentColor];
    SetConsoleTextAttribute(hConsole, (bgColor << 4) | fgColor);
#else
    // Linux终端颜色设置 - 背景色需要在数字前加4
    std::string bgCode = "4" + colorToLinuxConsole[backgroundColor].substr(1);
    std::string fgCode = colorToLinuxConsole[commentColor];
    std::cout << "\033[" << bgCode << "m\033[" << fgCode << "m";
#endif
}

/*
 * 重置终端颜色
 * 将终端颜色恢复为默认设置（白色文本，黑色背景）
 * 兼容Windows和Linux平台
 */
void resetColors() {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 7); // 默认白色文本和黑色背景
#else
    std::cout << "\033[0m";
#endif
}

/*
 * 保存设置到文件
 * 将当前语言和颜色设置保存到配置文件中
 * 配置文件保存在程序可执行文件所在的目录
 */
void saveSettings() {
    std::string exeDir = getExeDir();
    std::string filePath;
    
#ifdef _WIN32
    filePath = exeDir + "\\editor.txt";
#else
    filePath = exeDir + "/editor.txt";
#endif
    
    std::ofstream file(filePath.c_str());
    if (file.is_open()) {
        // 保存语言设置
        file << "language:" << static_cast<int>(currentLanguage) << std::endl;
        
        // 保存颜色设置
        file << "color:" << colorNames[backgroundColor] << ";" 
             << colorNames[codeColor] << ";" 
             << colorNames[commentColor] << std::endl;
        
        file.close();
    } else {
        std::cerr << "Failed to save settings to: " << filePath << std::endl;
    }
}

/*
 * 从文件加载设置
 * 从配置文件中加载保存的语言和颜色设置
 * 如果配置文件不存在或无法读取，则保持当前设置不变
 */
void loadSettings() {
    std::string exeDir = getExeDir();
    std::string filePath;
    
#ifdef _WIN32
    filePath = exeDir + "\\editor.txt";
#else
    filePath = exeDir + "/editor.txt";
#endif
    
    std::ifstream file(filePath.c_str());
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("language:") == 0) {
                int langValue = atoi(line.substr(9).c_str());
                if (langValue >= 0 && langValue <= 9) {
                    currentLanguage = static_cast<Language>(langValue);
                }
            } else if (line.find("color:") == 0) {
                std::string colorStr = line.substr(6);
                std::istringstream ss(colorStr);
                std::string bgColor, codeColorStr, commentColorStr;
                
                if (std::getline(ss, bgColor, ';') &&
                    std::getline(ss, codeColorStr, ';') &&
                    std::getline(ss, commentColorStr, ';')) {
                    
                    // 查找背景颜色
                    for (std::map<Color, std::string>::iterator it = colorNames.begin(); it != colorNames.end(); ++it) {
                        if (it->second == bgColor) {
                            backgroundColor = it->first;
                            break;
                        }
                    }
                    
                    // 查找代码颜色
                    for (std::map<Color, std::string>::iterator it = colorNames.begin(); it != colorNames.end(); ++it) {
                        if (it->second == codeColorStr) {
                            codeColor = it->first;
                            break;
                        }
                    }
                    
                    // 查找注释颜色
                    for (std::map<Color, std::string>::iterator it = colorNames.begin(); it != colorNames.end(); ++it) {
                        if (it->second == commentColorStr) {
                            commentColor = it->first;
                            break;
                        }
                    }
                }
            }
        }
        file.close();
    }
    
    // 应用更新的颜色设置
    applyColors();
}

/*
 * 语言设置菜单
 * 提供用户界面让用户选择程序界面的显示语言
 * 副作用：修改全局currentLanguage变量，并保存设置到配置文件
 */
void languageSettings() {
    bool inSettings = true; // 控制设置菜单循环的标志
    
    while (inSettings) { // 语言设置菜单主循环
        clearScreen(); // 清屏
        
        // 创建语言选项菜单
        std::vector<std::string> languageOptions;
        for (std::map<Language, std::string>::iterator it = languageNames.begin(); it != languageNames.end(); ++it) {
            languageOptions.push_back(it->second);
        }
        languageOptions.push_back(tr("back_to_main"));
        
        // 创建并运行语言选择菜单
        WinConsoleMenu languageMenu(tr("language_settings"), languageOptions);
        int choice = languageMenu.run(backgroundColor, codeColor);
        
        // 处理用户选择
        if (choice == (int)languageNames.size() || choice == -1) { // 如果选择返回主菜单或按ESC
            inSettings = false; // 退出语言设置菜单循环
        } else if (choice >= 0 && choice < (int)languageNames.size()) { // 如果选择了一个有效的语言选项
            // 将用户选择的索引转换为对应的Language枚举值
            int langIndex = 0;
            for (std::map<Language, std::string>::iterator it = languageNames.begin(); it != languageNames.end(); ++it) {
                if (langIndex == choice) { // 找到用户选择的语言
                    currentLanguage = it->first; // 更新全局语言设置
                    clearScreen();
                    printf("%s\n", tr("language_changed").c_str()); // 提示语言已更改
                    saveSettings(); // 保存设置到文件
                    pauseScreen(); // 暂停屏幕等待用户按键
                    break;
                }
                langIndex++; // 索引递增
            }
        }
    }
}

/*
 * 颜色设置菜单
 * 提供用户界面让用户自定义控制台显示的颜色设置
 * 支持自定义背景色、代码颜色和注释颜色
 * 副作用：修改全局颜色变量（backgroundColor、codeColor、commentColor），并保存设置到配置文件
 */
void colorSettings() {
    bool inColorSettings = true; // 控制颜色设置菜单循环的标志
    
    while (inColorSettings) { // 颜色设置菜单主循环
        clearScreen(); // 清屏
        
        // 创建颜色设置菜单项
        std::vector<std::string> colorSettingsOptions;
        colorSettingsOptions.push_back(tr("select_background_color"));
        colorSettingsOptions.push_back(tr("select_code_color"));
        colorSettingsOptions.push_back(tr("select_comment_color"));
        colorSettingsOptions.push_back(tr("reset_colors"));
        colorSettingsOptions.push_back(tr("back_to_main"));
        
        // 创建并运行颜色设置菜单
        WinConsoleMenu settingsMenu(tr("color_settings"), colorSettingsOptions);
        int choice = settingsMenu.run(backgroundColor, codeColor);
        
        // 根据用户选择执行相应操作
        switch (choice) {
            case 0: // 选择背景颜色
                {
                    clearScreen(); // 清屏
                    
                    // 创建颜色选择方式菜单
                std::vector<std::string> colorModeOptions;
                colorModeOptions.push_back(tr("predefined_colors"));
                colorModeOptions.push_back(tr("custom_rgb_color"));
                colorModeOptions.push_back(tr("custom_hsl_color"));
                colorModeOptions.push_back(tr("back_to_main"));
                
                // 创建并运行颜色选择方式菜单
                WinConsoleMenu colorModeMenu(tr("color_options"), colorModeOptions);
                int modeChoice = colorModeMenu.run(backgroundColor, codeColor);
                    
                    if (modeChoice == 0) { // 选择预定义颜色
                        // 创建颜色选项菜单
                        std::vector<std::string> colorOptions;
                        for (std::map<Color, std::string>::iterator it = colorNames.begin(); it != colorNames.end(); ++it) {
                            colorOptions.push_back(it->second);
                        }
                        colorOptions.push_back(tr("back_to_main"));
                        
                        // 创建并运行颜色选择菜单
                        WinConsoleMenu colorMenu(tr("select_background_color"), colorOptions);
                        int colorChoice = colorMenu.run(backgroundColor, codeColor);
                        
                        if (colorChoice >= 0 && colorChoice < (int)colorNames.size()) { // 如果选择了一个有效的颜色
                            int colorIndex = 0;
                            for (std::map<Color, std::string>::iterator it = colorNames.begin(); it != colorNames.end(); ++it) {
                                if (colorIndex == colorChoice) { // 找到用户选择的颜色
                                    backgroundColor = it->first; // 更新全局背景颜色设置
                                    clearScreen();
                                    printf("%s\n", tr("color_changed").c_str()); // 提示颜色已更改
                                    applyColors(); // 立即应用新的颜色设置
                                    saveSettings(); // 保存设置到文件
                                    pauseScreen(); // 暂停屏幕等待用户按键
                                    break;
                                }
                                colorIndex++; // 索引递增
                            }
                        }
                    } else if (modeChoice == 1) { // 选择自定义RGB颜色
                        clearScreen();
                        printf("%s\n", tr("enter_rgb").c_str());
                        
                        // 清空输入缓冲区
                        int c;
                        while ((c = getchar()) != '\n' && c != EOF);
                        
                        int r, g, b;
                        if (scanf("%d %d %d", &r, &g, &b) == 3 && r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
                            // 这里简化处理，我们根据RGB值选择最接近的预定义颜色
                            // 在实际应用中，可能需要扩展Color枚举或实现更复杂的颜色映射
                            Color closestColor = BLACK;
                            double minDistance = 100000;
                            
                            // 计算与每个预定义颜色的欧氏距离，选择最近的
                            for (int color = BLACK; color <= BROWN; color++) {
                                int cr = COLOR_RGB[color][0];
                                int cg = COLOR_RGB[color][1];
                                int cb = COLOR_RGB[color][2];
                                double distance = sqrt(pow(r - cr, 2) + pow(g - cg, 2) + pow(b - cb, 2));
                                if (distance < minDistance) {
                                    minDistance = distance;
                                    closestColor = static_cast<Color>(color);
                                }
                            }
                            
                            backgroundColor = closestColor;
                            clearScreen();
                            printf("%s\n", tr("color_changed").c_str());
                            applyColors();
                            saveSettings();
                            pauseScreen();
                        } else {
                            clearScreen();
                            printf("%s\n", tr("invalid_rgb").c_str());
                            pauseScreen();
                        }
                    } else if (modeChoice == 2) { // 选择自定义HSL颜色
                        clearScreen();
                        printf("%s\n", tr("enter_hsl").c_str());
                        
                        // 清空输入缓冲区
                        int c;
                        while ((c = getchar()) != '\n' && c != EOF);
                        
                        int h, s, l;
                        if (scanf("%d %d %d", &h, &s, &l) == 3 && h >= 0 && h <= 360 && s >= 0 && s <= 100 && l >= 0 && l <= 100) {
                            // HSL到RGB转换
                            double s_double = s / 100.0;
                            double l_double = l / 100.0;
                            
                            double C = (1 - fabs(2 * l_double - 1)) * s_double;
                            double X = C * (1 - fabs(fmod(h / 60.0, 2) - 1));
                            double m = l_double - C / 2;
                            
                            double r_double, g_double, b_double;
                            
                            if (h >= 0 && h < 60) {
                                r_double = C; g_double = X; b_double = 0;
                            } else if (h >= 60 && h < 120) {
                                r_double = X; g_double = C; b_double = 0;
                            } else if (h >= 120 && h < 180) {
                                r_double = 0; g_double = C; b_double = X;
                            } else if (h >= 180 && h < 240) {
                                r_double = 0; g_double = X; b_double = C;
                            } else if (h >= 240 && h < 300) {
                                r_double = X; g_double = 0; b_double = C;
                            } else {
                                r_double = C; g_double = 0; b_double = X;
                            }
                            
                            // 转换为0-255范围
                            int r = static_cast<int>((r_double + m) * 255);
                            int g = static_cast<int>((g_double + m) * 255);
                            int b = static_cast<int>((b_double + m) * 255);
                            
                            // 确保值在0-255范围内
                            r = std::max(0, std::min(255, r));
                            g = std::max(0, std::min(255, g));
                            b = std::max(0, std::min(255, b));
                            
                            // 使用与RGB相同的逻辑选择最接近的预定义颜色
                            Color closestColor = BLACK;
                            double minDistance = 100000;
                            
                            // 计算与每个预定义颜色的欧氏距离，选择最近的
                            for (int color = BLACK; color <= BROWN; color++) {
                                int cr = COLOR_RGB[color][0];
                                int cg = COLOR_RGB[color][1];
                                int cb = COLOR_RGB[color][2];
                                double distance = sqrt(pow(r - cr, 2) + pow(g - cg, 2) + pow(b - cb, 2));
                                if (distance < minDistance) {
                                    minDistance = distance;
                                    closestColor = static_cast<Color>(color);
                                }
                            }
                            
                            backgroundColor = closestColor;
                            clearScreen();
                            printf("%s\n", tr("color_changed").c_str());
                            applyColors();
                            saveSettings();
                            pauseScreen();
                        } else {
                            clearScreen();
                            printf("%s\n", tr("invalid_hsl").c_str());
                            pauseScreen();
                        }
                    }
                }
                break;
                
            case 1: // 选择代码颜色
                {
                    clearScreen(); // 清屏
                    
                    // 创建颜色选择方式菜单
                std::vector<std::string> colorModeOptions;
                colorModeOptions.push_back(tr("predefined_colors"));
                colorModeOptions.push_back(tr("custom_rgb_color"));
                colorModeOptions.push_back(tr("custom_hsl_color"));
                colorModeOptions.push_back(tr("back_to_main"));
                
                // 创建并运行颜色选择方式菜单
                WinConsoleMenu colorModeMenu(tr("color_options"), colorModeOptions);
                int modeChoice = colorModeMenu.run(backgroundColor, codeColor);
                    
                    if (modeChoice == 0) { // 选择预定义颜色
                        // 创建颜色选项菜单
                        std::vector<std::string> colorOptions;
                        for (std::map<Color, std::string>::iterator it = colorNames.begin(); it != colorNames.end(); ++it) {
                            colorOptions.push_back(it->second);
                        }
                        colorOptions.push_back(tr("back_to_main"));
                        
                        // 创建并运行颜色选择菜单
                        WinConsoleMenu colorMenu(tr("select_code_color"), colorOptions);
                        int colorChoice = colorMenu.run(backgroundColor, codeColor);
                        
                        if (colorChoice >= 0 && colorChoice < (int)colorNames.size()) { // 如果选择了一个有效的颜色
                            int colorIndex = 0;
                            for (std::map<Color, std::string>::iterator it = colorNames.begin(); it != colorNames.end(); ++it) {
                                if (colorIndex == colorChoice) { // 找到用户选择的颜色
                                    codeColor = it->first; // 更新全局代码颜色设置
                                    clearScreen();
                                    printf("%s\n", tr("color_changed").c_str()); // 提示颜色已更改
                                    applyColors(); // 立即应用新的颜色设置
                                    saveSettings(); // 保存设置到文件
                                    pauseScreen(); // 暂停屏幕等待用户按键
                                    break;
                                }
                                colorIndex++; // 索引递增
                            }
                        }
                    } else if (modeChoice == 1) { // 选择自定义RGB颜色
                        clearScreen();
                        printf("%s\n", tr("enter_rgb").c_str());
                        
                        // 清空输入缓冲区
                        int c;
                        while ((c = getchar()) != '\n' && c != EOF);
                        
                        int r, g, b;
                        if (scanf("%d %d %d", &r, &g, &b) == 3 && r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
                            // 这里简化处理，我们根据RGB值选择最接近的预定义颜色
                            // 在实际应用中，可能需要扩展Color枚举或实现更复杂的颜色映射
                            Color closestColor = WHITE;
                            double minDistance = 100000;
                            
                            // 计算与每个预定义颜色的欧氏距离，选择最近的
                            for (int color = BLACK; color <= BROWN; color++) {
                                int cr = COLOR_RGB[color][0];
                                int cg = COLOR_RGB[color][1];
                                int cb = COLOR_RGB[color][2];
                                double distance = sqrt(pow(r - cr, 2) + pow(g - cg, 2) + pow(b - cb, 2));
                                if (distance < minDistance) {
                                    minDistance = distance;
                                    closestColor = static_cast<Color>(color);
                                }
                            }
                            
                            codeColor = closestColor;
                            clearScreen();
                            printf("%s\n", tr("color_changed").c_str());
                            applyColors();
                            saveSettings();
                            pauseScreen();
                        } else {
                            clearScreen();
                            printf("%s\n", tr("invalid_rgb").c_str());
                            pauseScreen();
                        }
                    } else if (modeChoice == 2) { // 选择自定义HSL颜色
                        clearScreen();
                        printf("%s\n", tr("enter_hsl").c_str());
                        
                        // 清空输入缓冲区
                        int c;
                        while ((c = getchar()) != '\n' && c != EOF);
                        
                        int h, s, l;
                        if (scanf("%d %d %d", &h, &s, &l) == 3 && h >= 0 && h <= 360 && s >= 0 && s <= 100 && l >= 0 && l <= 100) {
                            // HSL到RGB转换
                            double s_double = s / 100.0;
                            double l_double = l / 100.0;
                            
                            double C = (1 - fabs(2 * l_double - 1)) * s_double;
                            double X = C * (1 - fabs(fmod(h / 60.0, 2) - 1));
                            double m = l_double - C / 2;
                            
                            double r_double, g_double, b_double;
                            
                            if (h >= 0 && h < 60) {
                                r_double = C; g_double = X; b_double = 0;
                            } else if (h >= 60 && h < 120) {
                                r_double = X; g_double = C; b_double = 0;
                            } else if (h >= 120 && h < 180) {
                                r_double = 0; g_double = C; b_double = X;
                            } else if (h >= 180 && h < 240) {
                                r_double = 0; g_double = X; b_double = C;
                            } else if (h >= 240 && h < 300) {
                                r_double = X; g_double = 0; b_double = C;
                            } else {
                                r_double = C; g_double = 0; b_double = X;
                            }
                            
                            // 转换为0-255范围
                            int r = static_cast<int>((r_double + m) * 255);
                            int g = static_cast<int>((g_double + m) * 255);
                            int b = static_cast<int>((b_double + m) * 255);
                            
                            // 确保值在0-255范围内
                            r = std::max(0, std::min(255, r));
                            g = std::max(0, std::min(255, g));
                            b = std::max(0, std::min(255, b));
                            
                            // 使用与RGB相同的逻辑选择最接近的预定义颜色
                            Color closestColor = WHITE;
                            double minDistance = 100000;
                            
                            // 计算与每个预定义颜色的欧氏距离，选择最近的
                            for (int color = BLACK; color <= BROWN; color++) {
                                int cr = COLOR_RGB[color][0];
                                int cg = COLOR_RGB[color][1];
                                int cb = COLOR_RGB[color][2];
                                double distance = sqrt(pow(r - cr, 2) + pow(g - cg, 2) + pow(b - cb, 2));
                                if (distance < minDistance) {
                                    minDistance = distance;
                                    closestColor = static_cast<Color>(color);
                                }
                            }
                            
                            codeColor = closestColor;
                            clearScreen();
                            printf("%s\n", tr("color_changed").c_str());
                            applyColors();
                            saveSettings();
                            pauseScreen();
                        } else {
                            clearScreen();
                            printf("%s\n", tr("invalid_hsl").c_str());
                            pauseScreen();
                        }
                    }
                }
                break;
                
            case 2: // 选择注释颜色
                {
                    clearScreen(); // 清屏
                    
                    // 创建颜色选择方式菜单
                std::vector<std::string> colorModeOptions;
                colorModeOptions.push_back(tr("predefined_colors"));
                colorModeOptions.push_back(tr("custom_rgb_color"));
                colorModeOptions.push_back(tr("custom_hsl_color"));
                colorModeOptions.push_back(tr("back_to_main"));
                
                // 创建并运行颜色选择方式菜单
                WinConsoleMenu colorModeMenu(tr("color_options"), colorModeOptions);
                int modeChoice = colorModeMenu.run(backgroundColor, codeColor);
                    
                    if (modeChoice == 0) { // 选择预定义颜色
                        // 创建颜色选项菜单
                        std::vector<std::string> colorOptions;
                        for (std::map<Color, std::string>::iterator it = colorNames.begin(); it != colorNames.end(); ++it) {
                            colorOptions.push_back(it->second);
                        }
                        colorOptions.push_back(tr("back_to_main"));
                        
                        // 创建并运行颜色选择菜单
                        WinConsoleMenu colorMenu(tr("select_comment_color"), colorOptions);
                        int colorChoice = colorMenu.run(backgroundColor, codeColor);
                        
                        if (colorChoice >= 0 && colorChoice < (int)colorNames.size()) { // 如果选择了一个有效的颜色
                            int colorIndex = 0;
                            for (std::map<Color, std::string>::iterator it = colorNames.begin(); it != colorNames.end(); ++it) {
                                if (colorIndex == colorChoice) { // 找到用户选择的颜色
                                    commentColor = it->first; // 更新全局注释颜色设置
                                    clearScreen();
                                    printf("%s\n", tr("color_changed").c_str()); // 提示颜色已更改
                                    applyColors(); // 立即应用新的颜色设置
                                    saveSettings(); // 保存设置到文件
                                    pauseScreen(); // 暂停屏幕等待用户按键
                                    break;
                                }
                                colorIndex++; // 索引递增
                            }
                        }
                    } else if (modeChoice == 1) { // 选择自定义RGB颜色
                        clearScreen();
                        printf("%s\n", tr("enter_rgb").c_str());
                        
                        // 清空输入缓冲区
                        int c;
                        while ((c = getchar()) != '\n' && c != EOF);
                        
                        int r, g, b;
                        if (scanf("%d %d %d", &r, &g, &b) == 3 && r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
                            // 这里简化处理，我们根据RGB值选择最接近的预定义颜色
                            // 在实际应用中，可能需要扩展Color枚举或实现更复杂的颜色映射
                            Color closestColor = GREEN;
                            double minDistance = 100000;
                            
                            // 计算与每个预定义颜色的欧氏距离，选择最近的
                            for (int color = BLACK; color <= BROWN; color++) {
                                int cr = COLOR_RGB[color][0];
                                int cg = COLOR_RGB[color][1];
                                int cb = COLOR_RGB[color][2];
                                double distance = sqrt(pow(r - cr, 2) + pow(g - cg, 2) + pow(b - cb, 2));
                                if (distance < minDistance) {
                                    minDistance = distance;
                                    closestColor = static_cast<Color>(color);
                                }
                            }
                            
                            commentColor = closestColor;
                            clearScreen();
                            printf("%s\n", tr("color_changed").c_str());
                            applyColors();
                            saveSettings();
                            pauseScreen();
                        } else {
                            clearScreen();
                            printf("%s\n", tr("invalid_rgb").c_str());
                            pauseScreen();
                        }
                    } else if (modeChoice == 2) { // 选择自定义HSL颜色
                        clearScreen();
                        printf("%s\n", tr("enter_hsl").c_str());
                        
                        // 清空输入缓冲区
                        int c;
                        while ((c = getchar()) != '\n' && c != EOF);
                        
                        int h, s, l;
                        if (scanf("%d %d %d", &h, &s, &l) == 3 && h >= 0 && h <= 360 && s >= 0 && s <= 100 && l >= 0 && l <= 100) {
                            // HSL到RGB转换
                            double s_double = s / 100.0;
                            double l_double = l / 100.0;
                            
                            double C = (1 - fabs(2 * l_double - 1)) * s_double;
                            double X = C * (1 - fabs(fmod(h / 60.0, 2) - 1));
                            double m = l_double - C / 2;
                            
                            double r_double, g_double, b_double;
                            
                            if (h >= 0 && h < 60) {
                                r_double = C; g_double = X; b_double = 0;
                            } else if (h >= 60 && h < 120) {
                                r_double = X; g_double = C; b_double = 0;
                            } else if (h >= 120 && h < 180) {
                                r_double = 0; g_double = C; b_double = X;
                            } else if (h >= 180 && h < 240) {
                                r_double = 0; g_double = X; b_double = C;
                            } else if (h >= 240 && h < 300) {
                                r_double = X; g_double = 0; b_double = C;
                            } else {
                                r_double = C; g_double = 0; b_double = X;
                            }
                            
                            // 转换为0-255范围
                            int r = static_cast<int>((r_double + m) * 255);
                            int g = static_cast<int>((g_double + m) * 255);
                            int b = static_cast<int>((b_double + m) * 255);
                            
                            // 确保值在0-255范围内
                            r = std::max(0, std::min(255, r));
                            g = std::max(0, std::min(255, g));
                            b = std::max(0, std::min(255, b));
                            
                            // 使用与RGB相同的逻辑选择最接近的预定义颜色
                            Color closestColor = GREEN;
                            double minDistance = 100000;
                            
                            // 计算与每个预定义颜色的欧氏距离，选择最近的
                            for (int color = BLACK; color <= BROWN; color++) {
                                int cr = COLOR_RGB[color][0];
                                int cg = COLOR_RGB[color][1];
                                int cb = COLOR_RGB[color][2];
                                double distance = sqrt(pow(r - cr, 2) + pow(g - cg, 2) + pow(b - cb, 2));
                                if (distance < minDistance) {
                                    minDistance = distance;
                                    closestColor = static_cast<Color>(color);
                                }
                            }
                            
                            commentColor = closestColor;
                            clearScreen();
                            printf("%s\n", tr("color_changed").c_str());
                            applyColors();
                            saveSettings();
                            pauseScreen();
                        } else {
                            clearScreen();
                            printf("%s\n", tr("invalid_hsl").c_str());
                            pauseScreen();
                        }
                    }
                }
                break;
                
            case 3: // 重置颜色
                backgroundColor = BLACK; // 重置为默认背景颜色（黑色）
                codeColor = WHITE; // 重置为默认代码颜色（白色）
                commentColor = GREEN; // 重置为默认注释颜色（绿色）
                clearScreen();
                printf("%s\n", tr("colors_reset").c_str()); // 提示颜色已重置
                applyColors(); // 立即应用默认颜色设置
                saveSettings(); // 保存设置到文件
                pauseScreen(); // 暂停屏幕等待用户按键
                break;
                
            case 4: // 返回主菜单
            case -1: // ESC键
                inColorSettings = false; // 设置循环标志为false，退出颜色设置菜单
                break;
                
            default: // 无效选项
                break;
        }
    }
}

// 编辑器设置菜单
void editorSettings() {
    bool inSettings = true;
    
    while (inSettings) {
        // 创建编辑器设置菜单项
        std::vector<std::string> editorSettingsOptions;
        editorSettingsOptions.push_back(tr("language_settings"));
        editorSettingsOptions.push_back(tr("color_settings"));
        editorSettingsOptions.push_back(tr("back_to_main"));
        
        // 创建并运行编辑器设置菜单
        WinConsoleMenu settingsMenu(tr("editor_settings"), editorSettingsOptions);
        int choice = settingsMenu.run(backgroundColor, codeColor);
        
        switch (choice) {
            case 0:
                languageSettings();
                break;
            case 1:
                colorSettings();
                break;
            case 2:
            case -1:
                inSettings = false;
                break;
            default:
                break;
        }
    }
}

class DirectoryReader {
public:
    // 获取 .bf 结尾的文件
    static std::vector<std::string> getBFFiles(const std::string& directoryPath) {
        std::vector<std::string> files;
        
#ifdef _WIN32
        // Windows 实现
        WIN32_FIND_DATAA findFileData;
        HANDLE hFind = FindFirstFileA((directoryPath + "\\*").c_str(), &findFileData);
        if (hFind == INVALID_HANDLE_VALUE) {
                std::cerr << "无法读取目录: " << directoryPath << std::endl;
            return files;
        }
        do {
            // 跳过 "." 和 ".."
            if (strcmp(findFileData.cFileName, ".") == 0 || 
                strcmp(findFileData.cFileName, "..") == 0) {
                continue;
            }
            // 如果文件不是目录并且以 .bf 结尾
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::string filename = findFileData.cFileName;
                if (hasBFExtension(filename)) {
                    files.push_back(filename);
                }
            }
        } while (FindNextFileA(hFind, &findFileData) != 0);
        FindClose(hFind);
#else
        // Linux 实现
        DIR* dir = opendir(directoryPath.c_str());
        if (dir == NULL) {
            std::cerr << "无法打开目录: " << directoryPath << std::endl;
            return files;
        }
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            // 跳过 "." 和 ".."
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            if (entry->d_type == DT_REG) {
                std::string filename = entry->d_name;
                if (hasBFExtension(filename)) {
                    files.push_back(filename);
                }
            }
        }
        closedir(dir);
#endif
        return files;
    }

    // 获取带有完整路径的 .bf 文件
    static std::vector<std::string> getBFFilesWithPath(const std::string& directoryPath) {
        std::vector<std::string> files;
        
    #ifdef _WIN32
        WIN32_FIND_DATAA findFileData;
        HANDLE hFind = FindFirstFileA((directoryPath + "\\*").c_str(), &findFileData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (strcmp(findFileData.cFileName, ".") != 0 && 
                    strcmp(findFileData.cFileName, "..") != 0) {
                    if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        std::string filename = findFileData.cFileName;
                        if (hasBFExtension(filename)) {
                            std::string fullPath = directoryPath + "\\" + filename;
                            files.push_back(fullPath);
                        }
                    }
                }
            } while (FindNextFileA(hFind, &findFileData) != 0);
            FindClose(hFind);
        }
    #else
        DIR* dir = opendir(directoryPath.c_str());
        if (dir != NULL) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    if (entry->d_type == DT_REG) {
                        std::string filename = entry->d_name;
                        if (hasBFExtension(filename)) {
                            std::string fullPath = directoryPath + "/" + filename;
                            files.push_back(fullPath);
                        }
                    }
                }
            }
            closedir(dir);
        }
    #endif
        return files;
    }

    // 递归获取所有子目录中的 .bf 文件
    static std::vector<std::string> getBFFilesRecursive(const std::string& directoryPath) {
        std::vector<std::string> allBFFiles;
        // 首先获取当前目录的 .bf 文件
        std::vector<std::string> currentDirFiles = getBFFilesWithPath(directoryPath);
        for (std::vector<std::string>::size_type i = 0; i < currentDirFiles.size(); i++) {
            allBFFiles.push_back(currentDirFiles[i]);
        }
        
#ifdef _WIN32
        // 递归搜索子目录 - Windows
        WIN32_FIND_DATAA findFileData;
        HANDLE hFind = FindFirstFileA((directoryPath + "\\*").c_str(), &findFileData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (strcmp(findFileData.cFileName, ".") != 0 && 
                    strcmp(findFileData.cFileName, "..") != 0) {
                    
                    if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        std::string subDirPath = directoryPath + "\\" + findFileData.cFileName;
                        std::vector<std::string> subDirFiles = getBFFilesRecursive(subDirPath);
                        for (std::vector<std::string>::size_type i = 0; i < subDirFiles.size(); i++) {
                            allBFFiles.push_back(subDirFiles[i]);
                        }
                    }
                }
            } while (FindNextFileA(hFind, &findFileData) != 0);
            FindClose(hFind);
        }
#else
        // 递归搜索子目录 - Linux
        DIR* dir = opendir(directoryPath.c_str());
        if (dir != NULL) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    if (entry->d_type == DT_DIR) {
                        std::string subDirPath = directoryPath + "/" + entry->d_name;
                        std::vector<std::string> subDirFiles = getBFFilesRecursive(subDirPath);
                        for (std::vector<std::string>::size_type i = 0; i < subDirFiles.size(); i++) {
                            allBFFiles.push_back(subDirFiles[i]);
                        }
                    }
                }
            }
            closedir(dir);
        }
#endif
        return allBFFiles;
    }

private:
    // 检查文件是否以 .bf 结尾（忽略大小写）
    static bool hasBFExtension(const std::string& filename) {
        if (filename.length() < 3) {
            return false;
        }
        std::string extension = filename.substr(filename.length() - 3);
        // 不区分大小写比较
    return (extension == ".bf" || extension == ".BF" || 
            extension == ".Bf" || extension == ".bF");
    }
};

/**
 * 获取可执行文件所在目录
 * 功能：返回当前运行程序的可执行文件所在的目录路径
 * 跨平台实现：
 *   - Windows平台：使用Windows API的GetModuleFileNameA函数获取完整路径
 *   - 非Windows平台：使用Linux/MacOS的readlink函数读取/proc/self/exe
 * 处理流程：
 *   1. 获取可执行文件的完整路径
 *   2. 找到最后一个目录分隔符的位置
 *   3. 截取从开始到目录分隔符的部分作为目录路径
 * 返回值：
 *   - 程序可执行文件所在的目录路径，如果获取失败则返回空字符串
 * 使用场景：
 *   - 定位与程序相关的资源文件
 *   - 确定保存用户数据的默认位置
 *   - 查找程序的配置文件
 * 注意：
 *   - 在Windows平台上使用反斜杠(\)作为目录分隔符
 *   - 在非Windows平台上使用正斜杠(/)作为目录分隔符
 *   - 函数会根据不同操作系统选择合适的实现方式
 */
std::string getExeDir() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (GetModuleFileNameA(NULL, path, MAX_PATH)) {
        std::string fullPath(path);
        std::string::size_type pos = fullPath.find_last_of("\\");
        if (pos != std::string::npos) {
            return fullPath.substr(0, pos);
        }
    }
#else
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count != -1) {
        std::string fullPath(path, count);
        std::string::size_type pos = fullPath.find_last_of("/");
        if (pos != std::string::npos) {
            return fullPath.substr(0, pos);
        }
    }
#endif
    return "";
}

/**
 * 创建目录
 * 功能：在指定路径创建一个新的目录
 * 参数：path - 要创建的目录的完整路径
 * 返回值：布尔值，表示目录是否创建成功
 * 跨平台实现：
 * - Windows平台：使用Windows API的CreateDirectoryA函数
 * - 非Windows平台：使用POSIX的mkdir函数，权限设置为0755（所有者可读写执行，组和其他可读执行）
 * 注意：
 * - 该函数不会递归创建父目录，如果父目录不存在，创建会失败
 * - 在Windows平台上，如果目录已存在，CreateDirectoryA函数会返回失败
 * - 在非Windows平台上，如果目录已存在，mkdir函数会返回失败并设置errno为EEXIST
 */
bool createDirectory(const std::string& path) {
#ifdef _WIN32
    return CreateDirectoryA(path.c_str(), NULL) != 0;
#else
    return mkdir(path.c_str(), 0755) == 0;
#endif
}

/**
 * 清屏函数
 * 功能：清除控制台屏幕上的所有内容
 * 平台兼容性：根据不同操作系统调用不同的系统命令
 * - Windows: 调用cls命令清屏
 * - 类Unix系统(Linux/MacOS): 调用clear命令清屏
 */
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// 重命名文件或目录
bool renameFileOrDirectory(const std::string& oldPath, const std::string& newPath) {
#ifdef _WIN32
    return MoveFileA(oldPath.c_str(), newPath.c_str()) != 0;
#else
    return rename(oldPath.c_str(), newPath.c_str()) == 0;
#endif
}

// 获取当前目录下的所有文件和目录（包括子目录）
std::vector<std::string> getAllFilesAndDirectories(const std::string& path) {
    std::vector<std::string> items;
    
#ifdef _WIN32
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA((path + "\\*").c_str(), &findFileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(findFileData.cFileName, ".") != 0 && 
                strcmp(findFileData.cFileName, "..") != 0) {
                std::string itemName = findFileData.cFileName;
                items.push_back(itemName);
            }
        } while (FindNextFileA(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(path.c_str());
    if (dir != NULL) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                std::string itemName = entry->d_name;
                items.push_back(itemName);
            }
        }
        closedir(dir);
    }
#endif
    
    return items;
}

// 检查路径是否为目录
bool isDirectory(const std::string& path) {
#ifdef _WIN32
    DWORD dwAttrib = GetFileAttributesA(path.c_str());
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
           (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat s;
    if(stat(path.c_str(), &s) == 0)
    {
        return S_ISDIR(s.st_mode);
    }
    return false;
#endif
}

/**
 * 在Windows平台下显示保存文件对话框
 * 功能：提供图形界面让用户选择Brainfuck程序文件的保存位置和名称
 * 参数：defaultFileName - 默认的文件名（不包含扩展名）
 * 返回值：用户选择的完整文件路径，如果取消则返回空字符串
 * Windows特定实现：
 * - 使用Windows API的OPENFILENAMEA结构和GetSaveFileNameA函数
 * - 配置了特定的过滤器、默认目录和保存选项
 * 对话框配置：
 * - 默认目录：程序所在目录下的Program文件夹
 * - 文件过滤器：Brainfuck文件 (*.bf)|*.bf|所有文件 (*.*)|*.*
 * - 默认选择第一个过滤器（Brainfuck文件）
 * - 自动添加.bf扩展名（如果用户未提供）
 * - 覆盖现有文件时会显示提示
 * 注意：该函数只在Windows平台下可用，在其他平台上返回空字符串
 */
std::string showSaveFileDialog(const std::string& defaultFileName) {
#ifdef _WIN32
    OPENFILENAMEA ofn;       // 通用文件对话框结构
    char szFile[260] = {0}; // 保存返回的文件路径
    
    // 初始化OPENFILENAME结构
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;      // 所有者窗口（无）
    ofn.lpstrFile = szFile;    // 保存返回的文件路径
    ofn.nMaxFile = sizeof(szFile);
    
    // 设置默认文件名
    if (!defaultFileName.empty()) {
        strcpy(ofn.lpstrFile, (defaultFileName + (defaultFileName.find(".bf") == std::string::npos ? ".bf" : "")).c_str());
    }
    
    // 设置过滤器：Brainfuck文件 (*.bf)|*.bf|所有文件 (*.*)|*.*|
    ofn.lpstrFilter = "Brainfuck Files (*.bf)\0*.bf\0All Files (*.*)\0*.*\0\0";
    ofn.nFilterIndex = 1;      // 默认过滤器索引（1表示第一个）
    ofn.lpstrFileTitle = NULL; // 保存返回的文件名（无）
    ofn.nMaxFileTitle = 0;
    
    // 设置默认保存目录为Program文件夹
    std::string programDir = getExeDir() + "\\Program";
    char szInitialDir[260] = {0};
    strcpy(szInitialDir, programDir.c_str());
    ofn.lpstrInitialDir = szInitialDir;
    
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT; // 文件路径必须存在，覆盖时提示
    
    // 显示保存文件对话框
    if (GetSaveFileNameA(&ofn) == TRUE) {
        return ofn.lpstrFile; // 返回用户选择的文件路径
    }
#endif
    return ""; // 在非Windows平台或用户取消时返回空字符串
}

/**
 * 在Windows平台下显示打开文件对话框
 * 功能：提供图形界面让用户选择并打开一个已有的Brainfuck程序文件
 * 返回值：用户选择的完整文件路径，如果取消则返回空字符串
 * Windows特定实现：
 * - 使用Windows API的OPENFILENAMEA结构和GetOpenFileNameA函数
 * - 配置了特定的过滤器、默认目录和打开选项
 * 对话框配置：
 * - 默认目录：程序所在目录下的Program文件夹
 * - 文件过滤器：Brainfuck文件 (*.bf)|*.bf|所有文件 (*.*)|*.*
 * - 默认选择第一个过滤器（Brainfuck文件）
 * - 要求选择的文件路径和文件必须存在
 * 注意：该函数只在Windows平台下可用，在其他平台上返回空字符串
 */
std::string showOpenFileDialog() {
#ifdef _WIN32
    OPENFILENAMEA ofn;       // 通用文件对话框结构
    char szFile[260] = {0}; // 保存返回的文件路径
    
    // 初始化OPENFILENAME结构
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;      // 所有者窗口（无）
    ofn.lpstrFile = szFile;    // 保存返回的文件路径
    ofn.nMaxFile = sizeof(szFile);
    
    // 设置过滤器：Brainfuck文件 (*.bf)|*.bf|所有文件 (*.*)|*.*|
    ofn.lpstrFilter = "Brainfuck Files (*.bf)\0*.bf\0All Files (*.*)\0*.*\0\0";
    ofn.nFilterIndex = 1;      // 默认过滤器索引（1表示第一个）
    ofn.lpstrFileTitle = NULL; // 保存返回的文件名（无）
    ofn.nMaxFileTitle = 0;
    
    // 设置默认打开目录为Program文件夹
    std::string programDir = getExeDir() + "\\Program";
    char szInitialDir[260] = {0};
    strcpy(szInitialDir, programDir.c_str());
    ofn.lpstrInitialDir = szInitialDir;
    
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; // 文件路径和文件必须存在
    
    // 显示打开文件对话框
    if (GetOpenFileNameA(&ofn) == TRUE) {
        return ofn.lpstrFile; // 返回用户选择的文件路径
    }
#endif
    return ""; // 在非Windows平台或用户取消时返回空字符串
}

// 重命名选中的项目
void renameSelectedItem(const std::string& path) {
    std::string newName;
    printf("%s", tr("enter_new_name").c_str());
    std::cin.ignore(); // 清空输入缓冲区
    std::getline(std::cin, newName);
    
    if (!newName.empty()) {
        // 获取路径的目录部分
        std::string directory = path;
#ifdef _WIN32
        size_t lastSlashPos = path.find_last_of('\\');
#else
        size_t lastSlashPos = path.find_last_of('/');
#endif
        
        if (lastSlashPos != std::string::npos) {
            directory = path.substr(0, lastSlashPos);
        } else {
            directory = ".";
        }
        
        // 构建新路径
        std::string newPath = directory;
#ifdef _WIN32
        newPath += "\\" + newName;
#else
        newPath += "/" + newName;
#endif
        
        if (renameFileOrDirectory(path, newPath)) {
            printf("%s\n", tr("rename_success").c_str());
        } else {
            printf("%s\n", tr("rename_failed").c_str());
        }
    }
    pauseScreen();
}

/**
 * 暂停屏幕
 * 功能：暂停程序执行，等待用户按键继续
 * 平台兼容性：根据不同操作系统采用不同实现
 * - Windows: 调用waitForKey函数等待任意键按下
 * - 类Unix系统(Linux/MacOS): 读取两个字符以确保正确等待用户输入
 * 注意：第二个getchar()是为了消耗可能存在的换行符
 */
void pauseScreen() {
#ifdef _WIN32
    waitForKey();
#else
    getchar();
    getchar();
#endif
}

int find_all_bf(std::string path) {
    // 使用递归搜索
    std::vector<std::string> bfFiles = DirectoryReader::getBFFilesRecursive(path);
    std::cout << trf("found_files", bfFiles.size()) << std::endl;
    for (std::vector<std::string>::size_type i = 0; i < bfFiles.size(); i++) {
        // 显示相对路径
        std::string relativePath = bfFiles[i].substr(path.length() + 1);
        std::cout << i + 1 << ". " << relativePath << std::endl;
    }
    return bfFiles.size();
}

/**
 * 常量定义
 * MEMORY_SIZE: Brainfuck标准内存大小，固定为30000字节
 * PROGRAM_SIZE: 程序最大长度限制，设置为10000字符
 */
const int MEMORY_SIZE = 30000;	// Brainfuck标准内存大小，符合语言规范
const int PROGRAM_SIZE = 10000;	// 程序最大长度限制，防止过长的程序导致性能问题

/**
 * 结构体：存储Brainfuck程序数据
 * 功能：同时保存程序的原始形式和过滤后的形式
 * 成员变量：
 * - original: 原始代码，包含所有注释和格式信息
 * - filtered: 过滤后的代码，只保留8个有效的Brainfuck指令字符（[ ] < > . , + -）
 */
struct ProgramData {
    std::string original;  // 原始代码（包含注释和格式）
    std::string filtered;  // 过滤后的代码（只包含有效Brainfuck指令）
};

/**
 * 显示带颜色的程序代码
 * 功能：在控制台中以不同颜色显示Brainfuck程序代码和注释，提高代码可读性
 * 颜色策略：
 *   - 有效Brainfuck指令字符使用代码颜色（通过applyColors()应用）
 *   - 注释内容使用注释颜色（通过applyCommentColor()应用）
 * 注释识别：
 *   - 多行注释：使用斜杠和星号的组合标记
 *   - 单行注释：使用双斜杠标记
 *   - 其他非指令字符也视为注释内容
 * 参数：
 *   - program：包含注释的完整Brainfuck程序代码
 * 副作用：
 *   - 改变控制台文本颜色
 *   - 在控制台输出格式化的带颜色代码
 * 注意：
 *   - 函数结束时会确保恢复到代码颜色
 *   - 确保控制台支持ANSI颜色代码显示
 */
void displayProgramWithColors(const std::string& program) {
    bool inMultiLineComment = false;
    bool inSingleLineComment = false;
    
    for (std::string::size_type i = 0; i < program.length(); i++) {
        char ch = program[i];
        char next_ch = (i + 1 < program.length()) ? program[i + 1] : '\0';
        
        // 检查多行注释（/* */）
        // 如果不在单行注释中，且遇到/*，则进入多行注释状态并应用注释颜色
        if (!inSingleLineComment && ch == '/' && next_ch == '*') {
            inMultiLineComment = true;
            applyCommentColor(); // 应用注释颜色
            printf("%c", ch); // 输出/字符
            i++; // 跳过下一个字符（*）
            if (i < program.length()) {
                printf("%c", program[i]); // 输出*字符
            }
            continue;
        }
        // 如果在多行注释中，且遇到*/，则退出多行注释状态并恢复代码颜色
        if (inMultiLineComment && ch == '*' && next_ch == '/') {
            inMultiLineComment = false;
            applyCommentColor(); // 应用注释颜色
            printf("%c", ch); // 输出*字符
            i++; // 跳过下一个字符（/）
            if (i < program.length()) {
                printf("%c", program[i]); // 输出/字符
            }
            applyColors(); // 回到代码颜色
            continue;
        }
        if (inMultiLineComment) {
            applyCommentColor();
            printf("%c", ch);
            continue;
        }
        
        // 检查单行注释（//）
        // 如果不在多行注释中，且遇到//，则进入单行注释状态并应用注释颜色
        if (!inMultiLineComment && ch == '/' && next_ch == '/') {
            inSingleLineComment = true;
            applyCommentColor(); // 应用注释颜色
            printf("%c", ch); // 输出第一个/字符
            i++; // 跳过下一个字符（第二个/）
            if (i < program.length()) {
                printf("%c", program[i]); // 输出第二个/字符
            }
            continue;
        }
        // 如果在单行注释中，且遇到换行符，则退出单行注释状态并恢复代码颜色
        if (inSingleLineComment && ch == '\n') {
            inSingleLineComment = false;
            applyColors(); // 回到代码颜色
            printf("%c", ch); // 输出换行符
            continue;
        }
        // 如果在单行注释中，应用注释颜色显示当前字符
        if (inSingleLineComment) {
            applyCommentColor();
            printf("%c", ch);
            continue;
        }
        
        // 检查是否为有效Brainfuck指令字符（[ ] < > . , + -）
        if (ch == '[' || ch == ']' || ch == '<' || ch == '>' || 
            ch == '.' || ch == ',' || ch == '+' || ch == '-') {
            applyColors(); // 使用代码颜色显示
            printf("%c", ch);
        } else {
            // 其他字符视为注释，应用注释颜色显示
            applyCommentColor();
            printf("%c", ch);
        }
    }
    applyColors(); // 确保最后回到代码颜色
}

/**
 * 输入Brainfuck程序并过滤注释和无效字符
 * 功能：接收用户输入的Brainfuck程序代码，并自动过滤出有效的Brainfuck指令
 * 处理流程：
 *   1. 接收用户输入，直到用户输入单独的'0'字符表示结束
 *   2. 保存原始输入（包含所有注释和格式）到ProgramData.original
 *   3. 过滤掉注释和无效字符，只保留有效的Brainfuck指令字符
 * 支持的注释格式：
 *   - 多行注释：使用斜杠和星号的组合标记，可跨越多行
 *   - 单行注释：使用双斜杠标记，从标记开始到行尾
 * 返回值：
 *   - ProgramData结构体，包含:
 *     - original：原始输入代码（包含注释和格式）
 *     - filtered：过滤后的代码（只包含8个有效的Brainfuck指令字符）
 * 使用示例：
 *   用户输入：>++++++++[<+++++++++>-]<.  # 输出 'A'
 *   处理结果：original保留完整输入，filtered只保留 '>++++++++[<+++++++++>-]<.'
 * 注意：
 *   - 单独输入'0'字符将结束输入过程
 *   - 输入过程中会提示用户如何输入和退出
 */
ProgramData input_Bf() {
    ProgramData data;
    std::string input;	// 存储输入的代码
    
    printf("%s\n", tr("input_program").c_str());
    printf("%s\n", tr("comments_supported").c_str());
    
    // 读取输入直到用户输入'0'
    std::string line;
    while (true) {
        std::getline(std::cin, line);
        if (line == "0") {
            break;
        }
        input += line + "\n";
    }
    
    data.original = input;
    
    // 过滤注释和空白，只保留有效指令
    bool inMultiLineComment = false; // 多行注释状态标记
    bool inSingleLineComment = false; // 单行注释状态标记
    
    for (std::string::size_type i = 0; i < input.length(); i++) {
        char ch = input[i];
        char next_ch = (i + 1 < input.length()) ? input[i + 1] : '\0';
        
        // 检查多行注释（/* */）
        // 如果不在单行注释中，且遇到/*，则进入多行注释状态
        if (!inSingleLineComment && ch == '/' && next_ch == '*') {
            inMultiLineComment = true;
            i++; // 跳过下一个字符（*）
            continue;
        }
        // 如果在多行注释中，且遇到*/，则退出多行注释状态
        if (inMultiLineComment && ch == '*' && next_ch == '/') {
            inMultiLineComment = false;
            i++; // 跳过下一个字符（/）
            continue;
        }
        // 如果在多行注释中，跳过当前字符
        if (inMultiLineComment) {
            continue;
        }
        
        // 检查单行注释（//）
        // 如果不在多行注释中，且遇到//，则进入单行注释状态
        if (!inMultiLineComment && ch == '/' && next_ch == '/') {
            inSingleLineComment = true;
            i++; // 跳过下一个字符（/）
            continue;
        }
        // 如果在单行注释中，且遇到换行符，则退出单行注释状态
        if (inSingleLineComment && ch == '\n') {
            inSingleLineComment = false;
            continue;
        }
        // 如果在单行注释中，跳过当前字符
        if (inSingleLineComment) {
            continue;
        }
        
        // 只保留有效的Brainfuck指令字符（[ ] < > . , + -）
        // 这8个字符是Brainfuck语言中唯一有效的指令字符
        if (ch == '[' || ch == ']' || ch == '<' || ch == '>' || 
            ch == '.' || ch == ',' || ch == '+' || ch == '-') {
            data.filtered += ch;
        }
    }
    
    return data;
}

/**
 * 带错误检查执行Brainfuck程序
 * 功能：执行给定的Brainfuck程序代码，提供完整的错误检查和边界条件处理
 * 执行流程：
 * 1. 初始化30000字节的内存空间，所有单元初始化为0
 * 2. 构建循环映射表，验证括号匹配情况
 * 3. 逐指令执行Brainfuck程序，处理所有8种有效指令
 * 错误处理：
 * - 指针越界：当内存指针试图超出30000字节边界时返回错误
 * - 编译错误：当检测到不匹配的括号时返回错误
 * 参数：program - 过滤后的Brainfuck程序代码（只包含8个有效指令字符）
 * 返回值：
 * - 0：执行成功
 * - 1：指针越界错误
 * - 2：编译错误（括号不匹配）
 */
int run(std::string program) {
    unsigned char memory[MEMORY_SIZE] = {0};	// Brainfuck内存数组，大小为30000字节，初始化为0
    int pointer = 0;							// 内存指针，指向当前操作的内存单元位置
    
    // 循环映射数组：存储每个循环指令的匹配位置
    // loop_map[i] = j 表示位置i的循环指令与位置j的循环指令匹配
    int loop_map[PROGRAM_SIZE];
    // 初始化循环映射数组为-1（表示未匹配）
    for(int i = 0; i < PROGRAM_SIZE; i++) loop_map[i] = -1;
    
    std::stack<int> loop_stack;	// 循环栈：用于构建循环映射时临时存储'['的位置
    
    // 第一次扫描：构建循环映射（括号匹配）
    for(int i = 0; i < (int)program.length(); i++) {
        if(program[i] == '[') {
            loop_stack.push(i);	// 将'['的位置压入栈中
        } else if(program[i] == ']') {
            if(loop_stack.empty()) {
                return 2; // 没有匹配的'['，返回编译错误
            }
            int start = loop_stack.top();	// 获取匹配的'['位置
            loop_stack.pop();					// 弹出栈顶元素
            loop_map[start] = i;			// 记录'['匹配的']'位置
            loop_map[i] = start;			// 记录']'匹配的'['位置
        }
    }
    
    // 检查是否有未匹配的'['
    if(!loop_stack.empty()) {
        return 2; // 有未匹配的'['，返回编译错误
    }
    
    // 第二次扫描：执行程序指令
    for(int i = 0; i < (int)program.length(); i++) {
        switch(program[i]) {
            case '>':	// 指针右移指令
                if(pointer == MEMORY_SIZE - 1) {
                    return 1; // 指针已到达内存边界，无法继续右移，返回指针越界错误
                }
                pointer++; // 内存指针向右移动一个单元
                break;
            case '<':	// 指针左移指令
                if(pointer == 0) {
                    return 1; // 指针已在内存起始位置，无法继续左移，返回指针越界错误
                }
                pointer--; // 内存指针向左移动一个单元
                break;
            case '+':	// 当前内存单元加1指令
                memory[pointer]++; // 对当前指针指向的内存单元值加1
                break;
            case '-':	// 当前内存单元减1指令
                memory[pointer]--; // 对当前指针指向的内存单元值减1
                break;
            case '.':	// 输出指令
                printf("%c", memory[pointer]); // 输出当前内存单元值对应的ASCII字符
                break;
            case ',':	// 输入指令
                // 清除输入缓冲区中的换行符
                int c;
                while ((c = getchar()) == '\n') {} // 循环读取直到遇到非换行符
                if (c != EOF) { // 确保读取的不是文件结束符
                    memory[pointer] = static_cast<unsigned char>(c); // 将读取的字符存入当前内存单元
                }
                break;
            case '[':	// 循环开始指令
                // 如果当前内存单元值为0，则跳转到匹配的']'位置（跳过循环体）
                if(memory[pointer] == 0) {
                    i = loop_map[i];	// 跳转至匹配的']'位置
                }
                // 如果当前内存单元值不为0，则继续执行（进入循环体）
                break;
            case ']':	// 循环结束指令
                // 如果当前内存单元值不为0，则跳转回匹配的'['位置（继续循环）
                if(memory[pointer] != 0) {
                    i = loop_map[i];	// 跳转至匹配的'['位置
                }
                // 如果当前内存单元值为0，则继续执行（跳出循环）
                break;
        }
    }
    return 0;
}

/**
 * 执行Brainfuck程序并显示结果
 * 功能：调用run()函数执行程序，并根据执行结果显示相应的消息
 * 处理流程：
 * 1. 显示"运行结果"标题
 * 2. 调用run()函数执行Brainfuck程序
 * 3. 根据run()函数返回的错误码显示相应的多语言提示信息
 * 参数：pro - 过滤后的Brainfuck程序代码（只包含8个有效指令字符）
 * 副作用：在控制台输出执行结果和状态消息
 */
void running(std::string pro){
    printf("\n%s\n", tr("run_results").c_str());
    int res=run(pro);
    if(res==0) printf("\n%s\n", tr("run_success").c_str());
    if(res==1) printf("\n%s\n", tr("pointer_error").c_str());
    if(res==2) printf("\n%s\n", tr("compile_error").c_str());
}

/**
 * 保存程序到文件
 * 功能：将Brainfuck程序代码（包含注释和过滤后的代码）保存到指定目录下的.bf文件
 * 处理流程：
 * 1. 获取可执行文件所在目录，并构建Program子目录路径
 * 2. 如果Program目录不存在，则创建它
 * 3. 构建完整的文件路径（目录+文件名+.bf扩展名）
 * 4. 将原始代码和过滤后的代码写入文件，包含适当的注释头
 * 平台兼容性：根据不同操作系统使用不同的路径分隔符
 * - Windows: 使用反斜杠(\)作为路径分隔符
 * - 类Unix系统: 使用正斜杠(/)作为路径分隔符
 * 参数：
 * - filename: 保存的文件名（不包含扩展名）
 * - programData: 包含原始代码和过滤后代码的ProgramData结构体
 * 返回值：true表示保存成功，false表示保存失败
 * 副作用：在控制台输出保存成功或失败的消息
 */
bool saveProgram(std::string filename, ProgramData programData) {
    std::string programDir = getExeDir();
#ifdef _WIN32
    programDir += "\\Program";
    // 创建Program目录（如果不存在）
    createDirectory(programDir);
    
    std::string fullPath = programDir + "\\" + filename + ".bf";
#else
    programDir += "/Program";
    // 创建Program目录（如果不存在）
    createDirectory(programDir);
    
    std::string fullPath = programDir + "/" + filename + ".bf";
#endif
    
    std::ofstream file(fullPath.c_str());
    if (file.is_open()) {
        // 写入原始代码（包含注释）
        file << "/* Brainfuck Program with Comments */\n";
        file << "/* Saved from Brainfuck IDE */\n\n";
        file << programData.original;
        file << "\n\n/* Filtered executable code: */\n";
        file << "/* " << programData.filtered << " */\n";
        
        file.close();
        printf("%s %s\n", tr("save_success").c_str(), fullPath.c_str());
        return true;
    } else {
        printf("%s\n", tr("save_failed").c_str());
        return false;
    }
}

/**
 * 从文件加载程序
 * 功能：从指定文件加载Brainfuck程序代码，过滤IDE自动添加的注释，并提取有效的Brainfuck指令
 * 处理流程：
 *   1. 打开指定的文件路径
 *   2. 逐行读取文件内容
 *   3. 跳过文件头部由IDE自动添加的注释行
 *   4. 处理用户添加的多行注释和单行注释
 *   5. 保存过滤后的原始代码（不含IDE注释）到ProgramData.original
 *   6. 同时提取并保存有效的Brainfuck指令字符到ProgramData.filtered
 * 参数：
 *   - filename：要加载的文件完整路径
 * 返回值：
 *   - ProgramData结构体，包含:
 *     - original：过滤掉IDE注释后的原始代码（保留用户添加的注释格式）
 *     - filtered：只包含8个有效Brainfuck指令字符的代码
 * 注意：
 *   - 如果文件无法打开，将返回空的ProgramData结构体
 *   - 支持的注释格式包括多行注释和单行注释
 */
ProgramData loadProgram(const std::string& filename) {
    ProgramData data; // 初始化返回的ProgramData结构体
    std::ifstream file(filename.c_str()); // 打开指定文件
    if (file.is_open()) { // 检查文件是否成功打开
        std::string line; // 用于存储每一行的内容
        bool inCommentBlock = false; // 多行注释块状态标记
        
        while (std::getline(file, line)) { // 逐行读取文件内容
            // 跳过文件头部的IDE自动添加的注释行
            if (line.find("/* Brainfuck Program with Comments */") != std::string::npos ||
                line.find("/* Saved from Brainfuck IDE */") != std::string::npos ||
                line.find("/* Filtered executable code: */") != std::string::npos) {
                continue; // 跳过这一行，继续下一行
            }
            
            // 检查是否进入多行注释块的开始
            if (line.length() >= 2 && line.substr(0, 2) == "/*") {
                inCommentBlock = true; // 进入多行注释状态
            }
            
            // 检查是否到达多行注释块的结束
            if (inCommentBlock && line.length() >= 2 && line.substr(line.length()-2) == "*/") {
                inCommentBlock = false; // 退出多行注释状态
                continue; // 跳过这一行，继续下一行
            }
            
            // 如果在多行注释块内，跳过当前行
            if (inCommentBlock) {
                continue;
            }
            
            // 添加入除IDE注释外的原始代码行
            data.original += line + "\n"; // 将当前行添加到原始代码中
            
            // 同时过滤有效指令字符
            for (std::string::size_type i = 0; i < line.length(); i++) {
                char ch = line[i];
                // 只保留8个有效的Brainfuck指令字符
                if (ch == '[' || ch == ']' || ch == '<' || ch == '>' || 
                    ch == '.' || ch == ',' || ch == '+' || ch == '-') {
                    data.filtered += ch; // 将有效指令添加到过滤后的代码中
                }
            }
        }
        file.close(); // 关闭文件
    } else {
        printf("无法打开文件: %s\n", filename.c_str()); // 文件打开失败时输出错误信息
    }
    return data;
}

// 重命名选中的文件或目录
void renameSelectedItem(const std::string& path, std::string& itemName) {
    clearScreen();
    printf("%s '%s'\n", tr("rename_item").c_str(), itemName.c_str());
    printf("%s: ", tr("enter_new_name").c_str());
    
    // 清空输入缓冲区
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    
    std::string newName;
    std::getline(std::cin, newName);
    
    if (!newName.empty() && newName != itemName) {
        std::string oldPath, newPath;
#ifdef _WIN32
        oldPath = path + "\\" + itemName;
        newPath = path + "\\" + newName;
#else
        oldPath = path + "/" + itemName;
        newPath = path + "/" + newName;
#endif
        
        if (renameFileOrDirectory(oldPath, newPath)) {
            printf("%s: '%s' -> '%s'\n", tr("item_renamed").c_str(), itemName.c_str(), newName.c_str());
            itemName = newName;
        } else {
            printf("%s\n", tr("rename_failed").c_str());
        }
        pauseScreen();
    }
}

/**
 * Brainfuck程序编辑器主界面
 * 功能：提供交互式界面让用户编辑、运行、保存Brainfuck程序
 * 主要功能：
 * - 创建新的Brainfuck程序
 * - 运行已编辑的程序
 * - 保存程序到文件
 * - 清除当前程序
 * - 更改语言设置
 * 参数：
 * - programData: 可选参数，用于初始加载的程序数据，默认为空
 * - fileName: 可选参数，初始文件名，默认为空字符串
 * - filePath: 可选参数，初始文件完整路径，默认为空字符串
 * 处理流程：
 * 1. 初始化编辑器状态变量
 * 2. 创建编辑器菜单选项（编辑、运行、保存、清除、语言设置、返回）
 * 3. 进入编辑器主循环，等待用户选择操作
 * 4. 根据用户选择执行相应的功能
 * 5. 当用户选择返回主菜单时退出循环
 * 副作用：
 * - 修改全局语言设置（通过languageSettings函数）
 * - 修改全局颜色设置（通过WinConsoleMenu的运行过程）
 * - 在控制台显示各种界面元素和消息
 */
void createEditor(ProgramData programData = ProgramData(), std::string fileName = "", std::string filePath = "") {
    bool editing = true; // 控制编辑器循环的标志
    std::string currentFileName = fileName;
    std::string currentFilePath = filePath; // 存储完整的文件路径
    
    while(editing) { // 编辑器主循环
        // 创建编辑器命令菜单
        std::vector<std::string> editorOptions;
        editorOptions.push_back(tr("edit_program"));
        editorOptions.push_back(tr("run_program"));
        editorOptions.push_back(tr("save_program"));
        editorOptions.push_back(tr("clear_program"));
        editorOptions.push_back(tr("language_settings_editor"));
        editorOptions.push_back(tr("back_menu"));
        
        // 创建并运行编辑器菜单
        WinConsoleMenu editorMenu(tr("program_interface"), editorOptions, currentFileName, currentFilePath, programData.original);
        int choice = editorMenu.run(backgroundColor, codeColor);
        
        // 根据用户选择执行相应操作
        switch(choice) {
            case 0: { // 编辑程序
                printf("%s\n", tr("input_program").c_str()); // 提示输入程序
                // 清空输入缓冲区中的换行符
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
                
                programData = input_Bf(); // 调用input_Bf函数获取用户输入的程序
                // 重置文件名和路径，因为用户正在创建一个新的未命名文件
                currentFileName = "";
                currentFilePath = "";
                break;
            }
            case 1: // 运行程序
                if (!programData.filtered.empty()) { // 检查程序是否为空
                    running(programData.filtered); // 调用running函数运行程序
                } else {
                    printf("%s\n", tr("program_empty").c_str()); // 提示程序为空
                }
                pauseScreen(); // 暂停屏幕等待用户按键
                break;
            case 2: // 保存程序
                if (!programData.original.empty()) { // 检查程序是否为空
                    // 获取程序目录
                    std::string programDir = getExeDir();
#ifdef _WIN32
                    programDir += "\\Program";
#else
                    programDir += "/Program";
#endif
                    
                    // 创建Program目录（如果不存在）
                    createDirectory(programDir);
                    
                    // 在Windows平台下使用Windows文件保存对话框，在其他平台下使用原来的目录浏览
#ifdef _WIN32
                    std::string fullPath = showSaveFileDialog(currentFileName);
#else
                    std::string fullPath = browseDirectoryAndManage(programDir, true, currentFileName);
#endif
                    
                    if (!fullPath.empty()) {
                        // 写入原始代码（包含注释）
                        std::ofstream file(fullPath.c_str());
                        if (file.is_open()) {
                            file << "/* Brainfuck Program with Comments */\n";
                            file << "/* Saved from Brainfuck IDE */\n\n";
                            file << programData.original;
                            file << "\n\n/* Filtered executable code: */\n";
                            file << "/* " << programData.filtered << " */\n";
                            
                            file.close();
                            printf("%s %s\n", tr("save_success").c_str(), fullPath.c_str());
                            
                            // 保存完整路径
                            currentFilePath = fullPath;
                            
                            // 更新当前文件名（不带路径和扩展名）
#ifdef _WIN32
                            std::string::size_type lastBackslashPos = fullPath.find_last_of('\\');
                            if (lastBackslashPos != std::string::npos) {
                                currentFileName = fullPath.substr(lastBackslashPos + 1);
                            } else {
                                currentFileName = fullPath;
                            }
#else
                            std::string::size_type lastSlashPos = fullPath.find_last_of('/');
                            if (lastSlashPos != std::string::npos) {
                                currentFileName = fullPath.substr(lastSlashPos + 1);
                            } else {
                                currentFileName = fullPath;
                            }
#endif
                            
                            // 移除扩展名
                            if (currentFileName.length() > 3) {
                                currentFileName = currentFileName.substr(0, currentFileName.length() - 3);
                            }
                        } else {
                            printf("%s\n", tr("save_failed").c_str());
                        }
                    }
                } else {
                    printf("%s\n", tr("program_empty").c_str()); // 提示程序为空
                }
                pauseScreen(); // 暂停屏幕等待用户按键
                break;
            case 3: // 清除程序
                programData.original.clear(); // 清空原始代码
                programData.filtered.clear(); // 清空过滤后的代码
                currentFileName = ""; // 清空当前文件名
                printf("%s\n", tr("program_cleared").c_str()); // 提示程序已清除
                pauseScreen(); // 暂停屏幕等待用户按键
                break;
            case 4: // 语言设置
                languageSettings(); // 调用languageSettings函数进入语言设置
                break;
            case 5: // 返回主菜单
                editing = false; // 设置循环标志为false，退出编辑器循环
                break;
            default: // 无效选择或取消
                break;
        }
    }
}

/**
 * 创建新程序
 * 功能：打开一个新的编辑器实例，供用户创建新的Brainfuck程序
 * 调用流程：
 *   1. 直接调用createEditor()函数，传入默认参数
 *   2. createEditor()函数会创建一个空的编辑器环境并显示
 * 使用场景：
 *   - 用户希望开始编写全新的Brainfuck程序时
 *   - 需要一个干净的编辑环境时
 * 交互过程：
 *   1. 用户调用此函数
 *   2. 系统立即创建新的编辑器实例
 *   3. 用户可以直接在编辑器中输入Brainfuck代码
 *   4. 支持保存、执行等所有编辑器功能
 * 注意：
 *   - 该函数是一个简洁的入口点，实际功能由createEditor()实现
 *   - 调用此函数将覆盖当前正在编辑的程序（如果有）
 */
void create() {
    createEditor();
}

/**
 * 打开已存在的程序
 * 功能：提供界面让用户选择并打开一个已有的Brainfuck程序文件
 * 平台特定实现：
 *   - Windows平台：使用Windows文件对话框（showOpenFileDialog()）
 *   - 非Windows平台：使用递归搜索和自定义菜单选择
 * 处理流程：
 *   1. 获取用户选择的BF文件路径
 *   2. 显示所选文件的完整路径
 *   3. 调用loadProgram()函数加载程序内容
 *   4. 如果程序不为空，显示带颜色的程序内容并等待用户进入编辑器
 *   5. 从文件路径中提取文件名（不含路径和扩展名）
 *   6. 调用createEditor()函数打开编辑器，并传递程序数据、文件名和完整路径
 * 参数：无
 * 副作用：
 *   - 在控制台显示文件选择界面和程序内容
 *   - 可能会创建并进入编辑器环境
 * 返回值：无
 * 注意：
 *   - 支持打开.bf或其他文本文件格式的Brainfuck程序
 *   - 如果文件无法打开或不包含有效的Brainfuck代码，会显示相应的错误信息
 */
void open() {
#ifdef _WIN32
    // 在Windows平台上使用文件对话框
    std::string selectedFile = showOpenFileDialog();
    
    if (selectedFile.empty()) {
        // 用户取消
        return;
    }
    
    clearScreen();
    printf("%s\n", tr("open_program").c_str());
    printf("%s\n", "------------------------");
#else
    // 非Windows平台使用原有的目录浏览方式
    std::string programDir = getExeDir();
#ifdef _WIN32
    programDir += "\\Program";
#else
    programDir += "/Program";
#endif
    
    // 使用递归搜索所有子目录中的 .bf 文件
    std::vector<std::string> bfFiles = DirectoryReader::getBFFilesRecursive(programDir);
    int fileCount = bfFiles.size();
    
    if (fileCount == 0) {
        printf("%s\n", tr("no_bf_files").c_str());
        pauseScreen();
        return;
    }
    
    // 创建文件选择菜单
    std::vector<std::string> fileOptions;
    fileOptions.push_back(tr("back_menu")); // 添加返回选项
    
    for (std::vector<std::string>::size_type i = 0; i < bfFiles.size(); i++) {
        // 显示相对路径（去掉共同目录前缀）
        std::string relativePath = bfFiles[i].substr(programDir.length() + 1);
        fileOptions.push_back(relativePath);
    }
    
    // 创建并运行文件选择菜单 - 注意这里我们使用自定义的文件名显示方式
    WinConsoleMenu fileMenu(trf("found_files", fileCount), fileOptions);
    int choice = fileMenu.run(backgroundColor, codeColor);
    
    if (choice <= 0 || choice > fileCount) { // 0是返回选项，1开始是文件
        return;
    }
    
    // 获取选择的文件
    std::string selectedFile = bfFiles[choice - 1];
#endif
    
    // 显示完整路径
    printf("%s: %s\n", tr("current_file").c_str(), selectedFile.c_str());
    
    // 加载程序
    ProgramData programData = loadProgram(selectedFile);
    if (!programData.original.empty()) {
        printf("%s\n", "程序内容 :");
        displayProgramWithColors(programData.original);
        printf("%s", tr("enter_to_editor").c_str());
        pauseScreen();
        
        // 从完整路径中提取文件名
        std::string fileName = "";
        std::string filePath = selectedFile;
        
#ifdef _WIN32
        std::string::size_type lastBackslashPos = selectedFile.find_last_of('\\');
        if (lastBackslashPos != std::string::npos) {
            fileName = selectedFile.substr(lastBackslashPos + 1);
        } else {
            fileName = selectedFile;
        }
#else
        std::string::size_type lastSlashPos = selectedFile.find_last_of('/');
        if (lastSlashPos != std::string::npos) {
            fileName = selectedFile.substr(lastSlashPos + 1);
        } else {
            fileName = selectedFile;
        }
#endif
        
        // 移除扩展名
        if (fileName.length() > 3) {
            fileName = fileName.substr(0, fileName.length() - 3);
        }
        
        // 进入编辑器进行编辑，传递文件名和路径
        // createEditor函数会在界面上方显示文件名、路径和程序内容
        createEditor(programData, fileName, filePath);
    } else {
        printf("%s\n", tr("file_empty").c_str());
        pauseScreen();
    }
}

/**
 * 程序主入口
 * 功能：程序的起点，负责初始化环境、加载设置并显示主菜单
 * 初始化流程：
 * 1. 在Windows平台下设置控制台为UTF-8编码和支持多语言的字体
 * 2. 初始化翻译映射表和语言名称映射
 * 3. 初始化颜色名称映射和颜色映射
 * 4. 创建Program目录（如果不存在）
 * 5. 加载用户的编辑器设置
 * 主循环处理：
 * 1. 创建主菜单选项（创建程序、打开程序、编辑器设置、退出）
 * 2. 显示主菜单并获取用户选择
 * 3. 根据用户选择执行相应的功能
 * 4. 当用户选择退出时，重置颜色并返回退出状态码
 * 返回值：程序退出状态码（0表示正常退出）
 * 副作用：
 * - 修改控制台设置（仅Windows平台）
 * - 创建程序目录
 * - 加载和保存用户设置
 * - 在控制台显示主菜单和各种状态消息
 */
int main(){ // 主函数入口，初始化设置并显示主菜单
    // 在Windows环境下设置控制台为UTF-8编码和支持阿拉伯语的字体
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8); // 设置输出代码页为UTF-8
        SetConsoleCP(CP_UTF8);       // 设置输入代码页为UTF-8
        
        // 设置控制台字体为支持UTF-8和RTL的字体
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_FONT_INFOEX cfi;
        cfi.cbSize = sizeof(CONSOLE_FONT_INFOEX);
        if (GetCurrentConsoleFontEx(hConsole, FALSE, &cfi)) {
            // 尝试使用Consolas字体，它支持UTF-8和多种语言
            wcscpy(cfi.FaceName, L"Consolas");
            // 设置字体大小为12
            cfi.dwFontSize.X = 8;
            cfi.dwFontSize.Y = 16;
            SetCurrentConsoleFontEx(hConsole, FALSE, &cfi);
        }
        
        // 尝试设置控制台模式以支持RTL文本
        DWORD consoleMode;
        if (GetConsoleMode(hConsole, &consoleMode)) {
            // 定义ENABLE_VIRTUAL_TERMINAL_PROCESSING常量（通常在较新的Windows SDK中定义）
            #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
            #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
            #endif
            // 尝试添加ENABLE_VIRTUAL_TERMINAL_PROCESSING模式，如果支持的话
            // 这可能有助于更好地处理Unicode文本
            consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hConsole, consoleMode);
        }
    #endif
    
    // 初始化翻译映射
    initializeTranslations();
    initializeLanguageNames();
    initializeColorNames();
    initializeColorMaps();
    
    // 创建Program目录
    std::string programDir = getExeDir();
#ifdef _WIN32
    programDir += "\\Program";
#else
    programDir += "/Program";
#endif
    createDirectory(programDir);
    
    // 加载编辑器设置
    loadSettings();
    
    // 创建主菜单选项
    std::vector<std::string> menuOptions;
    menuOptions.push_back(tr("create_program"));
    menuOptions.push_back(tr("open_program"));
    menuOptions.push_back(tr("editor_settings"));
    menuOptions.push_back(tr("exit_program"));
    
    // 创建主菜单
    WinConsoleMenu mainMenu(tr("main_menu_title"), menuOptions);
    
    while(true) {
        clearScreen();
        int choice = mainMenu.run(backgroundColor, codeColor);
        
        switch(choice) {
            case 0:
                create();
                break;
            case 1:
                open();
                break;
            case 2:
                editorSettings();
                break;
            case 3:
                printf("%s", tr("exit_message").c_str());
                #ifdef _WIN32
                    Sleep(200);
                #endif
                resetColors(); // 退出前恢复默认颜色
                return 0;
            default:
                break;
        }
    }
    return 0;
}
