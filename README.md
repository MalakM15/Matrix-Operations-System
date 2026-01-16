# Matrix-Operations-System

A high-performance C-based matrix operations system for creating, managing, and computing operations on matrices.
The program supports multiple execution modes, including:

* **Single-thread execution**
* **Multiprocessing using process pools and pipes for inter-process communication**
* **OpenMP parallel execution**

---

## Build & Run

In the terminal:

```bash
make clean
make
make run
```

The program uses a config file (default: `default_config.txt`) as defined in the Makefile.
You can:

* Edit `default_config.txt`, or
* Create a new config file and update its name in the Makefile.

After modifying the config file or Makefile:

```bash
make clean && make
make run
```
---

## Program Overview

When launched, the program displays the **Matrix Operations Menu**, offering the features below.

---

## 1. Enter a Matrix

* User enters number of rows and columns.
* User inputs each element with validation.
* Matrix is stored in memory and assigned a matrix ID.

---

## 2. Display a Matrix

* User enters a matrix ID.
* If it exists, the matrix is printed in formatted form.
* If not, an error message is shown.

---

## 3. Delete a Matrix

* User enters a matrix ID.
* If ID exists, the matrix is deleted.
* Otherwise, an error message is shown.

---

## 4. Modify a Matrix

Modify options:

* `0` Cancel
* `1` Modify a specific element
* `2` Modify a full row
* `3` Modify a full column

After updating, the program prints the modified matrix.

---

## 5. Read a Matrix from a File

* User enters a path (or `q` to cancel).
* File existence is validated.
* Matrix is loaded and stored with a new ID.

---

## 6. Read Matrices from a Folder

* User enters a folder path (or `q` to cancel).
* Program lists all files in the folder.
* User selects files by comma-separated indices.
* Each selected matrix is loaded and assigned a new ID.
* Program confirms the number of imported files.

---

## 7. Save a Matrix to a File

* User selects a matrix ID.
* User specifies destination folder and filename.
* Program saves the matrix and prints a confirmation.

---

## 8. Save All Matrices to a Folder

* User enters destination folder.
* Folder is created if needed.
* All matrices are saved with confirmation.

---

## 9. Display All Matrices

* Displays all matrices currently stored in memory.
* Shows matrix ID, dimensions, and formatted matrix data for each stored matrix.

---

## 10. Add Two Matrices

* User enters IDs of two matrices (same dimensions required).
* User selects execution mode:

  * Single-thread
  * Multi-process
  * OpenMP
* Result is displayed, execution time is shown, and result is stored with a new ID.

---

## 11. Subtract Two Matrices

* User enters IDs of two matrices (same dimensions required).
* User selects execution mode:

  * Single-thread
  * Multi-process
  * OpenMP
* Result is displayed, execution time is shown, and result is stored with a new ID.

---

## 12. Multiply Two Matrices

* User enters IDs of two matrices (valid dimension check).
* User selects execution mode.
* Result is displayed, timed, and stored with a new ID.

---

## 13. Find Determinant

* User enters a matrix ID (must be square).
* User selects execution mode.
* Determinant is computed, displayed, and timed.

---

## 14. Find Eigenvalues & Eigenvectors

* User enters matrix ID.
* User selects execution mode.
* Program computes:

  * top k eigenvalues
  * corresponding eigenvectors (as columns)
* Displays computation times.
* Eigenvalues and eigenvectors are stored in memory.

---

## 15. Exit

* Exits the program and frees all allocated memory.

---
