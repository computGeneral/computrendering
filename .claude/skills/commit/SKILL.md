---
name: commit
description: Create a git commit with stash and rebase workflow
---

You are a git commit assistant. When the user asks you to create a commit, follow these steps:

1. Run `git status` to see all untracked files
2. Run `git diff` to see both staged and unstaged changes
3. Run `git log -5 --oneline` to understand the commit message style in this repository

4. **Code Review — read every changed/new file and check for:**

   **Safety & Correctness:**
   - Memory leaks: every `new` must have a matching `delete` (or use RAII/smart pointers); every `malloc`/`calloc` must have a `free`
   - Null/dangling pointer dereference: check pointers before use, especially after `dynamic_cast`, `find()`, or allocation
   - Buffer overflows: array indices must be bounds-checked; `strcpy`/`sprintf` should use safe alternatives or verify sizes
   - Uninitialized variables: all variables must be initialized before first use
   - Resource leaks: open files/streams must be closed on all code paths (including error paths)
   - Use-after-free: no access to memory after it has been deleted/freed
   - Integer overflow/truncation: watch for narrowing conversions (e.g., U64 → U32)
   - Thread safety: shared mutable state must be protected if concurrency is possible

   **Coding Style (per CLAUDE.md conventions):**
   - Indentation: 4 spaces, no tabs
   - Naming: PascalCase classes, camelCase methods/variables, UPPER_CASE constants/macros
   - Use project types (`U32`, `S32`, `F32`, `U64`, etc.) instead of raw C++ types
   - Include guards: `#ifndef __HEADER_NAME__` / `#define __HEADER_NAME__` (no `#pragma once`)
   - Braces on same line as function/control statement
   - Space after keywords (`if`, `for`, `while`)
   - Prefixes: `cm`/`cmo` for perfmodel, `bm`/`bmo` for bhavmodel, `cgo` for common
   - Doxygen-style comments for classes and public methods (`//!` or `/*! */`)

   **If issues are found:**
   - Fix all issues automatically before proceeding to commit
   - Briefly list what was fixed in the commit message

5. **IMPORTANT - Before committing:**
   - Run `git stash` to save uncommitted changes
   - Run `git pull --rebase` to update from remote
   - If there are merge conflicts, STOP and inform the user:
     * "conflict found please resolve conflicts by hands then do /commit again"
     * Do NOT attempt to resolve conflicts automatically
     * Exit the skill immediately
   - If no conflicts, run `git stash pop` to restore your changes

6. After successful rebase and stash pop, analyze all staged and newly added changes and draft a commit message that:
   - Summarizes the nature of the changes (e.g., new feature, enhancement, bug fix, refactoring, test, docs)
   - Accurately reflects what was changed and why
   - Is concise (1-2 sentences)

7. Add relevant untracked files to the staging area and create the commit with the message.

8. Finally, run `git status` to verify the commit was created successfully.

IMPORTANT:
- NEVER update git config
- NEVER run destructive/irreversible commands like push --force or hard reset
- NEVER skip hooks (--no-verify, --no-gpg-sign, etc.)
- NEVER push to remote unless explicitly asked
- NEVER commit files that likely contain secrets (.env, credentials.json, etc)
- If commit fails due to pre-commit hook, fix the issue and create a NEW commit
- If there are no changes to commit, do not create an empty commit
- If `git pull --rebase` encounters conflicts, STOP and ask user to resolve them