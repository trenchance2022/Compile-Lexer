#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <set>
#include <string>
#include "parser.h"

// 符号表
std::set<std::string> symbolTable;  // 用于存放唯一的标识符

// 两个缓冲区
char buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE];
char *forward = buffer1; // 前向指针
int isBuffer1Active = 1; // 判断当前活跃缓冲区

// 关键字集合
const char *keywords[] = {
        "auto", "break", "case", "char", "const", "continue", "default", "do", "double",
        "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int",
        "long", "register", "restrict", "return", "short", "signed", "sizeof",
        "static", "struct", "switch", "typedef", "union", "unsigned", "void",
        "volatile", "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex",
        "_Generic", "_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local", NULL
};


// 统计信息
int lineCount = 1;          // 代码行数（初始化为1，因为至少有一行）
int identifierCount = 0;    // 标识符的数量（唯一）
int numberCount = 0;        // 数字的数量
int operatorCount = 0;      // 操作符的数量
int commentCount = 0;       // 注释的数量
int totalValidCharCount = 0; // 有效字符总数（包括重复的标识符、数字等）
int currentLine = 1;     // 当前行号
int currentColumn = 1;   // 当前列号

// 判断是否是关键字
int isKeyword(const char *word) {
    for (int i = 0; keywords[i] != NULL; i++) {
        if (strcmp(word, keywords[i]) == 0) {
            return 1;  // 是关键字
        }
    }
    return 0;  // 不是关键字
}


// 判断是否为操作符的单字符
int isOperator(char ch) {
    // 判断单字符是否为合法的操作符
    return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%' ||
           ch == '=' || ch == '!' || ch == '<' || ch == '>' || ch == '&' ||
           ch == '|' || ch == '^' || ch == '~' || ch == '?' || ch == ':';
}

// 分隔符集合
int isSeparator(char ch) {
    return ch == '{' || ch == '}' || ch == '(' || ch == ')' ||
           ch == '[' || ch == ']' || ch == ',' || ch == ';' ||
           ch == '.' || ch == ':' || ch == '#';
}


// 错误报告函数
void reportError(const char *errorMessage) {
    printf("Error at line %d, column %d: %s\n", lineCount, currentColumn, errorMessage);
}

// 输出Token
void printToken(Token token) {
    switch (token.type) {
        case IDENTIFIER:
            printf("<IDENTIFIER, %s>\n", token.value);
            break;
        case NUMBER:
            printf("<NUMBER, %s>\n", token.value);
            break;
        case OPERATOR:
            printf("<OPERATOR, %s>\n", token.value);
            break;
        case CHAR_CONSTANT:
            printf("<CHAR_CONSTANT, %s>\n", token.value);
            break;
        case STRING_CONSTANT:
            printf("<STRING_CONSTANT, %s>\n", token.value);
            break;
        case KEYWORD:
            printf("<KEYWORD, %s>\n", token.value);
            break;
        case PREPROCESSOR_DIRECTIVE:
            printf("<PREPROCESSOR_DIRECTIVE, %s>\n", token.value);
            break;
        case HEADER_FILE:
            printf("<HEADER_FILE, %s>\n", token.value);
            break;
        case COMMENT:
            printf("<COMMENT, %s>\n", token.value);
            break;
        case SEPARATOR:
            printf("<SEPARATOR, %s>\n", token.value);
            break;
        case MACRO_DEFINITION:
            printf("<MACRO_DEFINITION, %s>\n", token.value);
            break;
        case MACRO_PARAMETER:
            printf("<MACRO_PARAMETER, %s>\n", token.value);
            break;
        case MACRO_VALUE:
            printf("<MACRO_VALUE, %s>\n", token.value);
            break;
        default:
            printf("<UNKNOWN, %s>\n", token.value);
            break;
    }
}

// 装载新的缓冲区
void loadBuffer(char *buffer, FILE *sourceCode) {
    int bytesRead = fread(buffer, sizeof(char), BUFFER_SIZE - 1, sourceCode);
    if (bytesRead < BUFFER_SIZE - 1) {
        // 如果读取的字节数少于缓冲区大小，说明到达文件末尾，插入 EOF 标记
        buffer[bytesRead] = EOF;
    } else {
        buffer[BUFFER_SIZE - 1] = EOF;  // 正常情况下，在缓冲区末尾插入 EOF
    }
}

