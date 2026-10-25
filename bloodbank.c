/*
 * ==================================================================
 *  LifeLink - Blood Bank & Donor Finder System
 * ------------------------------------------------------------------
 *  Problem statement:
 *    In a medical emergency, finding a donor whose blood group is
 *    compatible with the patient AND who is currently eligible to
 *    donate (at least 90 days since the last donation) wastes
 *    precious time when records are kept on paper or in chat groups.
 *    LifeLink keeps a donor registry and the blood stock in one
 *    place, finds eligible compatible donors instantly, and warns
 *    the blood bank when any blood group is running low.
 *
 *  Syllabus concepts used (Modules 1 - 4):
 *    M1: constants, variables, operators, expressions, selection
 *        (if / switch), branching and looping statements, I/O.
 *    M2: arrays, function prototypes / definitions / calls, call by
 *        value and by reference, scope of variables, recursion
 *        (binary search).
 *    M3: pointers, pointer arithmetic, malloc / calloc / realloc /
 *        free, pointer to functions (qsort comparators), pointers
 *        and arrays.
 *    M4: structures, union (search key), file operations fopen /
 *        fread / fwrite / fseek / fprintf, binary file read & write.
 *
 *  Name   : M.V. VAMSI KRISHNA
 *  Reg No : 25BEV7052
 *  Course : Introduction to Programming (Slot F2+TF2)
 * ==================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define DATA_FILE     "lifelink.dat"
#define REPORT_FILE   "donor_report.txt"
#define NUM_GROUPS    8
#define MIN_GAP_DAYS  90   /* minimum days between two whole-blood donations */
#define LOW_STOCK     5    /* alert when units fall below this */
#define MIN_AGE       18
#define MAX_AGE       65

typedef struct {
    int d, m, y;           /* y == 0 means "never" */
} Date;

typedef struct {
    int  id;
    char name[40];
    int  age;
    char gender;           /* M / F / O */
    char group[4];         /* e.g. "AB+" */
    char phone[11];        /* 10 digits */
    char city[30];
    Date lastDonation;
    int  totalDonations;
} Donor;

typedef struct {
    Donor *list;           /* dynamic array, always sorted by id */
    int count;
    int capacity;
    int nextId;
} Registry;

/* search modes */
#define BY_GROUP 1
#define BY_CITY  2
#define BY_ID    3

/*
 * A search uses only ONE key at a time (a group, a city or an id),
 * so the keys share the same memory in a union instead of a struct.
 */
typedef union {
    int  id;
    char group[4];
    char city[30];
} SearchKey;

/* file layout: count | nextId | stock[8] | Donor records */
#define HEADER_SIZE ((long)((2 + NUM_GROUPS) * sizeof(int)))

static const char *GROUPS[NUM_GROUPS] = {"A+", "A-", "B+", "B-", "AB+", "AB-", "O+", "O-"};
static int  stock[NUM_GROUPS];  /* units of blood available per group */
static Date today;

/* ------------------------------------------------------------------
 *  String helpers
 * ------------------------------------------------------------------ */

static void trim(char *s)
{
    char *start = s;
    int n;

    while (*start && isspace((unsigned char)*start))
        start++;
    memmove(s, start, strlen(start) + 1);

    n = (int)strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1]))
        s[--n] = '\0';
}

