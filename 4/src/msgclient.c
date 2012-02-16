#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "mympl.h"

#define BUFSIZE 200

void print_msg(message);
void show_menu(void);

void notify(const char *);
void warn(const char *);
void error(const char *);

int main(void) {
    int ret, choice;
    char buf[BUFSIZE], name[MAX_NAME_LEN + 1], recipient[MAX_NAME_LEN + 1], msgtext[MAX_MSG_LEN + 1];
    message msg;

    printf("Please a enter a name to use (max 10 characters): ");
    fgets(buf, BUFSIZE, stdin);
    sscanf(buf, "%s", name);

    ret = reg(name);
    if (ret == REG_FULL) error("registry full");
    else if (ret == REG_ALRDYREG) error("name already taken");
    else if (ret == REG_FAIL) error("process could not be registered");
    else if (ret == SUCCESS) notify("successfully registered");

    while (1) {
        show_menu();
        fgets(buf, BUFSIZE, stdin);
        sscanf(buf, "%d", &choice);

        switch(choice) {
        case 1:
            printf("\nEnter a recipient's name (max 10 characters): ");
            fgets(buf, BUFSIZE, stdin);
            sscanf(buf, "%s", recipient);

            printf("\nEnter a message :\n");
            fgets(buf, BUFSIZE, stdin);
            sscanf(buf, "%[^\n]", msgtext);

            ret = sendmessage(recipient, msgtext);
            if (ret == SEND_NOTREGD) error("process not registered");
            else if (ret == SEND_NOSLOTS) warn("message memory full");
            else if (ret == SEND_FAIL) warn("message could not be sent");
            else notify("message sent");
            break;

        case 2:
            ret = listmessages();
            if (ret == LIST_FAIL) warn("messages could not be listed");
            else printf("\n  Total: %d received message(s)\n\n", ret);
            break;

        case 3:
            ret = readmessage(&msg);
            if (ret == READ_FAIL) warn("message could not be retrieved");
            if (ret == READ_NOMSG) warn("no messages");
            else print_msg(msg);
            break;

        case 4:
            ret = deregister();
            if (ret == DEREG_FAIL) error("process could not be de-registered");
            else exit(0);
            break;

        default:
            warn("invalid option");
            continue;
        }
    }

    return 0;
}

void print_msg(message msg) {
    char time_str[MAX_TIME_LEN + 1];
    char *msg_format =
        "\n"
        "  Sender: %s\n"
        "  Time: %s\n"
        "  Message Text: %s\n"
        "\n";
    strftime(time_str, MAX_TIME_LEN, "%d %b, %H:%M", localtime(&(msg.time)));

    printf(msg_format, msg.sender, time_str, msg.text);
}

void show_menu(void) {
    char *menu =
        "What would you like to do?\n"
        "\n"
        "  1. Send a message\n"
        "  2. List received messages\n"
        "  3. Read a message\n"
        "  4. Quit\n"
        "\n"
        "Enter 1, 2, 3, or 4: ";

    printf("%s", menu);
}

void put_to_stream(FILE *stream, const char *s) {
    fprintf(stream, "\n  %s\n\n", s);
}

void notify(const char *msg) {
    put_to_stream(stdout, msg);
}

void warn(const char *msg) {
    put_to_stream(stderr, msg);
}

void error(const char *msg) {
    warn(msg);
    exit(-1);
}
