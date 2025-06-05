#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_NAME "credit.dat"
#define MAX_RECORDS 100

// Client record structure
struct clientData {
    unsigned int acctNum;
    char lastName[15];
    char firstName[10];
    double balance;
};

// Function declarations
unsigned int displayMenu(void);
void createTextFile(FILE *readPtr);
void updateAccount(FILE *fPtr);
void addAccount(FILE *fPtr);
void deleteAccount(FILE *fPtr);
void initializeDataFile(void);
void displayAccountsTextFile(void); // NEW

int main() {
    FILE *cfPtr;
    unsigned int choice;

    do {
        choice = displayMenu();
        switch (choice) {
            case 1:
                cfPtr = fopen(FILE_NAME, "rb");
                if (!cfPtr) {
                    puts("Cannot open file.");
                    break;
                }
                createTextFile(cfPtr);
                fclose(cfPtr);
                break;
            case 2:
                cfPtr = fopen(FILE_NAME, "rb+");
                if (!cfPtr) {
                    puts("Cannot open file.");
                    break;
                }
                updateAccount(cfPtr);
                fclose(cfPtr);
                break;
            case 3:
                cfPtr = fopen(FILE_NAME, "rb+");
                if (!cfPtr) {
                    puts("Cannot open file.");
                    break;
                }
                addAccount(cfPtr);
                fclose(cfPtr);
                break;
            case 4:
                cfPtr = fopen(FILE_NAME, "rb+");
                if (!cfPtr) {
                    puts("Cannot open file.");
                    break;
                }
                deleteAccount(cfPtr);
                fclose(cfPtr);
                break;
            case 5:
                initializeDataFile();
                break;
            case 6:
                displayAccountsTextFile(); // NEW
                break;
            case 7:
                puts("Exiting program.");
                break;
            default:
                puts("Invalid choice. Please try again.");
        }
    } while (choice != 7);

    return 0;
}

unsigned int displayMenu(void) {
    unsigned int choice;
    puts("\n*** Account Management Menu ***");
    puts("1. Export accounts to text file (accounts.txt)");
    puts("2. Update an existing account");
    puts("3. Add a new account");
    puts("4. Delete an account");
    puts("5. Initialize blank data file");
    puts("6. Display accounts (table format)");
    puts("7. Exit");
    printf("Enter your choice: ");
    scanf("%u", &choice);
    return choice;
}

void createTextFile(FILE *readPtr) {
    FILE *outFile = fopen("accounts.txt", "w");
    if (!outFile) {
        puts("Failed to create accounts.txt.");
        return;
    }

    struct clientData client;
    fprintf(outFile, "%-6s %-15s %-10s %10s\n", "Account No", "Last Name", "First Name", "Balance");
    rewind(readPtr);

    while (fread(&client, sizeof(client), 1, readPtr)) {
        if (client.acctNum != 0) {
            fprintf(outFile, "%-6u %-15s %-10s %10.2f\n",
                    client.acctNum, client.lastName, client.firstName, client.balance);
        }
    }

    fclose(outFile);
    puts("Data successfully written to accounts.txt.");
}

void updateAccount(FILE *fPtr) {
    struct clientData client;
    unsigned int acct;
    double transaction;

    printf("Enter account number to update (1-%d): ", MAX_RECORDS);
    scanf("%u", &acct);
    if (acct < 1 || acct > MAX_RECORDS) {
        puts("Invalid account number.");
        return;
    }

    fseek(fPtr, (acct - 1) * sizeof(client), SEEK_SET);
    fread(&client, sizeof(client), 1, fPtr);

    if (client.acctNum == 0) {
        printf("No data found for account #%u.\n", acct);
    } else {
        printf("Current balance: %.2f\n", client.balance);
        printf("Enter transaction amount (+charge / -payment): ");
        scanf("%lf", &transaction);
        client.balance += transaction;

        fseek(fPtr, -(long)sizeof(client), SEEK_CUR);
        fwrite(&client, sizeof(client), 1, fPtr);

        printf("Updated balance for account #%u: %.2f\n", client.acctNum, client.balance);
    }
}

