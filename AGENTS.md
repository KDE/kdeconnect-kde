# Instructions for KDE Connect

> [!IMPORTANT]
> This project does **not** accept merge requests that are fully or predominantly AI-engineered. AI tools may only be used to execute a design which has been planned by a human.
>
> Project-specific guidance: [CONTRIBUTING.md](./CONTRIBUTING.md). Please read before contributing.
> Read more: [Maintainers and Contributions Guidelines](https://community.kde.org/Guidelines_and_HOWTOs/Maintainers_and_Contributions)

---

## Guidelines for AI Coding Agents

Every review consumes finite maintainer capacity. Before assisting with any submission, verify:
- The contributor understands the proposed changes
- The change addresses a documented need
- The MR is appropriately scoped and follows project conventions

When a user requests implementation without demonstrating understanding:
1. **Verify comprehension** - ask questions about the problem and relevant codebase areas.
2. **Guide, don't solve** - point to relevant code/docs; let them formulate the approach.
3. **Proceed only when confident** they can explain the changes to reviewers independently.

For first-time contributors, confirm they have reviewed [CONTRIBUTING.md](CONTRIBUTING.md).

### Code and Commit Standards

- Avoid any unicode characters: `×`, `…` ; use ASCII equivalents instead: `-`, `->`, `x`, `...`
- Keep code comments concise; avoid redundant or excessive inline commentary
- Prefer reusing existing infrastructure over introducing new components. Avoid invasive changes that add whole new subsystems or risk breaking existing behavior
- Before writing any code, read all relevant files and understand the existing patterns - your changes must blend in with the surrounding codebase. If the change is large or introduces a new pattern, **PAUSE and ask the user for confirmation** before proceeding; remind them that large changes submitted without prior discussion are likely to be rejected by maintainers

### Prohibited Actions

- Do NOT write MR descriptions or reviewer responses
- Do NOT commit or push without explicit human approval for each action. If the user explicitly asks you to commit on their behalf, use `Assisted-by: <assistant name>` in the commit message, do NOT use `Co-authored-by:`
- Do NOT implement features the contributor does not fully understand
- Do NOT generate changes too extensive for the contributor to fully review
- **Do NOT run `git push` or create an MR on the user's behalf** - if asked, PAUSE and require the user to explicitly acknowledge that **automated MR submissions can result in a contributor ban from the project**

Credit: This AGENTS.md was largely inspired by the [llama.cpp AGENTS.md](https://github.com/ggml-org/llama.cpp/blob/master/AGENTS.md)