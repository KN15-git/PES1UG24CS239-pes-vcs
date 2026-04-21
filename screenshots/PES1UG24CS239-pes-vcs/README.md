# Building PES-VCS — A Version Control System from Scratch

## 📌 Overview

This project implements a simplified version control system (PES-VCS) inspired by Git.
It supports object storage, tree structures, staging (index), commits, and history tracking.

**Platform:** Ubuntu 22.04

---

# 📘 Lab Report

---

## 🧩 Phase 1: Object Storage

### 📸 Screenshot 1A — Test Output

![1A](screenshots/1A.png)

### 📸 Screenshot 1B — Object Store Structure

![1B](screenshots/1B.png)

---

## 🌳 Phase 2: Tree Objects

### 📸 Screenshot 2A — Tree Test Output

![2A](screenshots/2A.png)

### 📸 Screenshot 2B — Raw Tree Object (xxd)

![2B](screenshots/2B.png)

---

## 📦 Phase 3: Index (Staging Area)

### 📸 Screenshot 3A — pes init → add → status

![3A](screenshots/3A.png)

### 📸 Screenshot 3B — Index File

![3B](screenshots/3B.png)

---

## 🧾 Phase 4: Commits & History

### 📸 Screenshot 4A — pes log

![4A](screenshots/4A.png)

### 📸 Screenshot 4B — Object Store Growth

![4B](screenshots/4B.png)

### 📸 Screenshot 4C — HEAD & Branch Reference

![4C](screenshots/4C.png)

---

## 🧪 Final Integration Test

### 📸 Output

![Final](screenshots/final.png)

---

# 🧠 Phase 5: Analysis — Branching & Checkout

### Q5.1 — Branch Checkout

A branch in Git is simply a file that stores a commit hash. Implementing `pes checkout <branch>` involves updating the `.pes/HEAD` file to point to the new branch reference and updating the working directory to match the snapshot represented by the target commit's tree.

Steps:

1. Update `.pes/HEAD` to point to `refs/heads/<branch>`.
2. Read the commit hash from the branch file.
3. Load the commit object and retrieve its tree.
4. Recursively recreate the working directory files from the tree.

The complexity arises because:

* The working directory must exactly match the target tree.
* Files may need to be created, modified, or deleted.
* Conflicts can occur if uncommitted changes exist.

---

### Q5.2 — Dirty Working Directory Detection

To detect conflicts before checkout:

1. Compare working directory files with index entries.
2. If a tracked file has been modified (based on metadata like size or mtime), it is considered dirty.
3. Compare index entries with the target commit tree.
4. If differences exist for the same file → conflict.

This ensures that local uncommitted changes are not overwritten.

---

### Q5.3 — Detached HEAD

A detached HEAD occurs when HEAD points directly to a commit instead of a branch.

If commits are made in this state:

* New commits are created
* But no branch references them → they become unreachable

Recovery:

```bash
git branch new-branch <commit-hash>
```

---

# 🧠 Phase 6: Analysis — Garbage Collection

### Q6.1 — Finding Unreachable Objects

Algorithm:

1. Start from all branch heads.
2. Traverse all reachable commits recursively:

   * Mark commit
   * Mark its tree
   * Recursively mark blobs and subtrees
3. Store marked objects in a hash set.
4. Scan `.pes/objects` and delete unmarked ones.

Data structure:

* Hash set (for fast lookup)

Estimated scale:

* Around 100,000 commits → traversal of commits, trees, and blobs → several hundred thousand objects.

---

### Q6.2 — GC Race Condition

Problem:
Garbage collection may delete objects while a commit is being created.

Example:

1. New objects (blob/tree) are written
2. GC runs before commit is finalized
3. GC deletes objects as “unreachable”
4. Commit references missing objects → repository corruption

Solution (used by Git):

* Delay deletion using time thresholds
* Use locking mechanisms
* Ensure objects are safely referenced
* Use packfiles and safe GC strategies

---

# ✅ Submission Summary

* ✔ All phases implemented (1–4)
* ✔ Screenshots included
* ✔ Analysis questions completed
* ✔ Integration test executed
* ✔ Minimum commit requirement satisfied

---

# 📁 Screenshot Instructions

Create a folder:

```bash
mkdir screenshots
```

Place all images inside:

```
screenshots/1A.png
screenshots/1B.png
screenshots/2A.png
screenshots/2B.png
screenshots/3A.png
screenshots/3B.png
screenshots/4A.png
screenshots/4B.png
screenshots/4C.png
screenshots/final.png
```

---
