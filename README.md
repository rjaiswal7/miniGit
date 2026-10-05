# miniGit

A lightweight Git-inspired version control system implemented in C++ from scratch.

The project demonstrates core version-control concepts including content-addressable object storage, blobs, trees, commits, a staging index, `HEAD`, branches, author configuration, and commit history.

> **Note:** This is a simplified implementation inspired by Git. It is not intended to be compatible with the actual Git implementation or repository format.

---

## Features

* Initialize a repository
* Add files to the staging area
* Store file contents as blob objects
* Generate SHA-256 object identifiers
* Maintain a simplified staging index
* Create tree objects representing a project snapshot
* Create commit objects
* Store commit author information
* Configure a default author for the repository
* Override the configured author for individual commits
* Maintain a `main` branch and `HEAD`
* Track parent relationships between commits
* Display commit history
* Content-based object storage and deduplication

---

## Supported Commands

### Initialize a repository

```bash
./miniGit init
```

Creates the repository metadata directory:

```text
.mygit/
├── HEAD
├── objects/
└── refs/
```

`HEAD` initially points to:

```text
ref: refs/heads/main
```

---

### Configure the author

```bash
./miniGit config user.name <name>
```

Example:

```bash
./miniGit config user.name "kk"
```

The configured author is stored in:

```text
.mygit/config
```

as:

```text
user.name=kk
```

The configured author is automatically used when creating commits unless an author is explicitly provided with `--author`.

---

### Add a file

```bash
./miniGit add <file>
```

Example:

```bash
./miniGit add hello.txt
```

The command:

1. Reads the file contents.
2. Creates a blob representation.
3. Generates a SHA-256 hash for the blob.
4. Stores the blob in the object database.
5. Updates the staging index.

The index maintains a mapping between file paths and blob hashes.

Example:

```text
hello.txt    <blob-hash>
```

---

### Commit changes

Using the configured author:

```bash
./miniGit commit -m "Initial commit"
```

An author can also be specified for an individual commit:

```bash
./miniGit commit -m "Initial commit" --author "alice"
```

When `--author` is provided, it takes precedence over the configured author.

The commit process is:

```text
Index
  ↓
Tree
  ↓
Commit
  ↓
Current Branch
```

A commit stores:

* The tree associated with the snapshot
* The parent commit, if one exists
* The author, if provided
* The commit message

The newly created commit becomes the commit pointed to by the current branch.

---

### View commit history

```bash
./miniGit log
```

The log command starts from the commit referenced by the current branch and follows the parent commit links until it reaches the first commit.

Example:

```text
commit <commit-hash>
update hello

commit <previous-commit-hash>
initial commit
```

---

## How to Run

### 1. Compile

```bash
g++ -std=c++17 main.cpp sha256.cpp -o miniGit
```

### 2. Initialize a repository

Run the executable inside the directory you want to track:

```bash
./miniGit init
```

This creates the `.mygit` directory containing the repository metadata and object database.

### 3. Configure the author

```bash
./miniGit config user.name "kk"
```

### 4. Add a file

```bash
./miniGit add filename.txt
```

The file content is stored as a blob object and its hash is recorded in the staging index.

### 5. Create a commit

```bash
./miniGit commit -m "Initial commit"
```

This creates a tree and commit object and updates the `main` branch reference.

Alternatively, specify an author for the individual commit:

```bash
./miniGit commit -m "Initial commit" --author "alice"
```

### 6. View commit history

```bash
./miniGit log
```

---

# Architecture

The project is based on a simplified version of Git's object model.

```text
                    HEAD
                     │
                     ▼
              refs/heads/main
                     │
                     ▼
                  Commit
                 /      \
                ▼        ▼
              Tree     Parent
               │          │
               ▼          ▼
             Blobs      Commit
```

The main components are:

### Blob

A blob represents the contents of a file.

The filename is not stored as part of the blob identity. The relationship between a filename and its blob is maintained by the index/tree representation.

Conceptually, the blob data is represented as:

```text
blob <size>\0<contents>
```

The SHA-256 hash of this representation is used as the blob's object identifier.

