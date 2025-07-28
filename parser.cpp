#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <set>
#include <string>
#include "parser.h"

// ���ű�
std::set<std::string> symbolTable;  // ���ڴ��Ψһ�ı�ʶ��

// ����������
char buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE];
char *forward = buffer1; // ǰ��ָ��
int isBuffer1Active = 1; // �жϵ�ǰ��Ծ������

// �ؼ��ּ���
const char *keywords[] = {
        "auto", "break", "case", "char", "const", "continue", "default", "do", "double",
        "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int",
        "long", "register", "restrict", "return", "short", "signed", "sizeof",
        "static", "struct", "switch", "typedef", "union", "unsigned", "void",
        "volatile", "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex",
        "_Generic", "_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local", NULL
};


// ͳ����Ϣ
int lineCount = 1;          // ������������ʼ��Ϊ1����Ϊ������һ�У�
int identifierCount = 0;    // ��ʶ����������Ψһ��
int numberCount = 0;        // ���ֵ�����
int operatorCount = 0;      // ������������
int commentCount = 0;       // ע�͵�����
int totalValidCharCount = 0; // ��Ч�ַ������������ظ��ı�ʶ�������ֵȣ�
int currentLine = 1;     // ��ǰ�к�
int currentColumn = 1;   // ��ǰ�к�

// �ж��Ƿ��ǹؼ���
int isKeyword(const char *word) {
    for (int i = 0; keywords[i] != NULL; i++) {
        if (strcmp(word, keywords[i]) == 0) {
            return 1;  // �ǹؼ���
        }
    }
    return 0;  // ���ǹؼ���
}


// �ж��Ƿ�Ϊ�������ĵ��ַ�
int isOperator(char ch) {
    // �жϵ��ַ��Ƿ�Ϊ�Ϸ��Ĳ�����
    return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%' ||
           ch == '=' || ch == '!' || ch == '<' || ch == '>' || ch == '&' ||
           ch == '|' || ch == '^' || ch == '~' || ch == '?' || ch == ':';
}

// �ָ�������
int isSeparator(char ch) {
    return ch == '{' || ch == '}' || ch == '(' || ch == ')' ||
           ch == '[' || ch == ']' || ch == ',' || ch == ';' ||
           ch == '.' || ch == ':' || ch == '#';
}


// ���󱨸溯��
void reportError(const char *errorMessage) {
    printf("Error at line %d, column %d: %s\n", lineCount, currentColumn, errorMessage);
}

// ���Token
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

// װ���µĻ�����
void loadBuffer(char *buffer, FILE *sourceCode) {
    int bytesRead = fread(buffer, sizeof(char), BUFFER_SIZE - 1, sourceCode);
    if (bytesRead < BUFFER_SIZE - 1) {
        // �����ȡ���ֽ������ڻ�������С��˵�������ļ�ĩβ������ EOF ���
        buffer[bytesRead] = EOF;
    } else {
        buffer[BUFFER_SIZE - 1] = EOF;  // ��������£��ڻ�����ĩβ���� EOF
    }
}

// �л�������
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

