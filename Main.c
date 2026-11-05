/*
 * ============================================================
 *  BASIC BANKING OPERATIONS SIMULATION
 *  Microproject: Develop a C program to simulate basic banking
 *                operations using structures and file handling
 * ============================================================
 *  Files used:
 *     bank.dat          -> binary file storing Account records
 *     transactions.txt  -> text file storing transaction log
 *     temp.dat          -> temporary file used while deleting
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define DATA_FILE   "bank.dat"
#define TEMP_FILE   "temp.dat"
#define TRANS_FILE  "transactions.txt"
#define MIN_BALANCE 500.0
#define FIRST_ACCNO 1001
#define MAX_TRIES   3

/* ---------- Structure that represents one bank account ---------- */
typedef struct {
    int    accNo;            /* unique account number            */
    char   name[50];         /* account holder name              */
    int    pin;              /* 4-digit PIN                      */
    char   type[10];         /* "Savings" or "Current"           */
    double balance;          /* current balance                  */
    char   dateCreated[12];  /* dd-mm-yyyy                       */
} Account;

/* ---------- Function prototypes ---------- */
/* input helpers */
static int  readInt(const char *prompt, int *value);
static int  readDouble(const char *prompt, double *value);
static int  readString(const char *prompt, char *dest, size_t size);
static void pauseScreen(void);
static void printLine(char ch, int n);

/* utility helpers */
static void getDate(char *buf, size_t size);
static void getDateTime(char *buf, size_t size);
static int  equalsIgnoreCase(const char *a, const char *b);
static int  generateAccNo(void);
static int  findAccount(int accNo, Account *out);
static int  saveAccountAt(int index, const Account *acc);
static int  verifyPin(const Account *acc);
static int  askAccountAndPin(Account *acc, int *index);
static void logTransaction(int accNo, const char *type, double amount,
                           double balance, const char *details);

/* banking operations */
static void createAccount(void);
static void depositMoney(void);
static void withdrawMoney(void);
static void balanceEnquiry(void);
static void transferMoney(void);
static void displayAccount(void);
static void displayAllAccounts(void);
static void modifyAccount(void);
static void deleteAccount(void);
static void transactionHistory(void);
static void showMenu(void);

/* ======================== INPUT HELPERS ======================== */

/* Reads an integer safely using fgets + strtol. Returns 1 if valid. */
static int readInt(const char *prompt, int *value)
{
    char buf[64];
    char *end;
    long v;

    printf("%s", prompt);
    if (fgets(buf, sizeof buf, stdin) == NULL)
        return 0;
    v = strtol(buf, &end, 10);
    if (end == buf)
        return 0;
    while (*end == ' ' || *end == '\t')
        end++;
    if (*end != '\n' && *end != '\0')
        return 0;
    *value = (int)v;
    return 1;
}

/* Reads a decimal number safely. Returns 1 if valid. */
static int readDouble(const char *prompt, double *value)
{
    char buf[64];
    char *end;
    double v;

    printf("%s", prompt);
    if (fgets(buf, sizeof buf, stdin) == NULL)
        return 0;
    v = strtod(buf, &end);
    if (end == buf)
        return 0;
    while (*end == ' ' || *end == '\t')
        end++;
    if (*end != '\n' && *end != '\0')
        return 0;
    *value = v;
    return 1;
}

/* Reads a non-empty line of text and removes the newline. */
static int readString(const char *prompt, char *dest, size_t size)
{
    size_t len;

    printf("%s", prompt);
    if (fgets(dest, (int)size, stdin) == NULL)
        return 0;
    len = strlen(dest);
    if (len > 0 && dest[len - 1] == '\n')
        dest[len - 1] = '\0';
    return dest[0] != '\0';
}

static void pauseScreen(void)
{
    char buf[16];
    printf("\nPress Enter to continue...");
    if (fgets(buf, sizeof buf, stdin) == NULL)
        return;
}

static void printLine(char ch, int n)
{
    int i;
    for (i = 0; i < n; i++)
        putchar(ch);
    putchar('\n');
}

/* ======================== UTILITY HELPERS ======================== */

static void getDate(char *buf, size_t size)
{
    time_t now = time(NULL);
    strftime(buf, size, "%d-%m-%Y", localtime(&now));
}

