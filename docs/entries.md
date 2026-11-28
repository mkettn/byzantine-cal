# Entries

An entry is something that is celebrated or remembered on one or more days of the year, for example
a saint with a feast day or a historical event. Entries are read from the files of a directory: every
file whose name ends in `.md` or `.yaml` (in lowercase, so not `.yml`) is read, in the byte order
of the file names.

## Markdown files

A `.md` file is one entry. It starts with a YAML header between two lines of `---`. Everything after
the header is the description of the entry, as Markdown.

```
---
name: Nicholas of Myra
type: saint
lang: en_US
days:
  - "06.12": Feast day
    "p+7": Translation of the relics
  - "09.05"
---

Bishop of Myra in Lycia, ...
```

## YAML files

A `.yaml` file has one entry for each YAML document in it, separated by `---`. The entries have the
same fields as the header above and no description. An empty document, for example after a last
`---`, is skipped, and so is an empty file.

```
name: Council of Nicaea
type: event
days: ["20.05"]
---
name: Council of Constantinople
type: event
days: ["09.07"]
```

## The fields

- **`name`** is required text. Other fields are ignored.
- **`type`** is optional text, for example `saint` or `event`. It is not checked: the templates
  decide which types they show.
- **`lang`** is optional text, the language of the entry, for example `en_US` or `de_DE`. When the
  entries of one language are wanted, the entries with another `lang` are ignored. An entry without
  `lang` is used for every language. The value is compared as it is, `de_AT` is not `de_DE`.
- **`days`** is a list with at least one item. An item is either a date, or a map of `date: title`
  (one or more pairs). The title is the reason for the day, and is empty for a plain date. The same
  date with two titles goes in two items.
- **Dates** have the syntax of the [calendar file](dates.md): `DD.MM` and Pascha-relative dates such
  as `p+7`, but no ranges. Put them in quotes. They are in the calendar that is chosen on the
  command line, so `--old` shifts them like the dates of the fasting rules.

An invalid file is an error with the file and line, like an invalid calendar file, also if its
entries are ignored because of their `lang`.

## From C++

`load_entry_file()` and `load_entries()` (`src/entry.h`) read a file and a directory into `Entry`s
with the `name`, `type`, `lang`, `description`, `days` (a `Day` and a `title`) and the `file`.
`load_entries()` takes the language to select as an optional second argument, compared exactly
(`de_de` is not `de_DE`). They throw `std::invalid_argument` for an invalid file and
`ConfigFileError` for one that cannot be read.

`YearCalendar::add_entry()` puts an entry on its days, see [Dates](dates.md#using-it-from-c).