// �����ַ�����
Token parseCharConstant() {
    char tokenValue[MAX_TOKEN_SIZE];
    int index = 0;
    Token token;
    int state = 0;  // ��ʼ״̬

    while (1) {
        char ch = *forward;

        // �ļ��������
        if (ch == EOF) {
            reportError("Unexpected EOF in character constant");
            token.type = UNKNOWN;
            return token;
        }

        // �кż��͸���
        if (ch == '\n') {
            currentLine++;
            currentColumn = 1;  // �����к�
        }

        switch (state) {
            case 0:  // ��ʼ״̬��������һ��������
                if (ch == '\'') {
                    forward++;
                    currentColumn++;
                    state = 1;  // ���봦���ַ�״̬
                } else {
                    reportError("Expected single quote at the beginning of character constant");
                    token.type = UNKNOWN;
                    return token;
                }
                break;

            case 1:  // �����ַ����ж�����ͨ�ַ�����ת���ַ�
                if (ch == '\\') {  // ����ת���ַ�
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 2;  // ���봦��ת���ַ�״̬
                } else if (ch != '\'' && ch != EOF && ch != '\n') {  // ������ͨ�ַ�
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 3;  // ׼������պ�״̬
                } else if (ch == '\n') {  // ����ַ��������������з��򱨴�
                    reportError("Unclosed character constant");
                    token.type = UNKNOWN;
                    return token;
                } else {
                    reportError("Empty or invalid character constant");
                    token.type = UNKNOWN;
                    return token;
                }
                break;

            case 2:  // ����ת���ַ�����ַ�
                if (ch != EOF && ch != '\n') {
                    tokenValue[index++] = ch;  // ��¼ת�����ַ�
                    forward++;
                    currentColumn++;
                    state = 3;  // ׼������պ�״̬
                } else if (ch == '\n') {  // ���ת���ַ�δ�պϾ��������з�
                    reportError("Unclosed character constant after escape sequence");
                    token.type = UNKNOWN;
                    return token;
                } else {
                    reportError("Unclosed character constant after escape sequence");
                    token.type = UNKNOWN;
                    return token;
                }
                break;

            case 3:  // ���պϵ�����
                if (ch == '\'') {
                    forward++;
                    currentColumn++;
                    tokenValue[index] = '\0';  // �����ַ�����
                    token.type = CHAR_CONSTANT;
                    strcpy(token.value, tokenValue);
                    totalValidCharCount += strlen(token.value);  // �����ַ�����
                    return token;  // �ɹ�����
                } else {
                    reportError("Unclosed character constant");
                    token.type = UNKNOWN;
                    return token;
                }
        }

        // �����к�
        if (ch != '\n' && ch != EOF) {
            currentColumn++;
        }
    }
}

