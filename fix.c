```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT_SIZE 256

void replace_tut_with_doctest(const char *input, char *output) {
    // Check for NULL pointers
    if (input == NULL || output == NULL) {
        return;
    }

    // Allocate memory for output
    char *temp = (char *)malloc(MAX_INPUT_SIZE * sizeof(char));
    if (temp == NULL) {
        return;
    }

    // Replace "tut" with "doctest" in the input string
    char *token = strtok(input, " ");
    while (token != NULL) {
        if (strcmp(token, "tut") == 0) {
            strncpy(temp, "doctest", MAX_INPUT_SIZE);
        } else {
            strncpy(temp, token, MAX_INPUT_SIZE);
        }
        temp += strlen(token);
        temp += 1; // Add space after token
        token = strtok(NULL, " ");
    }

    // Remove the last space added
    if (temp != NULL) {
        temp[-1] = '\0';
    }

    // Copy the modified string to output
    strncpy(output, temp, MAX_INPUT_SIZE);

    // Free the allocated memory
    free(temp);
}

int main() {
    char input[MAX_INPUT_SIZE];
    char output[MAX_INPUT_SIZE];

    // Read input from user
    printf("Enter a string: ");
    fgets(input, MAX_INPUT_SIZE, stdin);

    // Remove newline character if present
    input[strcspn(input, "\n")] = '\0';

    // Replace "tut" with "doctest"
    replace_tut_with_doctest(input, output);

    // Print the modified string
    printf("Modified string: %s\n", output);

    return 0;
}
```