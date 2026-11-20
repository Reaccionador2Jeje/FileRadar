# FileRadar — Ecosystem Integration

Status: planning document; the existing read-only inspection design remains the default.

## Integration contract
- Treat scans as read-only and never delete or modify scanned files.
- Keep symbolic-link behavior explicit and avoid following links by default.
- Use stable JSON report fields and version the report schema before breaking changes.
- Make scan errors visible rather than silently omitting unreadable paths.
- Bound memory use for large directories and files.

## Next tasks
1. Add fixture-based tests for duplicate detection and byte verification.
2. Add JSON schema validation tests.
3. Test unreadable files, permission errors and symbolic links.
4. Document performance limits and cancellation behavior.