// �����ַ�������
Token parseStringConstant() {
    char tokenValue[MAX_TOKEN_SIZE];
    int index = 0;
    Token token;
    int state = 0;  // ��ʼ״̬

    forward++;  // ������һ��˫����
    currentColumn++;

    while (1) {
        char ch = *forward;

        // �ļ��������
        if (ch == EOF) {
            reportError("Unexpected EOF in string constant");
            token.type = UNKNOWN;
            return token;
        }

        switch (state) {
            case 0:  // ������ȡ�ַ�
                if (ch == '"') {  // �������������˫����
                    forward++;
                    currentColumn++;
                    tokenValue[index] = '\0';  // �����ַ���
                    token.type = STRING_CONSTANT;
                    strcpy(token.value, tokenValue);
                    totalValidCharCount += strlen(token.value);  // ������Ч�ַ���
                    return token;  // �ɹ������ַ�������
                } else if (ch == '\\') {  // �������ת���ַ�
                    state = 1;  // ת�Ƶ�����ת���ַ���״̬
                    forward++;
                    currentColumn++;
                } else if (ch == '\n') {  // ����������з�����û�бպ�˫����
                    reportError("Unclosed string constant");
                    token.type = UNKNOWN;
                    return token;
                } else {  // ��ͨ�ַ�
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                }
                break;

            case 1:  // ����ת���ַ�
                // ��������ת���ַ�
                if (ch == 'n' || ch == 't' || ch == '\\' || ch == '"' || ch == '\'') {
                    tokenValue[index++] = '\\';  // ������б��
                    tokenValue[index++] = ch;  // ����ת�����ַ�
                } else {
                    reportError("Invalid escape sequence");
                    token.type = UNKNOWN;
                    return token;
                }
                forward++;
                currentColumn++;
                state = 0;  // ����������ȡ�ַ���״̬
                break;
        }

        // ����Ƿ񵽴��ļ�ĩβ
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

    // �����հ׷�
    while (isspace(*forward)) {
        forward++;
        currentColumn++;
    }

    // ����Ԥ����ָ��
    while (isalnum(*forward)) {
        tokenValue[index++] = *forward;
        forward++;
        currentColumn++;
    }
    tokenValue[index] = '\0';

    // �ж��Ƿ�Ϊ #define ָ��
    if (strcmp(tokenValue, "define") == 0) {
        token.type = PREPROCESSOR_DIRECTIVE;
        strcpy(token.value, "#define");
        totalValidCharCount += strlen(token.value);

        // ����Ԥ����ָ��Token
        return token;

    } else if (strcmp(tokenValue, "include") == 0) {
        token.type = PREPROCESSOR_DIRECTIVE;
        strcpy(token.value, "#include");
        totalValidCharCount += strlen(token.value);

        // ���� #include ָ��Token
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

    // �����հ׷���׼������������
    while (isspace(*forward)) {
        forward++;
        currentColumn++;
    }

    // ����������
    while (isalnum(*forward) || *forward == '_') {  // �����ƿ��԰�����ĸ�����ֺ��»���
        tokenValue[index++] = *forward;
        forward++;
        currentColumn++;
    }
    tokenValue[index] = '\0';

    // �����궨������Token
    token.type = MACRO_DEFINITION;
    strcpy(token.value, tokenValue);
    totalValidCharCount += strlen(token.value);

    Token macroNameToken = token;  // ���������Token

    // ����Ƿ��к���� (�� #define MAX(a, b))
    if (*forward == '(') {
        forward++;  // ���� '('
        currentColumn++;

        // ���������
        index = 0;
        while (*forward != ')' && *forward != EOF) {
            tokenValue[index++] = *forward;
            forward++;
            currentColumn++;
        }

        // ����Ƿ�����ƥ��� ')'
        if (*forward == ')') {
            forward++;  // ���� ')'
            currentColumn++;
        } else {
            reportError("Unclosed macro parameter list");
            token.type = UNKNOWN;
            return token;
        }

        tokenValue[index] = '\0';

        // �����������Token
        token.type = MACRO_PARAMETER;
        strcpy(token.value, tokenValue);
        totalValidCharCount += strlen(token.value);
        printToken(macroNameToken);  // ��ӡ������Token
        printToken(token);  // ��ӡ�����Token
    } else {
        printToken(macroNameToken);  // ֱ�Ӵ�ӡ������Token���޲�����
    }

    // �����궨���ֵ����
    index = 0;
    while (*forward != '\n' && *forward != EOF) {
        // ����Ƿ�����ע��
        if (*forward == '/' && *(forward + 1) == '/') {
            break;  // ��������ע�ͣ�ֹͣ�����궨��ֵ
        } else if (*forward == '/' && *(forward + 1) == '*') {
            break;  // ��������ע�ͣ�ֹͣ�����궨��ֵ
        }

        // �������׵Ķ���հף������궨���ֵ���浽 tokenValue ��
        if (!isspace(*forward) || (isspace(*forward) && index > 0)) {
            tokenValue[index++] = *forward;
        }
        forward++;
        currentColumn++;
    }
    tokenValue[index] = '\0';

    // �����궨��ֵToken
    token.type = MACRO_VALUE;
    strcpy(token.value, tokenValue);
    totalValidCharCount += strlen(token.value);

    return token;
}



Token parseHeaderFile() {
    char tokenValue[MAX_TOKEN_SIZE];
    int index = 0;
    Token token;
    char boundary;  // ���ڱ���������� ('>' �� '"')

    // �����հ׷���׼�������ļ�·��
    while (isspace(*forward)) {
        forward++;
        currentColumn++;
    }

    // ����ļ�·���Ŀ�ʼ��־�������� '<' �� '"'
    if (*forward == '<') {
        boundary = '>';  // �����ʼ������ '<'����������Ӧ���� '>'
        forward++;  // ���� '<'
        currentColumn++;
    } else if (*forward == '"') {
        boundary = '"';  // �����ʼ������ '"', ��������Ӧ���� '"'
        forward++;  // ���� '"'
        currentColumn++;
    } else {
        // ���û�� '<' �� '"' ��ʼ������
        reportError("Expected '<' or '\"' after #include");
        token.type = UNKNOWN;
        return token;
    }

    // ��ʼ����·����ֱ��������Ӧ�ıպϷ��� '>' �� '"'
    while (*forward != boundary && *forward != EOF) {
        // ��ֹ·���������»��������
        if (index >= MAX_TOKEN_SIZE - 1) {
            reportError("Header file path too long");
            token.type = UNKNOWN;
            return token;
        }

        // ����·���ַ�
        tokenValue[index++] = *forward;
        forward++;
        currentColumn++;
    }

    // ����Ƿ�����ƥ��ıպϷ���
    if (*forward == boundary) {
        tokenValue[index] = '\0';  // ����·���ַ���
        forward++;  // �����պϷ���
        currentColumn++;

        // ����ͷ�ļ�·����Token
        token.type = HEADER_FILE;
        strcpy(token.value, tokenValue);
        totalValidCharCount += strlen(token.value);  // �����ַ�����
        return token;
    } else {
        // δ�ҵ�ƥ��ıպϷ��ţ�����
        reportError("Unclosed header file path");
        token.type = UNKNOWN;
        return token;
    }
}


// ����ע��
Token parseComment() {
    Token token;
    char tokenValue[MAX_TOKEN_SIZE];
    int index = 0;

    if (*forward == '/' && *(forward + 1) == '/') {
        forward += 2;  // ���� `//`
        currentColumn += 2;

        // ��ȡ����ע��
        while (*forward != '\n' && *forward != EOF) {
            tokenValue[index++] = *forward;
            forward++;
            currentColumn++;
        }
        tokenValue[index] = '\0';
        token.type = COMMENT;
        strcpy(token.value, tokenValue);
        totalValidCharCount += strlen(token.value);  // �����ַ�����
        commentCount++;
    } else if (*forward == '/' && *(forward + 1) == '*') {
        forward += 2;  // ���� `/*`
        currentColumn += 2;

        // ��ȡ����ע��
        while (!(*forward == '*' && *(forward + 1) == '/') && *forward != EOF) {
            tokenValue[index++] = *forward;
            forward++;
            currentColumn++;
            if (*forward == '\n') {
                lineCount++;  // �������ע���еĻ��з�
                currentColumn = 0;
            }
        }
        if (*forward == EOF) {
            reportError("Unclosed comment");
        } else {
            forward += 2;  // ���� `*/`
            currentColumn += 2;
        }
        tokenValue[index] = '\0';
        token.type = COMMENT;
        strcpy(token.value, tokenValue);
        totalValidCharCount += strlen(token.value);  // �����ַ�����
        commentCount++;
    }

    return token;
}


// �����ʶ��
Token parseIdentifier() {
    char tokenValue[MAX_TOKEN_SIZE];
    int index = 0;
    Token token;

    // ��ʶ���ĵ�һ���ַ���������ĸ���»���
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

    // �����ַ���������ĸ�����ֻ��»���
    while (isalnum(*forward) || *forward == '_') {
        tokenValue[index++] = *forward;
        forward++;
        currentColumn++;
        totalValidCharCount++;
    }

    // ������ʶ���ַ���
    tokenValue[index] = '\0';

    // ������ʶ��Token
    token.type = IDENTIFIER;
    strcpy(token.value, tokenValue);

    // �����ű����Ƿ����д˱�ʶ��
    std::string identifierStr = std::string(tokenValue);
    if (symbolTable.find(identifierStr) == symbolTable.end()) {
        // �����ʶ�����ڷ��ű��У�����ӵ����ű����Ӽ���
        symbolTable.insert(identifierStr);
        identifierCount++;  // ͳ��Ψһ��ʶ��
    }

    return token;
}



// ��������
Token parseNumber() {
    char tokenValue[MAX_TOKEN_SIZE];
    int index = 0;
    Token token;
    int state = 0;  // ��ʼ״̬ S0
    long intValue = 0;  // ���ڴ洢ת���������ֵ
    double floatValue = 0.0;  // ���ڴ洢ת����ĸ�����ֵ
    int isFloat = 0;  // ��ʶ�Ƿ��Ǹ�����

    while (1) {
        char ch = *forward;

        switch (state) {
            case 0:  // S0: ��ʼ״̬
                if (ch == '0') {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 2;  // �����ǰ˽��ƻ�ʮ������
                } else if (isdigit(ch)) {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 1;  // ʮ��������
                } else {
                    reportError("Invalid number format");
                    return token;  // �Ƿ�״̬��ֱ�ӷ���
                }
                break;

            case 1:  // S1: ����ʮ��������
                if (isdigit(ch)) {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                } else if (ch == '.') {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    isFloat = 1;  // ��ʶΪ������
                    state = 5;  // ����С������
                } else if (ch == 'e' || ch == 'E') {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    isFloat = 1;  // ��ʶΪ������
                    state = 7;  // �����ѧ����������
                } else {
                    state = 9;  // ����ʶ����ɣ��������״̬
                }
                break;

            case 2:  // S2: ����˽��ƻ�ʮ������ǰ׺
                if (ch == 'x' || ch == 'X') {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 3;  // ����ʮ������״̬
                } else if (ch >= '0' && ch <= '7') {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 2;  // ����ʶ��˽�������
                } else if (ch >= '8' && ch <= '9') {
                    reportError("Invalid octal number");
                    forward++;  // �����Ƿ��ַ�����������
                    currentColumn++;
                } else {
                    state = 9;  // �˽���ʶ����ɣ��������״̬
                }
                break;

            case 3:  // S3: ����ʮ������ǰ׺
                if (isxdigit(ch)) {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 4;  // ����ʶ��ʮ����������
                } else {
                    reportError("Invalid hexadecimal number");
                    forward++;  // �����Ƿ��ַ�����������
                    currentColumn++;
                    state = 9;  // �Ƿ��ַ����������״̬
                }
                break;

            case 4:  // S4: ����ʮ����������
                if (isxdigit(ch)) {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                } else {
                    state = 9;  // ʮ������ʶ����ɣ��������״̬
                }
                break;

            case 5:  // S5: ����С����
                if (isdigit(ch)) {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 6;  // ����С��״̬
                } else {
                    reportError("Invalid floating-point number");
                    forward++;  // �����Ƿ��ַ�����������
                    currentColumn++;
                    state = 9;  // �Ƿ�С�����������״̬
                }
                break;

            case 6:  // S6: ����С������
                if (isdigit(ch)) {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                } else if (ch == 'e' || ch == 'E') {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 7;  // �����ѧ������
                } else {
                    state = 9;  // С��ʶ����ɣ��������״̬
                }
                break;

            case 7:  // S7: �����ѧ������
                if (ch == '+' || ch == '-') {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 8;  // ����ָ������
                } else if (isdigit(ch)) {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                    state = 8;  // ����ָ������
                } else {
                    reportError("Invalid scientific notation");
                    forward++;  // �����Ƿ��ַ�����������
                    currentColumn++;
                    state = 9;  // �Ƿ���ѧ���������������״̬
                }
                break;

            case 8:  // S8: �����ѧ��������ָ������
                if (isdigit(ch)) {
                    tokenValue[index++] = ch;
                    forward++;
                    currentColumn++;
                } else {
                    state = 9;  // ��ѧ������ʶ����ɣ��������״̬
                }
                break;

            case 9:  // S9: ����״̬���������ʶ��
                tokenValue[index] = '\0';  // �ַ���������
                token.type = NUMBER;

                if (isFloat) {
                    floatValue = strtod(tokenValue, NULL);  // ���������ַ���תΪ˫���ȸ�����
                    sprintf(token.value, "%.6g", floatValue);  // ��������ת��Ϊ�ַ����洢�� token.value ��
                } else if (tokenValue[0] == '0' && (tokenValue[1] == 'x' || tokenValue[1] == 'X')) {
                    intValue = strtol(tokenValue, NULL, 16);  // ����ʮ��������
                    sprintf(token.value, "%ld", intValue);  // ������ֵ�洢�� token.value ��
                } else if (tokenValue[0] == '0') {
                    intValue = strtol(tokenValue, NULL, 8);  // ����˽�����
                    sprintf(token.value, "%ld", intValue);  // ������ֵ�洢�� token.value ��
                } else {
                    intValue = strtol(tokenValue, NULL, 10);  // ����ʮ������
                    sprintf(token.value, "%ld", intValue);  // ������ֵ�洢�� token.value ��
                }

                numberCount++;
                totalValidCharCount += strlen(token.value);
                return token;  // ���ش�����Token
        }

        // �����з��������кź��к�
        if (ch == '\n') {
            currentLine++;
            currentColumn = 1;
        } else {
            currentColumn++;
        }

        // ����Ƿ񵽴��ļ�ĩβ�򻺳���ĩβ
        if (ch == EOF || forward == &buffer1[BUFFER_SIZE - 1] || forward == &buffer2[BUFFER_SIZE - 1]) {
            break;
        }
    }

    // ����ڽ��������е����ļ�ĩβ�򻺳���ĩβ���򷵻ؽ��
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

// ��ӡͳ�ƽ��
void printStatistics() {
    printf("\n--- Statistics ---\n");
    printf("Number of lines: %d\n", lineCount);
    printf("Number of unique identifiers: %d\n", identifierCount);
    printf("Number of numbers: %d\n", numberCount);
    printf("Number of operators: %d\n", operatorCount);
    printf("Number of comments: %d\n", commentCount);
    printf("Total number of valid characters (excluding whitespace): %d\n", totalValidCharCount);
}

// �ʷ�����������
void lexicalAnalyzer(FILE *sourceCode) {
    char ch;
    Token token;

    loadBuffer(buffer1, sourceCode); // ����װ�ص�һ��������

    while (1) {
        ch = *forward;
        if (ch == EOF) {
            if (forward == &buffer1[BUFFER_SIZE - 1]) {
                switchBuffer(sourceCode); // �л�������
                continue;
            }
            else if (forward == &buffer2[BUFFER_SIZE - 1]) {
                switchBuffer(sourceCode); // �л�������
                continue;
            }
            else {
                break; // ��ɷ���
            }
        }

        // �����ʶ����ؼ��֣�����ĸ��ͷ
        if (isalpha(ch)) {
            token = parseIdentifier(); // ���ñ�ʶ����������
            if (isKeyword(token.value)) {
                token.type = KEYWORD;  // ����ǹؼ��֣����޸�����Ϊ�ؼ���
            }
            printToken(token); // ���Token
        }
            // �������֣�������С��
        else if (isdigit(ch)) {
            token = parseNumber(); // �������ֽ�������
            printToken(token); // ���Token
        }
            // �����ַ�����
        else if (ch == '\'') {
            token = parseCharConstant(); // �����ַ�����
            printToken(token); // ���Token
        }
            // �����ַ�������
        else if (ch == '"') {
            token = parseStringConstant(); // �����ַ�������
            printToken(token); // ���Token
        }

            // ����ע�ͻ������
        else if (ch == '/') {
            if (*(forward + 1) == '/') {
                token = parseComment();  // ��������ע��
                printToken(token);  // ���ע��Token
                continue;  // ����ע�Ͳ��֣�������һ��ѭ��
            } else if (*(forward + 1) == '*') {
                token = parseComment();  // ��������ע��
                printToken(token);  // ���ע��Token
                continue;  // ����ע�Ͳ��֣�������һ��ѭ��
            } else if (*(forward + 1) == '=') {
                char tokenValue[3] = {ch, '=', '\0'};
                forward += 2;  // ���� `/=`
                currentColumn += 2;
                totalValidCharCount += 2;

                token.type = OPERATOR;
                strcpy(token.value, tokenValue);
                printToken(token);  // ��� `/=` ������Token
                operatorCount++;  // ͳ�Ʋ���������
            } else {
                // ��ͨ���������� `/`
                char tokenValue[2] = {ch, '\0'};
                forward++;
                currentColumn++;
                totalValidCharCount++;

                token.type = OPERATOR;
                strcpy(token.value, tokenValue);
                printToken(token);  // ��� `/` ������Token
                operatorCount++;  // ͳ�Ʋ���������
            }
        }

// ����Ԥ����ָ��
        else if (ch == '#') {
            forward++;  // ���� '#'
            currentColumn++;

            token = parsePreprocessorDirective();  // ����Ԥ����ָ��
            printToken(token);  // �ڴ˴���ӡԤ����ָ��Token

            // ����� #define��������궨��
            if (strcmp(token.value, "#define") == 0) {
                token = parseMacroDefinition();  // �����궨�����ƺͲ���
                printToken(token);  // ��ӡ��ֵToken
            }

            // ����� #include�������ͷ�ļ�·��
            if (strcmp(token.value, "#include") == 0) {
                token = parseHeaderFile();  // ����ͷ�ļ�·��
                printToken(token);  // ��ӡͷ�ļ�·��Token
            }

            continue;  // ����������������
        }


        // ���������
        else if (isOperator(ch)) {
            char tokenValue[3] = {ch, '\0', '\0'};  // ����������󳤶�Ϊ 2
            forward++;
            totalValidCharCount++;  // ͳ����Ч�ַ�

            // ��鸴�ϲ����������磺+=, -=, ==, !=, <=, >=, &&, ||
            if ((*forward == '=' && (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%' || ch == '=' || ch == '!' || ch == '<' || ch == '>')) ||
                (*forward == '&' && ch == '&') ||
                (*forward == '|' && ch == '|') ||
                (*forward == '+' && ch == '+') ||
                (*forward == '-' && ch == '-')) {

                tokenValue[1] = *forward;  // ׷�ӵڶ����ַ�
                forward++;  // ǰ��ָ��
                totalValidCharCount++;  // ͳ����Ч�ַ�
            }

            // ����Token����Ϊ������
            token.type = OPERATOR;
            strcpy(token.value, tokenValue);
            printToken(token);  // ���������Token
            operatorCount++;  // ͳ�Ʋ���������
        }

            // ����ָ���
        else if (isSeparator(ch)) {
            char tokenValue[2] = {ch, '\0'};
            forward++;
            token.type = SEPARATOR;
            strcpy(token.value, tokenValue);
            printToken(token); // ����ָ���Token
            totalValidCharCount++;  // �����ַ�����
        }
            // �����з���ͳ������
        else if (ch == '\n') {
            lineCount++;  // ͳ������
            forward++;
            currentColumn = 1;  // ����ʱ�����к�
        }
            // ���������հ׷�
        else if (isspace(ch)) {
            forward++;  // �����ո��Ʊ���ȣ��������ַ�����
            currentColumn++;
        }
        else {
            forward++;
            currentColumn++;
        }

        // ��ָ�뵽�ﻺ����ĩβʱ���л�������
        if (forward == &buffer1[BUFFER_SIZE - 1] || forward == &buffer2[BUFFER_SIZE - 1]) {
            switchBuffer(sourceCode);
        }
    }

    // ������ɣ����ͳ�ƽ��
    printStatistics();
}