// 切换缓冲区
void switchBuffer(FILE *sourceCode) {
    if (isBuffer1Active) {
        loadBuffer(buffer2, sourceCode);
        forward = buffer2;
        isBuffer1Active = 0;
    } else {
        loadBuffer(buffer1, sourceCode);
        forward = buffer1;
        isBuffer1Active = 1;
    }
}

// 处理字符常量
Token parseCharConstant() {
    char tokenValue[MAX_TOKEN_SIZE];
    int index = 0;
    Token token;
    int state = 0;  // 初始状态

    while (1) {
        char ch = *forward;

        // 文件结束检查
        if (ch == EOF) {
            reportError("Unexpected EOF in character constant");
            token.type = UNKNOWN;
            return token;
        }

        // 行号检查和更新
        if (ch == '\n') {
            currentLine++;
            currentColumn = 1;  // 重置列号
        }

        switch (state) {
            case 0:  // 初始状态，跳过第一个单引号
                if (ch == '\'') {
                    forward++;
                    currentColumn++;
                    state = 1;  // 进入处理字符状态
                } else {
                    reportError("Expected single quote at the beginning of character constant");
                    token.type = UNKNOWN;
                    return token;
                }
                break;

            case 1:  // 处理字符，判断是普通字符还是转义字符
                if (ch == '\\') {  // 处理转义字符
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 2;  // 进入处理转义字符状态
                } else if (ch != '\'' && ch != EOF && ch != '\n') {  // 处理普通字符
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 3;  // 准备进入闭合状态
                } else if (ch == '\n') {  // 如果字符常量中遇到换行符则报错
                    reportError("Unclosed character constant");
                    token.type = UNKNOWN;
                    return token;
                } else {
                    reportError("Empty or invalid character constant");
                    token.type = UNKNOWN;
                    return token;
                }
                break;

            case 2:  // 处理转义字符后的字符
                if (ch != EOF && ch != '\n') {
                    tokenValue[index++] = ch;  // 记录转义后的字符
                    forward++;
                    currentColumn++;
                    state = 3;  // 准备进入闭合状态
                } else if (ch == '\n') {  // 如果转义字符未闭合就遇到换行符
                    reportError("Unclosed character constant after escape sequence");
                    token.type = UNKNOWN;
                    return token;
                } else {
                    reportError("Unclosed character constant after escape sequence");
                    token.type = UNKNOWN;
                    return token;
                }
                break;

            case 3:  // 检查闭合单引号
                if (ch == '\'') {
                    forward++;
                    currentColumn++;
                    tokenValue[index] = '\0';  // 结束字符常量
                    token.type = CHAR_CONSTANT;
                    strcpy(token.value, tokenValue);
                    totalValidCharCount += strlen(token.value);  // 更新字符总数
                    return token;  // 成功解析
                } else {
                    reportError("Unclosed character constant");
                    token.type = UNKNOWN;
                    return token;
                }
        }

        // 更新列号
        if (ch != '\n' && ch != EOF) {
            currentColumn++;
        }
    }
}

