# Templates

The output is produced by a *backend* that renders the calendar of a year with a template. The
templates use [inja](https://github.com/pantor/inja), a template engine with a Jinja-like syntax, so
a new output format (HTML, ICS, ...) is another template.

## The Markdown template

`tpl/markdown.inja` is the template of the Markdown backend, which is the one the program uses. It
is installed with the program and found through the `template_dir` option of the
[configuration](configuration.md): the first directory that has a file called `markdown.inja` wins.
To change the output, copy it, edit the copy and put its directory first in `template_dir`.

The result has a heading with the year, the date of Pascha, and a table for each month that has a
day with a rule.

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
| `has_rule` | Whether a rule applies to the day. |
| `data` | The data of that rule as one line of YAML, empty without a rule. |

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

A template that cannot be read is an error with exit status 2. A template that is invalid, or that
fails when it is rendered (an unknown variable, a wrong argument), is an error with exit status 1.

## From C++

The backends are in `src/template_backend.h`. `TemplateBackend` reads the template from the file
whose path it is given. `MarkdownBackend` is the Markdown backend, and its template is called
`MarkdownBackend::kTemplateFile` (`markdown.inja`).

```cpp
bcal::ByzantineCalConfig config(bcal::ByzantineCalConfig::find_file(std::nullopt, installed_config));
bcal::MarkdownBackend backend(config.find_template(bcal::MarkdownBackend::kTemplateFile));
auto find = [&](const std::string& name) { return config.find_language_file(name); };
bcal::Language language(find("en_US"), find);
std::string text = backend.render(calendar, language);   // TemplateFileError / TemplateError on failure
```

`calendar_to_json()` builds the data described above, and `render_template()` renders a template
text with it. A new format needs its own template and a class like `MarkdownBackend` that names it.