/* case-insensitive compare (portable replacement for strcasecmp) */
static int compareIgnoreCase(const char *a, const char *b)
{
    while (*a && *b) {
        int diff = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (diff != 0)
            return diff;
        a++;
        b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

/* ------------------------------------------------------------------
 *  Safe input functions (fgets based, never leave junk in stdin)
 * ------------------------------------------------------------------ */

static void readLine(const char *prompt, char *buf, int size)
{
    printf("%s", prompt);
    fflush(stdout);

    if (fgets(buf, size, stdin) == NULL) {
        printf("\nInput ended. Exiting.\n");
        exit(0);
    }
    if (strchr(buf, '\n') == NULL) {        /* line too long: discard the rest */
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
    }
    trim(buf);
}

static int readInt(const char *prompt, int min, int max)
{
    char buf[32];
    char *end;
    long value;

    for (;;) {
        readLine(prompt, buf, sizeof buf);
        value = strtol(buf, &end, 10);
        if (end != buf && *end == '\0' && value >= min && value <= max)
            return (int)value;
        printf("  ! Please enter a number between %d and %d.\n", min, max);
    }
}

static int readYesNo(const char *prompt)
{
    char buf[8];

    for (;;) {
        readLine(prompt, buf, sizeof buf);
        if (buf[0] == 'y' || buf[0] == 'Y')
            return 1;
        if (buf[0] == 'n' || buf[0] == 'N')
            return 0;
        printf("  ! Please answer y or n.\n");
    }
}

/* letters, spaces and dots only (names and cities) */
static void readWords(const char *prompt, char *buf, int size)
{
    for (;;) {
        int ok = 1, i;

        readLine(prompt, buf, size);
        if (buf[0] == '\0')
            ok = 0;
        for (i = 0; buf[i] && ok; i++)
            if (!isalpha((unsigned char)buf[i]) && buf[i] != ' ' && buf[i] != '.')
                ok = 0;
        if (ok)
            return;
        printf("  ! Use letters and spaces only.\n");
    }
}

static void readPhone(const char *prompt, char *buf)
{
    char tmp[32];

    for (;;) {
        int ok, i;

        readLine(prompt, tmp, sizeof tmp);
        ok = strlen(tmp) == 10 && tmp[0] >= '6' && tmp[0] <= '9';
        for (i = 0; tmp[i] && ok; i++)
            if (!isdigit((unsigned char)tmp[i]))
                ok = 0;
        if (ok) {
            strcpy(buf, tmp);
            return;
        }
        printf("  ! Enter a valid 10-digit mobile number (starts with 6-9).\n");
    }
}

static char readGender(const char *prompt)
{
    char buf[8];

    for (;;) {
        char g;

        readLine(prompt, buf, sizeof buf);
        g = (char)toupper((unsigned char)buf[0]);
        if (buf[1] == '\0' && (g == 'M' || g == 'F' || g == 'O'))
            return g;
        printf("  ! Enter M, F or O.\n");
    }
}

/* ------------------------------------------------------------------
 *  Blood group logic
 * ------------------------------------------------------------------ */

/* returns index into GROUPS, or -1 if the text is not a blood group */
static int groupIndex(const char *g)
{
    int i;

    for (i = 0; i < NUM_GROUPS; i++)
        if (compareIgnoreCase(g, GROUPS[i]) == 0)
            return i;
    return -1;
}

static void readBloodGroup(const char *prompt, char *out)
{
    char buf[16];

    for (;;) {
        int idx;

        readLine(prompt, buf, sizeof buf);
        idx = groupIndex(buf);
        if (idx >= 0) {
            strcpy(out, GROUPS[idx]);   /* store in standard upper-case form */
            return;
        }
        printf("  ! Valid groups: A+ A- B+ B- AB+ AB- O+ O-\n");
    }
}

/*
 * Red-cell compatibility rule:
 *   - every antigen (A, B) the donor has, the recipient must also have
 *   - an Rh+ donor can only give to an Rh+ recipient
 */
static int canDonate(const char *donor, const char *recipient)
{
    if (strchr(donor, 'A') && !strchr(recipient, 'A'))
        return 0;
    if (strchr(donor, 'B') && !strchr(recipient, 'B'))
        return 0;
    if (donor[strlen(donor) - 1] == '+' && recipient[strlen(recipient) - 1] == '-')
        return 0;
    return 1;
}

/* ------------------------------------------------------------------
 *  Date logic
 * ------------------------------------------------------------------ */

static int isLeap(int y)
{
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

static int daysInMonth(int m, int y)
{
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (m == 2 && isLeap(y)) ? 29 : days[m - 1];
}

static int isValidDate(Date dt)
{
    return dt.y >= 1950 && dt.y <= 2100 &&
           dt.m >= 1 && dt.m <= 12 &&
           dt.d >= 1 && dt.d <= daysInMonth(dt.m, dt.y);
}

/* converts a date to a day number so two dates can be subtracted */
static long dateToDays(Date dt)
{
    int y = dt.y, m = dt.m;

    if (m < 3) {
        y--;
        m += 12;
    }
    return 365L * y + y / 4 - y / 100 + y / 400 + (153 * (m - 3) + 2) / 5 + dt.d;
}

static Date getToday(void)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    Date dt;

    dt.d = t->tm_mday;
    dt.m = t->tm_mon + 1;
    dt.y = t->tm_year + 1900;
    return dt;
}

static void formatDate(Date dt, char *out)
{
    if (dt.y == 0)
        strcpy(out, "Never");
    else
        sprintf(out, "%02d/%02d/%04d", dt.d, dt.m, dt.y);
}

static Date readDate(const char *prompt)
{
    char buf[32];
    Date dt;

    for (;;) {
        readLine(prompt, buf, sizeof buf);
        if (strcmp(buf, "0") == 0) {
            dt.d = dt.m = dt.y = 0;
            return dt;
        }
        if (sscanf(buf, "%d/%d/%d", &dt.d, &dt.m, &dt.y) == 3 && isValidDate(dt)) {
            if (dateToDays(dt) <= dateToDays(today))
                return dt;
            printf("  ! Date cannot be in the future.\n");
        } else {
            printf("  ! Invalid date. Use dd/mm/yyyy, or 0 if never donated.\n");
        }
    }
}

/* 0 = eligible now, otherwise number of days the donor still has to wait */
static int daysUntilEligible(const Donor *d)
{
    long gap;

    if (d->lastDonation.y == 0)
        return 0;
    gap = dateToDays(today) - dateToDays(d->lastDonation);
    return gap >= MIN_GAP_DAYS ? 0 : (int)(MIN_GAP_DAYS - gap);
}

/* ------------------------------------------------------------------
 *  Registry (dynamic array) operations
 * ------------------------------------------------------------------ */

static void ensureCapacity(Registry *r)
{
    Donor *bigger;
    int newCapacity;

    if (r->count < r->capacity)
        return;
    newCapacity = r->capacity ? r->capacity * 2 : 8;
    if (r->list == NULL)
        bigger = malloc(newCapacity * sizeof(Donor));             /* first block */
    else
        bigger = realloc(r->list, newCapacity * sizeof(Donor));   /* grow: 8, 16, 32 ... */
    if (bigger == NULL) {
        printf("Out of memory!\n");
        exit(1);
    }
    r->list = bigger;
    r->capacity = newCapacity;
}

/* recursive binary search - works because the list stays sorted by id */
static int findById(const Donor *list, int low, int high, int id)
{
    int mid;

    if (low > high)
        return -1;
    mid = low + (high - low) / 2;
    if (list[mid].id == id)
        return mid;
    if (list[mid].id < id)
        return findById(list, mid + 1, high, id);
    return findById(list, low, mid - 1, id);
}

static int askForDonor(const Registry *r)
{
    int id = readInt("Enter donor ID: ", 1, 999999);
    int idx = findById(r->list, 0, r->count - 1, id);

    if (idx < 0)
        printf("  ! No donor with ID %d.\n", id);
    return idx;
}

/* ------------------------------------------------------------------
 *  File handling
 *  Layout: count | nextId | stock[8] | Donor records
 * ------------------------------------------------------------------ */

static int saveData(const Registry *r)
{
    FILE *fp = fopen(DATA_FILE, "wb");

    if (fp == NULL) {
        printf("  ! Could not save data to %s\n", DATA_FILE);
        return 0;
    }
    fwrite(&r->count, sizeof(int), 1, fp);
    fwrite(&r->nextId, sizeof(int), 1, fp);
    fwrite(stock, sizeof(int), NUM_GROUPS, fp);
    fwrite(r->list, sizeof(Donor), (size_t)r->count, fp);
    fclose(fp);
    return 1;
}

/*
 * Random access: after a donation or an update only the stock and ONE
 * donor record change, so fseek jumps straight to them in the file
 * instead of rewriting the whole file.
 */
static int saveRecord(const Registry *r, int idx)
{
    FILE *fp = fopen(DATA_FILE, "r+b");

    if (fp == NULL)
        return saveData(r);
    fseek(fp, (long)(2 * sizeof(int)), SEEK_SET);                  /* stock */
    fwrite(stock, sizeof(int), NUM_GROUPS, fp);
    fseek(fp, HEADER_SIZE + (long)idx * (long)sizeof(Donor), SEEK_SET);   /* record idx */
    fwrite(&r->list[idx], sizeof(Donor), 1, fp);
    fclose(fp);
    return 1;
}

static int loadData(Registry *r)
{
    FILE *fp = fopen(DATA_FILE, "rb");
    int count, nextId;
    long fileSize;

    if (fp == NULL)
        return 0;

    /* fseek to the end to find the file size, then back to the start */
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (fread(&count, sizeof(int), 1, fp) != 1 ||
        fread(&nextId, sizeof(int), 1, fp) != 1 ||
        fread(stock, sizeof(int), NUM_GROUPS, fp) != NUM_GROUPS ||
        count < 0 ||
        fileSize != HEADER_SIZE + (long)count * (long)sizeof(Donor)) {
        printf("  ! %s is damaged - starting with sample data.\n", DATA_FILE);
        fclose(fp);
        return 0;
    }

    r->capacity = count > 8 ? count : 8;
    r->list = calloc((size_t)r->capacity, sizeof(Donor));   /* zero-filled */
    if (r->list == NULL || fread(r->list, sizeof(Donor), (size_t)count, fp) != (size_t)count) {
        free(r->list);
        r->list = NULL;
        r->capacity = 0;
        fclose(fp);
        return 0;
    }
    r->count = count;
    r->nextId = nextId;
    fclose(fp);
    return 1;
}

/* used on the very first run so the program has data to demonstrate */
static void loadSampleData(Registry *r)
{
    static const Donor samples[] = {
        {0, "Ravi Kumar",       24, 'M', "O+",  "9876543210", "Amaravati",  {12,  5, 2026}, 3},
        {0, "Sneha Reddy",      21, 'F', "A+",  "9123456780", "Vijayawada", { 1,  9, 2026}, 1},
        {0, "Arjun Varma",      30, 'M', "B+",  "9988776655", "Guntur",     { 0,  0,    0}, 0},
        {0, "Priya Sharma",     27, 'F', "O-",  "8877665544", "Amaravati",  {15,  6, 2026}, 2},
        {0, "Mohammed Imran",   35, 'M', "AB+", "7766554433", "Vijayawada", {20,  3, 2026}, 5},
        {0, "Divya Lakshmi",    22, 'F', "A-",  "9012345678", "Guntur",     {10, 10, 2026}, 1},
        {0, "Karthik Naidu",    29, 'M', "B-",  "8123456789", "Amaravati",  { 5,  1, 2026}, 4},
        {0, "Anjali Rao",       26, 'F', "AB-", "6301234567", "Vijayawada", { 0,  0,    0}, 0},
        {0, "Suresh Babu",      45, 'M', "O+",  "9440012345", "Guntur",     {30,  8, 2026}, 6},
        {0, "Lakshmi Prasanna", 32, 'F', "A+",  "9550067890", "Amaravati",  { 2,  2, 2026}, 2}
    };
    static const int sampleStock[NUM_GROUPS] = {12, 3, 15, 2, 7, 1, 20, 4};
    int i, n = (int)(sizeof samples / sizeof samples[0]);

    r->count = 0;
    r->nextId = 1;
    for (i = 0; i < n; i++) {
        ensureCapacity(r);
        r->list[r->count] = samples[i];
        r->list[r->count].id = r->nextId++;
        r->count++;
    }
    memcpy(stock, sampleStock, sizeof stock);
}

/* ------------------------------------------------------------------
 *  Display helpers
 * ------------------------------------------------------------------ */

static void printLine(void)
{
    printf("-------------------------------------------------------------------------------------------\n");
}

static void printHeader(void)
{
    printLine();
    printf("%-4s %-18s %-3s %-3s %-4s %-10s  %-11s %-12s %-5s %s\n",
           "ID", "Name", "Age", "Sex", "Grp", "Phone", "City", "Last Donated", "Times", "Status");
    printLine();
}

static void printDonor(const Donor *d)
{
    char last[16], status[16];
    int wait = daysUntilEligible(d);

    formatDate(d->lastDonation, last);
    if (wait == 0)
        strcpy(status, "Eligible");
    else
        sprintf(status, "Wait %dd", wait);

    printf("%-4d %-18.18s %-3d  %c  %-4s %-10s  %-11.11s %-12s %-5d %s\n",
           d->id, d->name, d->age, d->gender, d->group, d->phone,
           d->city, last, d->totalDonations, status);
}

static void printBar(int n, char symbol)
{
    int i;

    for (i = 0; i < n && i < 40; i++)
        putchar(symbol);
}

static int lowStockAlert(void)
{
    int i, alerts = 0;

    for (i = 0; i < NUM_GROUPS; i++) {
        if (stock[i] < LOW_STOCK) {
            if (alerts == 0)
                printf("\n  *** LOW STOCK ALERT ***\n");
            printf("  %-4s only %d unit(s) left - organise a donation drive!\n", GROUPS[i], stock[i]);
            alerts++;
        }
    }
    return alerts;
}

/* ------------------------------------------------------------------
 *  Sorting comparators (used through function pointers with qsort)
 * ------------------------------------------------------------------ */

static int byName(const void *a, const void *b)
{
    const Donor *x = *(const Donor *const *)a;
    const Donor *y = *(const Donor *const *)b;
    return compareIgnoreCase(x->name, y->name);
}

static int byGroup(const void *a, const void *b)
{
    const Donor *x = *(const Donor *const *)a;
    const Donor *y = *(const Donor *const *)b;
    int diff = groupIndex(x->group) - groupIndex(y->group);
    return diff ? diff : compareIgnoreCase(x->name, y->name);
}

static int byCity(const void *a, const void *b)
{
    const Donor *x = *(const Donor *const *)a;
    const Donor *y = *(const Donor *const *)b;
    int diff = compareIgnoreCase(x->city, y->city);
    return diff ? diff : compareIgnoreCase(x->name, y->name);
}

/* ------------------------------------------------------------------
 *  Menu features
 * ------------------------------------------------------------------ */

static void registerDonor(Registry *r)
{
    Donor d;

    printf("\n--- Register New Donor ---\n");
    readWords("Full name            : ", d.name, sizeof d.name);
    d.age = readInt("Age (18-65)          : ", MIN_AGE, MAX_AGE);
    d.gender = readGender("Gender (M/F/O)       : ");
    readBloodGroup("Blood group          : ", d.group);
    readPhone("Mobile number        : ", d.phone);
    readWords("City                 : ", d.city, sizeof d.city);
    d.lastDonation = readDate("Last donation date (dd/mm/yyyy, 0 if never): ");
    d.totalDonations = d.lastDonation.y ? 1 : 0;

    ensureCapacity(r);
    d.id = r->nextId++;
    r->list[r->count++] = d;
    saveData(r);

    printf("\n  Donor registered successfully with ID %d.\n", d.id);
    if (daysUntilEligible(&d) == 0)
        printf("  %s is eligible to donate right now.\n", d.name);
    else
        printf("  %s can donate again in %d day(s).\n", d.name, daysUntilEligible(&d));
}

static void listDonors(const Registry *r)
{
    const Donor **view;
    int (*compare)(const void *, const void *) = NULL;
    int i, choice;

    if (r->count == 0) {
        printf("\n  No donors registered yet.\n");
        return;
    }

    printf("\nSort by: 1. ID   2. Name   3. Blood group   4. City\n");
    choice = readInt("Choice: ", 1, 4);
    switch (choice) {
    case 2: compare = byName;  break;
    case 3: compare = byGroup; break;
    case 4: compare = byCity;  break;
    }

    /* sort an array of pointers so the original list keeps its id order */
    view = calloc((size_t)r->count, sizeof *view);
    if (view == NULL) {
        printf("Out of memory!\n");
        return;
    }
    for (i = 0; i < r->count; i++)
        view[i] = r->list + i;          /* same address as &r->list[i] */
    if (compare)
        qsort(view, (size_t)r->count, sizeof *view, compare);

    printf("\n");
    printHeader();
    for (i = 0; i < r->count; i++)
        printDonor(view[i]);
    printLine();
    printf("Total donors: %d\n", r->count);
    free(view);
}

static int matchesKey(const Donor *d, int mode, const SearchKey *key)
{
    switch (mode) {
    case BY_GROUP: return strcmp(d->group, key->group) == 0;
    case BY_CITY:  return compareIgnoreCase(d->city, key->city) == 0;
    default:       return d->id == key->id;
    }
}

static void searchDonors(const Registry *r)
{
    SearchKey key;
    const Donor *d;
    int mode, found = 0, eligible = 0;

    printf("\n--- Search Donors ---\n");
    printf("Search by: 1. Blood group   2. City   3. Donor ID\n");
    mode = readInt("Choice: ", 1, 3);
    switch (mode) {
    case BY_GROUP: readBloodGroup("Blood group : ", key.group);             break;
    case BY_CITY:  readWords("City        : ", key.city, sizeof key.city);  break;
    default:       key.id = readInt("Donor ID    : ", 1, 999999);          break;
    }

    printf("\n");
    /* pointer arithmetic: d walks from the first donor to the last */
    for (d = r->list; d < r->list + r->count; d++) {
        if (!matchesKey(d, mode, &key))
            continue;
        if (found == 0)
            printHeader();
        printDonor(d);
        found++;
        if (daysUntilEligible(d) == 0)
            eligible++;
    }

    if (found == 0) {
        printf("  No matching donor found.\n");
        return;
    }
    printLine();
    printf("Found %d donor(s), %d eligible to donate now.\n", found, eligible);
}

static void emergencyFinder(const Registry *r)
{
    char patient[4], city[30];
    int i, g, pass, found = 0, units = 0;
    const Donor *soonest = NULL;

    printf("\n--- EMERGENCY: Find Compatible Donors ---\n");
    readBloodGroup("Patient blood group   : ", patient);
    readLine("City (Enter for all)  : ", city, sizeof city);

    printf("\nPatient %s can receive from: ", patient);
    for (g = 0; g < NUM_GROUPS; g++) {
        if (canDonate(GROUPS[g], patient)) {
            printf("%s ", GROUPS[g]);
            units += stock[g];
        }
    }
    printf("\nCompatible units in blood bank right now: %d\n\n", units);

    /* pass 0: exact blood-group match first, pass 1: other compatible groups */
    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < r->count; i++) {
            const Donor *d = &r->list[i];
            int exact = strcmp(d->group, patient) == 0;

            if (!canDonate(d->group, patient))
                continue;
            if (city[0] && compareIgnoreCase(d->city, city) != 0)
                continue;
            if ((pass == 0) != exact)
                continue;

            if (daysUntilEligible(d) > 0) {
                if (soonest == NULL || daysUntilEligible(d) < daysUntilEligible(soonest))
                    soonest = d;
                continue;
            }
            if (found == 0)
                printHeader();
            printDonor(d);
            found++;
        }
    }

    if (found > 0) {
        printLine();
        printf("%d eligible donor(s) found. Exact matches are listed first - call them now.\n", found);
    } else {
        printf("  No eligible compatible donor available right now.\n");
        if (soonest)
            printf("  Next available: %s (%s, %s) in %d day(s), phone %s.\n",
                   soonest->name, soonest->group, soonest->city,
                   daysUntilEligible(soonest), soonest->phone);
    }
}