// 处理字符串常量
Token parseStringConstant() {
    char tokenValue[MAX_TOKEN_SIZE];
    int index = 0;
    Token token;
    int state = 0;  // 初始状态

    forward++;  // 跳过第一个双引号
    currentColumn++;

    while (1) {
        char ch = *forward;

        // 文件结束检查
        if (ch == EOF) {
            reportError("Unexpected EOF in string constant");
            token.type = UNKNOWN;
            return token;
        }

        switch (state) {
            case 0:  // 正常读取字符
                if (ch == '"') {  // 如果遇到结束的双引号
                    forward++;
                    currentColumn++;
                    tokenValue[index] = '\0';  // 结束字符串
                    token.type = STRING_CONSTANT;
                    strcpy(token.value, tokenValue);
                    totalValidCharCount += strlen(token.value);  // 更新有效字符数
                    return token;  // 成功解析字符串常量
                } else if (ch == '\\') {  // 如果遇到转义字符
                    state = 1;  // 转移到处理转义字符的状态
                    forward++;
                    currentColumn++;
                } else if (ch == '\n') {  // 如果遇到换行符，但没有闭合双引号
                    reportError("Unclosed string constant");
                    token.type = UNKNOWN;
                    return token;
                } else {  // 普通字符
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                }
                break;

            case 1:  // 处理转义字符
                // 处理常见的转义字符
                if (ch == 'n' || ch == 't' || ch == '\\' || ch == '"' || ch == '\'') {
                    tokenValue[index++] = '\\';  // 保留反斜杠
                    tokenValue[index++] = ch;  // 保留转义后的字符
                } else {
                    reportError("Invalid escape sequence");
                    token.type = UNKNOWN;
                    return token;
                }
                forward++;
                currentColumn++;
                state = 0;  // 返回正常读取字符的状态
                break;
        }

        // 检查是否到达文件末尾
        if (*forward == EOF) {
            break;
        }
    }

    token.type = UNKNOWN;
    return token;
}


Token parsePreprocessorDirective() {
    char tokenValue[MAX_TOKEN_SIZE];
    int index = 0;
    Token token;

    // 跳过空白符
    while (isspace(*forward)) {
        forward++;
        currentColumn++;
    }

    // 解析预处理指令
    while (isalnum(*forward)) {
        tokenValue[index++] = *forward;
        forward++;
        currentColumn++;
    }
    tokenValue[index] = '\0';

    // 判断是否为 #define 指令
    if (strcmp(tokenValue, "define") == 0) {
        token.type = PREPROCESSOR_DIRECTIVE;
        strcpy(token.value, "#define");
        totalValidCharCount += strlen(token.value);

        // 返回预处理指令Token
        return token;

    } else if (strcmp(tokenValue, "include") == 0) {
        token.type = PREPROCESSOR_DIRECTIVE;
        strcpy(token.value, "#include");
        totalValidCharCount += strlen(token.value);

        // 返回 #include 指令Token
        return token;
    } else {
        reportError("Unknown preprocessor directive");
        token.type = UNKNOWN;
        return token;
    }
}


Token parseMacroDefinition() {
    char tokenValue[MAX_TOKEN_SIZE];
    int index = 0;
    Token token;

    // 跳过空白符，准备解析宏名称
    while (isspace(*forward)) {
        forward++;
        currentColumn++;
    }

    // 解析宏名称
    while (isalnum(*forward) || *forward == '_') {  // 宏名称可以包含字母、数字和下划线
        tokenValue[index++] = *forward;
        forward++;
        currentColumn++;
    }
    tokenValue[index] = '\0';

    // 构建宏定义名称Token
    token.type = MACRO_DEFINITION;
    strcpy(token.value, tokenValue);
    totalValidCharCount += strlen(token.value);

    Token macroNameToken = token;  // 保存宏名称Token

    // 检查是否有宏参数 (如 #define MAX(a, b))
    if (*forward == '(') {
        forward++;  // 跳过 '('
        currentColumn++;

        // 解析宏参数
        index = 0;
        while (*forward != ')' && *forward != EOF) {
            tokenValue[index++] = *forward;
            forward++;
            currentColumn++;
        }

        // 检查是否遇到匹配的 ')'
        if (*forward == ')') {
            forward++;  // 跳过 ')'
            currentColumn++;
        } else {
            reportError("Unclosed macro parameter list");
            token.type = UNKNOWN;
            return token;
        }

        tokenValue[index] = '\0';

        // 创建宏参数的Token
        token.type = MACRO_PARAMETER;
        strcpy(token.value, tokenValue);
        totalValidCharCount += strlen(token.value);
        printToken(macroNameToken);  // 打印宏名称Token
        printToken(token);  // 打印宏参数Token
    } else {
        printToken(macroNameToken);  // 直接打印宏名称Token（无参数）
    }

    // 解析宏定义的值部分
    index = 0;
    while (*forward != '\n' && *forward != EOF) {
        // 检查是否遇到注释
        if (*forward == '/' && *(forward + 1) == '/') {
            break;  // 遇到单行注释，停止解析宏定义值
        } else if (*forward == '/' && *(forward + 1) == '*') {
            break;  // 遇到多行注释，停止解析宏定义值
        }

        // 跳过行首的多余空白，并将宏定义的值保存到 tokenValue 中
        if (!isspace(*forward) || (isspace(*forward) && index > 0)) {
            tokenValue[index++] = *forward;
        }
        forward++;
        currentColumn++;
    }
    tokenValue[index] = '\0';

    // 构建宏定义值Token
    token.type = MACRO_VALUE;
    strcpy(token.value, tokenValue);
    totalValidCharCount += strlen(token.value);

    return token;
}



