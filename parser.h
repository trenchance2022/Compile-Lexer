#ifndef PARSER_H
#define PARSER_H

// 常量定义
#define BUFFER_SIZE 65536  // 缓冲区大小
#define MAX_TOKEN_SIZE 256  // Token最大长度

// Token类型定义
enum TokenType {
    IDENTIFIER,
    NUMBER,
    OPERATOR,
    CHAR_CONSTANT,
    STRING_CONSTANT,
    KEYWORD,
    PREPROCESSOR_DIRECTIVE,
    HEADER_FILE,
    COMMENT,
    SEPARATOR,
    MACRO_DEFINITION,
    MACRO_PARAMETER,
    MACRO_VALUE,
    UNKNOWN
};

// Token结构
struct Token {
    TokenType type;  // Token类型
    char value[MAX_TOKEN_SIZE];  // Token的字符串值
};

// 函数声明
void lexicalAnalyzer(FILE *sourceCode);  // 词法分析器主函数
Token parseIdentifier();  // 解析标识符
Token parseNumber();  // 解析数字
Token parseCharConstant();  // 解析字符常量
Token parseStringConstant();  // 解析字符串常量
Token parsePreprocessorDirective();  // 解析预处理指令
Token parseHeaderFile();  // 解析头文件
Token parseComment();  // 解析注释
Token parseMacroDefinition();  // 解析宏定义


int isKeyword(const char *word);  // 检查是否是关键字
int isOperator(char ch);  // 判断是否是操作符
void printToken(Token token);  // 输出Token
void reportError(const char *errorMessage);  // 错误处理
void printStatistics();  // 打印统计信息
void loadBuffer(char *buffer, FILE *sourceCode);  // 装载缓冲区
void switchBuffer(FILE *sourceCode);  // 切换缓冲区

#endif  // PARSER_H

