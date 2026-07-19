# Publish NetForge on GitHub Pages

NetForge is a static site, so GitHub Pages can publish it directly from the
repository through the included [Pages workflow](.github/workflows/pages.yml).
The workflow uses GitHub's Pages artifact and deployment actions; it does not
need a server, database, build toolchain, or secrets in the repository.

## One-time setup

1. Create or choose a GitHub repository for this folder, for example
   `netforge-interview-studio`.
2. Push the `main` branch.
3. In **Settings → Pages**, choose **GitHub Actions** as the source if GitHub
   has not selected it automatically.
4. Wait for the `Deploy NetForge course to GitHub Pages` workflow to finish.

The project site will then be available at:

```text
https://<github-user>.github.io/<repository-name>/
```

For a repository named `<github-user>.github.io`, the URL is instead:

```text
https://<github-user>.github.io/
```

The dashboard stores progress in the learner's browser. Learning records and
capstone evidence remain local repository files; do not commit private
recordings, credentials, employer code, or sensitive hostnames.

## Local verification before pushing

```bash
node scripts/check-content.mjs
node --check assets/course.js
```

The repository's Pages workflow is intentionally a static upload. The local
`scripts/serve.sh` Markdown renderer remains useful for the richer local studio;
the HTML lessons and dashboard are the canonical Pages experience.

