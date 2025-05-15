#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void editor(const char *filename);
extern void print_syntax_highlighted(const char *line, int is_selected);

void create_file(const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file)
    {
        fclose(file);
        printf("File created successfully.\n");
    }
    else
    {
        printf("Failed to create file.\n");
    }
}
void view_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        printf("File not found.\n");
        return;
    }
    printf("Contents of %s:\n", filename);
    char ch[256];
    while (fgets(ch, sizeof(ch) ,file))
    {
        print_syntax_highlighted(ch,0);
    }
    fclose(file);
    printf("\nPress Enter to continue...\n");
    getchar();
}
void update_file(const char *filename)
{
    editor(filename);
}
void search_in_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        printf("File not found.\n");
        return;
    }
    printf("Enter the word to search: ");
    char word[100], line[256];
    scanf("%s", word);
    getchar();
    int found = 0, line_number = 1;
    while (fgets(line, sizeof(line), file))
    {
        if (strstr(line, word))
        {
            printf("Found '%s' on line %d: %s", word, line_number, line);
            found = 1;
        }
        line_number++;
    }
    if (!found)
    {
        printf("'%s' not found in the file.\n", word);
    }
    fclose(file);
    printf("Press Enter to continue...\n");
    getchar();
}
int main()
{
    while (1)
    {
        printf("Basic Text Editor\n");
        printf("1. Create File\n");
        printf("2. View File\n");
        printf("3. Update File\n");
        printf("4. Search in File\n");
        printf("5. Exit\n");
        printf("Enter your choice: ");
        int choice;
        scanf("%d", &choice);
        getchar();
        char filename[100];
        switch (choice)
        {
        case 1:
            printf("Enter file name to create: ");
            scanf("%s", filename);
            getchar();
            create_file(filename);
            break;
        case 2:
            printf("Enter file name to view: ");
            scanf("%s", filename);
            getchar();
            view_file(filename);
            break;
        case 3:
            printf("Enter file name to update: ");
            scanf("%s", filename);
            getchar();
            update_file(filename);
            break;
        case 4:
            printf("Enter file name to search: ");
            scanf("%s", filename);
            getchar();
            search_in_file(filename);
            break;
        case 5:
            exit(0);
        default:
            printf("Invalid choice. Try again.\n");
        }
    }
}