static void getDateTime(char *buf, size_t size)
{
    time_t now = time(NULL);
    strftime(buf, size, "%d-%m-%Y %H:%M:%S", localtime(&now));
}

static int equalsIgnoreCase(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        a++;
        b++;
    }
    return *a == *b;
}

/* Auto-generates a unique account number (last number + 1). */
static int generateAccNo(void)
{
    FILE *fp = fopen(DATA_FILE, "rb");
    Account acc;
    int last = FIRST_ACCNO - 1;

    if (fp == NULL)
        return FIRST_ACCNO;
    while (fread(&acc, sizeof acc, 1, fp) == 1)
        if (acc.accNo > last)
            last = acc.accNo;
    fclose(fp);
    return last + 1;
}

/*
 * Searches bank.dat for an account number.
 * Returns the record index (0,1,2...) or -1 if not found.
 */
static int findAccount(int accNo, Account *out)
{
    FILE *fp = fopen(DATA_FILE, "rb");
    Account acc;
    int index = 0;

    if (fp == NULL)
        return -1;
    while (fread(&acc, sizeof acc, 1, fp) == 1) {
        if (acc.accNo == accNo) {
            if (out != NULL)
                *out = acc;
            fclose(fp);
            return index;
        }
        index++;
    }
    fclose(fp);
    return -1;
}

