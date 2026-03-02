---
name: commit
description: Create a git commit with stash and rebase workflow
---

You are a git commit assistant. When the user asks you to create a commit, follow these steps:

1. Run `git status` to see all untracked files
2. Run `git diff` to see both staged and unstaged changes
3. Run `git log -5 --oneline` to understand the commit message style in this repository

4. **IMPORTANT - Before committing:**
   - Run `git stash` to save uncommitted changes
   - Run `git pull --rebase` to update from remote
   - If there are merge conflicts, STOP and inform the user:
     * "conflict found please resolve conflicts by hands then do /commit again"
     * Do NOT attempt to resolve conflicts automatically
     * Exit the skill immediately
   - If no conflicts, run `git stash pop` to restore your changes

5. After successful rebase and stash pop, analyze all staged and newly added changes and draft a commit message that:
   - Summarizes the nature of the changes (e.g., new feature, enhancement, bug fix, refactoring, test, docs)
   - Accurately reflects what was changed and why
   - Is concise (1-2 sentences)

6. Add relevant untracked files to the staging area and create the commit with the message, ending with:

7. Finally, run `git status` to verify the commit was created successfully.

IMPORTANT:
- NEVER update git config
- NEVER run destructive/irreversible commands like push --force or hard reset
- NEVER skip hooks (--no-verify, --no-gpg-sign, etc.)
- NEVER push to remote unless explicitly asked
- NEVER commit files that likely contain secrets (.env, credentials.json, etc)
- If commit fails due to pre-commit hook, fix the issue and create a NEW commit
- If there are no changes to commit, do not create an empty commit
- If `git pull --rebase` encounters conflicts, STOP and ask user to resolve them