void addAccount(FILE *fPtr) {
    struct clientData client = {0};
    unsigned int acct;

    printf("Enter new account number (1-%d): ", MAX_RECORDS);
    scanf("%u", &acct);
    if (acct < 1 || acct > MAX_RECORDS) {
        puts("Invalid account number.");
        return;
    }

    fseek(fPtr, (acct - 1) * sizeof(client), SEEK_SET);
    fread(&client, sizeof(client), 1, fPtr);

    if (client.acctNum != 0) {
        printf("Account #%u already exists.\n", acct);
    } else {
        printf("Enter last name, first name, balance: ");
        scanf("%14s%9s%lf", client.lastName, client.firstName, &client.balance);
        client.acctNum = acct;

        fseek(fPtr, (acct - 1) * sizeof(client), SEEK_SET);
        fwrite(&client, sizeof(client), 1, fPtr);

        puts("Account added successfully.");
    }
}

void deleteAccount(FILE *fPtr) {
    struct clientData client, blank = {0};
    unsigned int acct;

    printf("Enter account number to delete (1-%d): ", MAX_RECORDS);
    scanf("%u", &acct);
    if (acct < 1 || acct > MAX_RECORDS) {
        puts("Invalid account number.");
        return;
    }

    fseek(fPtr, (acct - 1) * sizeof(client), SEEK_SET);
    fread(&client, sizeof(client), 1, fPtr);

    if (client.acctNum == 0) {
        printf("Account #%u does not exist.\n", acct);
    } else {
        fseek(fPtr, (acct - 1) * sizeof(client), SEEK_SET);
        fwrite(&blank, sizeof(client), 1, fPtr);
        printf("Account #%u deleted.\n", acct);
    }
}

void initializeDataFile(void) {
    FILE *fp = fopen(FILE_NAME, "wb");
    if (!fp) {
        puts("Error creating data file.");
        return;
    }

    struct clientData blank = {0};
    for (int i = 0; i < MAX_RECORDS; ++i) {
        fwrite(&blank, sizeof(blank), 1, fp);
    }

    fclose(fp);
    puts("Data file initialized with 100 blank records.");
}

// NEW: Display the content of accounts.txt in table format
// Modified function to display accounts in alphabetical order by last name
void displayAccountsTextFile(void) {
    FILE *fp = fopen(FILE_NAME, "rb");
    if (!fp) {
        puts("Data file could not be opened.");
        return;
    }

    struct clientData accounts[MAX_RECORDS];
    struct clientData validAccounts[MAX_RECORDS];
    int validCount = 0;
    
    // Read all records from binary file
    fread(accounts, sizeof(struct clientData), MAX_RECORDS, fp);
    fclose(fp);
    
    // Extract only valid accounts (non-zero account numbers)
    for (int i = 0; i < MAX_RECORDS; i++) {
        if (accounts[i].acctNum != 0) {
            validAccounts[validCount] = accounts[i];
            validCount++;
        }
    }
    
    // Sort valid accounts alphabetically by last name using bubble sort
    for (int i = 0; i < validCount - 1; i++) {
        for (int j = 0; j < validCount - i - 1; j++) {
            if (strcmp(validAccounts[j].lastName, validAccounts[j + 1].lastName) > 0) {
                // Swap accounts
                struct clientData temp = validAccounts[j];
                validAccounts[j] = validAccounts[j + 1];
                validAccounts[j + 1] = temp;
            }
        }
    }
    
    // Display sorted accounts
    puts("\n******** Account Records (Alphabetical Order) ********\n");
    printf("%-10s %-15s %-12s %10s\n", "Account No", "Last Name", "First Name", "Balance");
    printf("%-10s %-15s %-12s %10s\n", "----------", "---------------", "------------", "----------");
    
    for (int i = 0; i < validCount; i++) {
        printf("%-10u %-15s %-12s %10.2f\n",
               validAccounts[i].acctNum,
               validAccounts[i].lastName,
               validAccounts[i].firstName,
               validAccounts[i].balance);
    }
    
    if (validCount == 0) {
        puts("No accounts found.");
    }
    
    puts("\n***************************************************\n");
}
