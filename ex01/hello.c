#include <stdio.h>

int main() {
    // 声明一个字符数组，用于存储键盘输入的内容
    char input_content[100];

    printf("请输入一些内容: ");
    
    // 使用 C 语言的输入函数获取键盘输入
    scanf("%99s", input_content);

    // 显示输出键盘输入的内容
    printf("你输入的内容是: %s\n", input_content);

    return 0;
}