static void recordDonation(Registry *r)
{
    Donor *d;
    int idx, wait, g;

    printf("\n--- Record a Donation ---\n");
    idx = askForDonor(r);
    if (idx < 0)
        return;
    d = &r->list[idx];

    printHeader();
    printDonor(d);
    printLine();

    wait = daysUntilEligible(d);
    if (wait > 0) {
        printf("  ! %s donated recently and must wait %d more day(s). Donation not recorded.\n",
               d->name, wait);
        return;
    }
    if (!readYesNo("Record one unit donated today? (y/n): "))
        return;

    d->lastDonation = today;
    d->totalDonations++;
    g = groupIndex(d->group);
    stock[g]++;
    saveRecord(r, idx);

    printf("\n  Thank you, %s! Donation #%d recorded.\n", d->name, d->totalDonations);
    printf("  %s stock is now %d unit(s). Next eligible donation in %d days.\n",
           d->group, stock[g], MIN_GAP_DAYS);
}

static void updateDonor(Registry *r)
{
    Donor *d;
    int idx, choice;

    printf("\n--- Update Donor Details ---\n");
    idx = askForDonor(r);
    if (idx < 0)
        return;
    d = &r->list[idx];

    printHeader();
    printDonor(d);
    printLine();

    printf("1. Phone   2. City   3. Age   4. Cancel\n");
    choice = readInt("Choice: ", 1, 4);
    switch (choice) {
    case 1: readPhone("New mobile number: ", d->phone);           break;
    case 2: readWords("New city: ", d->city, sizeof d->city);     break;
    case 3: d->age = readInt("New age: ", MIN_AGE, MAX_AGE);      break;
    default: return;
    }
    saveRecord(r, idx);
    printf("  Donor %d updated.\n", d->id);
}