---

### Tree

A tree represents a snapshot of the files currently recorded in the index.

This implementation uses a simplified text-based tree representation:

```text
filename    blob-hash
```

The tree data is hashed to produce its object identifier.

Unlike real Git, this implementation uses a flat tree representation rather than recursively representing directories with nested tree objects.

---

### Commit

A commit represents a snapshot and connects it to the previous version.

A simplified commit contains:

```text
tree <tree-hash>
parent <parent-hash>
author <author>
message <commit-message>
```

The `parent` field is omitted for the first commit.

The `author` field is included when author information is provided.

This creates a linked history:

```text
Commit C3
   │
   └── parent → Commit C2
                  │
                  └── parent → Commit C1
```

The commit object is hashed using SHA-256, and the resulting hash becomes the commit's object identifier.

---

## Object Storage

Objects are stored using the first two characters of their SHA-256 hash as a directory name.

For example, if an object's hash is:

```text
a1b2c3d4...
```

it is stored as:

```text
.mygit/objects/a1/b2c3d4...
```

This allows objects to be addressed using their content-derived hash.

It also provides deduplication: identical object data produces the same hash and therefore maps to the same object location.

---

## Index

The project maintains a simplified staging index at:

```text
.mygit/index
```

The index stores:

```text
<file-path>    <blob-hash>
```

Example:

```text
hello.txt      a1b2c3...
main.cpp       d4e5f6...
```

When a file is added again after modification, its entry is updated with the new blob hash.

This allows the tree to be created from the exact set of files currently staged.

> The index format used here is a simplified text-based representation. Real Git uses a more complex binary index format.

---

## HEAD and Branches

The repository's `HEAD` file contains:

```text
ref: refs/heads/main
```

This means `HEAD` points to the `main` branch.

The branch then points to the latest commit:

```text
HEAD
 ↓
refs/heads/main
 ↓
Commit C2
```

When a new commit is created, the `main` branch reference is updated to point to the new commit.

---

## Author Configuration

The repository can store a default author in:

```text
.mygit/config
```

For example:

```text
user.name=kk
```

When a commit is created without `--author`, this configured author is used.

An individual commit can override the configured author:

```bash
./miniGit commit -m "Update file" --author "alice"
```

This keeps author configuration separate from the commit creation logic while allowing per-commit overrides.

---

## Repository Structure

After initialization and creating some objects, the repository looks approximately like:

```text
.mygit/
├── HEAD
├── config
├── index
├── objects/
│   ├── <first-two-hash-characters>/
│   │   └── <remaining-hash>
│   └── ...
└── refs/
    └── heads/
        └── main
```

---

## Example Workflow

Initialize a repository:

```bash
./miniGit init
```

Configure the author:

```bash
./miniGit config user.name "kk"
```

Create a file:

```bash
echo "Hello World" > hello.txt
```

Stage it:

```bash
./miniGit add hello.txt
```

Create the first commit:

```bash
./miniGit commit -m "Initial commit"
```

Modify the file:

```bash
echo "Hello Git" > hello.txt
```

Stage the modification:

```bash
./miniGit add hello.txt
```

Create another commit:

```bash
./miniGit commit -m "Update hello"
```

View the history:

```bash
./miniGit log
```

The resulting history is:

```text
Commit 2
   │
   └── parent → Commit 1
```

---

## Limitations

This project intentionally simplifies several parts of real Git.

* The tree representation is simplified and flat rather than hierarchical.
* The index is a simplified text file rather than Git's binary index format.
* Objects are stored without Git's compression mechanism.
* The implementation primarily focuses on text-file handling.
* Only a basic `main` branch workflow is implemented.
* Branch creation and checkout are not implemented.
* There is no remote repository functionality.
* There is no merge, push, pull, or networking functionality.
* Commit metadata is simplified compared with real Git.
* The repository format is not compatible with actual Git repositories.

---

## Technologies Used

* **C++17**
* C++17 filesystem library
* Standard C++ file and stream APIs
* SHA-256
* Command-line argument handling
* Content-addressable object storage
* File-based repository metadata
