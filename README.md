# LifeLink – Blood Bank & Donor Finder System

A C mini-project (Internal Assessment, Component 3) that finds eligible, blood-compatible donors in seconds, tracks blood stock, and warns when a group runs low.

- **Student:** M.V. VAMSI KRISHNA (25BEV7052)
- **Course:** Introduction to Programming, Slot F2+TF2 – VIT-AP UNIVERSITY
- **Faculty:** Dr A. Manimuthu

## What it does

| Menu | Feature |
| --- | --- |
| 1 | Register a donor (validated name, age 18–65, blood group, 10-digit mobile, city, last donation date) |
| 2 | View all donors, sorted by ID, name, blood group or city |
| 3 | Search donors by blood group, city or ID (search key held in a `union`) |
| 4 | **Emergency finder** – compatible groups, units in stock, eligible donors with exact matches first |
| 5 | Record a donation (refused within 90 days of the last one) |
| 6 / 7 | Update or remove a donor |
| 8 | Blood stock – view with low-stock alerts, issue blood with safe substitutes only |
| 9 | Statistics |
| 10 | Export a text report (`donor_report.txt`) with `fprintf` |
| 0 | Save and exit |

Data is saved in the binary file `lifelink.dat`, so it survives between runs.

## Run the C program

```bash
gcc bloodbank.c -o bloodbank
./bloodbank
```

Works with any C99 compiler (GCC, Clang, MinGW, Code::Blocks). On Windows run `bloodbank.exe`.

## Web prototype

`prototype/index.html` is a clickable web version of LifeLink that uses the same rules and sample data. It runs in any browser with no install, and is published with GitHub Pages.

## Syllabus coverage (Modules 1–4)

Control structures, arrays, functions, call by value / reference, recursion (binary search), pointers and pointer arithmetic, `malloc` / `calloc` / `realloc` / `free`, function pointers (`qsort`), structures, a union, and file operations `fopen` / `fread` / `fwrite` / `fseek` / `fprintf`.

## Folder contents

| Path | Contents |
| --- | --- |
| `bloodbank.c` | Complete C source code |
| `prototype/` | Web prototype (`index.html`) |
| `screenshots/` | Output screenshots of the C program |