static void deleteDonor(Registry *r)
{
    int idx;

    printf("\n--- Remove Donor ---\n");
    idx = askForDonor(r);
    if (idx < 0)
        return;

    printHeader();
    printDonor(&r->list[idx]);
    printLine();
    if (!readYesNo("Remove this donor permanently? (y/n): "))
        return;

    /* shift the remaining donors left; order by id is preserved */
    memmove(&r->list[idx], &r->list[idx + 1], (size_t)(r->count - idx - 1) * sizeof(Donor));
    r->count--;
    saveData(r);
    printf("  Donor removed.\n");
}

static void viewStock(void)
{
    int g, total = 0;

    printf("\n--- Blood Stock ---\n");
    for (g = 0; g < NUM_GROUPS; g++) {
        printf("  %-4s %3d unit(s) |", GROUPS[g], stock[g]);
        printBar(stock[g], '#');
        if (stock[g] < LOW_STOCK)
            printf("  <-- LOW");
        printf("\n");
        total += stock[g];
    }
    printf("  Total: %d unit(s)\n", total);
    lowStockAlert();
}

static void issueBlood(Registry *r)
{
    char patient[4], other[4];
    int units, g, alt, anyAlt = 0;

    printf("\n--- Issue Blood to a Patient ---\n");
    readBloodGroup("Patient blood group : ", patient);
    units = readInt("Units required (1-10): ", 1, 10);
    g = groupIndex(patient);

    if (stock[g] >= units) {
        stock[g] -= units;
        saveData(r);
        printf("  Issued %d unit(s) of %s. Remaining %s stock: %d.\n", units, patient, patient, stock[g]);
        lowStockAlert();
        return;
    }

    printf("  ! Only %d unit(s) of %s available.\n", stock[g], patient);
    printf("  Compatible groups with enough stock: ");
    for (alt = 0; alt < NUM_GROUPS; alt++) {
        if (alt != g && canDonate(GROUPS[alt], patient) && stock[alt] >= units) {
            printf("%s(%d) ", GROUPS[alt], stock[alt]);
            anyAlt = 1;
        }
    }
    if (!anyAlt) {
        printf("none.\n  Use option 4 (Emergency) to call eligible donors.\n");
        return;
    }
    printf("\n");
    if (!readYesNo("Issue from a compatible group instead? (y/n): "))
        return;

    readBloodGroup("Group to issue from : ", other);
    alt = groupIndex(other);
    if (!canDonate(other, patient)) {
        printf("  ! %s is NOT compatible with %s. Not issued.\n", other, patient);
        return;
    }
    if (stock[alt] < units) {
        printf("  ! Not enough %s stock. Not issued.\n", other);
        return;
    }
    stock[alt] -= units;
    saveData(r);
    printf("  Issued %d unit(s) of %s for a %s patient. Remaining %s stock: %d.\n",
           units, other, patient, other, stock[alt]);
    lowStockAlert();
}