Token parseHeaderFile() {
    char tokenValue[MAX_TOKEN_SIZE];
    int index = 0;
    Token token;
    char boundary;  // 用于保存结束符号 ('>' 或 '"')

    // 跳过空白符，准备解析文件路径
    while (isspace(*forward)) {
        forward++;
        currentColumn++;
    }

    // 检测文件路径的开始标志，可能是 '<' 或 '"'
    if (*forward == '<') {
        boundary = '>';  // 如果起始符号是 '<'，结束符号应该是 '>'
        forward++;  // 跳过 '<'
        currentColumn++;
    } else if (*forward == '"') {
        boundary = '"';  // 如果起始符号是 '"', 结束符号应该是 '"'
        forward++;  // 跳过 '"'
        currentColumn++;
    } else {
        // 如果没有 '<' 或 '"' 开始，报错
        reportError("Expected '<' or '\"' after #include");
        token.type = UNKNOWN;
        return token;
    }

    // 开始解析路径，直到遇到对应的闭合符号 '>' 或 '"'
    while (*forward != boundary && *forward != EOF) {
        // 防止路径过长导致缓冲区溢出
        if (index >= MAX_TOKEN_SIZE - 1) {
            reportError("Header file path too long");
            token.type = UNKNOWN;
            return token;
        }

        // 解析路径字符
        tokenValue[index++] = *forward;
        forward++;
        currentColumn++;
    }

    // 检查是否遇到匹配的闭合符号
    if (*forward == boundary) {
        tokenValue[index] = '\0';  // 结束路径字符串
        forward++;  // 跳过闭合符号
        currentColumn++;

        // 返回头文件路径的Token
        token.type = HEADER_FILE;
        strcpy(token.value, tokenValue);
        totalValidCharCount += strlen(token.value);  // 更新字符计数
        return token;
    } else {
        // 未找到匹配的闭合符号，报错
        reportError("Unclosed header file path");
        token.type = UNKNOWN;
        return token;
    }
}


// 解析注释
Token parseComment() {
    Token token;
    char tokenValue[MAX_TOKEN_SIZE];
    int index = 0;

    if (*forward == '/' && *(forward + 1) == '/') {
        forward += 2;  // 跳过 `//`
        currentColumn += 2;

        // 读取单行注释
        while (*forward != '\n' && *forward != EOF) {
            tokenValue[index++] = *forward;
            forward++;
            currentColumn++;
        }
        tokenValue[index] = '\0';
        token.type = COMMENT;
        strcpy(token.value, tokenValue);
        totalValidCharCount += strlen(token.value);  // 更新字符总数
        commentCount++;
    } else if (*forward == '/' && *(forward + 1) == '*') {
        forward += 2;  // 跳过 `/*`
        currentColumn += 2;

        // 读取多行注释
        while (!(*forward == '*' && *(forward + 1) == '/') && *forward != EOF) {
            tokenValue[index++] = *forward;
            forward++;
            currentColumn++;
            if (*forward == '\n') {
                lineCount++;  // 处理多行注释中的换行符
                currentColumn = 0;
            }
        }
        if (*forward == EOF) {
            reportError("Unclosed comment");
        } else {
            forward += 2;  // 跳过 `*/`
            currentColumn += 2;
        }
        tokenValue[index] = '\0';
        token.type = COMMENT;
        strcpy(token.value, tokenValue);
        totalValidCharCount += strlen(token.value);  // 更新字符总数
        commentCount++;
    }

    return token;
}


