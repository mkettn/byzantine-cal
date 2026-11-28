# Templates

The output is produced by a *backend* that renders the calendar of a year with a template. The
templates use [inja](https://github.com/pantor/inja), a template engine with a Jinja-like syntax. The
program renders Markdown with the template `markdown.inja`; another format (HTML, ICS, ...) needs
another template and a backend class for it, see below.

## The Markdown template

`tpl/markdown.inja` is the template of the Markdown backend, which is the one the program uses. It
is installed with the program and found through the `template_dir` option of the
[configuration](configuration.md). To change the output, copy it, edit the copy and put its directory
first in `template_dir`.

The result has a heading with the year, the date of Pascha, and a table for each month that has a
day with an entry.

## Data

A template gets the calendar as JSON:

| Name | Content |
|---|---|
| `year` | The year. |
| `offset_days` | The offset that was added to the fixed dates (13 with `--old`). |
| `pascha` | The date of Pascha, `YYYY-MM-DD`. |
| `days` | Every day of the year, in order. |
| `months` | The 12 months, each with `number`, `name`, `rule_count` and its `days`. |
| `ui` | The texts of the [language](language.md): `ui.date` and so on. |

A day has:

| Name | Content |
|---|---|
| `date` | `YYYY-MM-DD`. |
| `year`, `month`, `day` | The parts of the date, as numbers. |
| `weekday` | 1 (Monday) to 7 (Sunday). |
| `weekday_name` | The name of the weekday in the language. |
| `has_rule` | Whether an entry of the calendar file applies to the day. |
| `data` | The YAML of the value of that entry on one line, empty without an entry. |

`months[].name` is the name of the month in the language. All dates are Gregorian.

## Syntax

The inja built-ins are available (`for`, `if`, `length`, `upper`, ...), see its documentation. The
backend adds:

- **`escape_markdown(text)`** puts a backslash before every character that has a meaning in
  Markdown, and turns newlines into `<br>`. Use it for anything that is not controlled by the
  template, which is the data and the language texts.
- **Whitespace:** a block (`{% ... %}`) that is alone on its line leaves no empty line behind.
- **Line statements** start with `%%`. Inja's default, `##`, would be a Markdown heading.

An example that lists the days with a rule:

```
# {{ year }}
{% for month in months %}
{% if month.rule_count > 0 %}

## {{ escape_markdown(month.name) }}
{% for day in month.days %}
{% if day.has_rule %}
- {{ day.date }} ({{ escape_markdown(day.weekday_name) }}): {{ escape_markdown(day.data) }}
{% endif %}
{% endfor %}
{% endif %}
{% endfor %}
```

A template that cannot be read, is invalid, or fails when it is rendered (an unknown variable, a wrong
argument) is an error, see the exit status in the [README](../README.md#run).

## From C++

The backends are in `src/template_backend.h`. `TemplateBackend` reads the template from the file
whose path it is given. `MarkdownBackend` is the Markdown backend, and its template is called
`MarkdownBackend::kTemplateFile` (`markdown.inja`).

```cpp
bcal::MarkdownBackend backend(config.find_template(bcal::MarkdownBackend::kTemplateFile));
std::string text = backend.render(calendar, language);   // TemplateFileError / TemplateError on failure
```

`config` is a `ByzantineCalConfig` ([Configuration](configuration.md#from-c)) and `language` a
`Language` ([Languages](language.md#from-c)).

`calendar_to_json()` builds the data described above, and `render_template()` renders a template
text with it. `TemplateFileError` (a `TemplateError`) is thrown for a template that cannot be read,
and `TemplateError` for one that is invalid or fails to render, for whatever reason. A new format
needs its own template and a class like `MarkdownBackend` that names it.