static void stockMenu(Registry *r)
{
    printf("\n1. View stock   2. Issue blood   3. Back\n");
    switch (readInt("Choice: ", 1, 3)) {
    case 1: viewStock();   break;
    case 2: issueBlood(r); break;
    default: break;
    }
}

static void statistics(const Registry *r)
{
    int perGroup[NUM_GROUPS] = {0};
    int i, g, eligible = 0, donations = 0, top = 0;

    for (i = 0; i < r->count; i++) {
        perGroup[groupIndex(r->list[i].group)]++;
        donations += r->list[i].totalDonations;
        if (daysUntilEligible(&r->list[i]) == 0)
            eligible++;
    }
    for (g = 1; g < NUM_GROUPS; g++)
        if (perGroup[g] > perGroup[top])
            top = g;

    printf("\n--- Statistics ---\n");
    printf("  Registered donors      : %d\n", r->count);
    printf("  Eligible to donate now : %d\n", eligible);
    printf("  Donations recorded     : %d\n", donations);
    if (r->count > 0)
        printf("  Most common group      : %s\n", GROUPS[top]);

    printf("\n  Donors per blood group:\n");
    for (g = 0; g < NUM_GROUPS; g++) {
        printf("  %-4s %2d |", GROUPS[g], perGroup[g]);
        printBar(perGroup[g] * 2, '*');
        printf("\n");
    }
}