// 处理标识符
Token parseIdentifier() {
    char tokenValue[MAX_TOKEN_SIZE];
    int index = 0;
    Token token;

    // 标识符的第一个字符必须是字母或下划线
    if (isalpha(*forward) || *forward == '_') {
        tokenValue[index++] = *forward;
        forward++;
        currentColumn++;
        totalValidCharCount++;
    } else {
        reportError("Invalid identifier start");
        token.type = UNKNOWN;
        return token;
    }

    // 后续字符可以是字母、数字或下划线
    while (isalnum(*forward) || *forward == '_') {
        tokenValue[index++] = *forward;
        forward++;
        currentColumn++;
        totalValidCharCount++;
    }

    // 结束标识符字符串
    tokenValue[index] = '\0';

    // 构建标识符Token
    token.type = IDENTIFIER;
    strcpy(token.value, tokenValue);

    // 检查符号表中是否已有此标识符
    std::string identifierStr = std::string(tokenValue);
    if (symbolTable.find(identifierStr) == symbolTable.end()) {
        // 如果标识符不在符号表中，则添加到符号表并增加计数
        symbolTable.insert(identifierStr);
        identifierCount++;  // 统计唯一标识符
    }

    return token;
}



// 处理数字
Token parseNumber() {
    char tokenValue[MAX_TOKEN_SIZE];
    int index = 0;
    Token token;
    int state = 0;  // 初始状态 S0
    long intValue = 0;  // 用于存储转换后的整数值
    double floatValue = 0.0;  // 用于存储转换后的浮点数值
    int isFloat = 0;  // 标识是否是浮点数

    while (1) {
        char ch = *forward;

        switch (state) {
            case 0:  // S0: 起始状态
                if (ch == '0') {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 2;  // 可能是八进制或十六进制
                } else if (isdigit(ch)) {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 1;  // 十进制数字
                } else {
                    reportError("Invalid number format");
                    return token;  // 非法状态，直接返回
                }
                break;

            case 1:  // S1: 处理十进制整数
                if (isdigit(ch)) {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                } else if (ch == '.') {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    isFloat = 1;  // 标识为浮点数
                    state = 5;  // 进入小数部分
                } else if (ch == 'e' || ch == 'E') {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    isFloat = 1;  // 标识为浮点数
                    state = 7;  // 进入科学计数法部分
                } else {
                    state = 9;  // 数字识别完成，进入接受状态
                }
                break;

            case 2:  // S2: 处理八进制或十六进制前缀
                if (ch == 'x' || ch == 'X') {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 3;  // 进入十六进制状态
                } else if (ch >= '0' && ch <= '7') {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 2;  // 继续识别八进制数字
                } else if (ch >= '8' && ch <= '9') {
                    reportError("Invalid octal number");
                    forward++;  // 跳过非法字符，继续分析
                    currentColumn++;
                } else {
                    state = 9;  // 八进制识别完成，进入接受状态
                }
                break;

            case 3:  // S3: 处理十六进制前缀
                if (isxdigit(ch)) {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 4;  // 继续识别十六进制数字
                } else {
                    reportError("Invalid hexadecimal number");
                    forward++;  // 跳过非法字符，继续分析
                    currentColumn++;
                    state = 9;  // 非法字符，进入接受状态
                }
                break;

            case 4:  // S4: 处理十六进制数字
                if (isxdigit(ch)) {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                } else {
                    state = 9;  // 十六进制识别完成，进入接受状态
                }
                break;

            case 5:  // S5: 处理小数点
                if (isdigit(ch)) {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 6;  // 进入小数状态
                } else {
                    reportError("Invalid floating-point number");
                    forward++;  // 跳过非法字符，继续分析
                    currentColumn++;
                    state = 9;  // 非法小数，进入接受状态
                }
                break;

            case 6:  // S6: 处理小数部分
                if (isdigit(ch)) {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                } else if (ch == 'e' || ch == 'E') {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 7;  // 进入科学计数法
                } else {
                    state = 9;  // 小数识别完成，进入接受状态
                }
                break;

            case 7:  // S7: 处理科学计数法
                if (ch == '+' || ch == '-') {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 8;  // 处理指数部分
                } else if (isdigit(ch)) {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 8;  // 处理指数部分
                } else {
                    reportError("Invalid scientific notation");
                    forward++;  // 跳过非法字符，继续分析
                    currentColumn++;
                    state = 9;  // 非法科学计数法，进入接受状态
                }
                break;

            case 8:  // S8: 处理科学计数法的指数部分
                if (isdigit(ch)) {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                } else {
                    state = 9;  // 科学计数法识别完成，进入接受状态
                }
                break;

            case 9:  // S9: 接受状态，完成数字识别
                tokenValue[index] = '\0';  // 字符串结束符
                token.type = NUMBER;

                if (isFloat) {
                    floatValue = strtod(tokenValue, NULL);  // 将浮点数字符串转为双精度浮点数
                    sprintf(token.value, "%.6g", floatValue);  // 将浮点数转换为字符串存储在 token.value 中
                } else if (tokenValue[0] == '0' && (tokenValue[1] == 'x' || tokenValue[1] == 'X')) {
                    intValue = strtol(tokenValue, NULL, 16);  // 处理十六进制数
                    sprintf(token.value, "%ld", intValue);  // 将整数值存储在 token.value 中
                } else if (tokenValue[0] == '0') {
                    intValue = strtol(tokenValue, NULL, 8);  // 处理八进制数
                    sprintf(token.value, "%ld", intValue);  // 将整数值存储在 token.value 中
                } else {
                    intValue = strtol(tokenValue, NULL, 10);  // 处理十进制数
                    sprintf(token.value, "%ld", intValue);  // 将整数值存储在 token.value 中
                }

                numberCount++;
                totalValidCharCount += strlen(token.value);
                return token;  // 返回处理后的Token
        }

        // 处理换行符，更新行号和列号
        if (ch == '\n') {
            currentLine++;
            currentColumn = 1;
        } else {
            currentColumn++;
        }

        // 检查是否到达文件末尾或缓冲区末尾
        if (ch == EOF || forward == &buffer1[BUFFER_SIZE - 1] || forward == &buffer2[BUFFER_SIZE - 1]) {
            break;
        }
    }

    // 如果在解析过程中到达文件末尾或缓冲区末尾，则返回结果
    tokenValue[index] = '\0';
    token.type = NUMBER;

    if (isFloat) {
        floatValue = strtod(tokenValue, NULL);
        sprintf(token.value, "%.6g", floatValue);
    } else {
        intValue = strtol(tokenValue, NULL, 10);
        sprintf(token.value, "%ld", intValue);
    }

    numberCount++;
    totalValidCharCount += strlen(token.value);

    return token;
}

