# Demo DAT format

**Confidence: record width confirmed; field semantics unresolved.**

The demo files are CRLF ASCII integers with no header in the observed shareware set. The number stream divides exactly into records of **14 signed integers**.

Examples:

- DOS `DEMOA2.DAT`: 2,101 records;
- Windows adds `Demoa1`, `Demoa3`, `Demob2` to the set in addition to the four DOS demo files.

No field names are assigned yet. The correct next step is to locate the 14-field playback/recording consumers and correlate state changes against captured runs.