/* writes a readable text report with fprintf, e.g. to print or share */
static void exportReport(const Registry *r)
{
    FILE *fp = fopen(REPORT_FILE, "w");
    const Donor *d;
    char date[16], last[16];
    int g, eligible = 0;

    if (fp == NULL) {
        printf("  ! Could not create %s\n", REPORT_FILE);
        return;
    }
    formatDate(today, date);

    fprintf(fp, "LifeLink - Blood Bank & Donor Finder\n");
    fprintf(fp, "Donor report generated on %s\n\n", date);
    fprintf(fp, "%-4s %-18s %-4s %-10s  %-11s %-12s %s\n",
            "ID", "Name", "Grp", "Phone", "City", "Last Donated", "Status");
    for (d = r->list; d < r->list + r->count; d++) {
        int wait = daysUntilEligible(d);

        formatDate(d->lastDonation, last);
        fprintf(fp, "%-4d %-18.18s %-4s %-10s  %-11.11s %-12s ",
                d->id, d->name, d->group, d->phone, d->city, last);
        if (wait == 0) {
            fprintf(fp, "Eligible\n");
            eligible++;
        } else {
            fprintf(fp, "Wait %d days\n", wait);
        }
    }

    fprintf(fp, "\nBlood stock (units):\n");
    for (g = 0; g < NUM_GROUPS; g++)
        fprintf(fp, "  %-4s %3d%s\n", GROUPS[g], stock[g], stock[g] < LOW_STOCK ? "  LOW" : "");
    fprintf(fp, "\nTotal donors: %d   Eligible now: %d\n", r->count, eligible);
    fclose(fp);

    printf("\n  Report of %d donors written to %s\n", r->count, REPORT_FILE);
}