// 打印统计结果
void printStatistics() {
    printf("\n--- Statistics ---\n");
    printf("Number of lines: %d\n", lineCount);
    printf("Number of unique identifiers: %d\n", identifierCount);
    printf("Number of numbers: %d\n", numberCount);
    printf("Number of operators: %d\n", operatorCount);
    printf("Number of comments: %d\n", commentCount);
    printf("Total number of valid characters (excluding whitespace): %d\n", totalValidCharCount);
}

// 词法分析器函数
void lexicalAnalyzer(FILE *sourceCode) {
    char ch;
    Token token;

    loadBuffer(buffer1, sourceCode); // 首先装载第一个缓冲区

    while (1) {
        ch = *forward;
        if (ch == EOF) {
            if (forward == &buffer1[BUFFER_SIZE - 1]) {
                switchBuffer(sourceCode); // 切换缓冲区
                continue;
            }
            else if (forward == &buffer2[BUFFER_SIZE - 1]) {
                switchBuffer(sourceCode); // 切换缓冲区
                continue;
            }
            else {
                break; // 完成分析
            }
        }

        // 处理标识符或关键字：以字母开头
        if (isalpha(ch)) {
            token = parseIdentifier(); // 调用标识符解析函数
            if (isKeyword(token.value)) {
                token.type = KEYWORD;  // 如果是关键字，则修改类型为关键字
            }
            printToken(token); // 输出Token
        }
            // 处理数字：整数或小数
        else if (isdigit(ch)) {
            token = parseNumber(); // 调用数字解析函数
            printToken(token); // 输出Token
        }
            // 处理字符常量
        else if (ch == '\'') {
            token = parseCharConstant(); // 解析字符常量
            printToken(token); // 输出Token
        }
            // 处理字符串常量
        else if (ch == '"') {
            token = parseStringConstant(); // 解析字符串常量
            printToken(token); // 输出Token
        }

            // 处理注释或操作符
        else if (ch == '/') {
            if (*(forward + 1) == '/') {
                token = parseComment();  // 解析单行注释
                printToken(token);  // 输出注释Token
                continue;  // 跳过注释部分，继续下一个循环
            } else if (*(forward + 1) == '*') {
                token = parseComment();  // 解析多行注释
                printToken(token);  // 输出注释Token
                continue;  // 跳过注释部分，继续下一个循环
            } else if (*(forward + 1) == '=') {
                char tokenValue[3] = {ch, '=', '\0'};
                forward += 2;  // 跳过 `/=`
                currentColumn += 2;
                totalValidCharCount += 2;

                token.type = OPERATOR;
                strcpy(token.value, tokenValue);
                printToken(token);  // 输出 `/=` 操作符Token
                operatorCount++;  // 统计操作符数量
            } else {
                // 普通除法操作符 `/`
                char tokenValue[2] = {ch, '\0'};
                forward++;
                currentColumn++;
                totalValidCharCount++;

                token.type = OPERATOR;
                strcpy(token.value, tokenValue);
                printToken(token);  // 输出 `/` 操作符Token
                operatorCount++;  // 统计操作符数量
            }
        }

// 处理预处理指令
        else if (ch == '#') {
            forward++;  // 跳过 '#'
            currentColumn++;

            token = parsePreprocessorDirective();  // 解析预处理指令
            printToken(token);  // 在此处打印预处理指令Token

            // 如果是 #define，则解析宏定义
            if (strcmp(token.value, "#define") == 0) {
                token = parseMacroDefinition();  // 解析宏定义名称和参数
                printToken(token);  // 打印宏值Token
            }

            // 如果是 #include，则解析头文件路径
            if (strcmp(token.value, "#include") == 0) {
                token = parseHeaderFile();  // 解析头文件路径
                printToken(token);  // 打印头文件路径Token
            }

            continue;  // 继续处理其他部分
        }


        // 处理操作符
        else if (isOperator(ch)) {
            char tokenValue[3] = {ch, '\0', '\0'};  // 操作符的最大长度为 2
            forward++;
            totalValidCharCount++;  // 统计有效字符

            // 检查复合操作符，例如：+=, -=, ==, !=, <=, >=, &&, ||
            if ((*forward == '=' && (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%' || ch == '=' || ch == '!' || ch == '<' || ch == '>')) ||
                (*forward == '&' && ch == '&') ||
                (*forward == '|' && ch == '|') ||
                (*forward == '+' && ch == '+') ||
                (*forward == '-' && ch == '-')) {

                tokenValue[1] = *forward;  // 追加第二个字符
                forward++;  // 前进指针
                totalValidCharCount++;  // 统计有效字符
            }

            // 设置Token类型为操作符
            token.type = OPERATOR;
            strcpy(token.value, tokenValue);
            printToken(token);  // 输出操作符Token
            operatorCount++;  // 统计操作符数量
        }

            // 处理分隔符
        else if (isSeparator(ch)) {
            char tokenValue[2] = {ch, '\0'};
            forward++;
            token.type = SEPARATOR;
            strcpy(token.value, tokenValue);
            printToken(token); // 输出分隔符Token
            totalValidCharCount++;  // 更新字符总数
        }
            // 处理换行符，统计行数
        else if (ch == '\n') {
            lineCount++;  // 统计行数
            forward++;
            currentColumn = 1;  // 换行时重置列号
        }
            // 跳过其他空白符
        else if (isspace(ch)) {
            forward++;  // 跳过空格、制表符等，不计入字符总数
            currentColumn++;
        }
        else {
            forward++;
            currentColumn++;
        }

        // 当指针到达缓冲区末尾时，切换缓冲区
        if (forward == &buffer1[BUFFER_SIZE - 1] || forward == &buffer2[BUFFER_SIZE - 1]) {
            switchBuffer(sourceCode);
        }
    }

    // 分析完成，输出统计结果
    printStatistics();
}