/* Overwrites one record in bank.dat using fseek. Returns 1 on success. */
static int saveAccountAt(int index, const Account *acc)
{
    FILE *fp = fopen(DATA_FILE, "rb+");

    if (fp == NULL) {
        printf("Error: cannot open %s\n", DATA_FILE);
        return 0;
    }
    fseek(fp, (long)index * (long)sizeof(Account), SEEK_SET);
    if (fwrite(acc, sizeof *acc, 1, fp) != 1) {
        printf("Error: could not write to file.\n");
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return 1;
}

/* Asks for the PIN (max 3 attempts). Returns 1 if correct. */
static int verifyPin(const Account *acc)
{
    int tries, pin;

    for (tries = 1; tries <= MAX_TRIES; tries++) {
        if (!readInt("Enter PIN: ", &pin))
            pin = -1;
        if (pin == acc->pin)
            return 1;
        printf("Incorrect PIN. Attempts left: %d\n", MAX_TRIES - tries);
    }
    printf("Too many wrong attempts. Transaction cancelled.\n");
    return 0;
}

/* Common step: ask account number, find it, verify PIN. */
static int askAccountAndPin(Account *acc, int *index)
{
    int accNo;

    if (!readInt("Enter account number: ", &accNo)) {
        printf("Invalid account number.\n");
        return 0;
    }
    *index = findAccount(accNo, acc);
    if (*index < 0) {
        printf("Account not found.\n");
        return 0;
    }
    return verifyPin(acc);
}

/* Appends one line to transactions.txt (fields separated by '|'). */
static void logTransaction(int accNo, const char *type, double amount,
                           double balance, const char *details)
{
    FILE *fp = fopen(TRANS_FILE, "a");
    char stamp[24];

    if (fp == NULL) {
        printf("Warning: could not write transaction log.\n");
        return;
    }
    getDateTime(stamp, sizeof stamp);
    fprintf(fp, "%s|%d|%s|%.2f|%.2f|%s\n",
            stamp, accNo, type, amount, balance, details);
    fclose(fp);
}

/* ======================== BANKING OPERATIONS ======================== */

static void createAccount(void)
{
    Account acc, other;
    FILE *fp;
    int choice, pin;
    double deposit;
    int duplicate = 0;

    printf("\n--- CREATE NEW ACCOUNT ---\n");
    memset(&acc, 0, sizeof acc);

    if (!readString("Enter account holder name: ", acc.name, sizeof acc.name)) {
        printf("Name cannot be empty.\n");
        return;
    }

    if (!readInt("Account type (1 = Savings, 2 = Current): ", &choice) ||
        (choice != 1 && choice != 2)) {
        printf("Invalid account type.\n");
        return;
    }
    strcpy(acc.type, choice == 1 ? "Savings" : "Current");

    /* duplicate check: same name + same type already exists */
    fp = fopen(DATA_FILE, "rb");
    if (fp != NULL) {
        while (fread(&other, sizeof other, 1, fp) == 1) {
            if (equalsIgnoreCase(other.name, acc.name) &&
                strcmp(other.type, acc.type) == 0) {
                duplicate = 1;
                break;
            }
        }
        fclose(fp);
    }
    if (duplicate) {
        printf("Duplicate account: %s already has a %s account.\n",
               acc.name, acc.type);
        return;
    }

    if (!readInt("Set a 4-digit PIN (1000-9999): ", &pin) ||
        pin < 1000 || pin > 9999) {
        printf("PIN must be a 4-digit number.\n");
        return;
    }
    acc.pin = pin;

    printf("Minimum opening balance is Rs. %.2f\n", MIN_BALANCE);
    if (!readDouble("Enter initial deposit: ", &deposit) ||
        deposit < MIN_BALANCE) {
        printf("Initial deposit must be at least Rs. %.2f\n", MIN_BALANCE);
        return;
    }
    acc.balance = deposit;
    acc.accNo = generateAccNo();
    getDate(acc.dateCreated, sizeof acc.dateCreated);

    fp = fopen(DATA_FILE, "ab");
    if (fp == NULL) {
        printf("Error: cannot open %s\n", DATA_FILE);
        return;
    }
    if (fwrite(&acc, sizeof acc, 1, fp) != 1) {
        printf("Error: could not save account.\n");
        fclose(fp);
        return;
    }
    fclose(fp);

    logTransaction(acc.accNo, "OPEN", deposit, acc.balance, "Account opened");
    printf("\nAccount created successfully!\n");
    printf("Your account number is: %d  (remember it!)\n", acc.accNo);
}

static void depositMoney(void)
{
    Account acc;
    int index;
    double amount;

    printf("\n--- DEPOSIT MONEY ---\n");
    if (!askAccountAndPin(&acc, &index))
        return;

    if (!readDouble("Enter amount to deposit: ", &amount) || amount <= 0) {
        printf("Invalid amount. It must be greater than 0.\n");
        return;
    }
    acc.balance += amount;
    if (!saveAccountAt(index, &acc))
        return;

    logTransaction(acc.accNo, "DEPOSIT", amount, acc.balance, "Cash deposit");
    printf("Deposited Rs. %.2f. New balance: Rs. %.2f\n", amount, acc.balance);
}

static void withdrawMoney(void)
{
    Account acc;
    int index;
    double amount;

    printf("\n--- WITHDRAW MONEY ---\n");
    if (!askAccountAndPin(&acc, &index))
        return;

    if (!readDouble("Enter amount to withdraw: ", &amount) || amount <= 0) {
        printf("Invalid amount. It must be greater than 0.\n");
        return;
    }
    if (acc.balance - amount < MIN_BALANCE) {
        printf("Insufficient balance. Minimum balance of Rs. %.2f must remain.\n",
               MIN_BALANCE);
        printf("Maximum you can withdraw: Rs. %.2f\n",
               acc.balance > MIN_BALANCE ? acc.balance - MIN_BALANCE : 0.0);
        return;
    }
    acc.balance -= amount;
    if (!saveAccountAt(index, &acc))
        return;

    logTransaction(acc.accNo, "WITHDRAW", amount, acc.balance, "Cash withdrawal");
    printf("Withdrawn Rs. %.2f. New balance: Rs. %.2f\n", amount, acc.balance);
}

static void balanceEnquiry(void)
{
    Account acc;
    int index;

    printf("\n--- BALANCE ENQUIRY ---\n");
    if (!askAccountAndPin(&acc, &index))
        return;
    printf("Account %d (%s) balance: Rs. %.2f\n", acc.accNo, acc.name, acc.balance);
}

static void transferMoney(void)
{
    Account from, to;
    int fromIdx, toIdx, toNo;
    double amount;
    char details[50];

    printf("\n--- TRANSFER MONEY ---\n");
    printf("Sender details:\n");
    if (!askAccountAndPin(&from, &fromIdx))
        return;

    if (!readInt("Enter receiver account number: ", &toNo)) {
        printf("Invalid account number.\n");
        return;
    }
    if (toNo == from.accNo) {
        printf("Cannot transfer to the same account.\n");
        return;
    }
    toIdx = findAccount(toNo, &to);
    if (toIdx < 0) {
        printf("Receiver account not found.\n");
        return;
    }

    if (!readDouble("Enter amount to transfer: ", &amount) || amount <= 0) {
        printf("Invalid amount. It must be greater than 0.\n");
        return;
    }
    if (from.balance - amount < MIN_BALANCE) {
        printf("Insufficient balance. Minimum balance of Rs. %.2f must remain.\n",
               MIN_BALANCE);
        return;
    }

    from.balance -= amount;
    to.balance += amount;
    if (!saveAccountAt(fromIdx, &from) || !saveAccountAt(toIdx, &to))
        return;

    snprintf(details, sizeof details, "Transfer to A/C %d", to.accNo);
    logTransaction(from.accNo, "TRANSFER-OUT", amount, from.balance, details);
    snprintf(details, sizeof details, "Transfer from A/C %d", from.accNo);
    logTransaction(to.accNo, "TRANSFER-IN", amount, to.balance, details);

    printf("Transferred Rs. %.2f to account %d (%s).\n", amount, to.accNo, to.name);
    printf("Your new balance: Rs. %.2f\n", from.balance);
}

static void displayAccount(void)
{
    Account acc;
    int index;

    printf("\n--- ACCOUNT DETAILS ---\n");
    if (!askAccountAndPin(&acc, &index))
        return;

    printf("\n");
    printLine('=', 40);
    printf("Account Number : %d\n", acc.accNo);
    printf("Holder Name    : %s\n", acc.name);
    printf("Account Type   : %s\n", acc.type);
    printf("Balance        : Rs. %.2f\n", acc.balance);
    printf("Date Created   : %s\n", acc.dateCreated);
    printLine('=', 40);
}

static void displayAllAccounts(void)
{
    FILE *fp = fopen(DATA_FILE, "rb");
    Account acc;
    int count = 0;
    double total = 0;

    printf("\n--- ALL ACCOUNTS ---\n");
    if (fp == NULL) {
        printf("No accounts found. Create an account first.\n");
        return;
    }
    printLine('-', 74);
    printf("%-8s %-22s %-10s %-14s %-12s\n",
           "Acc No", "Name", "Type", "Balance (Rs.)", "Created");
    printLine('-', 74);
    while (fread(&acc, sizeof acc, 1, fp) == 1) {
        printf("%-8d %-22s %-10s %-14.2f %-12s\n",
               acc.accNo, acc.name, acc.type, acc.balance, acc.dateCreated);
        total += acc.balance;
        count++;
    }
    printLine('-', 74);
    fclose(fp);
    if (count == 0)
        printf("No accounts found.\n");
    else
        printf("Total accounts: %d    Total deposits: Rs. %.2f\n", count, total);
}

static void modifyAccount(void)
{
    Account acc;
    int index, choice, pin;
    char newName[50];

    printf("\n--- MODIFY ACCOUNT ---\n");
    if (!askAccountAndPin(&acc, &index))
        return;

    printf("1. Change name\n2. Change PIN\n3. Change account type\n");
    if (!readInt("Enter choice: ", &choice)) {
        printf("Invalid choice.\n");
        return;
    }

    switch (choice) {
    case 1:
        if (!readString("Enter new name: ", newName, sizeof newName)) {
            printf("Name cannot be empty.\n");
            return;
        }
        strcpy(acc.name, newName);
        break;
    case 2:
        if (!readInt("Enter new 4-digit PIN: ", &pin) || pin < 1000 || pin > 9999) {
            printf("PIN must be a 4-digit number.\n");
            return;
        }
        acc.pin = pin;
        break;
    case 3:
        strcpy(acc.type, strcmp(acc.type, "Savings") == 0 ? "Current" : "Savings");
        printf("Account type changed to %s.\n", acc.type);
        break;
    default:
        printf("Invalid choice.\n");
        return;
    }

    if (saveAccountAt(index, &acc)) {
        logTransaction(acc.accNo, "MODIFY", 0, acc.balance, "Details modified");
        printf("Account updated successfully.\n");
    }
}

/* Deletes by copying every other record to temp.dat, then remove + rename. */
static void deleteAccount(void)
{
    Account acc, rec;
    int index, confirm;
    FILE *src, *tmp;

    printf("\n--- DELETE ACCOUNT ---\n");
    if (!askAccountAndPin(&acc, &index))
        return;

    printf("Are you sure you want to delete account %d? (1 = Yes, 0 = No): ", acc.accNo);
    if (!readInt("", &confirm) || confirm != 1) {
        printf("Deletion cancelled.\n");
        return;
    }

    src = fopen(DATA_FILE, "rb");
    tmp = fopen(TEMP_FILE, "wb");
    if (src == NULL || tmp == NULL) {
        printf("Error: cannot open files.\n");
        if (src) fclose(src);
        if (tmp) fclose(tmp);
        return;
    }
    while (fread(&rec, sizeof rec, 1, src) == 1)
        if (rec.accNo != acc.accNo)
            fwrite(&rec, sizeof rec, 1, tmp);
    fclose(src);
    fclose(tmp);

    if (remove(DATA_FILE) != 0 || rename(TEMP_FILE, DATA_FILE) != 0) {
        printf("Error: could not update the data file.\n");
        return;
    }
    logTransaction(acc.accNo, "CLOSE", 0, 0, "Account deleted");
    printf("Account %d deleted successfully.\n", acc.accNo);
}

static void transactionHistory(void)
{
    Account acc;
    int index, no, found = 0;
    char line[256], stamp[24], type[20], details[60];
    double amount, bal;
    FILE *fp;

    printf("\n--- TRANSACTION HISTORY ---\n");
    if (!askAccountAndPin(&acc, &index))
        return;

    fp = fopen(TRANS_FILE, "r");
    if (fp == NULL) {
        printf("No transactions recorded yet.\n");
        return;
    }
    printf("\n");
    printLine('-', 88);
    printf("%-20s %-14s %-12s %-12s %s\n",
           "Date & Time", "Type", "Amount", "Balance", "Details");
    printLine('-', 88);
    while (fgets(line, sizeof line, fp) != NULL) {
        if (sscanf(line, "%23[^|]|%d|%19[^|]|%lf|%lf|%59[^\n]",
                   stamp, &no, type, &amount, &bal, details) == 6 &&
            no == acc.accNo) {
            printf("%-20s %-14s %-12.2f %-12.2f %s\n",
                   stamp, type, amount, bal, details);
            found++;
        }
    }
    printLine('-', 88);
    fclose(fp);
    if (!found)
        printf("No transactions found for this account.\n");
}

/* ======================== MENU & MAIN ======================== */

static void showMenu(void)
{
    printf("\n");
    printLine('=', 44);
    printf("        BANKING MANAGEMENT SYSTEM\n");
    printLine('=', 44);
    printf("  1. Create New Account\n");
    printf("  2. Deposit Money\n");
    printf("  3. Withdraw Money\n");
    printf("  4. Balance Enquiry\n");
    printf("  5. Transfer Money\n");
    printf("  6. Display Account Details\n");
    printf("  7. Display All Accounts\n");
    printf("  8. Modify Account\n");
    printf("  9. Delete Account\n");
    printf(" 10. Transaction History\n");
    printf("  0. Exit\n");
    printLine('=', 44);
}

int main(void)
{
    int choice;

    do {
        showMenu();
        if (!readInt("Enter your choice: ", &choice)) {
            if (feof(stdin))
                break;
            choice = -1;
        }
        switch (choice) {
        case 1:  createAccount();      break;
        case 2:  depositMoney();       break;
        case 3:  withdrawMoney();      break;
        case 4:  balanceEnquiry();     break;
        case 5:  transferMoney();      break;
        case 6:  displayAccount();     break;
        case 7:  displayAllAccounts(); break;
        case 8:  modifyAccount();      break;
        case 9:  deleteAccount();      break;
        case 10: transactionHistory(); break;
        case 0:  printf("\nThank you for using the Banking System. Goodbye!\n"); break;
        default: printf("Invalid choice. Please enter a number from 0 to 10.\n");
        }
        if (choice != 0)
            pauseScreen();
    } while (choice != 0);

    return 0;
}
