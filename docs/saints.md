# Saints

A saint is described by a Markdown file. A directory holds one file per saint, and every file whose
name ends in `.md` (in lowercase) is read, in the byte order of the file names.

## The file

The file starts with a YAML header between two lines of `---`. Everything after the header is the
description of the saint, as Markdown.

```
---
name: Nicholas of Myra
lang: en_US
feast_days:
  - "06.12": Feast day
    "p+7": Translation of the relics
  - "09.05"
---

Bishop of Myra in Lycia, ...
```

- **`name`** is required text. Other fields in the header are ignored.
- **`lang`** is optional text, the language of the file, for example `en_US` or `de_DE`. When the
  saints of one language are wanted, the files with another `lang` are ignored. A file without
  `lang` is used for every language. The value is compared as it is, `de_AT` is not `de_DE`.
- **`feast_days`** is a list with at least one item. An item is either a date, or a map of
  `date: title` (one or more pairs). The title is the reason for the feast, and is empty for a plain
  date. The same date with two titles goes in two items.
- **Dates** have the syntax of the [calendar file](dates.md): `DD.MM` and Pascha-relative dates such
  as `p+7`, but no ranges. Put them in quotes.

An invalid file is an error with the file and line, like an invalid calendar file, also if it is
ignored because of its `lang`.

## From C++

`load_saint()` and `load_saints()` (`src/saint.h`) read a file and a directory into `Saint`s with the
`name`, the `lang`, the `description`, the `feasts` (a `Day` and a `title`) and the `file`. `load_saints()` takes the language
to select as an optional second argument. They throw
`std::invalid_argument` for an invalid file and `ConfigFileError` for one that cannot be read.
