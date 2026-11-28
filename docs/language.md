# Languages

Month and weekday names and the texts of the templates (table headings and the like) come from a
language file. The calendar file itself is not translated: names and other data in it are printed as
written.

## Choosing a language

The language is the `language` option of the [configuration](configuration.md), and
`--lang=<language>` overrides it:

```bash
byzantine-cal --year=2026 --lang=de_DE calendar.yml
```

The value is not checked and the environment (`LANG`, `LC_ALL`, ...) is not looked at. A language
is simply the name of a file: `language: foobar` uses `foobar.yaml` from the first `lang_dir` that
has it. It is an error if there is no such file, and also if no language is set.

The installed configuration sets `en_US` and installs the languages `en_US`, `de_DE` and `de_AT`.
Names like `de_DE` are only a convention.

## A language file

A YAML file with these keys, all of them optional:

```yaml
months: [January, February, March, April, May, June, July, August, September, October, November, December]
weekdays: [Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday]
ui:
  date: Date
  weekday: Weekday
  data: Data
  pascha: Pascha
```

| Key | Content |
|---|---|
| `months` | Exactly 12 names, January first. |
| `weekdays` | Exactly 7 names, Monday first. |
| `ui` | Texts for the templates, available there as `ui.date` and so on. The shipped Markdown template uses `date`, `weekday`, `data` and `pascha`. |
| `fallback` | The name of another language file, see below. |

Names must not be blank. Unknown keys and files with several YAML documents are errors.

## Fallbacks

A language file can name a fallback language, again the name of a language file:

```yaml
# de_CH.yaml
fallback: de_DE
months: [Januar, Februar, März, April, Mai, Juni, Juli, August, September, Oktober, November, Dezember]
```

The fallback is read first and the keys of the file replace its keys: `months` and `weekdays` as a
whole, `ui` entry by entry. The fallback of the fallback comes before that, and so on. So a file only
needs to hold what differs: `de_AT.yaml` (`fallback: de_DE`) has only the months, and `de_DE.yaml`
has `fallback: en_US`.

- **Every file is read only once.** If a fallback refers to a file that was read already, a circular
  reference, that is logged as a warning and ignored.
- **A missing fallback file is an error.**
- **The months and the weekdays must be there at the end** of the chain. The `ui` texts the template
  needs have to be there as well, or the template fails with a "variable not found" error.

## Adding a language

Write `fr_FR.yaml` next to the others (or in your own `lang_dir`, see
[Configuration](configuration.md)), with a `fallback` if it should reuse another language, and run
with `--lang=fr_FR` or set `language: fr_FR`.
