# LifeLink – Blood Bank & Donor Finder System

A C mini-project (Internal Assessment, Component 3) that finds eligible, blood-compatible donors in seconds, tracks blood stock, and warns when a group runs low.

- **Student:** M.V. VAMSI KRISHNA (25BEV7052)
- **Course:** Introduction to Programming, Slot F2+TFF2 – VIT-AP UNIVERSITY
- **Faculty:** Dr A. Manimuthu

## What it does

| Menu | Feature |
| --- | --- |
| 1 | Register a donor like a government portal: personal details (name as on Aadhaar, father's/spouse's name, date of birth, gender, blood group, weight, mobile, email), address (street, city, district, state/UT, PIN), Aadhaar number (Verhoeff checksum + consent), donation history, e-RaktKosh health screening and a declaration |
| 2 | View all donors, sorted by ID, name, blood group or city |
| 3 | Search donors by blood group, city, district or ID (search key held in a `union`); ID search shows the full donor card |
| 4 | **Emergency finder** – compatible groups, units in stock, eligible donors with exact matches first |
| 5 | Record a donation – NBTC gap check, then the 7-question pre-donation screening |
| 6 / 7 | Update (mobile, email, address, weight) or remove a donor |
| 8 | Blood stock – view with low-stock alerts, issue blood with safe substitutes only |
| 9 | Statistics |
| 10 | Export a text report (`donor_report.txt`) with `fprintf` |
| 0 | Save and exit |

Data is saved in the binary file `lifelink.dat`, so it survives between runs. Only the last 4 Aadhaar digits (and a fingerprint to block duplicates) are stored.

## Eligibility rules (NBTC 2017, Govt. of India)

- Age 18–65 years; a first-time donor must not be above 60
- Body weight at least 45 kg
- At least **90 days (men)** or **120 days (women)** between whole-blood donations
- Health screening questions as on the [e-RaktKosh](https://eraktkosh.mohfw.gov.in/BLDAHIMS/bloodbank/eraktkoshfaq.cnt) portal

## Run the C program

```bash
gcc bloodbank.c -o bloodbank
./bloodbank
```

Works with any C99 compiler (GCC, Clang, MinGW, Code::Blocks). On Windows run `bloodbank.exe`.

## Web prototype

`prototype/index.html` is a clickable web version of LifeLink with the same registration fields, rules and sample data. It runs in any browser with no install.

It also adds features that go beyond the C program:

| Feature | What it does |
| --- | --- |
| Donor card | Printable donor ID card with blood group, next donation date and a QR code |
| Badges and lives saved | First Drop, Life Saver, Silver Heart, Gold Hero and Legend badges; each donation counts as up to 3 lives saved |
| Certificate | Printable Certificate of Appreciation for donors who have donated |
| Can I donate? | Quick eligibility check (age, weight, last donation, health questions) with the date you can donate next |
| Emergency requests | Hospitals post blood requests; compatible, eligible donors respond; Requested → Donor found → Fulfilled timeline; share on WhatsApp |
| Donation camps | Upcoming camps with seat booking; only donors eligible on the camp date can book |
| Blood bag stock | Each blood group shown as a blood bag, marked low (under 5 units) or critical (under 3) |
| Compatibility chart | Pick a blood group to see who it can give to or receive from |
| Dashboard charts and map | Donations per month, top donors, and a map of donors and blood centres by city |

Hospitals, camps and blood centres in the prototype are sample data. Everything is stored only in the browser.

- Live: https://lifelinkbloodbank.vercel.app
- Also on GitHub Pages: https://krishmanikanta31-pixel.github.io/LifeLink-Blood-Bank/prototype/

## Syllabus coverage (Modules 1–4)

Control structures, arrays (including 2-D arrays for the Aadhaar checksum), functions, call by value / reference, recursion (binary search), pointers and pointer arithmetic, `malloc` / `calloc` / `realloc` / `free`, function pointers (our own sort with a compare function), structures, an array of structures, a union, and file operations `fopen` / `fread` / `fwrite` / `fseek` / `fprintf`.

## Folder contents

| Path | Contents |
| --- | --- |
| `bloodbank.c` | Complete C source code |
| `LifeLink_Report_25BEV7052.pdf` | Report: source code and output screenshots |
| `prototype/` | Web prototype (`index.html`) |
| `screenshots/` | Output screenshots of the C program |