/* ------------------------------------------------------------------
 *  Main
 * ------------------------------------------------------------------ */

static void printMenu(const Registry *r)
{
    char date[16];
    int g, total = 0;

    for (g = 0; g < NUM_GROUPS; g++)
        total += stock[g];
    formatDate(today, date);

    printf("\n==================================================\n");
    printf("      LifeLink - Blood Bank & Donor Finder\n");
    printf("==================================================\n");
    printf(" Today: %s   Donors: %d   Stock: %d units\n", date, r->count, total);
    printf("--------------------------------------------------\n");
    printf(" 1. Register new donor\n");
    printf(" 2. View all donors\n");
    printf(" 3. Search donors (blood group / city / ID)\n");
    printf(" 4. EMERGENCY - find compatible donors\n");
    printf(" 5. Record a donation\n");
    printf(" 6. Update donor details\n");
    printf(" 7. Remove donor\n");
    printf(" 8. Blood stock (view / issue)\n");
    printf(" 9. Statistics\n");
    printf("10. Export donor report (text file)\n");
    printf(" 0. Save and exit\n");
    printf("--------------------------------------------------\n");
}

int main(void)
{
    Registry reg = {NULL, 0, 0, 1};
    int choice;

    today = getToday();
    if (!loadData(&reg)) {
        loadSampleData(&reg);
        saveData(&reg);
        printf("First run: loaded %d sample donors into %s.\n", reg.count, DATA_FILE);
    }
    lowStockAlert();

    do {
        printMenu(&reg);
        choice = readInt("Enter your choice: ", 0, 10);
        switch (choice) {
        case 1: registerDonor(&reg);   break;
        case 2: listDonors(&reg);      break;
        case 3: searchDonors(&reg);    break;
        case 4: emergencyFinder(&reg); break;
        case 5: recordDonation(&reg);  break;
        case 6: updateDonor(&reg);     break;
        case 7: deleteDonor(&reg);     break;
        case 8: stockMenu(&reg);       break;
        case 9: statistics(&reg);      break;
        case 10: exportReport(&reg);   break;
        case 0:
            if (saveData(&reg))
                printf("\nData saved to %s. Stay healthy - donate blood, save lives!\n", DATA_FILE);
            break;
        }
    } while (choice != 0);

    free(reg.list);
    return 0;
}
