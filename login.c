#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ID_LEN 50
#define MAX_PW_LEN 50

typedef struct {
    char id[MAX_ID_LEN];
    char pw[MAX_PW_LEN];
} user;
int check_login(const char *input_id, const char *input_pw) {
    FILE *file = fopen("userlist.txt", "r");
    if (file == NULL) {
        perror("Failed to open userlist.txt");
        return 0;
    }

    user u;
    while (fscanf(file, "%s %s", u.id, u.pw) != EOF) {
        if (strcmp(u.id, input_id) == 0 && strcmp(u.pw, input_pw) == 0) {
            fclose(file);
            return 1;  // 로그인 성공
        }
    }

    fclose(file);
    return 0;  // 로그인 실패
}


int main() {
    char input_id[MAX_ID_LEN];
    char input_pw[MAX_PW_LEN];

    printf("ID: ");
    scanf("%s", input_id);

    printf("Password: ");
    scanf("%s", input_pw);

    if (check_login(input_id, input_pw)) {
        printf("Login successful!\n");
    } else {
        printf("Invalid ID or Password!\n");
    }

    return 0;
}
